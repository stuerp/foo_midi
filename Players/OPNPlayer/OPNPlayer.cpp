
/** $VER: OPNPlayer.cpp (2025.07.08) **/

#include "pch.h"

#include "OPNPlayer.h"
#include "Resource.h"

#include "Tomsoft.wopn.h"
#include "fmmidi.wopn.h"
#include "gems-fmlib-gmize.wopn.h"
#include "gs-by-papiezak-and-sneakernets.wopn.h"
#include "xg.wopn.h"

#include "Encoding.h"

OPNPlayer::OPNPlayer() : player_t()
{
    ::memset(_Devices, 0, sizeof(_Devices));
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

/// <summary>
/// Sets the file path of the WOPN bank file.
/// </summary>
void OPNPlayer::SetBankFilePath(const std::string & filePath) noexcept
{
    _BankFilePath = filePath;
}

bool OPNPlayer::Startup()
{
    if (_IsStarted)
        return true;

    const int ChipsPerPort = (int) (_ChipCount / _countof(_Devices));
    const int ChipsRound = (_ChipCount % _countof(_Devices)) != 0;
    const int ChipsMin = _ChipCount < _countof(_Devices);

    for (size_t i = 0; i < _countof(_Devices); ++i)
    {
        auto * Device = ::opn2_init((long) _SampleRate);
        
        if (Device == nullptr)
        {
            _ErrorMessage = ::opn2_errorString();

            return false;
        }

    #ifdef _DEBUG
        ::opn2_setDebugMessageHook
        (
            Device,
            [](void * context, const char * format ...) noexcept
            {
                std::string Line; Line.resize(1024);

                std::va_list args;

                va_start(args, format);

                (void) ::vsnprintf(Line.data(), Line.size(), format, args);
                console::print(STR_COMPONENT_BASENAME " OPN Player says ", Line.c_str());

                va_end(args);
            },
            nullptr
        );
    #endif

        ::opn2_reset(Device);

        ::opn2_setSoftPanEnabled        (Device, _IsSoftPanningEnabled ? 1 : 0); // Use -1 for default value.
        ::opn2_setScaleModulators       (Device, -1); // -1 = default. Use 1 to turn on modulators scaling by volume.
        ::opn2_setFullRangeBrightness   (Device, -1); // -1 = default. Use 1 to turn on a full-ranged XG CC74 Brightness.
        ::opn2_setAutoArpeggio          (Device, -1); // -1 = default. Use 1 to turn on

        ::opn2_setVolumeRangeModel      (Device, OPNMIDI_VolumeModel_AUTO);
        ::opn2_setChannelAllocMode      (Device, OPNMIDI_ChanAlloc_AUTO);

        ::opn2_switchEmulator(Device, (int) _EmulatorCore);

        const void * BankData;
        size_t BankSize;

        switch (_BankNumber)
        {
            default:
            case 0:
                BankData = bnk_xg;
                BankSize = sizeof(bnk_xg);
                break;

            case 1:
                BankData = bnk_gs;
                BankSize = sizeof(bnk_gs);
                break;

            case 2:
                BankData = bnk_gems;
                BankSize = sizeof(bnk_gems);
                break;

            case 3:
                BankData = bnk_Tomsoft;
                BankSize = sizeof(bnk_Tomsoft);
                break;

            case 4:
                BankData = bnk_fmmidi;
                BankSize = sizeof(bnk_fmmidi);
                break;
        }

        ::opn2_openBankData(Device, BankData, (long) BankSize);

        ::opn2_setNumChips(Device, ChipsPerPort + ChipsRound * (i == 0) + ChipsMin * (i != 0)); // Set number of concurrent emulated chips to excite channels limit of one chip.
        ::opn2_setDeviceIdentifier(Device, (unsigned int) i); // Set 4-bit device identifier. Used by the SysEx processor.

        _Devices[i] = Device;
    }

    console::print(STR_COMPONENT_BASENAME " is using LibOPNMIDI " OPNMIDI_VERSION " with emulator ", ::opn2_chipEmulatorName(_Devices[0]), ".");

    _IsStarted = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void OPNPlayer::Shutdown()
{
    for (size_t i = 0; i < _countof(_Devices); ++i)
    {
        ::opn2_close(_Devices[i]);
        _Devices[i] = nullptr;
    }

    _IsStarted = false;
}

void OPNPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    const uint32_t MaxFrames = 256;
    const uint32_t MaxChannels = 2;

#ifndef OldCode
    audio_sample srcFrames[MaxFrames * MaxChannels];

    const OPNMIDI_AudioFormat AudioFormat = { OPNMIDI_SampleType_F64, sizeof(*srcFrames), sizeof(srcFrames[0]) * MaxChannels };

    while (dstCount != 0)
    {
        const uint32_t srcCount = std::min(dstCount, MaxFrames);

        ::memset(dstFrames, 0, (srcCount * MaxChannels) * sizeof(dstFrames[0]));

        for (size_t i = 0; i < _countof(_Devices); ++i)
        {
            ::opn2_generateFormat(_Devices[i], (int) (srcCount * MaxChannels), (OPN2_UInt8 *) srcFrames, (OPN2_UInt8 *)(srcFrames + 1), &AudioFormat);

            // Convert the rendered output.
            for (size_t j = 0; j < (srcCount * MaxChannels); ++j)
                dstFrames[j] += (audio_sample) srcFrames[j];
        }

        dstFrames += (srcCount * MaxChannels);
        dstCount -= srcCount;
    }
#else
    int16_t srcData[MaxFrames * MaxChannels];

    while (frameCount != 0)
    {
        uint32_t ToDo = frameCount;

        if (ToDo > MaxFrames)
            ToDo = MaxFrames;

        ::memset(dstData, 0, ToDo * MaxChannels * sizeof(audio_sample));

        for (size_t i = 0; i < _countof(_Devices); ++i)
        {
            ::opn2_generate(_Devices[i], (int) (ToDo * MaxChannels), srcData);

            // Convert the rendered output.
            for (size_t j = 0; j < (ToDo * MaxChannels); ++j)
                dstData[j] += (audio_sample) srcData[j] * (1.0f / 32768.0f);
        }

        dstData += (ToDo * MaxChannels);
        frameCount -= ToDo;
    }
#endif
}

void OPNPlayer::SendEvent(uint32_t data)
{
    OPN2_UInt8 Event[3]
    {
        static_cast<OPN2_UInt8>(data),
        static_cast<OPN2_UInt8>(data >>  8),
        static_cast<OPN2_UInt8>(data >> 16)
    };

    size_t Port = (data >> 24) & _countof(_Devices);

    if (Port > (_countof(_Devices) - 1))
        Port = 0;

    const OPN2_UInt8 Status = data & 0xF0;
    const OPN2_UInt8 Channel = data & 0x0F;

    switch (Status)
    {
        case midi::StatusCodes::NoteOff:
            ::opn2_rt_noteOff(_Devices[Port], Channel, Event[1]);
            break;

        case midi::StatusCodes::NoteOn:
            ::opn2_rt_noteOn(_Devices[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCodes::KeyPressure:
            ::opn2_rt_noteAfterTouch(_Devices[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCodes::ControlChange:
            ::opn2_rt_controllerChange(_Devices[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCodes::ProgramChange:
            ::opn2_rt_patchChange(_Devices[Port], Channel, Event[1]);
            break;

        case midi::StatusCodes::ChannelPressure:
            ::opn2_rt_channelAfterTouch(_Devices[Port], Channel, Event[1]);
            break;

        case midi::StatusCodes::PitchBendChange:
            ::opn2_rt_pitchBendML(_Devices[Port], Channel, Event[2], Event[1]);
            break;
    }
}

void OPNPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    if (portNumber >= 3)
        portNumber = 0;

    ::opn2_rt_systemExclusive(_Devices[portNumber], data, size);

    if (portNumber == 0)
    {
        ::opn2_rt_systemExclusive(_Devices[1], data, size);
        ::opn2_rt_systemExclusive(_Devices[2], data, size);
    }
}
