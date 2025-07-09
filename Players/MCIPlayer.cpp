
/** $VER: MCIPlayer.cpp (2025.07.09) P. Stuer - Implements a player using the Windows MCI API **/

#include "pch.h"

#include "MCIPlayer.h"

#include "Resource.h"

#pragma region Public

MCIPlayer::MCIPlayer() noexcept : player_t(), _DeviceId(), _hStream()
{
}

MCIPlayer::~MCIPlayer()
{
    Shutdown();
}

#pragma endregion

#pragma region MIDIPlayer

/// <summary>
/// Starts the player.
/// </summary>
bool MCIPlayer::Startup()
{
    if (_IsStarted)
        return true;

    UINT DeviceCount = ::midiOutGetNumDevs();

    if (DeviceCount == 0)
        return false;

    MIDIOUTCAPS moc = { };

    MMRESULT Result = ::midiOutGetDevCapsW(MIDI_MAPPER, &moc, sizeof(moc));

    if (Result != MMSYSERR_NOERROR)
        return false;

    Result = ::midiStreamOpen(&_hStream, &_DeviceId, 1, NULL, NULL, CALLBACK_NULL); LogMessage(Result);

    if (Result != MMSYSERR_NOERROR)
        return false;

    _IsStarted = true;

    return true;
}

/// <summary>
/// Shuts the player down.
/// </summary>
void MCIPlayer::Shutdown()
{
    if (_hStream != 0)
    {
        MMRESULT Result = ::midiStreamClose(_hStream); LogMessage(Result);
        _hStream = 0;
    }

    _IsStarted = false;
}

/// <summary>
/// Renders a chunk of audio samples.
/// </summary>
void MCIPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    const uint32_t MaxChannels = 2;

    ::memset(dstFrames, 0, ((size_t) dstCount * MaxChannels) * sizeof(audio_sample));

    if (_Events.empty())
        return;

    MIDIHDR mh =
    {
        .lpData = (LPSTR) _Events.data(),
        .dwBufferLength = (DWORD) _Events.size(),
        .dwBytesRecorded = mh.dwBufferLength,
        .dwFlags = 0
    };

    MMRESULT Result = ::midiOutPrepareHeader((HMIDIOUT) _hStream, &mh, sizeof(MIDIHDR));

    Result = ::midiStreamOut(_hStream, &mh, sizeof(mh));

    Result = ::midiStreamRestart(_hStream);

    Result = ::midiOutUnprepareHeader((HMIDIOUT) _hStream, &mh, sizeof(MIDIHDR));

    _Events.clear();
}

/// <summary>
/// Sends a MIDI event to the device.
/// </summary>
void MCIPlayer::SendEvent(uint32_t data)
{
    auto Status     = (uint8_t) (data);
    auto Data1      = (uint8_t) (data >>  8);
    auto Data2      = (uint8_t) (data >> 16);
//  auto PortNumber = (uint8_t) (data >> 24);

    const MIDIEVENT me =
    {
        .dwDeltaTime = 0,
        .dwStreamID  = 0,
        .dwEvent     = (DWORD) (MEVT_SHORTMSG << 24) | (Status | (Data1 << 8) | (Data2 << 16))
    };

    _Events.push_back(me);
}

/// <summary>
/// Sends a SysEx to the device.
/// </summary>
void MCIPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    _Header.lpData = (LPSTR) data;
    _Header.dwBufferLength = (DWORD) size;
    _Header.dwFlags = 0;
/*
    MMRESULT Result = ::midiOutPrepareHeader(_hDevice, &_Header, sizeof(_Header)); LogMessage(Result);

    if (Result == MMSYSERR_NOERROR)
    {
        Result = ::midiOutLongMsg(_hDevice, &_Header, sizeof _Header);
        LogMessage(Result);
    }

    Result = ::midiOutUnprepareHeader(_hDevice, &_Header, sizeof(_Header)); LogMessage(Result);
*/
}

#pragma endregion

#pragma region Private

void MCIPlayer::LogMessage(MMRESULT result) const
{
    if (result == MMSYSERR_NOERROR)
        return;

    CHAR Text[256];

    ::midiOutGetErrorTextA(result, Text, _countof(Text));

    console::print(STR_COMPONENT_BASENAME, " MCI Player reports ", Text);
}

#pragma endregion
