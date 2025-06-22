
/** $VER: OPNPlayer.cpp (2025.06.22) **/

#include "pch.h"

#include "OPNPlayer.h"

#include "Tomsoft.wopn.h"
#include "fmmidi.wopn.h"
#include "gems-fmlib-gmize.wopn.h"
#include "gs-by-papiezak-and-sneakernets.wopn.h"
#include "xg.wopn.h"

#include "Encoding.h"

OPNPlayer::OPNPlayer() : player_t()
{
    ::memset(_Player, 0, sizeof(_Player));
}

OPNPlayer::~OPNPlayer()
{
    Shutdown();
}

/// <summary>
/// Sets the emulator core to use.
/// </summary>
void OPNPlayer::SetEmulatorCore(uint32_t emulatorCore)
{
    _EmulatorCore = emulatorCore;
}

/// <summary>
/// Sets the index of the selected patches bank (0 to opn_getBanksCount()).
/// </summary>
void OPNPlayer::SetBankNumber(uint32_t bankNumber)
{
    if (bankNumber > 4)
        throw std::out_of_range(::FormatText("Invalid bank number %u. Must be less than 5", bankNumber));

    _BankNumber = bankNumber;
}

/// <summary>
/// Sets the number of emulated chips (from 1 to 100). Emulation of multiple chips extends polyphony limits.
/// </summary>
void OPNPlayer::SetChipCount(unsigned chipCount)
{
    _ChipCount = chipCount;
}

/// <summary>
/// Enables or disables soft panning with chip emulators.
/// </summary>
void OPNPlayer::SetSoftPanning(bool enabled) noexcept
{
    _IsSoftPanningEnabled = enabled;
}

bool OPNPlayer::Startup()
{
    if (_Player[0] && _Player[1] && _Player[2])
        return true;

    const int ChipsPerPort = (int) (_ChipCount / _countof(_Player));
    const int ChipsRound = (_ChipCount % _countof(_Player)) != 0;
    const int ChipsMin = _ChipCount < _countof(_Player);

    for (size_t i = 0; i < _countof(_Player); ++i)
    {
        OPN2_MIDIPlayer * Player = this->_Player[i] = ::opn2_init((long)_SampleRate);
        
        if (Player == nullptr)
            return false;

        const void * Bank;
        size_t BankSize;

        switch (_BankNumber)
        {
            default:
            case 0:
                Bank = bnk_xg;
                BankSize = sizeof(bnk_xg);
                break;

            case 1:
                Bank = bnk_gs;
                BankSize = sizeof(bnk_gs);
                break;

            case 2:
                Bank = bnk_gems;
                BankSize = sizeof(bnk_gems);
                break;

            case 3:
                Bank = bnk_Tomsoft;
                BankSize = sizeof(bnk_Tomsoft);
                break;

            case 4:
                Bank = bnk_fmmidi;
                BankSize = sizeof(bnk_fmmidi);
                break;
        }

        ::opn2_openBankData(Player, Bank, (long) BankSize);

        ::opn2_setNumChips(Player, ChipsPerPort + ChipsRound * (i == 0) + ChipsMin * (i != 0));
        ::opn2_setSoftPanEnabled(Player, _IsSoftPanningEnabled);
        ::opn2_setDeviceIdentifier(Player, (unsigned int) i);
        ::opn2_switchEmulator(Player, (int) _EmulatorCore);
        ::opn2_reset(Player);
    }

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void OPNPlayer::Shutdown()
{
    for (size_t i = 0; i < _countof(_Player); ++i)
    {
        ::opn2_close(_Player[i]);
        _Player[i] = nullptr;
    }

    _IsInitialized = false;
}

void OPNPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
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
            ::opn2_generate(_Player[i], (int) (ToDo * 2), Data);

            // Convert the format of the rendered output.
            for (size_t j = 0, k = (ToDo * 2); j < k; j++)
                sampleData[j] += (audio_sample) Data[j] * (1.0f / 32768.0f);
        }

        sampleData += (ToDo * 2);
        sampleCount -= ToDo;
    }
}

void OPNPlayer::SendEvent(uint32_t data)
{
    OPN2_UInt8 Event[3]
    {
        static_cast<OPN2_UInt8>(data),
        static_cast<OPN2_UInt8>(data >>  8),
        static_cast<OPN2_UInt8>(data >> 16)
    };

    size_t Port = (data >> 24) & _countof(_Player);

    if (Port > (_countof(_Player) - 1))
        Port = 0;

    const OPN2_UInt8 Status = data & 0xF0;
    const OPN2_UInt8 Channel = data & 0x0F;

    switch (Status)
    {
        case midi::StatusCodes::NoteOff:
            ::opn2_rt_noteOff(_Player[Port], Channel, Event[1]);
            break;

        case midi::StatusCodes::NoteOn:
            ::opn2_rt_noteOn(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCodes::KeyPressure:
            ::opn2_rt_noteAfterTouch(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCodes::ControlChange:
            ::opn2_rt_controllerChange(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCodes::ProgramChange:
            ::opn2_rt_patchChange(_Player[Port], Channel, Event[1]);
            break;

        case midi::StatusCodes::ChannelPressure:
            ::opn2_rt_channelAfterTouch(_Player[Port], Channel, Event[1]);
            break;

        case midi::StatusCodes::PitchBendChange:
            ::opn2_rt_pitchBendML(_Player[Port], Channel, Event[2], Event[1]);
            break;
    }
}

void OPNPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    if (portNumber >= 3)
        portNumber = 0;

    ::opn2_rt_systemExclusive(_Player[portNumber], data, size);

    if (portNumber == 0)
    {
        ::opn2_rt_systemExclusive(_Player[1], data, size);
        ::opn2_rt_systemExclusive(_Player[2], data, size);
    }
}
