
/** $VER: ADLPlayer.cpp (2025.07.25) **/

#include "pch.h"

#include "ADLPlayer.h"
#include "Resource.h"
#include "Log.h"

#include "Encoding.h"

ADLPlayer::ADLPlayer() : player_t()
{
    ::memset(_Devices, 0, sizeof(_Devices));
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
void ADLPlayer::SetSoftPanning(bool enabled) noexcept
{
    _IsSoftPanningEnabled = enabled;
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
    if (_IsStarted)
        return true;

    const int ChipsPerPort = (int) (_ChipCount / _countof(_Devices));
    const int ChipsRound   = (_ChipCount % _countof(_Devices)) != 0;
    const int ChipsMin     = _ChipCount < _countof(_Devices);

    for (size_t i = 0; i < _countof(_Devices); ++i)
    {
        auto * Device = ::adl_init((long) _SampleRate);

        if (Device == nullptr)
        {
            _ErrorMessage = ::adl_errorString();

            return false;
        }

    #ifdef _DEBUG
        ::adl_setDebugMessageHook
        (
            Device,
            [](void * context, const char * format ...) noexcept
            {
                char Line[1024] = { };

                std::va_list args;

                va_start(args, format);

                (void) ::vsnprintf(Line, sizeof(Line) - 1, format, args);

                Log.AtDebug().Write(STR_COMPONENT_BASENAME " ADL Player says %s", Line);

                va_end(args);
            },
            nullptr
        );
    #endif

        ::adl_reset(Device);

        ::adl_setSoftPanEnabled     (Device, _IsSoftPanningEnabled);
        ::adl_setHVibrato           (Device, -1); // -1 for default. Use 1 to turn on deep vibrato.
        ::adl_setHTremolo           (Device, -1); // -1 for default. Use 1 to turn on deep tremolo.
        ::adl_setScaleModulators    (Device, -1); // -1 for default. Use 1 to turn on modulators scaling by volume.
        ::adl_setFullRangeBrightness(Device, -1); // -1 for default. Use 1 to turn on a full-ranged XG CC74 brightness.
        ::adl_setAutoArpeggio       (Device, -1); // -1 for default. Use 1 to turn on auto-arpeggio.

        ::adl_setChannelAllocMode   (Device, ADLMIDI_ChanAlloc_AUTO);
        ::adl_setVolumeRangeModel   (Device, ADLMIDI_VolumeModel_AUTO);

        ::adl_switchEmulator(Device, (int) _EmulatorCore);

        if (_BankFilePath.empty() || (::adl_openBankFile(Device, _BankFilePath.c_str()) == -1))
            ::adl_setBank(Device, (int) _BankNumber);

        ::adl_setNumChips           (Device, ChipsPerPort + ChipsRound * (i == 0) + ChipsMin * (i != 0)); // Set number of concurrent emulated chips to excite channels limit of one chip.
        ::adl_setDeviceIdentifier   (Device, (unsigned int) i); // Set 4-bit device identifier. Used by the SysEx processor.

//      ::adl_setNumFourOpsChn      (Device, (int) _4OpChannelCount); // Set total count of 4-operator channels between all emulated chips.

        _Devices[i] = Device;
    }

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is using LibADLMIDI " ADLMIDI_VERSION " with emulator %s.", ::adl_chipEmulatorName(_Devices[0]));

    _IsStarted = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void ADLPlayer::Shutdown()
{
    for (size_t i = 0; i < _countof(_Devices); ++i)
    {
        ::adl_close(_Devices[i]);
        _Devices[i] = nullptr;
    }

    _IsStarted = false;
}

void ADLPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    console::printf("%9d: %6d frames", (uint32_t) ::GetTickCount64(), dstCount);

    const uint32_t MaxFrames = 256;
    const uint32_t MaxChannels = 2;

#ifndef OldCode
    audio_sample srcFrames[MaxFrames * MaxChannels];

#ifdef _M_X64
    const ADLMIDI_AudioFormat AudioFormat = { ADLMIDI_SampleType_F64, sizeof(srcFrames[0]), sizeof(srcFrames[0]) * MaxChannels };
#else
#ifdef _M_IX86
    const ADLMIDI_AudioFormat AudioFormat = { ADLMIDI_SampleType_F32, sizeof(srcFrames[0]), sizeof(srcFrames[0]) * MaxChannels };
#else
    #error "Unsupported processor architecture"
#endif
#endif

    while (dstCount != 0)
    {
        const uint32_t srcCount = std::min(dstCount, MaxFrames);

        ::memset(dstFrames, 0, (srcCount * MaxChannels) * sizeof(dstFrames[0]));

        for (size_t i = 0; i < _countof(_Devices); ++i)
        {
            ::adl_generateFormat(_Devices[i], (int) (srcCount * MaxChannels), (ADL_UInt8 *) srcFrames, (ADL_UInt8 *)(srcFrames + 1), &AudioFormat);

            // Convert the rendered output.
            for (size_t j = 0; j < (srcCount * MaxChannels); ++j)
                dstFrames[j] += (audio_sample) srcFrames[j];
        }

        dstFrames += (srcCount * MaxChannels);
        dstCount -= srcCount;
    }
#else
    int16_t Data[MaxFrames * MaxChannels];

    const ADLMIDI_AudioFormat AudioFormat = { ADLMIDI_SampleType_S16, sizeof(Data[0]), sizeof(Data[0]) * MaxChannels };

    while (dstCount != 0)
    {
        size_t ToDo = dstCount;

        if (ToDo > MaxFrames)
            ToDo = MaxFrames;

        ::memset(dstFrames, 0, ToDo * MaxChannels * sizeof(audio_sample));

        for (size_t i = 0; i < _countof(_Devices); ++i)
        {
            ::adl_generateFormat(_Devices[i], (int) (ToDo * MaxChannels), (ADL_UInt8 *) Data, (ADL_UInt8 *)(Data + 1), &AudioFormat);

            // Convert the format of the rendered output.
            for (size_t j = 0; j < (ToDo * MaxChannels); ++j)
                dstFrames[j] += (audio_sample) Data[j] * (audio_sample) (1.0 / 32768.0);
        }

        dstFrames += (ToDo * MaxChannels);
        dstCount -= (uint32_t) ToDo;
    }
#endif
}

void ADLPlayer::SendEvent(uint32_t data)
{
    ADL_UInt8 Event[3]
    {
        (ADL_UInt8) (data),
        (ADL_UInt8) (data >>  8),
        (ADL_UInt8) (data >> 16)
    };

    size_t Port = (data >> 24) & _countof(_Devices);

    if (Port > (_countof(_Devices) - 1))
        Port = 0;

    const ADL_UInt8 Status = data & 0xF0;
    const ADL_UInt8 Channel = data & 0x0F;

    switch (Status)
    {
        case midi::StatusCode::NoteOff:
            ::adl_rt_noteOff(_Devices[Port], Channel, Event[1]);
            break;

        case midi::StatusCode::NoteOn:
            ::adl_rt_noteOn(_Devices[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCode::KeyPressure:
            ::adl_rt_noteAfterTouch(_Devices[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCode::ControlChange:
            ::adl_rt_controllerChange(_Devices[Port], Channel, Event[1], Event[2]);
            break;

        case midi::StatusCode::ProgramChange:
            ::adl_rt_patchChange(_Devices[Port], Channel, Event[1]);
            break;

        case midi::StatusCode::ChannelPressure:
            ::adl_rt_channelAfterTouch(_Devices[Port], Channel, Event[1]);
            break;

        case midi::StatusCode::PitchBendChange:
            ::adl_rt_pitchBendML(_Devices[Port], Channel, Event[2], Event[1]);
            break;
    }
}

void ADLPlayer::SendSysEx(const uint8_t * event, size_t size, uint32_t)
{
    ::adl_rt_systemExclusive(_Devices[0], event, size);
    ::adl_rt_systemExclusive(_Devices[1], event, size);
    ::adl_rt_systemExclusive(_Devices[2], event, size);
}
