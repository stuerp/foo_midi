
/** $VER: MCIPlayer.cpp (2023.12.23) Implements a player using the Windows MCI API **/

#include "framework.h"

#include "MCIPlayer.h"

#define NOMINMAX

#pragma region Public

MCIPlayer::MCIPlayer() noexcept : player_t()
{
    _hDevice = 0;
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
    if (_hDevice != 0)
        return true;

    UINT DeviceCount = ::midiOutGetNumDevs();

    if (DeviceCount == 0)
        return false;

    MIDIOUTCAPS moc = { };

    MMRESULT Result = ::midiOutGetDevCapsW(MIDI_MAPPER, &moc, sizeof(moc));

    if (Result != MMSYSERR_NOERROR)
        return false;

    Result = ::midiOutOpen(&_hDevice, MIDI_MAPPER, NULL, NULL, CALLBACK_NULL); LogMessage(Result);

    if (Result != MMSYSERR_NOERROR)
        return false;

    return true;
}

/// <summary>
/// Shuts the player down.
/// </summary>
void MCIPlayer::Shutdown()
{
    if (_hDevice != 0)
    {
        MMRESULT Result = ::midiOutReset(_hDevice); LogMessage(Result);

        Result = ::midiOutClose(_hDevice); LogMessage(Result);
        _hDevice = 0;
    }
}

/// <summary>
/// Renders a chunk of audio samples.
/// </summary>
void MCIPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    ::memset(sampleData, 0, ((size_t) sampleCount * 2) * sizeof(audio_sample));
}

/// <summary>
/// Sends a MIDI event to the device.
/// </summary>
void MCIPlayer::SendEvent(uint32_t data)
{
    MMRESULT Result = ::midiOutShortMsg(_hDevice, data); LogMessage(Result);
}

/// <summary>
/// Sends a SysEx to the device.
/// </summary>
void MCIPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    _Header.lpData = (LPSTR) data;
    _Header.dwBufferLength = (DWORD) size;
    _Header.dwFlags = 0;

    MMRESULT Result = ::midiOutPrepareHeader(_hDevice, &_Header, sizeof(_Header)); LogMessage(Result);

    if (Result == MMSYSERR_NOERROR)
    {
        Result = ::midiOutLongMsg(_hDevice, &_Header, sizeof _Header);
        LogMessage(Result);
    }

    Result = ::midiOutUnprepareHeader(_hDevice, &_Header, sizeof(_Header)); LogMessage(Result);
}

#pragma endregion

#pragma region("Private")

void MCIPlayer::LogMessage(MMRESULT result) const
{
    if (result == MMSYSERR_NOERROR)
        return;

    CHAR Text[256];

    ::midiOutGetErrorTextA(result, Text, _countof(Text));

    pfc::outputDebugLine(Text);
}

#pragma endregion
