 
/** $VER: InputDecoder.cpp (2025.07.09) **/

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

#include <Encoding.h>

#include <libsf.h>

#include "PreferencesFM.h"
#include "KaraokeProcessor.h"

/* KEEP? 06/07/25
volatile int _IsRunning = 0;

critical_section _Lock;
volatile uint32_t _CurrentSampleRate;
*/

/// <summary>
/// Creates a new instance.
/// </summary>
InputDecoder::InputDecoder() noexcept :
    _Flags(),

    _FileStats {},
    _FileStats2 {},

    _IsSysExFile(false),

    _IsMT32(),
    _IsXG(),

    _DetectRPGMakerLoops((bool) CfgDetectRPGMakerLoops),
    _DetectLeapFrogLoops((bool) CfgDetectLeapFrogLoops),
    _DetectXMILoops((bool) CfgDetectXMILoops),
    _DetectTouhouLoops((bool) CfgDetectTouhouLoops),
    _DetectFF7Loops((bool) CfgDetectFF7Loops),

    _Player(nullptr),
    _Host(nullptr),

    _PlayerType(),

    _SampleRate((uint32_t) CfgSampleRate),
    _ActualSampleRate(_SampleRate),

    _Time(),
    _TotalTime(),

    _LoopType(LoopTypes::NeverLoop),
    _LoopCount((uint32_t) AdvCfgLoopCount.get()),
    _LoopRange(),

    _FadeDuration((uint32_t) AdvCfgFadeTimeInMS.get()),
    _FadeRange(),

    _ExtraPercussionChannel(~0U),

    _FluidSynthInterpolationMethod((uint32_t) CfgFluidSynthInterpolationMode),

    _BASSMIDIVolume((float) CfgBASSMIDIVolume),
    _BASSMIDIInterpolationMode((uint32_t) CfgBASSMIDIResamplingMode)
{
    _CleanFlags = (uint32_t) (CfgEmuDeMIDIExclusion ? midi::container_t::CleanFlagEMIDI : 0) |
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
    for (const auto & sf : _SoundFonts)
    {
        if (sf.IsEmbedded())
            ::DeleteFileA(sf.FilePath().c_str());
    }

    if (_Player != nullptr)
    {
    //  console::print(::FormatText("%08X: " STR_COMPONENT_BASENAME " is deleting player 0x%016llX.", ::GetCurrentThreadId(), _Player).c_str());

        delete _Player;
        _Player = nullptr;
    }

    if ((_Flags & input_flag_playback) == 0)
    {
        delete _Host;
        _Host = nullptr;
    }

    if (_PlayerType == PlayerTypes::EmuDeMIDI)
    {
/*
        insync(_Lock);
        _IsRunning -= 1;
*/
    }
}

#pragma region input_impl

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
                midi::processor_t::Process(Object, pfc::wideFromUTF8(_FilePath), _Container);
            }
            catch (std::exception & e)
            {
                const pfc::string Message = "Failed to read SysEx file: ";

                throw exception_io_data(Message + e.what());
            }

            return;
        }
    }

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

                (uint16_t) CfgDefaultTempo
            );

            midi::processor_t::Process(Object, pfc::wideFromUTF8(_FilePath), _Container, Options);
        }
        catch (std::exception & e)
        {
            const pfc::string Message = "Failed to read MIDI file: ";

            throw exception_io_data(Message + e.what());
        }

        if (_Container.IsEmpty())
            throw exception_io_data("Invalid MIDI file: No tracks found");

        // Validate the MIDI data.
        {
            bool HasDuration = false;

            for (size_t i = 0, TrackCount = _Container.GetTrackCount(); i < TrackCount; ++i)
            {
                if (_Container.GetDuration(i) != 0)
                {
                    HasDuration = true;
                    break;
                }
            }

            if (!HasDuration)
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

    if (AdvCfgSkipToFirstNote)
        _Container.TrimStart();

    _LoopRange.Clear();
}

#pragma endregion

#pragma region input_decoder

/// <summary>
/// Initializes the decoder before playing the specified subsong.
/// Resets playback position to the beginning of specified subsong. This must be called first, before any other input_decoder methods (other than those inherited from input_info_reader).
/// It is legal to set initialize() more than once, with same or different subsong, to play either the same subsong again or another subsong from same file without full reopen.
/// </summary>
void InputDecoder::decode_initialize(unsigned subSongIndex, unsigned flags, abort_callback & abortHandler)
{
//  console::print(::FormatText("%08X: " STR_COMPONENT_BASENAME " is initializing the decoder.", ::GetCurrentThreadId()).c_str());

    _Flags = flags;

    if (_IsSysExFile)
        throw pfc::exception("You cannot play SysEx files");

    _IsFirstBlock = true;

    _PlayerType = (PlayerTypes) (uint32_t) CfgPlayerType;
    _LoopType = (LoopTypes) ((_Flags & input_flag_playback) ? (int) CfgLoopTypePlayback : (int) CfgLoopTypeOther);

    // Initialize time parameters.
    {
        _Time = 0;
        _TotalTime = (uint32_t) ::MulDiv((int) GetDuration(subSongIndex), (int) _SampleRate, 1'000);

        _LoopRange.Set(_Container.GetLoopBeginTimestamp(subSongIndex, true), _Container.GetLoopEndTimestamp(subSongIndex, true));

        InitializeFade();
    }

    _ExtraPercussionChannel = _Container.GetExtraPercussionChannel();

    // The preset collects a number of settings that will configure the player.
    preset_t Preset;

    // Load the preset from the song if it has one.
    {
        file_info_impl FileInfo;

        get_info(subSongIndex, FileInfo, abortHandler);

        {
            const char * PresetText = FileInfo.meta_get(TagPreset, 0);

            if (PresetText != nullptr)
            {
                console::print(STR_COMPONENT_BASENAME " is using preset \"", PresetText, "\" from tags.");

                Preset.Deserialize(PresetText);
            }
        }
    
        // Load the SysEx from the song if it has one.
        {
            MIDISysExDumps SysExDumps;

            const char * MIDISysExDumps = FileInfo.meta_get(TagMIDISysExDumps, 0);

            if (MIDISysExDumps != nullptr)
            {
                console::print(STR_COMPONENT_BASENAME " is using SysEx file \"", MIDISysExDumps , "\".");

                SysExDumps.Deserialize(MIDISysExDumps, _FilePath);
            }

            SysExDumps.Merge(_Container, abortHandler);
        }
    }

    // Set the player based on the preset. It can be overridden by the metadata or the sound fonts.
    _PlayerType = Preset._PlayerType;

    // Set the player depending on the metadata and some configuration settings.
    {
        midi::metadata_table_t MetaData;

        _Container.GetMetaData(subSongIndex, MetaData);

        for (const midi::metadata_item_t & Item : MetaData)
        {
            if (pfc::stricmp_ascii(Item.Name.c_str(), "type") == 0)
            {
                _IsMT32 = (Item.Value == "MT-32");
                _IsXG = (Item.Value == "XG");
            }
        }

        if (_IsMT32 && CfgUseMT32EmuWithMT32)
            _PlayerType = PlayerTypes::MT32Emu;
        else
        if (_IsXG && CfgUseVSTiWithXG)
        {
            _PlayerType = PlayerTypes::VSTi;

            pfc::string FilePath;

            AdvCfgVSTiXGPlugin.get(FilePath);

            Preset._PlugInFilePath = FilePath;
        }
    }

    // Load the sound fonts.
    GetSoundFonts(Preset._SoundFontFilePath, abortHandler);

    // Create and initialize the MIDI player.
    switch (_PlayerType)
    {
        case PlayerTypes::Unknown:
            break;

        // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413))
        case PlayerTypes::EmuDeMIDI:
        {
            auto Player = new EdMPlayer;

            _Player = Player;

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
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
            _Player->SetSampleRate(_SampleRate);

            _IsEndOfContainer = false;

            return;
        }

        // VSTi
        case PlayerTypes::VSTi:
        {
            if (Preset._PlugInFilePath.is_empty())
                throw pfc::exception("No plug-in specified in preset");

            auto Player = new VSTi::Player;

            if (!Player->LoadVST(Preset._PlugInFilePath.c_str()))
                throw pfc::exception(pfc::string("Unable to load VSTi plug-in from \"") + Preset._PlugInFilePath + "\"");

            if (Preset._VSTiConfig.size() != 0)
                Player->SetChunk(Preset._VSTiConfig.data(), Preset._VSTiConfig.size());

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
            {
                const std::string ErrorMessage = _Player->GetErrorMessage();

                if (!ErrorMessage.empty())
                    throw pfc::exception(ErrorMessage.c_str());
                else
                    throw pfc::exception("Failed to load MIDI stream");
            }

            _IsEndOfContainer = false;

            return;
        }

        // CLAP (CLever Audio Plug-in API)
        case PlayerTypes::CLAP:
        {
            if (Preset._PlugInFilePath.is_empty())
                throw pfc::exception("No plug-in specified in preset");

            if (_Flags & input_flag_playback)
                _Host = &CLAP::_Host; // Use the global instance for playback.
            else
                _Host = new CLAP::Host();

            if (!_Host->Load(Preset._PlugInFilePath.c_str(), Preset._CLAPPlugInIndex))
                return;

            if (!_Host->IsPlugInLoaded())
                return;
/*            
            if ((_Flags & input_flag_playback) && !core_api::is_quiet_mode_enabled())
            {
                fb2k::inMainThread2([this]() { _Host->ShowGUI(core_api::get_main_window()); });
            }
*/
            auto Player = new CLAPPlayer(_Host);

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // BASS MIDI
        case PlayerTypes::BASSMIDI:
        {
            {
                _ActiveVoiceCount = 0;
                _PeakVoiceCount = 0;

                if (_SoundFonts.empty())
                    throw pfc::exception("No compatible sound fonts found");
            }

            auto Player = new BMPlayer;

            Player->SetInterpolationMode(_BASSMIDIInterpolationMode);
            Player->SetVoiceCount(Preset._VoiceCount);
            Player->EnableEffects(Preset._EffectsEnabled);
            Player->SetSoundFonts(_SoundFonts);

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
            {
                const std::string ErrorMessage = _Player->GetErrorMessage();

                if (!ErrorMessage.empty())
                    throw pfc::exception(ErrorMessage.c_str());
                else
                    throw pfc::exception("Failed to load MIDI stream");
            }

            _IsEndOfContainer = false;

            return;
        }

        // FluidSynth
        case PlayerTypes::FluidSynth:
        {
            {
                _ActiveVoiceCount = 0;
                _PeakVoiceCount = 0;

                if (_SoundFonts.empty())
                    throw pfc::exception("No compatible sound fonts found");
            }

            auto Player = new FSPlayer;

            pfc::string DirectoryPath = CfgFluidSynthDirectoryPath;

            if (DirectoryPath.isEmpty())
            {
                console::warning(STR_COMPONENT_BASENAME " will attempt to load the FluidSynth libraries from the plugin install path because the FluidSynth directory path was not configured.");

                DirectoryPath = core_api::get_my_full_path();
                DirectoryPath.truncate(DirectoryPath.scan_filename());
            }

            Player->SetAbortHandler(&abortHandler);
            Player->Initialize(pfc::wideFromUTF8(DirectoryPath));

            Player->SetInterpolationMode(_FluidSynthInterpolationMethod);
            Player->SetVoiceCount(Preset._VoiceCount);
            Player->EnableEffects(Preset._EffectsEnabled);
            Player->EnableDynamicLoading(AdvCfgLoadSoundFontDynamically.get());
            Player->SetSoundFonts(_SoundFonts);

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
            {
                const std::string ErrorMessage = _Player->GetErrorMessage();

                if (!ErrorMessage.empty())
                    throw pfc::exception(ErrorMessage.c_str());
                else
                    throw pfc::exception("Failed to load MIDI stream");
            }

            _IsEndOfContainer = false;

            return;
        }

        // MT32Emu (MT-32)
        case PlayerTypes::MT32Emu:
        {
            auto Player = new MT32Player(_IsMT32, Preset._MT32EmuGMSet);

            pfc::string DirectoryPath = CfgMT32ROMDirectoryPath;

            if (DirectoryPath.is_empty())
            {
                console::warning(STR_COMPONENT_BASENAME " is attempting to load the MT-32 ROMs from the plugin install path because the MT32Emu ROM path was not configured.");

                DirectoryPath = core_api::get_my_full_path();
                DirectoryPath.truncate(DirectoryPath.scan_filename());
            }

            Player->SetAbortHandler(&abortHandler);
            Player->SetROMDirectory(DirectoryPath.c_str());
/*
            if (!Player->IsConfigValid())
                throw pfc::exception("The MT32Emu driver needs a valid MT-32 or CM32L ROM set to play.");
*/
            _Player = Player;

            _Player->SetSampleRate(_SampleRate);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // DirectX
        case PlayerTypes::DirectX:
        {
            throw pfc::exception("Player not implemented");
        }

        // LibADLMIDI
        case PlayerTypes::ADL:
        {
            auto Player = new ADLPlayer;

            Player->SetBankNumber(Preset._ADLBankNumber);
            Player->SetBankFilePath(Preset._ADLBankFilePath);
            Player->SetChipCount(Preset._ADLChipCount);
            Player->Set4OpChannelCount(Preset._ADLChipCount * 4);
            Player->SetSoftPanning(Preset._ADLSoftPanning); // Call after SetBankNumber()!
            Player->SetEmulatorCore(Preset._ADLEmulatorCore);

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // LibOPNMIDI
        case PlayerTypes::OPN:
        {
            auto Player = new OPNPlayer;

            Player->SetBankNumber(Preset._OPNBankNumber);
            Player->SetBankFilePath(Preset._OPNBankFilePath);
            Player->SetChipCount(Preset._OPNChipCount);
            Player->SetSoftPanning(Preset._OPNSoftPanning); // Call after SetBankNumber()!
            Player->SetEmulatorCore(Preset._OPNEmulatorCore);

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // OPL
        case PlayerTypes::OPL:
        {
            throw pfc::exception("Player not implemented");
        }

        // Nuked OPL3
        case PlayerTypes::NukedOPL3:
        {
            auto Player = new NukedOPL3Player();

            Player->SetSynth(Preset._NukeSynth);
            Player->SetBankNumber(Preset._NukeBank);
            Player->SetSoftPanning(Preset._NukeUsePanning);

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // Secret Sauce
        case PlayerTypes::SecretSauce:
        {
            auto Player = new SCPlayer();

            pfc::string PathName;

            AdvCfgSecretSauceDirectoryPath.get(PathName);

            if (PathName.is_empty())
            {
                console::warning(STR_COMPONENT_BASENAME " is attempting to load Secret Sauce from the plugin install path because the path was not configured.");

                PathName = core_api::get_my_full_path();
                PathName.truncate(PathName.scan_filename());
            }

            Player->SetRootPath(PathName.c_str());

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // MCI
        case PlayerTypes::MCI:
        {
            auto Player = new MCIPlayer;

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // Nuked SC-55
        case PlayerTypes::NukedSC55:
        {
            auto Player = new NukedSC55Player;

            _Player = Player;

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }

        // FMMIDI (yuno) (Yamaha YM2608)
        case PlayerTypes::FMMIDI:
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

            _Player->SetSampleRate(_SampleRate);
            _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

            if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                throw pfc::exception("Failed to load MIDI stream");

            _IsEndOfContainer = false;

            return;
        }
    }

    throw pfc::exception("No MIDI player specified");
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

        audioChunk.set_srate(_ActualSampleRate);
        audioChunk.set_channels(ChannelCount);
        audioChunk.set_sample_count(FramesDone);

        Success = true;
    }

    // Scale the samples if fading was requested.
    if (Success && _FadeRange.IsSet())
    {
        uint32_t BlockBegin = _Time;
        uint32_t BlockEnd   = _Time + (uint32_t) audioChunk.get_sample_count();

        _Time = BlockEnd;

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
    _Time = (uint32_t) (timeInSeconds * _ActualSampleRate);
    _IsFirstBlock = true;
    _IsEndOfContainer = false;

    uint32_t OffsetInMs = (uint32_t) (timeInSeconds * 1'000.);

    if (OffsetInMs > _LoopRange.End())
        OffsetInMs = _LoopRange.Begin() + (OffsetInMs - _LoopRange.Begin()) % ((_LoopRange.Size() != 0) ? _LoopRange.Size() : 1);

    const uint32_t OffsetInSamples = (uint32_t) ::MulDiv((int) OffsetInMs, (int) _ActualSampleRate, 1'000);

    if ((_TotalTime != 0U) && (OffsetInSamples >= (_TotalTime - _ActualSampleRate)))
    {
        _IsEndOfContainer = true;

        return;
    }

    if (_Player)
        _Player->Seek(OffsetInSamples);
}

/// <summary>
/// Signals dynamic VBR bitrate, etc. Called after each decode_run() (or not called at all if caller doesn't care about dynamic info).
/// </summary>
bool InputDecoder::decode_get_dynamic_info(file_info & fileInfo, double & timestampDelta)
{
    bool Success = false;

    if (_IsFirstBlock)
    {
        {
            fileInfo.info_set_int(InfoSampleRate, _ActualSampleRate);
        }

        {
            assert(_countof(PlayerTypeNames) == ((size_t) PlayerTypes::Max + 1));

            const char * Value = "Unknown";

            if ((_PlayerType >= PlayerTypes::Min) && (_PlayerType <= PlayerTypes::Max))
                Value = PlayerTypeNames[(size_t) _PlayerType];
            else
                Value = "VSTi";

            fileInfo.info_set(InfoMIDIPlayer, Value);
        }

        {
            pfc::string Value;

            #pragma warning(disable: 4062) // enumerator x in switch of y is not explicitly handled by a case label

            switch (_PlayerType)
            {
                case PlayerTypes::ADL:
                {
                    const auto TargetId = (int) CfgADLEmulator;

                    auto Match = std::ranges::find_if(_OPNEmulators, [TargetId](const emulator_t & em)
                    {
                        return em.Id == TargetId;
                    });

                    Value = (Match != _ADLEmulators.end()) ? ::WideToUTF8(Match->Name).c_str() : "";
                    break;
                }

                case PlayerTypes::OPN:
                {
                    const auto TargetId = (int) CfgOPNEmulator;

                    auto Match = std::ranges::find_if(_OPNEmulators, [TargetId](const emulator_t & em)
                    {
                        return em.Id == TargetId;
                    });

                    Value = (Match != _OPNEmulators.end()) ? ::WideToUTF8(Match->Name).c_str() : "";
                    break;
                }

                case PlayerTypes::VSTi:
                case PlayerTypes::CLAP:
                {
                    Value = CfgPlugInName.get();
                    break;
                }
            };

            #pragma warning(default: 4062)

            fileInfo.info_set(InfoMIDIPlayerExt, Value);
        }

        {
            if (_ExtraPercussionChannel != ~0L)
                fileInfo.info_set(InfoMIDIExtraPercusionChannel, pfc::format_int((t_int64) _ExtraPercussionChannel + 1));
        }

        _IsFirstBlock = false;

        Success = true;
        timestampDelta = 0.;
    }

    if ((_PlayerType == PlayerTypes::FluidSynth) || (_PlayerType == PlayerTypes::BASSMIDI))
    {
        uint32_t VoiceCount = 0;

        if (_PlayerType == PlayerTypes::FluidSynth)
        {
            auto Player = (FSPlayer *) _Player;

            VoiceCount = Player->GetActiveVoiceCount();
        }
        else
        if (_PlayerType == PlayerTypes::BASSMIDI)
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

#pragma endregion

#pragma region input_info_reader

/// <summary>
/// Retrieves information about the specified subsong.
/// </summary>
void InputDecoder::get_info(t_uint32 subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        return;

    ConvertMetaDataToTags(subSongIndex, fileInfo, abortHandler);

    fileInfo.set_length((double) _Container.GetDuration(subSongIndex, true) * 0.001); // Without loops, fade-out or decay.
}

#pragma endregion

#pragma region input_info_writer

/// <summary>
/// Sets the tags for the specified file.
/// </summary>
void InputDecoder::retag_set_info(t_uint32, const file_info & fileInfo, abort_callback & abortHandler) const
{
    if (_IsSysExFile)
        throw pfc::exception("You cannot tag SysEx files.");

    file_info_impl fi(fileInfo);

    // Move the SysEx dumps from the information field to a tag.
    {
        // Remove all instances of the tag.
        fi.meta_remove_field(TagMIDISysExDumps);

        // Get the value from the information field.
        const char * SysExDumps = fi.info_get(TagMIDISysExDumps);

        // Add the tag.
        if (SysExDumps != nullptr)
            fi.meta_set(TagMIDISysExDumps, SysExDumps);
    }

    // Update the metadb.
    {
        file::ptr Stream;

        filesystem::g_open_tempmem(Stream, abortHandler);

        tag_processor::write_apev2(Stream, fi, abortHandler);

        Stream->seek(0, abortHandler);

        pfc::array_t<t_uint8> Tags;

        Tags.set_count((t_size) Stream->get_size_ex(abortHandler));

        Stream->read_object(Tags.get_ptr(), Tags.get_count(), abortHandler);

        static_api_ptr_t<metadb_index_manager>()->set_user_data(GUIDTagMIDIHash, _Hash, Tags.get_ptr(), Tags.get_count());
    }
}

#pragma endregion

#pragma region Private

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
/// Gets the total duration of the specified sub-song taking into account looping, fade-out and decay time (in ms).
/// </summary>
uint32_t InputDecoder::GetDuration(size_t subSongIndex) noexcept
{
    uint32_t LengthInMs = _Container.GetDuration(subSongIndex, true);

    if (_LoopType == LoopTypes::NeverLoopAddDecayTime)
        LengthInMs += (uint32_t) CfgDecayTime;
    else
    if (_LoopType > LoopTypes::LoopAndFadeWhenDetected)
    {
        if (!_LoopRange.HasBegin())
            _LoopRange.SetBegin(0);

        if (!_LoopRange.HasEnd())
            _LoopRange.SetEnd(LengthInMs);

        LengthInMs = _LoopRange.Begin() + (_LoopRange.Size() * _LoopCount) + _FadeDuration;
    }

    return LengthInMs;
}

/// <summary>
/// Initializes the fade-out range.
/// </summary>
void InputDecoder::InitializeFade() noexcept
{
    _FadeRange.Clear();

    switch (_LoopType)
    {
        case LoopTypes::NeverLoop:
            break;

        case LoopTypes::NeverLoopAddDecayTime:
            break;

        case LoopTypes::LoopAndFadeWhenDetected:
        {
            if (_LoopRange.IsSet())
            {
                uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _SampleRate, 1'000);
                uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                          (int) _SampleRate, 1'000);

                _FadeRange.Set(Begin, End);
            }
            else
                _FadeRange.Set(_TotalTime, _TotalTime);
            break;
        }

        case LoopTypes::LoopAndFadeAlways:
        {
            uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _SampleRate, 1'000);
            uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                          (int) _SampleRate, 1'000);

            _FadeRange.Set(Begin, End);
            break;
        }

        case LoopTypes::PlayIndefinitelyWhenDetected:
            break;

        case LoopTypes::PlayIndefinitely:
            break;
    }
}

#pragma endregion

#pragma region Tags

/// <summary>
/// Converts the MIDI metadata to tags.
/// </summary>
void InputDecoder::ConvertMetaDataToTags(size_t subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    _IsMT32 = false;
    _IsXG = false;

    KaraokeProcessor kp;

    {
        midi::metadata_table_t MetaData;

        _Container.GetMetaData(subSongIndex, MetaData);

        midi::metadata_item_t TrackItem;

        bool HasTitle = MetaData.GetItem("title", TrackItem);

        {
            for (const auto & Item : MetaData)
            {
                if (pfc::stricmp_ascii(Item.Name.c_str(), "type") == 0)
                {
                    fileInfo.info_set(TagMIDIType, Item.Value.c_str());

                    _IsMT32 = (Item.Value == "MT-32");
                    _IsXG = (Item.Value == "XG");
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "lyrics_type") == 0)
                {
                    fileInfo.info_set(TagMIDILyricsType, Item.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "lyrics") == 0)
                {
                    kp.AddUnsyncedLyrics(Item.Timestamp, Item.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "soft_karaoke_lyrics") == 0)
                {
                    kp.AddSyncedLyrics(Item.Timestamp, Item.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "soft_karaoke_text") == 0)
                {
                    kp.AddSyncedText(Item.Value.c_str());
                }
                else
                {
                    std::string Name = Item.Name;

                    if (!HasTitle && (pfc::stricmp_ascii(Name.c_str(), "display_name") == 0))
                        Name = "title";

                    AddTag(fileInfo, Name.c_str(), Item.Value.c_str(), 0);
                }
            }
        }
    }

    // Read a WRD file in the same path and convert it to lyrics.
    {
        std::filesystem::path FilePath(pfc::wideFromUTF8(_FilePath.c_str()).c_str());

        FilePath.replace_extension(L".wrd");

        std::ifstream Stream(FilePath);

        if (Stream.is_open())
        {
            std::string Line;

            while (std::getline(Stream, Line))
            {
                if (Line[0] != '@')
                    kp.AddUnsyncedLyrics(0, Line.c_str());
            }
        }
    }

    if (!kp.GetUnsyncedLyrics().empty())
    {
        const auto Lyrics = kp.GetUnsyncedLyrics();

        fileInfo.meta_set("lyrics", (const char *) Lyrics.c_str());
    }

    if (!kp.GetSyncedLyrics().empty())
    {
        const auto Lyrics = kp.GetSyncedLyrics();

        fileInfo.meta_set("syncedlyrics", (const char *) Lyrics.c_str());
    }

    // General info
    fileInfo.info_set_int   ("channels", 2);
    fileInfo.info_set       ("encoding", "Synthesized");

    // Specific info
    fileInfo.info_set_int   (TagMIDIFormat,       _Container.GetFormat());
    fileInfo.info_set_int   (TagMIDITrackCount,   _Container.GetFormat() == 2 ? 1 : _Container.GetTrackCount());
    fileInfo.info_set_int   (TagMIDIChannelCount, _Container.GetChannelCount(subSongIndex));
    fileInfo.info_set_int   (TagMIDITicks,        _Container.GetDuration(subSongIndex));

    {
        uint32_t LoopBegin     = _Container.GetLoopBeginTimestamp(subSongIndex);
        uint32_t LoopEnd       = _Container.GetLoopEndTimestamp  (subSongIndex);
        uint32_t LoopBeginInMS = _Container.GetLoopBeginTimestamp(subSongIndex, true);
        uint32_t LoopEndInMS   = _Container.GetLoopEndTimestamp  (subSongIndex, true);

        if (LoopBegin != ~0U)     fileInfo.info_set_int(TagMIDILoopStart, LoopBegin);
        if (LoopEnd != ~0U)       fileInfo.info_set_int(TagMIDILoopEnd, LoopEnd);
        if (LoopBeginInMS != ~0U) fileInfo.info_set_int(TagMIDILoopStartInMs, LoopBeginInMS);
        if (LoopEndInMS != ~0U)   fileInfo.info_set_int(TagMIDILoopEndInMs, LoopEndInMS);
    }

    // Add a tag that identifies the embedded sound font, if present.
    try
    {
        const auto & Data = _Container.GetSoundFontData();

        if (Data.size() > 12)
        {
            std::string TagValue("DLS");

            if (::memcmp(Data.data() + 8, "DLS ", 4) != 0)
            {
                sf::bank_t sf;

                riff::memory_stream_t ms;

                if (ms.Open(Data.data(), Data.size()))
                {
                    sf::reader_t sr;

                    if (sr.Open(&ms, riff::reader_t::option_t::None))
                    {
                        sr.Process({ false }, sf); // Don't load the sample data.

                        TagValue = ::FormatText("SF %d.%d", sf.Major, sf.Minor);
                    }

                    ms.Close();
                }
            }

            fileInfo.info_set(TagMIDIEmbeddedSoundFont, TagValue.c_str());
        }
    }
    catch (std::exception e)
    {
        console::print(STR_COMPANY_NAME " is unable to create tag \"", TagMIDIEmbeddedSoundFont, "\": ", e.what());
    }

    {
        pfc::string FileHashString;

        for (size_t i = 0U; i < 16; ++i)
            FileHashString += pfc::format_uint((t_uint8)_FileHash.m_data[i], 2, 16);

        fileInfo.info_set(TagMIDIHash, FileHashString);
    }

    {
        service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<FileHasher>;

        _Hash = IndexClient->transform(fileInfo, playable_location_impl(_FilePath, (t_uint32) subSongIndex));

        pfc::array_t<t_uint8> Tags;

        static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTagMIDIHash, _Hash, Tags);

        t_size TagCount = Tags.get_count();

        if (TagCount > 0)
        {
            file::ptr File;

            filesystem::g_open_tempmem(File, abortHandler);

            File->write_object(Tags.get_ptr(), TagCount, abortHandler);

            fileInfo.meta_remove_all();

            tag_processor::read_trailing(File, fileInfo, abortHandler);

            fileInfo.info_set("tagtype", "apev2 db");
        }
    }
}

/// <summary>
/// Adds the specified tag.
/// </summary>
void InputDecoder::AddTag(file_info & fileInfo, const char * name, const char * value, t_size maxLength)
{
    if ((name == nullptr) || (value == nullptr))
        return;

    if (value[0] == '\0')
        return;

    pfc::string Value;

    if ((maxLength != 0) && value[maxLength - 1])
    {
        Value.set_string(value, maxLength);
        value = Value;
    }
    else
        maxLength = ::strlen(value);

    if (!pfc::is_lower_ascii(value) && !pfc::is_valid_utf8(value, maxLength))
    {
        if (IsShiftJIS(value, maxLength))
        {
            Value = pfc::stringcvt::string_utf8_from_codepage(932, value);
        }
        else
        if (IsEUCJP(value, maxLength))
        {
            Value = pfc::stringcvt::string_utf8_from_codepage(20932, value);

            // Try other code pages.
            if (Value.is_empty())
                Value = pfc::stringcvt::string_utf8_from_codepage(51932, value); // EUC Japanese
        }
        else
            Value = pfc::stringcvt::string_utf8_from_ansi(value);

        if (!Value.is_empty())
            value = Value;
    }

    fileInfo.meta_add(name, value);
}

#pragma endregion

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

static input_factory_t<InputDecoder> InputDecoderFactory;
