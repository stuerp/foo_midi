
/** $VER: MCIPlayer.cpp (2026.09.02) P. Stuer - Implements a player that streams to a Windows MIDI output port **/

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
    _hStream(),
    _PendingTick(),
    _FrameIndex(),
    _LastEventTick(),
    _IsWatchdogRunning(false),
    _LastRenderTime(),
    _IsSilenced(false)
{
    _Pending.reserve(MaxBufferSize);

    for (auto & Buffer : _Buffers)
        Buffer.Data.resize(MaxBufferSize);
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

    UINT DeviceId = _DeviceId;

    MMRESULT Result = ::midiStreamOpen(&_hStream, &DeviceId, 1, 0, 0, CALLBACK_NULL);

    if (Result != MMSYSERR_NOERROR)
    {
        LogMessage(Result, "midiStreamOpen");

        _ErrorMessage = "Failed to open MIDI output device \"" + GetDeviceName(_DeviceId) + "\".";
        _hStream = 0;

        return false;
    }

    // Make 1 tick equal 1 ms so that event delta times can be expressed in milliseconds.
    {
        MIDIPROPTIMEDIV TimeDiv = { sizeof(MIDIPROPTIMEDIV), TicksPerQuarterNote };

        Result = ::midiStreamProperty(_hStream, (LPBYTE) &TimeDiv, MIDIPROP_SET | MIDIPROP_TIMEDIV);
        LogMessage(Result, "midiStreamProperty(MIDIPROP_TIMEDIV)");

        MIDIPROPTEMPO Tempo = { sizeof(MIDIPROPTEMPO), MicroSecondsPerQuarterNote };

        Result = ::midiStreamProperty(_hStream, (LPBYTE) &Tempo, MIDIPROP_SET | MIDIPROP_TEMPO);
        LogMessage(Result, "midiStreamProperty(MIDIPROP_TEMPO)");
    }

    _Pending.clear();
    _PendingTick = 0;
    _FrameIndex = 0;
    _LastEventTick = 0;
    _IsSilenced = false;
    _LastRenderTime = ::timeGetTime();

    Result = ::midiStreamRestart(_hStream);
    LogMessage(Result, "midiStreamRestart");

    _IsStarted = true;

    StartWatchdog();

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is streaming MIDI to \"%s\".", GetDeviceName(_DeviceId).c_str());

    return true;
}

/// <summary>
/// Shuts the player down.
/// </summary>
void MCIPlayer::Shutdown()
{
    StopWatchdog();

    if (_hStream != 0)
    {
        MMRESULT Result = ::midiStreamStop(_hStream);
        LogMessage(Result, "midiStreamStop");

        SilenceDevice();

        ReleaseBuffers(true);

        Result = ::midiStreamClose(_hStream);
        LogMessage(Result, "midiStreamClose");

        _hStream = 0;
    }

    _Pending.clear();
    _PendingTick = 0;
    _LastEventTick = 0;

    _IsStarted = false;
}

/// <summary>
/// Resets the player. Called when seeking backwards.
/// </summary>
bool MCIPlayer::Reset()
{
    if (!_IsStarted || (_hStream == 0))
        return false;

    // Drop everything that is still queued and stop the notes that it left sounding.
    MMRESULT Result = ::midiStreamStop(_hStream);
    LogMessage(Result, "midiStreamStop");

    SilenceDevice();

    ReleaseBuffers(true);

    _Pending.clear();
    _PendingTick = 0;
    _LastEventTick = 0;
    _FrameIndex = 0;
    _IsSilenced = false;
    _LastRenderTime = ::timeGetTime();

    Result = ::midiStreamRestart(_hStream);
    LogMessage(Result, "midiStreamRestart");

    return true;
}

/// <summary>
/// Renders a chunk of audio samples. The device produces the audio so all this returns is silence.
/// </summary>
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
            AppendShortMessage(0, (uint32_t) (0xB0 | Channel) | (121u << 8));  // CC 121 Reset All Controllers
    }

    _FrameIndex += frameCount;

    ReleaseBuffers(false);

    // Only send a buffer to the device once it holds a worthwhile amount of playing time.
    if (!_Pending.empty() && (((_LastEventTick - _PendingTick) >= FlushIntervalInMS) || (_Pending.size() >= (MaxBufferSize / 2))))
        Flush();

    ThrottleToDevice();
}

/// <summary>
/// Queues a MIDI event.
/// </summary>
void MCIPlayer::SendEvent(uint32_t data)
{
    const uint32_t Message = data & 0x00FFFFFFu;    // The most significant byte holds the port number which this player does not use.

    AppendShortMessage(GetDeltaTime(), Message);
}

/// <summary>
/// Queues a system exclusive message.
/// </summary>
void MCIPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    if ((data == nullptr) || (size == 0))
        return;

    AppendLongMessage(GetDeltaTime(), data, size);
}

#pragma endregion

#pragma region Private

/// <summary>
/// Gets the tick that corresponds with the current position in the sample stream.
/// </summary>
uint32_t MCIPlayer::GetCurrentTick() const noexcept
{
    if (_SampleRate == 0)
        return _LastEventTick;

    return (uint32_t) ((_FrameIndex * 1'000ull) / (uint64_t) _SampleRate);
}

/// <summary>
/// Gets the delta time, in ticks, between the previously queued event and the current position in the sample stream.
/// </summary>
uint32_t MCIPlayer::GetDeltaTime() noexcept
{
    const uint32_t Tick = GetCurrentTick();

    // The base class renders the frames between two events before it sends the second one, so the position never runs behind.
    const uint32_t DeltaTime = (Tick > _LastEventTick) ? (Tick - _LastEventTick) : 0;

    _LastEventTick = Tick;

    return DeltaTime;
}

/// <summary>
/// Adds a short message to the pending stream buffer.
/// </summary>
void MCIPlayer::AppendShortMessage(uint32_t deltaTime, uint32_t message)
{
    if (_hStream == 0)
        return;

    if ((_Pending.size() + sizeof(MIDIEVENT)) > MaxBufferSize)
        Flush();

    if (_Pending.empty())
        _PendingTick = _LastEventTick;

    const MIDIEVENT me =
    {
        .dwDeltaTime = deltaTime,
        .dwStreamID  = 0,
        .dwEvent     = (DWORD) ((MEVT_SHORTMSG << 24) | (message & 0x00FFFFFFu))
    };

    const uint8_t * Data = (const uint8_t *) &me;

    _Pending.insert(_Pending.end(), Data, Data + sizeof(MIDIEVENT));
}

/// <summary>
/// Adds a long message (system exclusive) to the pending stream buffer.
/// </summary>
void MCIPlayer::AppendLongMessage(uint32_t deltaTime, const uint8_t * data, size_t size)
{
    if (_hStream == 0)
        return;

    // The message data that follows the event header is padded to a DWORD boundary.
    const size_t PaddedSize = (size + 3) & ~(size_t) 3;
    const size_t TotalSize  = sizeof(MIDIEVENT) + PaddedSize;

    if (TotalSize > MaxBufferSize)
    {
        Log.AtWarn().Write(STR_COMPONENT_BASENAME " skipped a %d byte system exclusive message because it does not fit in a stream buffer.", (int) size);

        return;
    }

    if ((_Pending.size() + TotalSize) > MaxBufferSize)
        Flush();

    if (_Pending.empty())
        _PendingTick = _LastEventTick;

    const MIDIEVENT me =
    {
        .dwDeltaTime = deltaTime,
        .dwStreamID  = 0,
        .dwEvent     = (DWORD) ((MEVT_LONGMSG << 24) | (size & 0x00FFFFFFu))
    };

    const uint8_t * Header = (const uint8_t *) &me;

    _Pending.insert(_Pending.end(), Header, Header + sizeof(MIDIEVENT));
    _Pending.insert(_Pending.end(), data, data + size);
    _Pending.insert(_Pending.end(), PaddedSize - size, (uint8_t) 0);
}

/// <summary>
/// Sends the pending events to the device.
/// </summary>
bool MCIPlayer::Flush() noexcept
{
    if ((_hStream == 0) || _Pending.empty())
        return true;

    buffer_t * Buffer = GetFreeBuffer();

    if (Buffer == nullptr)
    {
        Log.AtWarn().Write(STR_COMPONENT_BASENAME " ran out of MIDI stream buffers and dropped %d bytes of events.", (int) _Pending.size());

        _Pending.clear();

        return false;
    }

    ::memcpy(Buffer->Data.data(), _Pending.data(), _Pending.size());

    Buffer->Header = { };
    Buffer->Header.lpData          = (LPSTR) Buffer->Data.data();
    Buffer->Header.dwBufferLength  = (DWORD) _Pending.size();
    Buffer->Header.dwBytesRecorded = (DWORD) _Pending.size();

    MMRESULT Result = ::midiOutPrepareHeader((HMIDIOUT) _hStream, &Buffer->Header, sizeof(MIDIHDR));

    if (Result != MMSYSERR_NOERROR)
    {
        LogMessage(Result, "midiOutPrepareHeader");

        _Pending.clear();

        return false;
    }

    Buffer->IsPrepared = true;

    Result = ::midiStreamOut(_hStream, &Buffer->Header, sizeof(MIDIHDR));

    if (Result != MMSYSERR_NOERROR)
    {
        LogMessage(Result, "midiStreamOut");

        ::midiOutUnprepareHeader((HMIDIOUT) _hStream, &Buffer->Header, sizeof(MIDIHDR));

        Buffer->IsPrepared = false;

        _Pending.clear();

        return false;
    }

    _Pending.clear();
    _PendingTick = _LastEventTick;

    return true;
}

/// <summary>
/// Gets a stream buffer that is not in use by the device, waiting a short while if necessary.
/// </summary>
MCIPlayer::buffer_t * MCIPlayer::GetFreeBuffer() noexcept
{
    for (uint32_t i = 0; i < 200; ++i)  // Waits up to about 1 s.
    {
        ReleaseBuffers(false);

        for (auto & Buffer : _Buffers)
        {
            if (!Buffer.IsPrepared)
                return &Buffer;
        }

        ::Sleep(5);
    }

    return nullptr;
}

/// <summary>
/// Unprepares the stream buffers that the device has finished with. Unprepares all of them when forced.
/// </summary>
void MCIPlayer::ReleaseBuffers(bool force) noexcept
{
    if (_hStream == 0)
        return;

    for (auto & Buffer : _Buffers)
    {
        if (!Buffer.IsPrepared)
            continue;

        if (force)
        {
            // midiStreamStop() marks every buffer that is still queued as done.
            for (uint32_t i = 0; ((Buffer.Header.dwFlags & MHDR_DONE) == 0) && (i < 200); ++i)
                ::Sleep(5);
        }
        else
        if ((Buffer.Header.dwFlags & MHDR_DONE) == 0)
            continue;

        ::midiOutUnprepareHeader((HMIDIOUT) _hStream, &Buffer.Header, sizeof(MIDIHDR));

        Buffer.IsPrepared = false;
    }
}

/// <summary>
/// Gets the position of the stream, in ms, since it was last restarted.
/// </summary>
uint32_t MCIPlayer::GetStreamPositionInMS() const noexcept
{
    if (_hStream == 0)
        return 0;

    MMTIME mmt = { };

    mmt.wType = TIME_MS;

    if (::midiStreamPosition(_hStream, &mmt, sizeof(mmt)) != MMSYSERR_NOERROR)
        return 0;

    return (mmt.wType == TIME_MS) ? (uint32_t) mmt.u.ms : 0;
}

/// <summary>
/// Waits until the events that have been queued are close enough to what the device is playing.
/// </summary>
/// <remarks>
/// foobar2000 pulls audio from a decoder as fast as its output buffer allows. Because this player renders
/// silence there is nothing to slow it down, so without throttling the whole song would be queued at once and
/// the transport controls would bear no relation to what can be heard. Blocking the decoder thread here keeps
/// the position indicator, Stop and track changes within a fraction of a second of the device.
/// </remarks>
void MCIPlayer::ThrottleToDevice() noexcept
{
    if ((_hStream == 0) || !_IsStarted)
        return;

    for (uint32_t i = 0; i < 400; ++i)  // Waits up to about 4 s.
    {
        const uint32_t Position = GetStreamPositionInMS();

        if (_LastEventTick <= Position)
            break;

        const uint32_t Lead = _LastEventTick - Position;

        if (Lead <= TargetLeadInMS)
            break;

        ::Sleep(std::min(Lead - TargetLeadInMS, 10u));
    }
}

/// <summary>
/// Stops every note that is sounding on the device.
/// </summary>
void MCIPlayer::SilenceDevice() noexcept
{
    if (_hStream == 0)
        return;

    const HMIDIOUT hOut = (HMIDIOUT) _hStream;

    for (uint8_t Channel = 0; Channel < 16; ++Channel)
    {
        ::midiOutShortMsg(hOut, (DWORD) (0xB0u | Channel) | (120u << 8)); // CC 120 All Sound Off
        ::midiOutShortMsg(hOut, (DWORD) (0xB0u | Channel) | (123u << 8)); // CC 123 All Notes Off
        ::midiOutShortMsg(hOut, (DWORD) (0xB0u | Channel) | ( 64u << 8)); // CC  64 Hold Pedal off
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

                if (!_IsWatchdogRunning)
                    break;

                if (_IsSilenced)
                    continue;

                const uint32_t Now  = ::timeGetTime();
                const uint32_t Then = _LastRenderTime.load();

                if ((Now - Then) < WatchdogTimeoutInMS)
                    continue;

                // Wait until everything that was queued has been played. Nothing is going to switch those notes off.
                if (_LastEventTick > (GetStreamPositionInMS() + 10))
                    continue;

                _IsSilenced = true;

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
