 
/** $VER: InputDecoder.cpp (2024.08.04) **/

#include "framework.h"

#include "InputDecoder.h"

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include <pfc/string-conv-lite.h>

#include "KaraokeProcessor.h"
#include "Exceptions.h"

#include <math.h>
#include <string.h>

#include <Encoding.h>

volatile int _IsRunning = 0;

critical_section _Lock;
volatile uint32_t _CurrentSampleRate;

const GUID GUIDTagMIDIHash = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };

const char * PlayerTypeNames[] =
{
    "Emu de MIDI",
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
};

#pragma region input_impl

/// <summary>
/// Opens the specified file and parses it.
/// </summary>
void InputDecoder::open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler)
{
    if (file.is_empty())
        filesystem::g_open(file, filePath, filesystem::open_mode_read, abortHandler);

    if (_strnicmp(filePath, "file://", 7) == 0)
        filePath += 7;

    _FilePath = filePath;

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

    {
        _IsSysExFile = IsSysExFileExtension(pfc::string_extension(_FilePath));

        if (_IsSysExFile)
        {
            try
            {
                midi_processor_t::Process(Object, pfc::wideFromUTF8(_FilePath), _Container);
            }
            catch (std::exception & e)
            {
                pfc::string8 Message = "Failed to read SysEx file: ";

                throw exception_io_data(Message + e.what());
            }

            return;
        }
    }

    {
        try
        {
            midi_processor_options_t Options
            (
                (uint16_t) CfgLoopExpansion,
                CfgWriteBarMarkers,
                CfgWriteSysExNames,
                CfgExtendLoops,
                CfgWolfteamLoopMode,
                CfgKeepDummyChannels,
                CfgIncludeControlData,

                (uint16_t) CfgDefaultTempo
            );

            midi_processor_t::Process(Object, pfc::wideFromUTF8(_FilePath), _Container, Options);
        }
        catch (std::exception & e)
        {
            pfc::string8 Message = "Failed to read MIDI file: ";

            throw exception_io_data(Message + e.what());
        }

        _TrackCount = _Container.GetTrackCount();

        if (_TrackCount == 0)
            throw exception_io_data("Invalid MIDI file: No tracks found");

        // Check if we read a valid MIDI file.
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

    // Calculate the hash of the MIDI file.
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
        throw exception_midi("You cannot play SysEx files.");

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

    {
        file_info_impl FileInfo;

        get_info(subSongIndex, FileInfo, abortHandler);

        // Load the preset from the song if it has one.
        {
            const char * MIDIPresetText = FileInfo.meta_get(TagMIDIPreset, 0);

            if (MIDIPresetText)
            {
                console::print(STR_COMPONENT_BASENAME " is using preset \"", MIDIPresetText, "\" from tags.");

                Preset.Deserialize(MIDIPresetText);
            }
        }

        // Load the SysEx from the song if it has one.
        {
            MIDISysExDumps SysExDumps;

            const char * MIDISysExDumps = FileInfo.meta_get(TagMIDISysExDumps, 0);

            if (MIDISysExDumps)
            {
                console::print(STR_COMPONENT_BASENAME " is using SysEx file \"", MIDISysExDumps , "\".");

                SysExDumps.Deserialize(MIDISysExDumps, _FilePath);
            }

            SysExDumps.Merge(_Container, abortHandler);
        }
    }

    for (const auto & sf : _SoundFonts)
    {
        if (sf.IsEmbedded())
            ::DeleteFileA(sf.FilePath().c_str());
    }

    _SoundFonts.clear();

    /** IMPORTANT: The following sequence of adding SoundFonts is optimal for FluidSynth. For BASSMIDI, we'll reverse it. **/

    // First, add the embedded sound font, if present.
    {
        const auto & Data = _Container.GetSoundFontData();

        if (Data.size() > 12)
        {
            bool IsDLS = (::memcmp(Data.data() + 8, "DLS ", 4) == 0);

            char TempPath[MAX_PATH] = {};

            if (::GetTempPathA(_countof(TempPath), TempPath) != 0)
            {
                char TempFilePath[MAX_PATH] = {};

                if (::GetTempFileNameA(TempPath, "SF-", 0, TempFilePath) != 0)
                {
                    std::string FilePath = TempFilePath;

                    // BASSMIDI requires SF2 and SF3 sound fonts with an .sf2 extension. FluidSynth also supports DLS.
                    FilePath.append(IsDLS ? ".dls" : ".sf2");

                    {
                        FILE * fp = nullptr;

                        if (::fopen_s(&fp, FilePath.c_str(), "wb") == 0)
                        {
                            ::fwrite(Data.data(), Data.size(), 1, fp);

                            ::fclose(fp);

                            _SoundFonts.push_back({ FilePath, IsDLS ? 1 : _Container.GetBankOffset(), true, IsDLS });
                        }
                    }

                    ::DeleteFileA(TempFilePath);
                }
            }
        }
    }
        
    // Then, add the sound font named like the MIDI file, if present.
    {
        pfc::string8 FilePath = _FilePath;
        pfc::string8 TempSoundFontFilePath;

        bool FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);

        if (!FoundSoundFile)
        {
            size_t FileExtensionIndex = FilePath.find_last('.');

            if (FileExtensionIndex > FilePath.scan_filename())
            {
                FilePath.truncate(FileExtensionIndex);

                FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);
            }

            if (!FoundSoundFile)
            {
                FilePath.truncate(FilePath.scan_filename());

                TempSoundFontFilePath = "";
                TempSoundFontFilePath.add_byte(FilePath[FilePath.length() - 1]);
                FilePath.truncate(FilePath.length() - 1);

                size_t FileNameIndex = FilePath.scan_filename();

                if (FileNameIndex != pfc::infinite_size)
                {
                    FilePath += TempSoundFontFilePath;
                    FilePath.add_string(&FilePath[FileNameIndex], FilePath.length() - FileNameIndex - 1);

                    FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);
                }
            }
        }

        if (FoundSoundFile)
            _SoundFonts.push_back({ TempSoundFontFilePath.c_str(), 0, false, false });
    }

    // Finally, add the default sound font.
    {
        if (!Preset._SoundFontFilePath.isEmpty())
            _SoundFonts.push_back({ Preset._SoundFontFilePath.c_str(), 0, false, false });
    }

    // Update the player type.
    {
        _PlayerType = Preset._PlayerType;

        {
            if ((_PlayerType != PlayerType::FluidSynth))
            {
                if (!_SoundFonts.empty())
                {
                    _PlayerType = PlayerType::BASSMIDI;

                    for (const auto & sf : _SoundFonts)
                    {
                        if (sf.IsDLS() && FluidSynth::Exists())
                        {
                            _PlayerType = PlayerType::FluidSynth;
                            break;
                        }
                    }
                }
            }
        }

        {
            midi_metadata_table_t MetaData;

            _Container.GetMetaData(subSongIndex, MetaData);

            for (const midi_metadata_item_t & Item : MetaData)
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

                pfc::string8 FilePath;

                AdvCfgVSTiXGPlugin.get(FilePath);

                Preset._VSTiFilePath = FilePath;
            }
        }

        if ((_PlayerType == PlayerType::BASSMIDI) || (_PlayerType == PlayerType::FluidSynth))
        {
            if (_PlayerType == PlayerType::BASSMIDI)
                std::reverse(_SoundFonts.begin(), _SoundFonts.end());

            for (const auto & sf : _SoundFonts)
                console::print(STR_COMPONENT_BASENAME, " uses SoundFont \"", sf.FilePath().c_str(), "\".");
        }
    }

    // Update sample rate before initializing the fade-out range.
    if (_PlayerType == PlayerType::SuperMunt)
        _SampleRate = (uint32_t) MT32Player::GetSampleRate();

    // Initialize the fade-out range. Case "Never loop", "Never, add 1s decay time", "Loop and fade when detected" or "Always loop and fade",
    _FadeRange.Clear();

    {
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
                    throw exception_midi("No VSTi specified in preset.");

                auto Player = new VSTiPlayer;

                if (!Player->LoadVST(Preset._VSTiFilePath))
                    throw exception_midi(pfc::string8("Unable to load VSTi from \"") + Preset._VSTiFilePath + "\".");
            
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
                _ActiveVoiceCount = 0;
                _PeakVoiceCount = 0;

                if (_SoundFonts.empty())
                    throw exception_midi("No compatible SoundFonts found.");
            }

            pfc::string8 FluidSynthDirectoryPath = CfgFluidSynthDirectoryPath;

            if (FluidSynthDirectoryPath.isEmpty())
            {
                console::warning(STR_COMPONENT_BASENAME " is attempting to load the FluidSynth libraries from the plugin install path because the FluidSynth directory was not configured.");

                FluidSynthDirectoryPath = core_api::get_my_full_path();
                FluidSynthDirectoryPath.truncate(FluidSynthDirectoryPath.scan_filename());
            }

            {
                auto Player = new FSPlayer;

                Player->SetAbortHandler(&abortHandler);

                if (!Player->Initialize(pfc::wideFromUTF8(FluidSynthDirectoryPath)))
                    throw exception_midi("FluidSynth path not configured.");

                Player->SetSoundFonts(_SoundFonts);

                Player->SetInterpolationMode(_FluidSynthInterpolationMethod);
                Player->EnableEffects(Preset._EffectsEnabled);
                Player->SetVoiceCount(Preset._VoiceCount);
                Player->EnableDynamicLoading(AdvCfgLoadSoundFontDynamically.get());

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

        // Munt (MT-32)
        case PlayerType::SuperMunt:
        {
            {
                auto Player = new MT32Player(!_IsMT32, Preset._MuntGMSet);

                pfc::string8 BasePath = CfgMT32ROMDirectoryPath;

                if (BasePath.is_empty())
                {
                    console::warning(STR_COMPONENT_BASENAME " is attempting to load the MT-32 ROMs from the plugin install path because the SuperMunt ROM path was not configured.");

                    BasePath = core_api::get_my_full_path();
                    BasePath.truncate(BasePath.scan_filename());
                }

                Player->setBasePath(BasePath);
                Player->SetAbortHandler(&abortHandler);

                if (!Player->isConfigValid())
                    throw exception_midi("The Munt driver needs to be configured with a valid MT-32 or CM32L ROM set.");

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
                        throw exception_midi("No compatible SoundFonts found.");
                }

                auto Player = new BMPlayer;

                Player->SetSoundFonts(_SoundFonts);

                Player->SetVolume(_BASSMIDIVolume);
                Player->SetInterpolationMode(_BASSMIDIInterpolationMode);
                Player->EnableEffects(Preset._EffectsEnabled);
                Player->SetVoiceCount(Preset._VoiceCount);

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

                Player->setBank(Preset._OPNBankNumber);
                Player->setChipCount(Preset._ADLChipCount);
                Player->setFullPanning(Preset._ADLUsePanning);
                Player->setCore(Preset._OPNEmulatorCore);

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

                pfc::string8 PathName;

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

    }

    throw exception_midi("No MIDI player specified.");
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
        const uint32_t ChannelCount = _Player->GetChannelCount();

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

    uint32_t TimeInSamples = (uint32_t) ::MulDiv((int) OffsetInMs, (int) _SampleRate, 1000);

    if ((_LengthInSamples != 0U) && (TimeInSamples >= (_LengthInSamples - _SampleRate)))
    {
        _IsEndOfContainer = true;
        return;
    }

#ifdef DXISUPPORT
    if (_PlayerType == PlayerType::DirectX)
    {
        dxiProxy->setPosition(seek_msec);

        samples_done = done;

        return;
    }
    else
#endif
    if (_Player)
    {
        _Player->Seek(TimeInSamples);
        return;
    }
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

            Tag.set_count((t_size)TagFile->get_size_ex(abortHandler));

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

/// <summary>
/// Gets the path name of the matching SoundFont file for the specified file, if any.
/// </summary>
bool InputDecoder::GetSoundFontFilePath(const pfc::string8 & filePath, pfc::string8 & soundFontPath, abort_callback & abortHandler) noexcept
{
    static const char * FileExtensions[] =
    {
        "json",
        "sflist",
        "sf2pack",
        "sfogg",
        "sf2",
        "sf3"
    };

    soundFontPath = filePath;

    size_t Length = soundFontPath.length();

    for (const char * & FileExtension : FileExtensions)
    {
        soundFontPath.truncate(Length);
        soundFontPath += ".";
        soundFontPath += FileExtension;

        if (filesystem::g_exists(soundFontPath, abortHandler))
            return true;
    }

    return false;
}

#pragma endregion

#pragma region Tags

/// <summary>
/// Changes the extension of the file name in the specified file path.
/// </summary>
pfc::string8 ChangeExtension(const pfc::string8 & filePath, const pfc::string8 & fileExtension)
{
    char FilePath[MAX_PATH];

    ::strcpy_s(FilePath, _countof(FilePath), filePath);

    char * FileExtension = ::strrchr(FilePath, '.');

    if (FileExtension != nullptr)
        ::strcpy_s(FileExtension + 1, _countof(FilePath) - (FileExtension - FilePath) - 1, fileExtension.c_str());

    return pfc::string8(FilePath);
}

/// <summary>
/// Converts the MIDI metadata to tags.
/// </summary>
void InputDecoder::ConvertMetaDataToTags(size_t subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    _IsMT32 = false;
    _IsXG = false;

    KaraokeProcessor kp;

    {
        midi_metadata_table_t MetaData;

        _Container.GetMetaData(subSongIndex, MetaData);

        midi_metadata_item_t TrackItem;

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
        pfc::string8 FilePath = ChangeExtension(_FilePath, "wrd");

        FILE * fp = nullptr;

        ::fopen_s(&fp, FilePath.c_str(), "r");

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

    if (!kp.GetUnsyncedLyrics().is_empty())
    {
        auto Lyrics = kp.GetUnsyncedLyrics();

        std::string UTF8 = TextToUTF8(Lyrics.c_str());

        fileInfo.meta_set("lyrics", UTF8.c_str());
    }

    if (!kp.GetSyncedLyrics().is_empty())
    {
        auto Lyrics = kp.GetSyncedLyrics();

        std::string UTF8 = TextToUTF8(Lyrics.c_str());

        fileInfo.meta_set("syncedlyrics", UTF8.c_str());
    }

    // General info
    fileInfo.info_set_int("channels", 2);
    fileInfo.info_set("encoding", "Synthesized");

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

        if (LoopBegin != ~0U) fileInfo.info_set_int(TagMIDILoopStart, LoopBegin);
        if (LoopEnd != ~0U) fileInfo.info_set_int(TagMIDILoopEnd, LoopEnd);
        if (LoopBeginInMS != ~0U) fileInfo.info_set_int(TagMIDILoopStartInMs, LoopBeginInMS);
        if (LoopEndInMS != ~0U) fileInfo.info_set_int(TagMIDILoopEndInMs, LoopEndInMS);
    }

    {
        pfc::string8 FileHashString;

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

    pfc::string8 Value;

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
