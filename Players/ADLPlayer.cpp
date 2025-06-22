
/** $VER: ADLPlayer.cpp (2025.06.22) **/

#include "pch.h"

#include "ADLPlayer.h"

#include "Encoding.h"

ADLPlayer::ADLPlayer() : player_t()
{
    ::memset(_Player, 0, sizeof(_Player));
}

ADLPlayer::~ADLPlayer()
{
    Shutdown();
}

/// <summary>
/// Sets the number of emulated chips (from 1 to 100). Emulation of multiple chips extends polyphony limits.
/// </summary>
void ADLPlayer::SetChipCount(uint32_t chipCount)
{
    if (chipCount < 1 || chipCount > 100)
        throw std::out_of_range(::FormatText("Invalid chip count %u. Should be between 1 and 100", chipCount));

    _ChipCount = chipCount;
}

/// <summary>
/// Sets the number of 4-operator channels between all chips.
/// </summary>
void ADLPlayer::Set4OpChannelCount(uint32_t fourOpChannelCount) noexcept
{
    _4OpChannelCount = fourOpChannelCount;
}

/// <summary>
/// Sets the emulator core to use.
/// </summary>
void ADLPlayer::SetEmulatorCore(uint32_t emulatorCore) noexcept
{
    _EmulatorCore = emulatorCore;
}

/// <summary>
/// Sets the index of the selected patches bank (0 to adl_getBanksCount()).
/// </summary>
void ADLPlayer::SetBankNumber(uint32_t bankNumber)
{
    if (bankNumber >= (uint32_t) ::adl_getBanksCount())
        throw std::out_of_range(::FormatText("Invalid bank number %u. Must be less than %u", bankNumber, ::adl_getBanksCount()));

    _BankNumber = bankNumber;
}

/// <summary>
/// Enables or disables soft panning with chip emulators.
/// </summary>
void ADLPlayer::SetFullPanning(bool enabled) noexcept
{
    _FullPanning = enabled;
}

/// <summary>
/// Sets the file path of the WOPL bank file.
/// </summary>
void ADLPlayer::SetBankFilePath(const std::string & filePath) noexcept
{
    _BankFilePath = filePath;
}

bool ADLPlayer::Startup()
{
    if (_Player[0] && _Player[1] && _Player[2])
        return true;

    const int ChipsPerPort = (int) (_ChipCount / _countof(_Player));
    const int ChipsRound   = (_ChipCount % _countof(_Player)) != 0;
    const int ChipsMin     = _ChipCount < _countof(_Player);

    for (size_t i = 0; i < _countof(_Player); ++i)
    {
        ADL_MIDIPlayer * Player = _Player[i] = ::adl_init((long) _SampleRate);

        if (Player == nullptr)
            return false;

        if (_BankFilePath.empty() || (::adl_openBankFile(Player, _BankFilePath.c_str()) == -1))
            ::adl_setBank(Player, (int) _BankNumber);

        ::adl_setNumChips(Player, ChipsPerPort + ChipsRound * (i == 0) + ChipsMin * (i != 0));
        ::adl_setNumFourOpsChn(Player, (int) _4OpChannelCount);
        ::adl_setSoftPanEnabled(Player, _FullPanning);
        ::adl_setDeviceIdentifier(Player, (unsigned int) i); // Set 4-bit device identifier. Used by the SysEx processor.
        ::adl_switchEmulator(Player, (int) _EmulatorCore);
        ::adl_reset(Player);
    }

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void ADLPlayer::Shutdown()
{
    for (size_t i = 0; i < _countof(_Player); ++i)
    {
        ::adl_close(_Player[i]);
        _Player[i] = nullptr;
    }

    _IsInitialized = false;
}

void ADLPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    int16_t Data[256 * sizeof(audio_sample)];

    while (sampleCount != 0)
    {
        uint32_t ToDo = sampleCount;

        if (ToDo > 256)
            ToDo = 256;

        ::memset(sampleData, 0, ((size_t) ToDo * 2) * sizeof(audio_sample));

        for (size_t i = 0; i < _countof(_Player); ++i)
        {
            ::adl_generate(_Player[i], (int) (ToDo * 2), Data);

            // Convert the format of the rendered output.
            for (size_t j = 0; j < ((size_t) ToDo * 2); ++j)
                sampleData[j] += (audio_sample) Data[j] * (1.0f / 32768.0f);
        }

        sampleData += (ToDo * 2);
        sampleCount -= ToDo;
    }
}

void ADLPlayer::SendEvent(uint32_t message)
{
    ADL_UInt8 Event[3]
    {
        (ADL_UInt8) (message),
        (ADL_UInt8) (message >>  8),
        (ADL_UInt8) (message >> 16)
    };

    size_t Port = (message >> 24) & _countof(_Player);

    const ADL_UInt8 Status = message & 0xF0;
    const ADL_UInt8 Channel = message & 0x0F;

    if (Port > 2)
        Port = 0;

    switch (Status)
    {
        case 0x80:
            ::adl_rt_noteOff(_Player[Port], Channel, Event[1]);
            break;

        case 0x90:
            ::adl_rt_noteOn(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xA0:
            ::adl_rt_noteAfterTouch(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xB0:
            ::adl_rt_controllerChange(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xC0:
            ::adl_rt_patchChange(_Player[Port], Channel, Event[1]);
            break;

        case 0xD0:
            ::adl_rt_channelAfterTouch(_Player[Port], Channel, Event[1]);
            break;

        case 0xE0:
            ::adl_rt_pitchBendML(_Player[Port], Channel, Event[2], Event[1]);
            break;
    }
}

void ADLPlayer::SendSysEx(const uint8_t * event, size_t size, uint32_t)
{
    ::adl_rt_systemExclusive(_Player[0], event, size);
    ::adl_rt_systemExclusive(_Player[1], event, size);
    ::adl_rt_systemExclusive(_Player[2], event, size);
}
