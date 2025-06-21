 
/** $VER: InputDecoder.cpp (2025.06.18) **/

#include "pch.h"

#include "InputDecoder.h"

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include <pfc/string-conv-lite.h>

#include "KaraokeProcessor.h"

#include <math.h>
#include <string.h>
#include <filesystem>

#include <Encoding.h>

#include <libsf.h>

volatile int _IsRunning = 0;

critical_section _Lock;
volatile uint32_t _CurrentSampleRate;

const GUID GUIDTagMIDIHash = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };

const char * PlayerTypeNames[] =
{
    "LibEDMIDI",
    "VSTi",
    "FluidSynth",
    "SuperMunt",
    "BASS MIDI",
    "DirectX",
    "LibADLMIDI",
    "LibOPNMIDI",
    "OPL",
    "Nuke",
    "Secret Sauce",
    "MCI",
    "Nuked SC-55",
    "FMMIDI"
};

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

        _TrackCount = _Container.GetTrackCount();

        if (_TrackCount == 0)
            throw exception_io_data("Invalid MIDI file: No tracks found");

        // Validate the MIDI data.
        {
            bool HasDuration = false;

            for (size_t i = 0; i < _TrackCount; ++i)
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
    if (_IsSysExFile)
        throw midi::exception_t("You cannot play SysEx files.");

    _IsFirstChunk = true;

    _PlayerType = (PlayerType) (uint32_t) CfgPlayerType;
    _LoopType = (flags & input_flag_playback) ? _LoopTypePlayback : _LoopTypeOther;
    _TimeInSamples = 0;

    // Initialize time.
    _LoopRange.Set(_Container.GetLoopBeginTimestamp(subSongIndex, true), _Container.GetLoopEndTimestamp(subSongIndex, true));

    _LengthInSamples = (uint32_t) ::MulDiv((int) GetDuration(subSongIndex), (int) _SampleRate, 1000);

    _Container.SetTrackCount(_TrackCount);

    _ExtraPercussionChannel = _Container.GetExtraPercussionChannel();

    // The preset collects a number of settings that will configure the player.
    MIDIPreset Preset;

    // Load the preset from the song if it has one.
    {
        file_info_impl FileInfo;

        get_info(subSongIndex, FileInfo, abortHandler);

        {
            const char * MIDIPresetText = FileInfo.meta_get(TagMIDIPreset, 0);

            if (MIDIPresetText != nullptr)
            {
                console::print(STR_COMPONENT_BASENAME " is using preset \"", MIDIPresetText, "\" from tags.");

                Preset.Deserialize(MIDIPresetText);
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

        if (_IsMT32 && CfgUseSuperMuntWithMT32)
            _PlayerType = PlayerType::SuperMunt;
        else
        if (_IsXG && CfgUseVSTiWithXG)
        {
            _PlayerType = PlayerType::VSTi;

            pfc::string FilePath;

            AdvCfgVSTiXGPlugin.get(FilePath);

            Preset._VSTiFilePath = FilePath;
        }
    }

    // Load the sound fonts.
    GetSoundFonts(Preset._SoundFontFilePath, abortHandler);

    // Update sample rate before initializing the fade-out range.
    if (_PlayerType == PlayerType::SuperMunt)
        _SampleRate = (uint32_t) MT32Player::GetSampleRate();

    // Initialize the fade-out range.
    {
        _FadeRange.Clear();

        switch (_LoopType)
        {
            case LoopType::NeverLoop:
                break;

            case LoopType::NeverLoopAddDecayTime:
                break;

            case LoopType::LoopAndFadeWhenDetected:
            {
                if (_LoopRange.IsSet())
                {
                    uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _SampleRate, 1000);
                    uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                          (int) _SampleRate, 1000);

                    _FadeRange.Set(Begin, End);
                }
                else
                    _FadeRange.Set(_LengthInSamples, _LengthInSamples);
                break;
            }

            case LoopType::LoopAndFadeAlways:
            {
                uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _SampleRate, 1000);
                uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                          (int) _SampleRate, 1000);

                _FadeRange.Set(Begin, End);
                break;
            }

            case LoopType::PlayIndefinitelyWhenDetected:
                break;

            case LoopType::PlayIndefinitely:
                break;
        }
    }

    // Create and initialize the MIDI player.
    delete _Player;
    _Player = nullptr;

    switch (_PlayerType)
    {
        case PlayerType::Unknown:
            break;

        // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413))
        case PlayerType::EmuDeMIDI:
        {
            {
                auto Player = new EdMPlayer;

                _Player = Player;
            }

            {
                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    {
                        insync(_Lock);

                        _IsRunning += 1;

                        if (_IsRunning == 1)
                            _CurrentSampleRate = _SampleRate;
                        else
                        if (_SampleRate != _CurrentSampleRate)
                            _SampleRate = _CurrentSampleRate;
                    }

                    _Player->SetSampleRate(_SampleRate);

                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // VSTi
        case PlayerType::VSTi:
        {
            {
                if (Preset._VSTiFilePath.is_empty())
                    throw midi::exception_t("No VSTi specified in preset.");

                auto Player = new VSTiPlayer;

                if (!Player->LoadVST(Preset._VSTiFilePath))
                    throw midi::exception_t(pfc::string("Unable to load VSTi from \"") + Preset._VSTiFilePath + "\".");
            
                if (Preset._VSTiConfig.size() != 0)
                    Player->SetChunk(Preset._VSTiConfig.data(), Preset._VSTiConfig.size());

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
                else
                {
                    std::string ErrorMessage;

                    if (_Player->GetErrorMessage(ErrorMessage))
                        throw exception_io_data(ErrorMessage.c_str());
                }
            }
            break;
        }

        // FluidSynth
        case PlayerType::FluidSynth:
        {
            {
                {
                    _ActiveVoiceCount = 0;
                    _PeakVoiceCount = 0;

                    if (_SoundFonts.empty())
                        throw midi::exception_t("No compatible sound fonts found.");
                }

                pfc::string FluidSynthDirectoryPath = CfgFluidSynthDirectoryPath;

                if (FluidSynthDirectoryPath.isEmpty())
                {
                    console::warning(STR_COMPONENT_BASENAME " will attempt to load the FluidSynth libraries from the plugin install path because the FluidSynth directory path was not configured.");

                    FluidSynthDirectoryPath = core_api::get_my_full_path();
                    FluidSynthDirectoryPath.truncate(FluidSynthDirectoryPath.scan_filename());
                }

                auto Player = new FSPlayer;

                Player->SetAbortHandler(&abortHandler);
                Player->Initialize(pfc::wideFromUTF8(FluidSynthDirectoryPath));

                {
                    DWORD Version = Player->GetVersion();

                    console::print(STR_COMPONENT_BASENAME " is using FluidSynth ", (Version >> 24) & 0xFF, ".", (Version >> 16) & 0xFF, ".", (Version >> 8) & 0xFF, ".");
                }

                Player->SetInterpolationMode(_FluidSynthInterpolationMethod);
                Player->SetVoiceCount(Preset._VoiceCount);
                Player->EnableEffects(Preset._EffectsEnabled);
                Player->EnableDynamicLoading(AdvCfgLoadSoundFontDynamically.get());
                Player->SetSoundFonts(_SoundFonts);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    std::string ErrorMessage;

                    if (_Player->GetErrorMessage(ErrorMessage))
                        throw exception_io_data(ErrorMessage.c_str());

                    break;
                }

                _IsEndOfContainer = false;

                return;
            }
            break;
        }

        // Munt (MT-32)
        case PlayerType::SuperMunt:
        {
            {
                auto Player = new MT32Player(!_IsMT32, Preset._MuntGMSet);

                pfc::string BasePath = CfgMT32ROMDirectoryPath;

                if (BasePath.is_empty())
                {
                    console::warning(STR_COMPONENT_BASENAME " is attempting to load the MT-32 ROMs from the plugin install path because the SuperMunt ROM path was not configured.");

                    BasePath = core_api::get_my_full_path();
                    BasePath.truncate(BasePath.scan_filename());
                }

                Player->SetBasePath(BasePath);
                Player->SetAbortHandler(&abortHandler);

                if (!Player->IsConfigValid())
                    throw midi::exception_t("The Munt driver needs to be configured with a valid MT-32 or CM32L ROM set.");

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // BASS MIDI
        case PlayerType::BASSMIDI:
        {
            {
                {
                    _ActiveVoiceCount = 0;
                    _PeakVoiceCount = 0;

                    if (_SoundFonts.empty())
                        throw midi::exception_t("No compatible sound fonts found.");
                }

                auto Player = new BMPlayer;

                {
                    DWORD BASSVersion = Player->GetVersion();

                    console::print(STR_COMPONENT_BASENAME " is using BASS ", (BASSVersion >> 24) & 0xFF, ".", (BASSVersion >> 16) & 0xFF, ".", (BASSVersion >> 8) & 0xFF, ".", BASSVersion & 0xFF, ".");

                    DWORD BASSMIDIVersion = Player->GetMIDIVersion();

                    console::print(STR_COMPONENT_BASENAME " is using BASS MIDI ", (BASSMIDIVersion >> 24) & 0xFF, ".", (BASSMIDIVersion >> 16) & 0xFF, ".", (BASSMIDIVersion >> 8) & 0xFF, ".", BASSMIDIVersion & 0xFF, ".");
                }

                Player->SetInterpolationMode(_BASSMIDIInterpolationMode);
                Player->SetVoiceCount(Preset._VoiceCount);
                Player->EnableEffects(Preset._EffectsEnabled);
                Player->SetSoundFonts(_SoundFonts);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (!_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    std::string ErrorMessage;

                    if (_Player->GetErrorMessage(ErrorMessage))
                        throw exception_io_data(ErrorMessage.c_str());

                    break;
                }

                _IsEndOfContainer = false;

                return;
            }
            break;
        }

        // DirectX
        case PlayerType::DirectX:
        {
    #ifdef DXISUPPORT
            pfc::array_t<t_uint8> serialized_midi_file;
            midi_file.SerializeAsSMF(serialized_midi_file);

            delete dxiProxy;
            dxiProxy = nullptr;

            dxiProxy = new DXiProxy;
            if (SUCCEEDED(dxiProxy->initialize()))
            {
                dxiProxy->SetSampleRate(srate);
                if (SUCCEEDED(dxiProxy->setSequence(serialized_midi_file.get_ptr(), serialized_midi_file.get_count())))
                {
                    if (SUCCEEDED(dxiProxy->setPlugin(thePreset.dxi_plugin)))
                    {
                        dxiProxy->Stop();
                        dxiProxy->Play(TRUE);

                        eof = false;
                        _DontLoop = true;

                        if (doing_loop)
                        {
                            set_loop();
                        }

                        return;
                    }
                }
            }
    #endif
            break;
        }

        // ADL
        case PlayerType::ADL:
        {
            {
                auto Player = new ADLPlayer;

                Player->SetBank(Preset._ADLBankNumber);
                Player->SetBankFilePath(Preset._ADLBankFilePath);
                Player->SetChipCount(Preset._ADLChipCount);
                Player->SetFullPanning(Preset._ADLUsePanning);
                Player->Set4OpCount(Preset._ADLChipCount * 4 /*cfg_adl_4op*/);
                Player->SetCore(Preset._ADLEmulatorCore);
                Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // OPN
        case PlayerType::OPN:
        {
            {
                auto Player = new OPNPlayer;

                Player->SetBank(Preset._OPNBankNumber);
                Player->SetChipCount(Preset._ADLChipCount);
                Player->SetFullPanning(Preset._ADLUsePanning);
                Player->SetCore(Preset._OPNEmulatorCore);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // OPL
        case PlayerType::OPL:
        {
            break;
        }

        // Nuke
        case PlayerType::Nuke:
        {
            {
                auto Player = new NukePlayer();

                Player->SetSynth(Preset._NukeSynth);
                Player->SetBank(Preset._NukeBank);
                Player->SetExtp(Preset._NukeUsePanning);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // Secret Sauce
        case PlayerType::SecretSauce:
        {
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

                Player->SetRootPath(PathName);

                _Player = Player;
            }

            {
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // MCI
        case PlayerType::MCI:
        {
            {
                auto Player = new MCIPlayer;

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // Nuked SC-55
        case PlayerType::NukedSC55:
        {
/*
            {
                auto Player = new NukeSC55Player;

                _Player = Player;

                Player->SetBasePath(LR"(F:\MIDI\_foobar2000 Support\Nuked SC55mk2\SC-55mk2-v1.01)");
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
*/
            break;
        }

        // fmmidi (yuno) (Yamaha YM2608)
        case PlayerType::FMMIDI:
        {
            {
                auto Player = new FMMPlayer;

                pfc::string ProgramsFilePath = CfgProgramsFilePath;

                if (ProgramsFilePath.empty())
                {
                    ProgramsFilePath = pfc::io::path::getParent(core_api::get_my_full_path());

                    ProgramsFilePath.add_filename(FMMPlayer::DefaultProgramsFileName.c_str());
                }

                Player->SetProgramsFilePath(pfc::stringcvt::string_os_from_utf8(ProgramsFilePath).get_ptr());
/*
    auto SystemMode = midisynth::system_mode_t::system_mode_default;

    switch (_MIDIFlavor)
    {
        case MIDIFlavor::GM: SystemMode = midisynth::system_mode_t::system_mode_default; break;
        case MIDIFlavor::GM2: SystemMode = midisynth::system_mode_t::system_mode_gm2; break;
        case MIDIFlavor::XG: SystemMode = midisynth::system_mode_t::system_mode_xg; break;

        case MIDIFlavor::Default:

        case MIDIFlavor::SC55:
        case MIDIFlavor::SC88:
        case MIDIFlavor::SC88Pro:
        case MIDIFlavor::SC8850:

        default: SystemMode = midisynth::system_mode_t::system_mode_gs; break;
    }

    for (size_t i = 0; i < 4; ++i)
        _Synthesizers[i]->set_system_mode(SystemMode);

*/
                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }
    }

    throw midi::exception_t("No MIDI player specified.");
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

#ifdef DXISUPPORT
    if (_PlayerType == PlayerType::DirectX)
    {
        unsigned todo = 4096;

        if (_DontLoop)
        {
            if (length_samples && samples_done + todo > length_samples)
            {
                todo = length_samples - samples_done;
                if (!todo) return false;
            }
        }

    #if audio_sample_size != 32
        sample_buffer.grow_size(todo * 2);

        float * ptr = sample_buffer.get_ptr();

        thePlayer->FillBuffer(ptr, todo);

        audioChunk.set_data_32(ptr, todo, 2, srate);
    #else
        audioChunk.set_data_size(todo * 2);

        float * ptr = audioChunk.get_data();

        dxiProxy->fillBuffer(ptr, todo);

        audioChunk.set_srate(srate);
        audioChunk.set_channels(2);
        audioChunk.set_sample_count(todo);
    #endif

        samples_done += todo;

        rv = true;
    }
    else
#endif

    if (_Player)
    {
        const uint32_t SamplesToDo = 4096;
        const uint32_t ChannelCount = _Player->GetAudioChannelCount();

        audioChunk.set_data_size((t_size) SamplesToDo * ChannelCount);

        audio_sample * Samples = audioChunk.get_data();

        _Player->SetAbortHandler(&abortHandler);

        uint32_t SamplesDone = _Player->Play(Samples, SamplesToDo);

        if (SamplesDone == 0)
        {
            std::string ErrorMessage;

            if (_Player->GetErrorMessage(ErrorMessage) != 0)
                throw exception_io_data(ErrorMessage.c_str());

            return false;
        }

        audioChunk.set_srate(_SampleRate);
        audioChunk.set_channels(ChannelCount);
        audioChunk.set_sample_count(SamplesDone);

        Success = true;
    }

    // Scale the samples if fading was requested.
    if (Success && _FadeRange.IsSet())
    {
        uint32_t BeginOfChunk = _TimeInSamples;
        uint32_t EndOfChunk   = _TimeInSamples + (uint32_t) audioChunk.get_sample_count();

        _TimeInSamples = EndOfChunk;

        if (EndOfChunk >= _FadeRange.Begin())
        {
            for (size_t i = std::max(_FadeRange.Begin(), BeginOfChunk), j = std::min(EndOfChunk, _FadeRange.End()); i < j; ++i)
            {
                audio_sample * Sample = audioChunk.get_data() + (i - BeginOfChunk) * 2;

                audio_sample Scale = (audio_sample) (_FadeRange.End() - i) / (audio_sample) _FadeRange.Size();

                Sample[0] *= Scale;
                Sample[1] *= Scale;
            }

            if (EndOfChunk > _FadeRange.End())
            {
                uint32_t SamplesRemaining = 0;

                if (_FadeRange.End() > BeginOfChunk)
                    SamplesRemaining = _FadeRange.End() - BeginOfChunk;

                audioChunk.set_sample_count(SamplesRemaining);

                _IsEndOfContainer = true;

                if (SamplesRemaining == 0)
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
    _TimeInSamples = (uint32_t) (timeInSeconds * _SampleRate);
    _IsFirstChunk = true;
    _IsEndOfContainer = false;

    uint32_t OffsetInMs = (uint32_t) (timeInSeconds * 1000.);

    if (OffsetInMs > _LoopRange.End())
        OffsetInMs = _LoopRange.Begin() + (OffsetInMs - _LoopRange.Begin()) % ((_LoopRange.Size() != 0) ? _LoopRange.Size() : 1);

    const uint32_t OffsetInSamples = (uint32_t) ::MulDiv((int) OffsetInMs, (int) _SampleRate, 1000);

    if ((_LengthInSamples != 0U) && (OffsetInSamples >= (_LengthInSamples - _SampleRate)))
    {
        _IsEndOfContainer = true;

        return;
    }

#ifdef DXISUPPORT
    if (_PlayerType == PlayerType::DirectX)
    {
        dxiProxy->setPosition(seek_msec);

        samples_done = done;
    }
    else
#endif
    if (_Player)
        _Player->Seek(OffsetInSamples);
}

/// <summary>
/// Signals dynamic VBR bitrate, etc. Called after each decode_run() (or not called at all if caller doesn't care about dynamic info).
/// </summary>
bool InputDecoder::decode_get_dynamic_info(file_info & fileInfo, double & timestampDelta)
{
    bool Success = false;

    if (_IsFirstChunk)
    {
        {
            fileInfo.info_set_int(TagSampleRate, _SampleRate);
        }

        {
            assert(_countof(PlayerTypeNames) == ((size_t) PlayerType::Max + 1));

            const char * PlayerName = "Unknown";

            if ((_PlayerType >= PlayerType::Min) && (_PlayerType <= PlayerType::Max))
                PlayerName = PlayerTypeNames[(size_t) _PlayerType];
            else
                PlayerName = "VSTi";

            fileInfo.info_set(TagMIDIPlayer, PlayerName);
        }

        {
            if (_ExtraPercussionChannel != ~0L)
                fileInfo.info_set(TagMIDIExtraPercusionChannel, pfc::format_int((t_int64) _ExtraPercussionChannel + 1));
        }

        _IsFirstChunk = false;

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
            fileInfo.info_set_int(TagMIDIActiveVoices, (t_int64) VoiceCount);

            _ActiveVoiceCount = VoiceCount;

            Success = true;
        }

        if (VoiceCount > _PeakVoiceCount)
        {
            fileInfo.info_set_int(TagMIDIPeakVoices, (t_int64) VoiceCount);

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

    fileInfo.set_length(_Container.GetDuration(subSongIndex, true) * 0.001);
}

#pragma endregion

#pragma region input_info_writer

/// <summary>
/// Set the tags for the specified file.
/// </summary>
void InputDecoder::retag_set_info(t_uint32, const file_info & fileInfo, abort_callback & abortHandler) const
{
    if (_IsSysExFile)
        throw exception_io_data("You cannot tag SysEx files.");

    file_info_impl fi(fileInfo);

    {
        fi.meta_remove_field(TagMIDISysExDumps);

        const char * SysExDumps = fi.info_get(TagMIDISysExDumps);

        if (SysExDumps)
            fi.meta_set(TagMIDISysExDumps, SysExDumps);
    }

    {
        file::ptr TagFile;

        filesystem::g_open_tempmem(TagFile, abortHandler);

        tag_processor::write_apev2(TagFile, fi, abortHandler);

        {
            pfc::array_t<t_uint8> Tag;

            TagFile->seek(0, abortHandler);

            Tag.set_count((t_size) TagFile->get_size_ex(abortHandler));

            TagFile->read_object(Tag.get_ptr(), Tag.get_count(), abortHandler);

            static_api_ptr_t<metadb_index_manager>()->set_user_data(GUIDTagMIDIHash, _Hash, Tag.get_ptr(), Tag.get_count());
        }
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
/// Gets the total duration of the specified sub-song taking into account any looping and decay time, in ms.
/// </summary>
uint32_t InputDecoder::GetDuration(size_t subSongIndex)
{
    uint32_t LengthInMs = _Container.GetDuration(subSongIndex, true);

    if (_LoopType == LoopType::NeverLoopAddDecayTime)
        LengthInMs += (uint32_t) CfgDecayTime;
    else
    if (_LoopType > LoopType::LoopAndFadeWhenDetected)
    {
        if (!_LoopRange.HasBegin())
            _LoopRange.SetBegin(0);

        if (!_LoopRange.HasEnd())
            _LoopRange.SetEnd(LengthInMs);

        LengthInMs = _LoopRange.Begin() + (_LoopRange.Size() * _LoopCount) + _FadeDuration;
    }

    return LengthInMs;
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

        FILE * fp = nullptr;

        ::_wfopen_s(&fp, FilePath.c_str(), L"r");

        if (fp != nullptr)
        {
            char Line[256];

            while (!::feof(fp) && (::fgets(Line, _countof(Line), fp) != NULL))
            {
                if (Line[0] != '@')
                    kp.AddUnsyncedLyrics(0, Line);
            }

            ::fclose(fp);
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
        uint32_t LoopBegin = _Container.GetLoopBeginTimestamp(subSongIndex);
        uint32_t LoopEnd = _Container.GetLoopEndTimestamp(subSongIndex);
        uint32_t LoopBeginInMS = _Container.GetLoopBeginTimestamp(subSongIndex, true);
        uint32_t LoopEndInMS = _Container.GetLoopEndTimestamp(subSongIndex, true);

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

static input_factory_t<InputDecoder> InputDecoderFactory;
