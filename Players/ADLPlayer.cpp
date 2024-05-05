
/** $VER: ADLPlayer.cpp (2023.09.27) **/

#include "framework.h"

#include "ADLPlayer.h"

ADLPlayer::ADLPlayer() : MIDIPlayer()
{
    ::memset(_Player, 0, sizeof(_Player));
}

ADLPlayer::~ADLPlayer()
{
    Shutdown();
}

void ADLPlayer::SetCore(uint32_t emuCore)
{
    _EmuCore = emuCore;
}

void ADLPlayer::SetBank(uint32_t bankNumber)
{
    _BankNumber = bankNumber;
}

void ADLPlayer::SetChipCount(uint32_t chipCount)
{
    _ChipCount = chipCount;
}

void ADLPlayer::Set4OpCount(uint32_t count)
{
    _4OpCount = count;
}

void ADLPlayer::SetFullPanning(bool enabled)
{
    _FullPanning = enabled;
}

void ADLPlayer::SetBankFilePath(pfc::string8 filePath)
{
    _BankFilePath = filePath;
}

bool ADLPlayer::Startup()
{
    if (_Player[0] && _Player[1] && _Player[2])
        return true;

    int chips_per_port = (int)(_ChipCount / 3);
    int chips_round = (_ChipCount % 3) != 0;
    int chips_min = _ChipCount < 3;

    for (size_t i = 0; i < 3; i++)
    {
        ADL_MIDIPlayer * Player = this->_Player[i] = ::adl_init((long) _SampleRate);

        if (Player == nullptr)
            return false;

        if ((_BankFilePath.length() == 0) || (::adl_openBankFile(Player, _BankFilePath.c_str()) == -1))
            ::adl_setBank(Player, (int)_BankNumber);

        ::adl_setNumChips(Player, chips_per_port + chips_round * (i == 0) + chips_min * (i != 0));
        ::adl_setNumFourOpsChn(Player, (int) _4OpCount);
        ::adl_setSoftPanEnabled(Player, _FullPanning);
        ::adl_setDeviceIdentifier(Player, (unsigned int) i);
        ::adl_switchEmulator(Player, (int) _EmuCore);
        ::adl_reset(Player);
    }

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void ADLPlayer::Shutdown()
{
    for (size_t i = 0; i < 3; i++)
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

        for (size_t i = 0; i < 3; ++i)
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
        static_cast<ADL_UInt8>(message),
        static_cast<ADL_UInt8>(message >> 8),
        static_cast<ADL_UInt8>(message >> 16)
    };

    unsigned Port = (message >> 24) & 3;

    const ADL_UInt8 Channel = message & 0x0F;
    const ADL_UInt8 Command = message & 0xF0;

    if (Port > 2)
        Port = 0;

    switch (Command)
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
