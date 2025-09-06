 
/** $VER: InputDecoder.cpp (2025.08.31) **/

#include "pch.h"

#include "InputDecoder.h"

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include <pfc/string-conv-lite.h>
#include <helpers/atl-misc.h>

#include <math.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <ranges>

#include <libsf.h>

#include "PreferencesFM.h"
#include "KaraokeProcessor.h"
#include "CLAPHost.h"
#include "Exception.h"

#include "Log.h"

/* KEEP? 06/07/25
volatile int _IsRunning = 0;

critical_section _Lock;
volatile uint32_t _CurrentSampleRate;
*/

/// <summary>
/// Creates a new instance.
/// </summary>
InputDecoder::InputDecoder() noexcept :
    _DecoderFlags(),

    _FileStats {},
    _FileStats2 {},

    _IsSysExFile(false),

    _IsMT32(),
    _IsGS(),
    _IsXG(),

    _DetectRPGMakerLoops((bool) CfgDetectRPGMakerLoops),
    _DetectLeapFrogLoops((bool) CfgDetectLeapFrogLoops),
    _DetectXMILoops((bool) CfgDetectXMILoops),
    _DetectTouhouLoops((bool) CfgDetectTouhouLoops),
    _DetectFF7Loops((bool) CfgDetectFF7Loops),

    _Player(nullptr),
    _PlayerType(),

    _CLAPPlugIn(),

    _RequestedSampleRate((uint32_t) CfgSampleRate),
    _ActualSampleRate(_RequestedSampleRate),

    _FrameIndex(),
    _FrameCount(),

    _LoopType(LoopType::NeverLoop),
    _LoopCount(1),
    _LoopRange(),

    _FadeRange(),
    _FadeTime(),

    _DecayTime(),

    _ExtraPercussionChannel(~0U),

    _BASSMIDIGain((float) CfgBASSMIDIGain),
    _BASSMIDIInterpolationMode((uint32_t) CfgBASSMIDIResamplingMode),

    _FluidSynthInterpolationMethod((uint32_t) CfgFluidSynthInterpolationMode)
{
    _CleanFlags = (uint32_t) (CfgExcludeEMIDITrackDesignation ? midi::container_t::CleanFlagEMIDI : 0) |
                             (CfgFilterInstruments  ? midi::container_t::CleanFlagInstruments : 0) |
                             (CfgFilterBanks        ? midi::container_t::CleanFlagBanks : 0);
/* KEEP? 06/07/25
    _CurrentSampleRate = _SampleRate;
*/
}

/// <summary>
/// Deletes this instance.
/// </summary>
InputDecoder::~InputDecoder() noexcept
{
    for (const auto & sf : _Soundfonts)
    {
        if (sf.IsTemporary)
        {
            // This may fail if the sound font cache still has a lock on the file. The cache will clean it up when it's done with it.
            ::DeleteFileA((const char *) sf.FilePath.string().c_str());
        }
    }

    if (_Player != nullptr)
    {
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " is deleting player 0x%016llX.", _Player);

        delete _Player;
        _Player = nullptr;
    }

/* KEEP? 06/07/25
    if (_PlayerType == PlayerType::EmuDeMIDI)
    {
        insync(_Lock);
        _IsRunning -= 1;
    }
*/
}

/// <summary>
/// Opens the specified file and parses it.
/// </summary>
void InputDecoder::open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler)
{
    if (file.is_empty())
        filesystem::g_open(file, filePath, filesystem::open_mode_read, abortHandler);

    _FilePath = filesystem::g_get_native_path(filePath);

    {
        _FileStats = file->get_stats(abortHandler);

        if ((_FileStats.m_size == 0) || (_FileStats.m_size > (t_filesize)(1 << 30)))
            throw exception_io_unsupported_format();

        _FileStats2 = file->get_stats2_((uint32_t)stats2_all, abortHandler);

        if ((_FileStats2.m_size == 0) || (_FileStats2.m_size > (t_filesize)(1 << 30)))
            throw exception_io_unsupported_format();
    }

    std::vector<uint8_t> Object((size_t) _FileStats.m_size);

    file->read_object(Object.data(), (t_size) _FileStats.m_size, abortHandler);

    // Try to process the data as a SysEx sequence.
    {
        _IsSysExFile = IsSysExFileExtension(pfc::string_extension(_FilePath));

        if (_IsSysExFile)
        {
            try
            {
                midi::processor_t::Process(Object, pfc::wideFromUTF8(_FilePath), _Container, midi::DefaultOptions);
            }
            catch (const std::exception & e)
            {
                const pfc::string Message = "Failed to read SysEx file: ";

                throw exception_io_data(Message + e.what());
            }

            return;
        }
    }

    // Store these values early on to make sure they don't change during playback. The user may be changing those in the Preferences dialog.
    _LoopCount = (uint32_t) CfgLoopCount;
    _FadeTime  = (uint32_t) CfgFadeTime;
    _DecayTime = (uint32_t) CfgDecayTime;

    // Try to process the data as a MIDI sequence.
    {
        try
        {
            midi::processor_options_t Options
            (
                (uint16_t) CfgLoopExpansion,
                CfgWriteBarMarkers,
                CfgWriteSysExNames,
                CfgExtendLoops,
                CfgWolfteamLoopMode,
                CfgKeepMutedChannels,
                CfgIncludeControlData,

                (uint16_t) CfgDefaultTempo,

                true, // End of Track is required
                CfgDetectExtraDrum
            );

            midi::processor_t::Process(Object, pfc::wideFromUTF8(_FilePath), _Container, Options);
        }
        catch (const std::exception & e)
        {
            const pfc::string Message = "Failed to read MIDI file: ";

            throw exception_io_data(Message + e.what());
        }

        if (_Container.IsEmpty())
            throw exception_io_data("Invalid MIDI file: No tracks found");

        // Validate the MIDI data.
        {
            bool IsLengthSet = false;

            for (size_t i = 0, TrackCount = _Container.GetTrackCount(); i < TrackCount; ++i)
            {
                if (_Container.GetDuration(i) != 0)
                {
                    IsLengthSet = true;
                    break;
                }
            }

            if (!IsLengthSet)
                throw exception_io_data("Invalid MIDI file: No timestamps found in any of the tracks");
        }

        _Container.DetectLoops(_DetectXMILoops, _DetectFF7Loops, _DetectRPGMakerLoops, _DetectTouhouLoops, _DetectLeapFrogLoops);
    }

    // Calculate the hash of the MIDI data.
    {
        Object.resize(0);

        _Container.SerializeAsSMF(Object);

        {
            static_api_ptr_t<hasher_md5> Hasher;

            hasher_md5_state HasherState;

            Hasher->initialize(HasherState);
            Hasher->process(HasherState, Object.data(), Object.size());

            _FileHash = Hasher->get_result(HasherState);
        }
    }

    if (CfgSkipToFirstNote)
        _Container.TrimStart();

    _LoopRange.Clear();
}

/// <summary>
/// Initializes the decoder before playing the specified subsong.
/// Resets playback position to the beginning of specified subsong. This must be called first, before any other input_decoder methods (other than those inherited from input_info_reader).
/// It is legal to set initialize() more than once, with same or different subsong, to play either the same subsong again or another subsong from same file without full reopen.
/// </summary>
void InputDecoder::decode_initialize(unsigned subSongIndex, unsigned flags, abort_callback & abortHandler)
{
    Log.AtDebug().Write(STR_COMPONENT_BASENAME " is initializing the decoder.");

    if (_IsSysExFile)
        throw pfc::exception("You cannot play SysEx files");

    _DecoderFlags = flags;
    _IsFirstBlock = true;

    // Initialize time parameters.
    {
        _FrameIndex = 0;
        _FrameCount = (uint32_t) ::MulDiv((int) GetLength(subSongIndex), (int) _RequestedSampleRate, 1'000);

        _LoopType = (LoopType) ((_DecoderFlags & input_flag_playback) ? (int) CfgLoopTypePlayback : (int) CfgLoopTypeOther);

        _LoopRange.Set(_Container.GetLoopBeginTimestamp(subSongIndex, true), _Container.GetLoopEndTimestamp(subSongIndex, true));

        InitializeFade();
    }

    _ExtraPercussionChannel = _Container.GetExtraPercussionChannel();

    // The preset collects the settings to configure the player.
    preset_t Preset;

    // Load the preset from the song if it has one.
    {
        file_info_impl FileInfo;

        get_info(subSongIndex, FileInfo, abortHandler);

        {
            const char * PresetText = FileInfo.meta_get(TagPreset, 0);

            if (PresetText != nullptr)
            {
                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is using preset \"%s\" from tags.", PresetText);

                Preset.Deserialize(PresetText);
            }
        }
    
        // Load the SysEx from the song if it has one.
        {
            MIDISysExDumps SysExDumps;

            const char * MIDISysExDumps = FileInfo.meta_get(TagMIDISysExDumps, 0);

            if (MIDISysExDumps != nullptr)
            {
                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is using SysEx file \"%s\".", MIDISysExDumps);

                SysExDumps.Deserialize(MIDISysExDumps, _FilePath);
            }

            SysExDumps.Merge(_Container, abortHandler);
        }
    }

    OverridePlayerSelection(Preset, subSongIndex, abortHandler);

    // Get the soundfonts if the player requires them.
    if ((_PlayerType == PlayerType::BASSMIDI) || (_PlayerType == PlayerType::FluidSynth))
        GetSoundfonts(Preset._SoundfontFilePath.c_str(), abortHandler);

    // Create and initialize the MIDI player.
    switch (_PlayerType)
    {
        default:
        case PlayerType::Unknown:
        {
            throw pfc::exception("No player selected");
        }

        // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413))
        case PlayerType::EmuDeMIDI:
        {
            auto Player = new EdMPlayer;

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");
/* KEEP? 06/07/25
            {
                insync(_Lock);

                _IsRunning += 1;

                if (_IsRunning == 1)
                    _CurrentSampleRate = _SampleRate;
                else
                if (_SampleRate != _CurrentSampleRate)
                    _SampleRate = _CurrentSampleRate;
            }
*/
            break;
        }

        // VSTi
        case PlayerType::VSTi:
        {
            if (Preset._PlugInFilePath.is_empty())
                throw pfc::exception("No plug-in specified in preset");

            auto Player = new VSTi::Player;

            if (!Player->LoadVST((const char8_t *) Preset._PlugInFilePath.c_str()))
                throw pfc::exception(pfc::string("Unable to load VSTi plug-in from \"") + Preset._PlugInFilePath + "\"");

            if (Preset._VSTiConfig.size() != 0)
                Player->SetChunk(Preset._VSTiConfig.data(), Preset._VSTiConfig.size());

            _PlugInName = Player->Name;

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
            {
                const std::string ErrorMessage = _Player->GetErrorMessage();

                throw pfc::exception(!ErrorMessage.empty() ? ErrorMessage.c_str() : "Failed to load MIDI stream");
            }

            break;
        }

        // CLAP (CLever Audio Plug-in API)
        case PlayerType::CLAP:
        {
            if (!_CLAPHost.IsPlugInLoaded())
                throw pfc::exception("No CLAP plug-in loaded into host");

            auto Player = new CLAPPlayer();

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // BASS MIDI
        case PlayerType::BASSMIDI:
        {
            {
                _ActiveVoiceCount = 0;
                _PeakVoiceCount = 0;

                if (_Soundfonts.empty())
                    throw pfc::exception("No compatible soundfonts found");
            }

            auto Player = new BMPlayer;

            Player->SetSoundfonts(_Soundfonts);

            Player->SetInterpolationMode(_BASSMIDIInterpolationMode);
            Player->SetVoiceCount(Preset._VoiceCount);
            Player->EnableEffects(Preset._EffectsEnabled);

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
            {
                const std::string ErrorMessage = _Player->GetErrorMessage();

                throw pfc::exception(!ErrorMessage.empty() ? ErrorMessage.c_str() : "Failed to load MIDI stream");
            }

            break;
        }

        // FluidSynth
        case PlayerType::FluidSynth:
        {
            {
                _ActiveVoiceCount = 0;
                _PeakVoiceCount = 0;

                if (_Soundfonts.empty())
                    throw pfc::exception("No compatible soundfonts found");
            }

            auto Player = new FSPlayer;

            pfc::string DirectoryPath = CfgFluidSynthDirectoryPath;

            if (DirectoryPath.isEmpty())
            {
                Log.AtWarn().Write(STR_COMPONENT_BASENAME " will attempt to load the FluidSynth libraries from the component directory because the location was not configured.");

                DirectoryPath = core_api::get_my_full_path();
                DirectoryPath.truncate(DirectoryPath.scan_filename());
            }

            Player->SetAbortHandler(&abortHandler);
            Player->Initialize(pfc::wideFromUTF8(DirectoryPath));

            Player->SetInterpolationMode(_FluidSynthInterpolationMethod);
            Player->SetVoiceCount(Preset._VoiceCount);
            Player->EnableEffects(Preset._EffectsEnabled);
            Player->EnableDynamicLoading(CfgFluidSynthDynSampleLoading);
            Player->SetSoundfonts(_Soundfonts);

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
            {
                const std::string ErrorMessage = _Player->GetErrorMessage();

                if (!ErrorMessage.empty())
                    throw pfc::exception(ErrorMessage.c_str());
                else
                    throw pfc::exception("Failed to load MIDI stream");
            }

            break;
        }

        // MT32Emu (MT-32)
        case PlayerType::MT32Emu:
        {
            auto Player = new MT32Player(_IsMT32, Preset._MT32EmuGMSet);

            pfc::string DirectoryPath = CfgMT32ROMDirectoryPath;

            if (DirectoryPath.is_empty())
            {
                Log.AtWarn().Write(STR_COMPONENT_BASENAME " is attempting to load the MT-32 ROMs from the component directory because the location was not configured.");

                DirectoryPath = core_api::get_my_full_path();
                DirectoryPath.truncate(DirectoryPath.scan_filename());
            }

            Player->SetAbortHandler(&abortHandler);
            Player->SetROMDirectory(std::u8string((const char8_t *) DirectoryPath.c_str()));
/*
            if (!Player->IsConfigValid())
                throw pfc::exception("The MT32Emu driver needs a valid MT-32 or CM32L ROM set to play.");
*/
            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // DirectX
        case PlayerType::DirectX:
        {
            throw pfc::exception("Player not implemented");
        }

        // LibADLMIDI
        case PlayerType::ADL:
        {
            auto Player = new ADLPlayer;

            Player->SetBankNumber(Preset._ADLBankNumber);
            Player->SetBankFilePath(Preset._ADLBankFilePath);
            Player->SetChipCount(Preset._ADLChipCount);
            Player->Set4OpChannelCount(Preset._ADLChipCount * 4);
            Player->SetSoftPanning(Preset._ADLSoftPanning); // Call after SetBankNumber()!
            Player->SetEmulatorCore(Preset._ADLEmulatorCore);

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // LibOPNMIDI
        case PlayerType::OPN:
        {
            auto Player = new OPNPlayer;

            Player->SetBankNumber(Preset._OPNBankNumber);
            Player->SetBankFilePath(Preset._OPNBankFilePath);
            Player->SetChipCount(Preset._OPNChipCount);
            Player->SetSoftPanning(Preset._OPNSoftPanning); // Call after SetBankNumber()!
            Player->SetEmulatorCore(Preset._OPNEmulatorCore);

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // OPL
        case PlayerType::OPL:
        {
            throw pfc::exception("Player not implemented");
        }

        // Nuked OPL3
        case PlayerType::NukedOPL3:
        {
            auto Player = new NukedOPL3Player();

            Player->SetSynth(Preset._NukeSynth);
            Player->SetBankNumber(Preset._NukeBank);
            Player->SetSoftPanning(Preset._NukeUsePanning);

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // Secret Sauce
        case PlayerType::SecretSauce:
        {
            auto Player = new SCPlayer();

            fs::path PathName = (const char8_t *) (const char *) CfgSecretSauceDirectoryPath.get();

            if (PathName.empty())
            {
                Log.AtWarn().Write(STR_COMPONENT_BASENAME " will attempt to load Secret Sauce from the component directory because the location was not configured.");

                PathName = (const char8_t *) core_api::get_my_full_path();
                PathName.remove_filename();
            }

            Player->SetRootPath(PathName);

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // MCI
        case PlayerType::MCI:
        {
            auto Player = new MCIPlayer;

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // Nuked SC-55
        case PlayerType::NukedSC55:
        {
            auto Player = new NukedSC55Player;

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }

        // FMMIDI (yuno) (Yamaha YM2608)
        case PlayerType::FMMIDI:
        {
            auto Player = new FMMPlayer;

            pfc::string ProgramsFilePath = CfgProgramsFilePath;

            if (ProgramsFilePath.empty())
            {
                ProgramsFilePath = pfc::io::path::getParent(core_api::get_my_full_path());

                ProgramsFilePath.add_filename(FMMPlayer::DefaultProgramsFileName.c_str());
            }

            Player->SetProgramsFilePath(pfc::stringcvt::string_os_from_utf8(ProgramsFilePath).get_ptr());

            _Player = Player;

            _Player->SetSampleRate(_RequestedSampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _DecayTime, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            break;
        }
    }

    _IsEndOfContainer = false;
}

/// <summary>
/// Reads/decodes one chunk of audio data.
/// Use false return value to signal end of file (no more data to return). Before calling decode_run(), decoding must be initialized by a decode_initialize() call.
/// </summary>
bool InputDecoder::decode_run(audio_chunk & audioChunk, abort_callback & abortHandler)
{
    abortHandler.check();

    if (_IsEndOfContainer)
        return false;

    bool Success = false;

    if (_Player)
    {
        auto OldActualSampleRate = _ActualSampleRate;

        _Player->SetAbortHandler(&abortHandler);

        const uint32_t FrameCount = 4096;
        const uint32_t ChannelCount = _Player->GetAudioChannelCount();

        audioChunk.set_data_size((t_size) FrameCount * ChannelCount);

        audio_sample * FrameData = audioChunk.get_data();

        uint32_t FramesDone = _Player->Play(FrameData, FrameCount);

        if (FramesDone == 0)
        {
            const std::string ErrorMessage = _Player->GetErrorMessage();

            if (!ErrorMessage.empty())
                throw exception_io_data(ErrorMessage.c_str());

            return false;
        }

        _ActualSampleRate = _Player->GetSampleRate(); // Allows us to store the actual sample rate in the info field.

        // Recalculate some sample rate-dependent parameters if the player changed the actual sample rate.
        if (_ActualSampleRate != OldActualSampleRate)
        {
            _FrameCount = (uint32_t) ::MulDiv((int) _FrameCount, (int) _ActualSampleRate, (int) _RequestedSampleRate);

            if (_FadeRange.HasBegin())
                _FadeRange.SetBegin((uint32_t) ::MulDiv((int) _FadeRange.Begin(), (int) _ActualSampleRate, (int) _RequestedSampleRate));

            if (_FadeRange.HasEnd())
                _FadeRange.SetEnd((uint32_t) ::MulDiv((int) _FadeRange.End(), (int) _ActualSampleRate, (int) _RequestedSampleRate));
        }

        audioChunk.set_srate(_ActualSampleRate);
        audioChunk.set_channels(ChannelCount);
        audioChunk.set_sample_count(FramesDone);

        Success = true;
    }

    // Scale the samples if fading was requested.
    if (Success && _FadeRange.IsSet())
    {
        uint32_t BlockBegin = _FrameIndex;
        uint32_t BlockEnd   = _FrameIndex + (uint32_t) audioChunk.get_sample_count();

        _FrameIndex = BlockEnd;

        if (BlockEnd >= _FadeRange.Begin())
        {
            for (size_t i = std::max(_FadeRange.Begin(), BlockBegin), j = std::min(BlockEnd, _FadeRange.End()); i < j; ++i)
            {
                audio_sample * Sample = audioChunk.get_data() + (i - BlockBegin) * 2;

                audio_sample Scale = (audio_sample) (_FadeRange.End() - i) / (audio_sample) _FadeRange.Size();

                Sample[0] *= Scale;
                Sample[1] *= Scale;
            }

            if (BlockEnd > _FadeRange.End())
            {
                uint32_t FramesRemaining = 0;

                if (_FadeRange.End() > BlockBegin)
                    FramesRemaining = _FadeRange.End() - BlockBegin;

                audioChunk.set_sample_count(FramesRemaining);

                _IsEndOfContainer = true;

                if (FramesRemaining == 0)
                    return false;
            }
        }
    }

    _AudioChunkDuration = audioChunk.get_duration();

    return Success;
}

/// <summary>
/// Seeks to the specified time offset. Before seeking or other decoding calls, decoding must be initialized with decode_initialize() call.
/// </summary>
void InputDecoder::decode_seek(double timeInSeconds, abort_callback &)
{
    _FrameIndex = (uint32_t) (timeInSeconds * _ActualSampleRate);

    _IsFirstBlock = true;
    _IsEndOfContainer = false;

    uint32_t Time = (uint32_t) (timeInSeconds * 1'000.);

    if (Time > _LoopRange.End())
        Time = _LoopRange.Begin() + (Time - _LoopRange.Begin()) % ((_LoopRange.Size() != 0) ? _LoopRange.Size() : 1);

    const uint32_t FrameIndex = (uint32_t) ::MulDiv((int) Time, (int) _ActualSampleRate, 1'000);

    if ((_FrameCount != 0U) && (FrameIndex >= (_FrameCount - _ActualSampleRate)))
    {
        _IsEndOfContainer = true;

        return;
    }

    if (_Player)
        _Player->Seek(FrameIndex);
}

/// <summary>
/// Signals dynamic VBR bitrate, etc. Called after each decode_run() (or not called at all if caller doesn't care about dynamic info).
/// </summary>
bool InputDecoder::decode_get_dynamic_info(file_info & fileInfo, double & timestampDelta)
{
    bool Success = false;

    if (_IsFirstBlock)
    {
        // Set the "sample_rate" information field.
        {
            fileInfo.info_set_int(InfoSampleRate, _ActualSampleRate);
        }

        // Set the "midi_player" information field.
        {
            assert(_countof(PlayerTypeNames) == ((size_t) PlayerType::Max + 1));

            const char * Value = "Unknown";

            if ((_PlayerType >= PlayerType::Min) && (_PlayerType <= PlayerType::Max))
                Value = PlayerTypeNames[(size_t) _PlayerType];
            else
                Value = "VSTi";

            fileInfo.info_set(InfoMIDIPlayer, Value);
        }

        // Set the "midi_player_ext" information field.
        {
            std::string Value;

            #pragma warning(disable: 4062) // enumerator x in switch of y is not explicitly handled by a case label

            switch (_PlayerType)
            {
                case PlayerType::ADL:
                {
                    const auto TargetId = (int) CfgADLEmulator;

                    auto Match = std::ranges::find_if(_ADLEmulators, [TargetId](const emulator_t & em)
                    {
                        return em.Id == TargetId;
                    });

                    Value = (Match != _ADLEmulators.end()) ? msc::WideToUTF8(Match->Name).c_str() : "";
                    break;
                }

                case PlayerType::OPN:
                {
                    const auto TargetId = (int) CfgOPNEmulator;

                    auto Match = std::ranges::find_if(_OPNEmulators, [TargetId](const emulator_t & em)
                    {
                        return em.Id == TargetId;
                    });

                    Value = (Match != _OPNEmulators.end()) ? msc::WideToUTF8(Match->Name).c_str() : "";
                    break;
                }

                case PlayerType::NukedOPL3:
                {
                    const auto SynthId = (uint32_t) CfgNukeSynthesizer;
                    const auto BankId  = (uint32_t) CfgNukeBank;

                    auto Match = std::ranges::find_if(_NukedPresets, [SynthId, BankId](const NukedPreset & np)
                    {
                        return (np.SynthId == SynthId) && (np.BankId == BankId);
                    });

                    Value = (Match != _NukedPresets.end()) ? Match->Name.c_str() : "";
                    break;
                }

                case PlayerType::VSTi:
                case PlayerType::CLAP:
                {
                    Value = !_IsPlayerTypeOverriden ? CfgPlugInName.get().c_str() : _PlugInName.c_str();
                    break;
                }
            };

            #pragma warning(default: 4062)

            fileInfo.info_set(InfoMIDIPlayerExt, Value.c_str());
        }

        // Set the "extra_percussion_channel" information field.
        {
            if (_ExtraPercussionChannel != ~0L)
                fileInfo.info_set(InfoMIDIExtraPercusionChannel, pfc::format_int((t_int64) _ExtraPercussionChannel + 1));
        }

        _IsFirstBlock = false;

        Success = true;
        timestampDelta = 0.;
    }

    if ((_PlayerType == PlayerType::FluidSynth) || (_PlayerType == PlayerType::BASSMIDI))
    {
        uint32_t VoiceCount = 0;

        if (_PlayerType == PlayerType::FluidSynth)
        {
            auto Player = (FSPlayer *) _Player;

            VoiceCount = Player->GetActiveVoiceCount();
        }
        else
        if (_PlayerType == PlayerType::BASSMIDI)
        {
            auto Player = (BMPlayer *) _Player;

            VoiceCount = Player->GetActiveVoiceCount();
        }

        if (VoiceCount != _ActiveVoiceCount)
        {
            fileInfo.info_set_int(InfoMIDIActiveVoices, (t_int64) VoiceCount);

            _ActiveVoiceCount = VoiceCount;

            Success = true;
        }

        if (VoiceCount > _PeakVoiceCount)
        {
            fileInfo.info_set_int(InfoMIDIPeakVoices, (t_int64) VoiceCount);

            _PeakVoiceCount = VoiceCount;

            Success = true;
        }

        if (Success)
            timestampDelta = _AudioChunkDuration;
    }

    return Success;
}

/// <summary>
/// Initializes the Index Manager.
/// </summary>
void InputDecoder::InitializeIndexManager()
{
    try
    {       
        static_api_ptr_t<metadb_index_manager>()->add(new service_impl_t<FileHasher>, GUIDTagMIDIHash, system_time_periods::week * 4);
    }
    catch (...) { }
}

/// <summary>
/// Gets the total length of the specified sub-song taking into account looping, fade-out and decay time (in ms).
/// </summary>
uint32_t InputDecoder::GetLength(size_t subSongIndex) noexcept
{
    uint32_t TotalTime = _Container.GetDuration(subSongIndex, true);

    if (_LoopType == LoopType::NeverLoopAddDecayTime)
        TotalTime += _DecayTime;
    else
    if (_LoopType > LoopType::LoopWhenDetectedAndFade)
    {
        if (!_LoopRange.HasBegin())
            _LoopRange.SetBegin(0);

        if (!_LoopRange.HasEnd())
            _LoopRange.SetEnd(TotalTime);

        TotalTime = _LoopRange.Begin() + (_LoopRange.Size() * _LoopCount) + _FadeTime;
    }

    return TotalTime;
}

/// <summary>
/// Initializes the fade-out range.
/// </summary>
void InputDecoder::InitializeFade() noexcept
{
    _FadeRange.Clear();

    switch (_LoopType)
    {
        case LoopType::NeverLoop:
            break;

        case LoopType::NeverLoopAddDecayTime:
            break;

        case LoopType::LoopWhenDetectedAndFade:
        {
            if (_LoopRange.IsSet())
            {
                uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _RequestedSampleRate, 1'000);
                uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeTime,                                              (int) _RequestedSampleRate, 1'000);

                _FadeRange.Set(Begin, End);
            }
            else
                _FadeRange.Set(_FrameCount, _FrameCount);
            break;
        }

        case LoopType::RepeatAndFade:
        {
            uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _RequestedSampleRate, 1'000);
            uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeTime,                                              (int) _RequestedSampleRate, 1'000);

            _FadeRange.Set(Begin, End);
            break;
        }

        case LoopType::LoopWhenDetectedForever:
            break;

        case LoopType::RepeatForever:
            break;
    }
}

/// <summary>
/// Overrides the selected player depending on the metadata and some configuration settings.
/// </summary>
void InputDecoder::OverridePlayerSelection(preset_t & preset, size_t subSongIndex, abort_callback & abortHandler) noexcept
{
    _PlayerType = preset._PlayerType;
    _IsPlayerTypeOverriden = false;

    // Should the player be overriden by the metadata?
    midi::metadata_table_t MetaData;

    _Container.GetMetaData(subSongIndex, MetaData);

    for (const midi::metadata_item_t & Item : MetaData)
    {
        if (pfc::stricmp_ascii(Item.Name.c_str(), "type") == 0)
        {
            _IsMT32 = (Item.Value == "MT-32");
            _IsGS = (Item.Value == "GS");
            _IsXG = (Item.Value == "XG");
        }
    }

    if (_IsMT32 && CfgUseMT32EmuWithMT32)
    {
        _PlayerType = PlayerType::MT32Emu;
        _IsPlayerTypeOverriden = true;
    }
    else
    if (_IsGS && CfgUseSCWithGS && !CfgSecretSauceDirectoryPath.get().isEmpty())
    {
        _PlayerType = PlayerType::SecretSauce;
        _IsPlayerTypeOverriden = true;
    }
    else
    if (_IsXG && CfgUseVSTiWithXG)
    {
        pfc::string FilePath = CfgVSTiXGPlugInFilePath;

        if (FilePath.isEmpty())
            return;

        try
        {
            auto Player = new VSTi::Player;

            if (Player->LoadVST((const char8_t *) FilePath.c_str()))
            {
                _PlayerType = PlayerType::VSTi;
                _IsPlayerTypeOverriden = true;

                preset._PlugInFilePath = FilePath;
                preset._VSTiConfig = CfgVSTiConfig[Player->Id];

                delete Player;
            }
        }
        catch (...)
        {
        }
    }
}

const char * InputDecoder::PlayerTypeNames[15] =
{
    "LibEDMIDI",
    "VSTi",
    "FluidSynth",
    "LibMT32Emu",
    "BASS MIDI",
    "DirectX",      // Not implemented
    "LibADLMIDI",
    "LibOPNMIDI",
    "OPL",          // Not implemented
    "Nuked OPL3",
    "Secret Sauce",
    "MCI",          // Not implemented
    "Nuked SC-55",
    "FMMIDI",
    "CLAP",
};

static input_factory_t<InputDecoder> _Factory;
