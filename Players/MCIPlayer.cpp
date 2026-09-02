
/** $VER: MCIPlayer.cpp (2026.09.02) P. Stuer - Implements a player that sends to a Windows MIDI output port **/

#include "pch.h"

#include "MCIPlayer.h"

#include "Resource.h"
#include "Log.h"

#include <algorithm>
#include <condition_variable>
#include <map>
#include <mutex>
#include <vector>

#pragma comment(lib, "winmm.lib")

#pragma region Connection

/// <summary>
/// A connection to a MIDI output device, shared by every player that sends to that device.
/// </summary>
class MCIPlayer::Connection
{
public:
    /// <summary>
    /// Gets the connection to a device, opening it if it is not open yet.
    /// </summary>
    static std::shared_ptr<Connection> Acquire(UINT deviceId, std::string & errorMessage) noexcept;

    /// <summary>
    /// Closes every connection and stops the thread that closes idle ones.
    /// </summary>
    static void CloseAll() noexcept;

    /// <summary>
    /// Lets go of the connection. It is closed after a grace period if nobody acquires it again.
    /// </summary>
    void Release() noexcept;

    /// <summary>
    /// Waits until the player is the only one sending to the device. Returns false when it gave up.
    /// </summary>
    bool TakeOwnership(const MCIPlayer * player, foobar2000_io::abort_callback * abortHandler, uint32_t timeoutInMS) noexcept;

    /// <summary>
    /// Lets another player send to the device. Stops the notes the player left sounding.
    /// </summary>
    void DropOwnership(const MCIPlayer * player) noexcept;

    void Send(uint32_t message) noexcept;
    void SendSysEx(const uint8_t * data, size_t size) noexcept;
    void Silence() noexcept;

    const std::string & GetDeviceName() const noexcept { return _DeviceName; }

private:
    Connection(UINT deviceId, HMIDIOUT hDevice, const std::string & deviceName) noexcept;

    void Close() noexcept;              // The caller holds _Mutex.
    void SilenceUnlocked() noexcept;    // The caller holds _Mutex.

    static void LogError(MMRESULT result, const char * functionName) noexcept;

    static const uint32_t OpenAttempts = 12;            // Retries while a driver is still releasing a previous client.
    static const uint32_t OpenRetryIntervalInMS = 250;
    static const uint32_t GracePeriodInMS = 10'000;     // How long an idle connection stays open. Unloading and reloading a plug-in is slow.

    static std::mutex _RegistryMutex;
    static std::map<UINT, std::shared_ptr<Connection>> _Registry;
    static std::thread _Reaper;
    static std::atomic<bool> _IsReaperRunning;

    mutable std::mutex _Mutex;                          // Serialises every call into the driver.
    std::condition_variable _OwnerChanged;

    UINT _DeviceId;
    HMIDIOUT _hDevice;
    std::string _DeviceName;

    uint32_t _RefCount;                                 // Number of players holding the connection.
    const MCIPlayer * _Owner;                           // The player that is allowed to send.
    uint32_t _IdleSince;                                // Result of ::timeGetTime() when the last player let go.

    std::vector<uint8_t> _SysEx;                        // Holds a system exclusive message while the device sends it.
};

std::mutex MCIPlayer::Connection::_RegistryMutex;
std::map<UINT, std::shared_ptr<MCIPlayer::Connection>> MCIPlayer::Connection::_Registry;
std::thread MCIPlayer::Connection::_Reaper;
std::atomic<bool> MCIPlayer::Connection::_IsReaperRunning(false);

MCIPlayer::Connection::Connection(UINT deviceId, HMIDIOUT hDevice, const std::string & deviceName) noexcept
:
    _DeviceId(deviceId),
    _hDevice(hDevice),
    _DeviceName(deviceName),
    _RefCount(0),
    _Owner(nullptr),
    _IdleSince(0)
{
}

/// <summary>
/// Gets the connection to a device, opening it if it is not open yet.
/// </summary>
std::shared_ptr<MCIPlayer::Connection> MCIPlayer::Connection::Acquire(UINT deviceId, std::string & errorMessage) noexcept
{
    std::lock_guard<std::mutex> RegistryLock(_RegistryMutex);

    auto It = _Registry.find(deviceId);

    if (It != _Registry.end())
    {
        std::lock_guard<std::mutex> Lock(It->second->_Mutex);

        ++It->second->_RefCount;

        return It->second;
    }

    const std::string DeviceName = MCIPlayer::GetDeviceName(deviceId);

    HMIDIOUT hDevice = 0;

    // Some drivers refuse a connection for a while after the previous client disconnected. The VST MIDI
    // Synth driver does that whenever it is unloading a plug-in, which takes seconds with a large one.
    MMRESULT Result = MMSYSERR_NOERROR;

    for (uint32_t Attempt = 0; Attempt < OpenAttempts; ++Attempt)
    {
        Result = ::midiOutOpen(&hDevice, deviceId, 0, 0, CALLBACK_NULL);

        if (Result == MMSYSERR_NOERROR)
            break;

        hDevice = 0;

        ::Sleep(OpenRetryIntervalInMS);
    }

    if (Result != MMSYSERR_NOERROR)
    {
        LogError(Result, "midiOutOpen");

        errorMessage = "Failed to open MIDI output device \"" + DeviceName + "\". It may be in use by another application, or still releasing a plug-in from a previous track.";

        Log.AtError().Write(STR_COMPONENT_BASENAME " could not open a MIDI output device. %s", errorMessage.c_str());

        return nullptr;
    }

    // Ask for a 1 ms timer while the connection is open so that the waits between events are accurate.
    ::timeBeginPeriod(1);

    std::shared_ptr<Connection> NewConnection(new Connection(deviceId, hDevice, DeviceName));

    NewConnection->_RefCount = 1;

    _Registry[deviceId] = NewConnection;

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " opened MIDI output device \"%s\".", DeviceName.c_str());

    // Start the thread that closes connections that nobody uses anymore.
    if (!_IsReaperRunning)
    {
        _IsReaperRunning = true;

        try
        {
            _Reaper = std::thread([]()
            {
                while (_IsReaperRunning)
                {
                    ::Sleep(250);

                    if (!_IsReaperRunning)
                        break;

                    std::lock_guard<std::mutex> RegistryLock(_RegistryMutex);

                    const uint32_t Now = ::timeGetTime();

                    for (auto Entry = _Registry.begin(); Entry != _Registry.end();)
                    {
                        bool IsIdle = false;

                        {
                            std::lock_guard<std::mutex> Lock(Entry->second->_Mutex);

                            if ((Entry->second->_RefCount == 0) && ((Now - Entry->second->_IdleSince) >= GracePeriodInMS))
                            {
                                Entry->second->Close();

                                IsIdle = true;
                            }
                        }

                        Entry = IsIdle ? _Registry.erase(Entry) : std::next(Entry);
                    }
                }
            });
        }
        catch (...)
        {
            _IsReaperRunning = false;
        }
    }

    return NewConnection;
}

/// <summary>
/// Closes every connection and stops the thread that closes idle ones.
/// </summary>
void MCIPlayer::Connection::CloseAll() noexcept
{
    _IsReaperRunning = false;

    if (_Reaper.joinable())
    {
        try
        {
            _Reaper.join();
        }
        catch (...)
        {
        }
    }

    std::lock_guard<std::mutex> RegistryLock(_RegistryMutex);

    for (auto & Entry : _Registry)
    {
        std::lock_guard<std::mutex> Lock(Entry.second->_Mutex);

        Entry.second->Close();
    }

    _Registry.clear();
}

/// <summary>
/// Lets go of the connection. It is closed after a grace period if nobody acquires it again.
/// </summary>
void MCIPlayer::Connection::Release() noexcept
{
    std::lock_guard<std::mutex> Lock(_Mutex);

    if (_RefCount != 0)
        --_RefCount;

    if (_RefCount == 0)
        _IdleSince = ::timeGetTime();
}

/// <summary>
/// Waits until the player is the only one sending to the device. Returns false when it gave up.
/// </summary>
bool MCIPlayer::Connection::TakeOwnership(const MCIPlayer * player, foobar2000_io::abort_callback * abortHandler, uint32_t timeoutInMS) noexcept
{
    std::unique_lock<std::mutex> Lock(_Mutex);

    const uint32_t StartTime = ::timeGetTime();

    while (_hDevice != 0)
    {
        if ((_Owner == nullptr) || (_Owner == player))
        {
            _Owner = player;

            return true;
        }

        if ((abortHandler != nullptr) && abortHandler->is_aborting())
            return false;

        if ((::timeGetTime() - StartTime) >= timeoutInMS)
            return false;

        _OwnerChanged.wait_for(Lock, std::chrono::milliseconds(50));
    }

    return false;
}

/// <summary>
/// Lets another player send to the device. Stops the notes the player left sounding.
/// </summary>
void MCIPlayer::Connection::DropOwnership(const MCIPlayer * player) noexcept
{
    {
        std::lock_guard<std::mutex> Lock(_Mutex);

        if (_Owner != player)
            return;

        SilenceUnlocked();

        _Owner = nullptr;
    }

    _OwnerChanged.notify_all();
}

/// <summary>
/// Sends a short message to the device.
/// </summary>
void MCIPlayer::Connection::Send(uint32_t message) noexcept
{
    std::lock_guard<std::mutex> Lock(_Mutex);

    if (_hDevice != 0)
        ::midiOutShortMsg(_hDevice, (DWORD) message);
}

/// <summary>
/// Sends a system exclusive message to the device.
/// </summary>
void MCIPlayer::Connection::SendSysEx(const uint8_t * data, size_t size) noexcept
{
    std::lock_guard<std::mutex> Lock(_Mutex);

    if ((_hDevice == 0) || (data == nullptr) || (size == 0))
        return;

    _SysEx.assign(data, data + size);

    MIDIHDR Header = { };

    Header.lpData         = (LPSTR) _SysEx.data();
    Header.dwBufferLength = (DWORD) _SysEx.size();

    MMRESULT Result = ::midiOutPrepareHeader(_hDevice, &Header, sizeof(Header));

    if (Result != MMSYSERR_NOERROR)
    {
        LogError(Result, "midiOutPrepareHeader");

        return;
    }

    Result = ::midiOutLongMsg(_hDevice, &Header, sizeof(Header));

    if (Result != MMSYSERR_NOERROR)
        LogError(Result, "midiOutLongMsg");
    else
    {
        // The buffer has to stay put until the device is done with it.
        for (uint32_t i = 0; ((Header.dwFlags & MHDR_DONE) == 0) && (i < 200); ++i)
            ::Sleep(5);
    }

    ::midiOutUnprepareHeader(_hDevice, &Header, sizeof(Header));
}

/// <summary>
/// Stops every note that is sounding on the device.
/// </summary>
void MCIPlayer::Connection::Silence() noexcept
{
    std::lock_guard<std::mutex> Lock(_Mutex);

    SilenceUnlocked();
}

/// <summary>
/// Stops every note that is sounding on the device. The caller holds the lock.
/// </summary>
void MCIPlayer::Connection::SilenceUnlocked() noexcept
{
    if (_hDevice == 0)
        return;

    for (uint8_t Channel = 0; Channel < 16; ++Channel)
    {
        ::midiOutShortMsg(_hDevice, (DWORD) (0xB0u | Channel) | (120u << 8)); // CC 120 All Sound Off
        ::midiOutShortMsg(_hDevice, (DWORD) (0xB0u | Channel) | (123u << 8)); // CC 123 All Notes Off
        ::midiOutShortMsg(_hDevice, (DWORD) (0xB0u | Channel) | ( 64u << 8)); // CC  64 Hold Pedal off
    }
}

/// <summary>
/// Closes the device. The caller holds the lock.
/// </summary>
void MCIPlayer::Connection::Close() noexcept
{
    if (_hDevice == 0)
        return;

    SilenceUnlocked();

    ::midiOutReset(_hDevice);
    ::midiOutClose(_hDevice);

    _hDevice = 0;
    _Owner = nullptr;

    ::timeEndPeriod(1);

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " closed MIDI output device \"%s\".", _DeviceName.c_str());
}

/// <summary>
/// Writes a multimedia error to the log.
/// </summary>
void MCIPlayer::Connection::LogError(MMRESULT result, const char * functionName) noexcept
{
    if (result == MMSYSERR_NOERROR)
        return;

    CHAR Text[MAXERRORLENGTH] = { };

    if (::midiOutGetErrorTextA(result, Text, _countof(Text)) != MMSYSERR_NOERROR)
        ::strncpy_s(Text, "Unknown error", _TRUNCATE);

    Log.AtError().Write(STR_COMPONENT_BASENAME " MIDI Out player: %s failed. %s", functionName, Text);
}

#pragma endregion

#pragma region Public

MCIPlayer::MCIPlayer() noexcept
:
    player_t(),
    _DeviceId(),
    _AbortHandler(nullptr),
    _FrameIndex(),
    _StartTime(),
    _IsWatchdogRunning(false),
    _LastRenderTime(),
    _IsSilenced(false)
{
}

MCIPlayer::~MCIPlayer()
{
    Shutdown();

    if (_Connection)
    {
        _Connection->Release();
        _Connection.reset();
    }
}

/// <summary>
/// Gets the number of MIDI output devices.
/// </summary>
uint32_t MCIPlayer::GetDeviceCount() noexcept
{
    return (uint32_t) ::midiOutGetNumDevs();
}

/// <summary>
/// Gets the name of a MIDI output device. Returns an empty string when the device does not exist.
/// </summary>
std::string MCIPlayer::GetDeviceName(uint32_t deviceId) noexcept
{
    MIDIOUTCAPSW moc = { };

    if (::midiOutGetDevCapsW((UINT_PTR) deviceId, &moc, sizeof(moc)) != MMSYSERR_NOERROR)
        return { };

    const int Size = ::WideCharToMultiByte(CP_UTF8, 0, moc.szPname, -1, nullptr, 0, nullptr, nullptr);

    if (Size <= 1)
        return { };

    std::string Name((size_t) (Size - 1), '\0');

    ::WideCharToMultiByte(CP_UTF8, 0, moc.szPname, -1, Name.data(), Size, nullptr, nullptr);

    return Name;
}

/// <summary>
/// Closes every device connection. Called when foobar2000 quits.
/// </summary>
void MCIPlayer::CloseAllConnections() noexcept
{
    Connection::CloseAll();
}

#pragma endregion

#pragma region player_t

/// <summary>
/// Starts the player.
/// </summary>
bool MCIPlayer::Startup()
{
    if (_IsStarted)
        return true;

    if (::midiOutGetNumDevs() == 0)
    {
        _ErrorMessage = "No MIDI output devices are available.";

        return false;
    }

    if (!_Connection)
    {
        _Connection = Connection::Acquire(_DeviceId, _ErrorMessage);

        if (!_Connection)
            return false;
    }

    // foobar2000 decodes the next track before the current one has finished. Wait until that one is done with the device.
    if (!_Connection->TakeOwnership(this, _AbortHandler, OwnershipTimeoutInMS))
    {
        _ErrorMessage = "Gave up waiting for MIDI output device \"" + _Connection->GetDeviceName() + "\". Another track is still using it.";

        return false;
    }

    _FrameIndex = 0;
    _StartTime = ::timeGetTime();
    _IsSilenced = false;
    _LastRenderTime = _StartTime;

    _IsStarted = true;

    StartWatchdog();

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is sending MIDI to \"%s\".", _Connection->GetDeviceName().c_str());

    return true;
}

/// <summary>
/// Shuts the player down.
/// </summary>
void MCIPlayer::Shutdown()
{
    StopWatchdog();

    if (_Connection)
        _Connection->DropOwnership(this);

    _IsStarted = false;
}

/// <summary>
/// Resets the player. Called when seeking backwards.
/// </summary>
bool MCIPlayer::Reset()
{
    if (!_IsStarted || !_Connection)
        return false;

    _Connection->Silence();

    _FrameIndex = 0;
    _StartTime = ::timeGetTime();
    _IsSilenced = false;
    _LastRenderTime = _StartTime;

    return true;
}

/// <summary>
/// Renders a chunk of audio samples. The device produces the audio so all this returns is silence.
/// </summary>
/// <remarks>
/// The base class renders the frames between two events before it sends the second one, so waiting here
/// until the sample position catches up with the clock puts every event on the device at the right moment.
/// </remarks>
void MCIPlayer::Render(audio_sample * frameData, uint32_t frameCount)
{
    const uint32_t ChannelCount = GetAudioChannelCount();

    if ((frameData != nullptr) && (frameCount != 0))
        ::memset(frameData, 0, ((size_t) frameCount * ChannelCount) * sizeof(audio_sample));

    _LastRenderTime = ::timeGetTime();

    if (_IsSilenced.exchange(false) && _Connection)
    {
        // The watchdog silenced the device while playback was stalled. Reset the controllers so that the
        // notes that follow are not affected by the All Sound Off that was sent.
        for (uint8_t Channel = 0; Channel < 16; ++Channel)
            _Connection->Send((uint32_t) (0xB0u | Channel) | (121u << 8));   // CC 121 Reset All Controllers
    }

    _FrameIndex += frameCount;

    WaitForPlayingTime();
}

/// <summary>
/// Sends a MIDI event to the device.
/// </summary>
void MCIPlayer::SendEvent(uint32_t data)
{
    if (_Connection)
        _Connection->Send(data & 0x00FFFFFFu);  // The most significant byte holds the port number which this player does not use.
}

/// <summary>
/// Sends a system exclusive message to the device.
/// </summary>
void MCIPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    if (_Connection)
        _Connection->SendSysEx(data, size);
}

#pragma endregion

#pragma region Private

/// <summary>
/// Gets the position in the sample stream, in ms.
/// </summary>
uint32_t MCIPlayer::GetPlayingTime() const noexcept
{
    if (_SampleRate == 0)
        return 0;

    return (uint32_t) ((_FrameIndex * 1'000ull) / (uint64_t) _SampleRate);
}

/// <summary>
/// Waits until the clock catches up with the position in the sample stream.
/// </summary>
/// <remarks>
/// foobar2000 pulls audio from a decoder as fast as its output buffer allows. Because this player renders
/// silence there is nothing to slow it down, so without this the whole song would be sent to the device in
/// an instant. Blocking the decoder thread keeps the position indicator, Stop and track changes in step
/// with what can be heard.
/// </remarks>
void MCIPlayer::WaitForPlayingTime() noexcept
{
    if (!_IsStarted)
        return;

    const uint32_t Target = _StartTime + GetPlayingTime();

    for (uint32_t Waited = 0; Waited < MaxWaitInMS; Waited += 5)
    {
        const uint32_t Now = ::timeGetTime();

        // Casting to a signed value keeps the comparison correct when timeGetTime() wraps around.
        const int32_t Remaining = (int32_t) (Target - Now);

        if (Remaining <= 0)
        {
            // Playback was paused or the machine stalled. Give up the time that was lost instead of
            // racing through the events that should have been played while we were not running.
            if (Remaining < -((int32_t) MaxDriftInMS))
                _StartTime += (uint32_t) (-Remaining);

            return;
        }

        if ((_AbortHandler != nullptr) && _AbortHandler->is_aborting())
            return;

        ::Sleep((DWORD) std::min(Remaining, 5));

        _LastRenderTime = ::timeGetTime();
    }
}

/// <summary>
/// Starts the thread that silences the device when the decoder stops feeding us.
/// </summary>
/// <remarks>
/// foobar2000 does not tell a decoder that playback was paused. It just stops asking for audio, which would
/// leave whatever the device was playing sounding until playback resumes. This notices that and stops the notes.
/// </remarks>
void MCIPlayer::StartWatchdog()
{
    if (_IsWatchdogRunning)
        return;

    _IsWatchdogRunning = true;

    try
    {
        _Watchdog = std::thread([this]()
        {
            while (_IsWatchdogRunning)
            {
                ::Sleep(50);

                if (!_IsWatchdogRunning || _IsSilenced)
                    continue;

                const uint32_t Now  = ::timeGetTime();
                const uint32_t Then = _LastRenderTime.load();

                if ((Now - Then) < WatchdogTimeoutInMS)
                    continue;

                _IsSilenced = true;

                if (_Connection)
                    _Connection->Silence();
            }
        });
    }
    catch (...)
    {
        _IsWatchdogRunning = false;
    }
}

/// <summary>
/// Stops the watchdog thread.
/// </summary>
void MCIPlayer::StopWatchdog() noexcept
{
    _IsWatchdogRunning = false;

    if (_Watchdog.joinable())
    {
        try
        {
            _Watchdog.join();
        }
        catch (...)
        {
        }
    }
}

#pragma endregion
