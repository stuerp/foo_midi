
/** $VER: MCIPlayer.cpp (2026.09.02) P. Stuer - Implements a player that sends to a Windows MIDI output port **/

#include "pch.h"

#include "MCIPlayer.h"

#include "Resource.h"
#include "Log.h"

#include <algorithm>

#pragma comment(lib, "winmm.lib")

#pragma region Public

MCIPlayer::MCIPlayer() noexcept
:
    player_t(),
    _DeviceId(),
    _hDevice(),
    _IsTimerResolutionSet(false),
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

    // Some drivers refuse a connection for a while after the previous client disconnected. The VST MIDI
    // Synth driver does that whenever it is unloading a plug-in, which takes seconds with a large one.
    MMRESULT Result = MMSYSERR_NOERROR;

    for (uint32_t Attempt = 0; Attempt < OpenAttempts; ++Attempt)
    {
        Result = ::midiOutOpen(&_hDevice, _DeviceId, 0, 0, CALLBACK_NULL);

        if (Result == MMSYSERR_NOERROR)
            break;

        _hDevice = 0;

        ::Sleep(OpenRetryIntervalInMS);
    }

    if (Result != MMSYSERR_NOERROR)
    {
        LogMessage(Result, "midiOutOpen");

        _ErrorMessage = "Failed to open MIDI output device \"" + GetDeviceName(_DeviceId) + "\". It may be in use by another application, or still releasing a plug-in from a previous track.";

        Log.AtError().Write(STR_COMPONENT_BASENAME " could not open a MIDI output device. %s", _ErrorMessage.c_str());

        _hDevice = 0;

        return false;
    }

    // Ask for a 1 ms timer so that the waits between events are accurate.
    _IsTimerResolutionSet = (::timeBeginPeriod(1) == TIMERR_NOERROR);

    _FrameIndex = 0;
    _StartTime = ::timeGetTime();
    _IsSilenced = false;
    _LastRenderTime = _StartTime;

    _IsStarted = true;

    StartWatchdog();

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is sending MIDI to \"%s\".", GetDeviceName(_DeviceId).c_str());

    return true;
}

/// <summary>
/// Shuts the player down.
/// </summary>
void MCIPlayer::Shutdown()
{
    StopWatchdog();   // Before taking the lock: the watchdog takes it too.

    {
        std::lock_guard<std::mutex> Lock(_DeviceMutex);

        if (_hDevice != 0)
        {
            SilenceDevice();

            ::midiOutReset(_hDevice);
            ::midiOutClose(_hDevice);

            _hDevice = 0;
        }
    }

    if (_IsTimerResolutionSet)
    {
        ::timeEndPeriod(1);

        _IsTimerResolutionSet = false;
    }

    _IsStarted = false;
}

/// <summary>
/// Resets the player. Called when seeking backwards.
/// </summary>
bool MCIPlayer::Reset()
{
    if (!_IsStarted || (_hDevice == 0))
        return false;

    {
        std::lock_guard<std::mutex> Lock(_DeviceMutex);

        SilenceDevice();
    }

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

    if (_IsSilenced.exchange(false))
    {
        // The watchdog silenced the device while playback was stalled. Reset the controllers so that the
        // notes that follow are not affected by the All Sound Off that was sent.
        for (uint8_t Channel = 0; Channel < 16; ++Channel)
            Send((uint32_t) (0xB0u | Channel) | (121u << 8));   // CC 121 Reset All Controllers
    }

    _FrameIndex += frameCount;

    WaitForPlayingTime();
}

/// <summary>
/// Sends a MIDI event to the device.
/// </summary>
void MCIPlayer::SendEvent(uint32_t data)
{
    // The most significant byte holds the port number which this player does not use.
    Send(data & 0x00FFFFFFu);
}

/// <summary>
/// Sends a system exclusive message to the device.
/// </summary>
void MCIPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    if ((_hDevice == 0) || (data == nullptr) || (size == 0))
        return;

    _SysEx.assign(data, data + size);

    std::lock_guard<std::mutex> Lock(_DeviceMutex);

    if (_hDevice == 0)
        return;

    MIDIHDR Header = { };

    Header.lpData         = (LPSTR) _SysEx.data();
    Header.dwBufferLength = (DWORD) _SysEx.size();

    MMRESULT Result = ::midiOutPrepareHeader(_hDevice, &Header, sizeof(Header));

    if (Result != MMSYSERR_NOERROR)
    {
        LogMessage(Result, "midiOutPrepareHeader");

        return;
    }

    Result = ::midiOutLongMsg(_hDevice, &Header, sizeof(Header));

    if (Result != MMSYSERR_NOERROR)
        LogMessage(Result, "midiOutLongMsg");
    else
    {
        // The buffer has to stay put until the device is done with it.
        for (uint32_t i = 0; ((Header.dwFlags & MHDR_DONE) == 0) && (i < 200); ++i)
            ::Sleep(5);
    }

    ::midiOutUnprepareHeader(_hDevice, &Header, sizeof(Header));
}

#pragma endregion

#pragma region Private

/// <summary>
/// Sends a short message to the device.
/// </summary>
void MCIPlayer::Send(uint32_t message) noexcept
{
    std::lock_guard<std::mutex> Lock(_DeviceMutex);

    if (_hDevice != 0)
        ::midiOutShortMsg(_hDevice, (DWORD) message);
}

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

        ::Sleep((DWORD) std::min(Remaining, 5));

        _LastRenderTime = ::timeGetTime();
    }
}

/// <summary>
/// Stops every note that is sounding on the device. The caller must hold the device lock.
/// </summary>
void MCIPlayer::SilenceDevice() noexcept
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

                std::lock_guard<std::mutex> Lock(_DeviceMutex);

                SilenceDevice();
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

/// <summary>
/// Writes a multimedia error to the log.
/// </summary>
void MCIPlayer::LogMessage(MMRESULT result, const char * functionName) const noexcept
{
    if (result == MMSYSERR_NOERROR)
        return;

    CHAR Text[MAXERRORLENGTH] = { };

    if (::midiOutGetErrorTextA(result, Text, _countof(Text)) != MMSYSERR_NOERROR)
        ::strncpy_s(Text, "Unknown error", _TRUNCATE);

    Log.AtError().Write(STR_COMPONENT_BASENAME " MIDI Out player: %s failed. %s", functionName, Text);
}

#pragma endregion
