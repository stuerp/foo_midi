
/** OPN Player **/

#include "OPNPlayer.h"

#include "Tomsoft.wopn.h"
#include "fmmidi.wopn.h"
#include "gems-fmlib-gmize.wopn.h"
#include "gs-by-papiezak-and-sneakernets.wopn.h"
#include "xg.wopn.h"

OPNPlayer::OPNPlayer() : MIDIPlayer()
{
    ::memset(_Player, 0, sizeof(_Player));
}

OPNPlayer::~OPNPlayer()
{
    Shutdown();
}

bool OPNPlayer::Startup()
{
    if (_Player[0] && _Player[1] && _Player[2])
        return true;

    int chips_per_port = (int)_ChipCount / 3;
    int chips_round = (_ChipCount % 3) != 0;
    int chips_min = _ChipCount < 3;

    for (size_t i = 0; i < 3; i++)
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

        ::opn2_openBankData(Player, Bank, (long)BankSize);

        ::opn2_setNumChips(Player, chips_per_port + chips_round * (i == 0) + chips_min * (i != 0));
        ::opn2_setSoftPanEnabled(Player, _FullPanning);
        ::opn2_setDeviceIdentifier(Player, (unsigned int)i);
        ::opn2_switchEmulator(Player, (int)_EmuCore);
        ::opn2_reset(Player);
    }

    _IsInitialized = true;

    SetFilter(_FilterType, _FilterEffects);

    return true;
}

void OPNPlayer::Shutdown()
{
    for (size_t i = 0; i < 3; i++)
    {
        ::opn2_close(_Player[i]);
        _Player[i] = nullptr;
    }

    _IsInitialized = false;
}

void OPNPlayer::Render(audio_sample * out, unsigned long count)
{
    int16_t buffer[256 * sizeof(audio_sample)];

    while (count)
    {
        size_t todo = count;

        if (todo > 256)
            todo = 256;

        ::memset(out, 0, (todo * 2) * sizeof(audio_sample));

        for (size_t i = 0; i < 3; i++)
        {
            ::opn2_generate(_Player[i], (int)(todo * 2), buffer);

            for (size_t j = 0, k = (todo * 2); j < k; j++)
                out[j] += (audio_sample) buffer[j] * (1.0f / 32768.0f);
        }

        out += (todo * 2);
        count -= (unsigned long)todo;
    }
}

void OPNPlayer::setCore(unsigned emuCore)
{
    _EmuCore = emuCore;
}

void OPNPlayer::setBank(unsigned bankNumber)
{
    _BankNumber = bankNumber;
}

void OPNPlayer::setChipCount(unsigned chipCount)
{
    _ChipCount = chipCount;
}

void OPNPlayer::setFullPanning(bool enabled)
{
    _FullPanning = enabled;
}

void OPNPlayer::SendEvent(uint32_t message)
{
    OPN2_UInt8 Event[3]
    {
        static_cast<OPN2_UInt8>(message),
        static_cast<OPN2_UInt8>(message >> 8),
        static_cast<OPN2_UInt8>(message >> 16)
    };

    unsigned Port = (message >> 24) & 3;

    const OPN2_UInt8 Channel = message & 0x0F;
    const OPN2_UInt8 Command = message & 0xF0;

    if (Port > 2)
        Port = 0;

    switch (Command)
    {
        case 0x80:
            ::opn2_rt_noteOff(_Player[Port], Channel, Event[1]);
            break;

        case 0x90:
            ::opn2_rt_noteOn(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xA0:
            ::opn2_rt_noteAfterTouch(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xB0:
            ::opn2_rt_controllerChange(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xC0:
            ::opn2_rt_patchChange(_Player[Port], Channel, Event[1]);
            break;

        case 0xD0:
            ::opn2_rt_channelAfterTouch(_Player[Port], Channel, Event[1]);
            break;

        case 0xE0:
            ::opn2_rt_pitchBendML(_Player[Port], Channel, Event[2], Event[1]);
            break;
    }
}

void OPNPlayer::SendSysEx(const uint8_t * event, size_t size, size_t port)
{
    if (port >= 3)
        port = 0;

    ::opn2_rt_systemExclusive(_Player[port], event, size);

    if (port == 0)
    {
        ::opn2_rt_systemExclusive(_Player[1], event, size);
        ::opn2_rt_systemExclusive(_Player[2], event, size);
    }
}
