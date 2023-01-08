 
/** $VER: InputDecoder.cpp (2023.01.04) **/

#pragma warning(disable: 5045 26481 26485)

#include "InputDecoder.h"

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

volatile int _IsRunning = 0;

critical_section _Lock;
volatile unsigned int _CurrentSampleRate;

const GUID GUIDTagMIDIHash = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };

/// <summary>
/// Opens the specified file and parses it.
/// </summary>
void InputDecoder::open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler)
{
    if (file.is_empty())
        filesystem::g_open(file, filePath, filesystem::open_mode_read, abortHandler);

    _FilePath = filePath;

    {
        _FileStats = file->get_stats(abortHandler);

        if ((_FileStats.m_size == 0) || (_FileStats.m_size > (t_size)(1 << 30)))
            throw exception_io_unsupported_format();

        _FileStats2 = file->get_stats2_((uint32_t)stats2_all, abortHandler);

        if ((_FileStats2.m_size == 0) || (_FileStats2.m_size > (t_size)(1 << 30)))
            throw exception_io_unsupported_format();
    }

    std::vector<uint8_t> Data;

    Data.resize(_FileStats.m_size);

    file->read_object(&Data[0], _FileStats.m_size, abortHandler);

    {
        _IsSysExFile = IsSysExFileExtension(pfc::string_extension(filePath));

        if (_IsSysExFile)
        {
            if (!midi_processor::process_syx_file(Data, _Container))
                throw exception_io_data("Invalid SysEx dump");

            return;
        }
    }

    if (!midi_processor::process_file(Data, pfc::string_extension(filePath), _Container))
        throw exception_io_data("Invalid MIDI file");

    {
        _TrackCount = (size_t)_Container.get_track_count();

        if (_TrackCount == 0)
            throw exception_io_data("Invalid MIDI file");

        // Check if we read a valid MIDI file.
        {
            bool HasDuration = false;

            for (size_t i = 0; i < _TrackCount; ++i)
            {
                if (_Container.get_timestamp_end((unsigned long)i))
                {
                    HasDuration = true;
                    break;
                }
            }

            if (!HasDuration)
                throw exception_io_data("Invalid MIDI file");
        }

        _Container.scan_for_loops(_UseXMILoops, _UseFF7Loops, _UseRPGMLoops, _UseThLoops);
    }

    // Calculate the hash of the MIDI stream.
    {
        Data.resize(0);

        _Container.serialize_as_standard_midi_file(Data);

        hasher_md5_state HasherState;
        static_api_ptr_t<hasher_md5> Hasher;

        Hasher->initialize(HasherState);
        Hasher->process(HasherState, &Data[0], Data.size());

        _FileHash = Hasher->get_result(HasherState);
    }

    if (AdvCfgSkipToFirstNote)
        _Container.trim_start();

    _LoopRange.Clear();
    _LoopInMs.Clear();
}

/// <summary>
/// Retrieves information about specified subsong.
/// </summary>
void InputDecoder::get_info(t_uint32 subsongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        return;

    // General tags
    fileInfo.info_set_int(TagChannels, 2);
    fileInfo.info_set(TagEncoding, "Synthesized");

    // Specific tags
    midi_meta_data MetaData;

    _Container.get_meta_data(subsongIndex, MetaData);

    midi_meta_data_item Item;

    {
        {
            bool HasTitle = MetaData.get_item("title", Item);

            for (t_size i = 0; i < MetaData.get_count(); ++i)
            {
                const midi_meta_data_item& mdi = MetaData[i];

            #ifdef _DEBUG
                pfc::string8 Text; Text << mdi.m_name.c_str() << ":" << mdi.m_value.c_str(); console::print(Text);
            #endif

                if (pfc::stricmp_ascii(mdi.m_name.c_str(), "type") == 0)
                    continue;

                std::string Name = mdi.m_name;

                if (!HasTitle && (pfc::stricmp_ascii(Name.c_str(), "display_name") == 0))
                    Name = "title";

                AddTag(fileInfo, Name.c_str(), mdi.m_value.c_str(), 0);
            }
        }
    }

    fileInfo.info_set_int(TagMIDIFormat,       _Container.get_format());
    fileInfo.info_set_int(TagMIDITrackCount,  (_Container.get_format() == 2) ? 1 : _Container.get_track_count());
    fileInfo.info_set_int(TagMIDIChannelCount, _Container.get_channel_count(subsongIndex));
    fileInfo.info_set_int(TagMIDITicks,        _Container.get_timestamp_end(subsongIndex));

    if (MetaData.get_item("type", Item))
        fileInfo.info_set(TagMIDIType, Item.m_value.c_str());

    {
        unsigned long LoopBegin = _Container.get_timestamp_loop_start(subsongIndex);
        unsigned long LoopEnd = _Container.get_timestamp_loop_end(subsongIndex);
        unsigned long LoopBeginInMS = _Container.get_timestamp_loop_start(subsongIndex, true);
        unsigned long LoopEndInMS = _Container.get_timestamp_loop_end(subsongIndex, true);

        if (LoopBegin != ~0) fileInfo.info_set_int(TagMIDILoopStart, LoopBegin);
        if (LoopEnd != ~0) fileInfo.info_set_int(TagMIDILoopEnd, LoopEnd);
        if (LoopBeginInMS != ~0) fileInfo.info_set_int(TagMIDILoopStartInMs, LoopBeginInMS);
        if (LoopEndInMS != ~0) fileInfo.info_set_int(TagMIDILoopEndInMs, LoopEndInMS);
    }

    {
        unsigned long LengthInMs = _Container.get_timestamp_end(subsongIndex, true);

        double LengthInSeconds = double(LengthInMs) * 0.001;

        if (_LoopTypeOther == 1)
            LengthInSeconds += 1.;

        if ((_LoopTypeOther > 2) || !_LoopRange.IsEmpty())
        {
            if (!_LoopInMs.HasBegin())
                _LoopInMs.SetBegin(0);

            if (!_LoopInMs.HasEnd())
                _LoopInMs.SetEnd(LengthInMs);

            LengthInSeconds = (double) (_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount) + _FadeDuration) * 0.001;
        }

        fileInfo.set_length(LengthInSeconds);
    }

    {
        pfc::string8 FileHashString;

        for (size_t i = 0; i < 16; ++i)
            FileHashString += pfc::format_uint((t_uint8)_FileHash.m_data[i], 2, 16);

        fileInfo.info_set(TagMIDIHash, FileHashString);
    }

    {
        service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<FileHasher>;

        _Hash = IndexClient->transform(fileInfo, playable_location_impl(_FilePath, subsongIndex));

        pfc::array_t<t_uint8> Tags;

        static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTagMIDIHash, _Hash, Tags);

        if (Tags.get_count())
        {
            file::ptr File;

            filesystem::g_open_tempmem(File, abortHandler);

            File->write_object(Tags.get_ptr(), Tags.get_count(), abortHandler);

            fileInfo.meta_remove_all();

            tag_processor::read_trailing(File, fileInfo, abortHandler);

            fileInfo.info_set("tagtype", "apev2 db");
        }
    }

    {
        const char * MIDIPreset = fileInfo.meta_get(TagMIDIPreset, 0);

        if (MIDIPreset)
        {
            fileInfo.info_set(TagMIDIPreset, MIDIPreset);
            fileInfo.meta_remove_field(TagMIDIPreset);
        }
    }

    {
        const char * MIDISysExDumps = fileInfo.meta_get(TagMIDISysExDumps, 0);

        if (MIDISysExDumps)
        {
            fileInfo.info_set(TagMIDISysExDumps, MIDISysExDumps);
            fileInfo.meta_remove_field(TagMIDISysExDumps);
        }
    }
}

/// <summary>
/// Initializes the decoder before playing a song.
/// </summary>
void InputDecoder::decode_initialize(unsigned subsongIndex, unsigned flags, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        throw exception_io_data("You cannot play SysEx dumps");

    _LoopType = (flags & input_flag_playback) ? _LoopTypePlayback : _LoopTypeOther;
    _SamplesPlayed = 0;

    InitializeTime(subsongIndex);

    MIDIPreset Preset;

    {
        file_info_impl FileInfo;

        midi_meta_data MetaData;

        _Container.get_meta_data(subsongIndex, MetaData);

        midi_meta_data_item item;

        FileInfo.info_set_int(TagMIDIFormat,       _Container.get_format());
        FileInfo.info_set_int(TagMIDITrackCount,   _Container.get_format() == 2 ? 1 : _Container.get_track_count());
        FileInfo.info_set_int(TagMIDIChannelCount, _Container.get_channel_count(subsongIndex));
        FileInfo.info_set_int(TagMIDITicks,        _Container.get_timestamp_end(subsongIndex));

        if (MetaData.get_item("type", item))
            FileInfo.info_set(TagMIDIType, item.m_value.c_str());

        {
            unsigned long LoopBegin = _Container.get_timestamp_loop_start(subsongIndex);
            unsigned long LoopEnd = _Container.get_timestamp_loop_end(subsongIndex);
            unsigned long LoopBeginInMS = _Container.get_timestamp_loop_start(subsongIndex, true);
            unsigned long LoopEndInMS = _Container.get_timestamp_loop_end(subsongIndex, true);

            if (LoopBegin != ~0) FileInfo.info_set_int(TagMIDILoopStart, LoopBegin);
            if (LoopEnd != ~0) FileInfo.info_set_int(TagMIDILoopEnd, LoopEnd);
            if (LoopBeginInMS != ~0) FileInfo.info_set_int(TagMIDILoopStartInMs, LoopBeginInMS);
            if (LoopEndInMS != ~0) FileInfo.info_set_int(TagMIDILoopEndInMs, LoopEndInMS);
        }

        {
            pfc::string8 FileHashString;

            for (size_t i = 0; i < 16; ++i)
                FileHashString += pfc::format_uint((t_uint8) _FileHash.m_data[i], 2, 16);

            FileInfo.info_set(TagMIDIHash, FileHashString);
        }

        {
            service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<FileHasher>;

            _Hash = IndexClient->transform(FileInfo, playable_location_impl(_FilePath, subsongIndex));

            pfc::array_t<t_uint8> Tags;

            static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTagMIDIHash, _Hash, Tags);

            if (Tags.get_count())
            {
                file::ptr File;

                filesystem::g_open_tempmem(File, abortHandler);

                File->write_object(Tags.get_ptr(), Tags.get_count(), abortHandler);

                FileInfo.meta_remove_all();

                tag_processor::read_trailing(File, FileInfo, abortHandler);

                FileInfo.info_set("tagtype", "apev2 db");
            }
        }
    
        {
            const char * MIDIPreset = FileInfo.meta_get(TagMIDIPreset, 0);

            if (MIDIPreset)
                Preset.Deserialize(MIDIPreset);
        }

        MIDISysExDumps SysExDumps;

        {
            const char * MIDISysExDumps = FileInfo.meta_get(TagMIDISysExDumps, 0);

            if (MIDISysExDumps)
                SysExDumps.Deserialize(MIDISysExDumps, _FilePath);
        }

        _Container.set_track_count((unsigned int)_TrackCount);

        SysExDumps.Merge(_Container, abortHandler);

        _PlayerType = Preset._PlayerType;
        _IsFirstChunk = true;
    }

    midi_meta_data MetaData;

    {
        _Container.get_meta_data(subsongIndex, MetaData);

        {
            midi_meta_data_item Item;

            if (MetaData.get_item("type", Item) && (pfc::stricmp_ascii(Item.m_value.c_str(), "MT-32") == 0))
                _PlayerType= PlayerTypeSuperMunt;
        }
    }

    pfc::string8 SoundFontFilePath;

    // Gets a SoundFont file that has been configured for the file, if any.
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
        {
            SoundFontFilePath = TempSoundFontFilePath;
            _PlayerType = PlayerTypeBASSMIDI;
        }
    }

    if (_PlayerType == PlayerTypeSuperMunt)
        _SampleRate = (unsigned int)MT32Player::GetSampleRate();

    // Initialize the fade-out range.
    if ((flags & input_flag_no_looping) || (_LoopType < 4))
    {
        if ((_LoopType > 2) || !_LoopRange.IsEmpty())
        {
            unsigned int Begin = (unsigned int)::MulDiv((int)(_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount)), (int)_SampleRate, 1000);
            unsigned int End = Begin + _SampleRate * _FadeDuration / 1000;

            _FadeRange.Set(Begin, End);
            _IsLooping = true;
        }
        else
        {
            _FadeRange.Set(_LengthInSamples, _LengthInSamples);
            _IsLooping = false;
        }
    }
    else
    {
        if ((_LoopType > 4) || !_LoopRange.IsEmpty())
        {
            _FadeRange.Clear();
            _IsLooping = true;
        }
        else
        {
            _FadeRange.Set(_LengthInSamples, _LengthInSamples);
            _IsLooping = false;
        }
    }

    // VSTi
    if (_PlayerType == PlayerTypeVSTi)
    {
        {
            delete _Player;

            if (Preset._VSTiFilePath.is_empty())
            {
                console::print("No VST instrument configured.");
                throw exception_io_data();
            }

            VSTiPlayer * Player = new VSTiPlayer;

            if (!Player->LoadVST(Preset._VSTiFilePath))
            {
                pfc::string8 Text = "Unable to load VSTi plugin: ";

                Text += Preset._VSTiFilePath;

                console::print(Text);

                return;
            }
            
            if (Preset._VSTConfig.size())
                Player->setChunk(&Preset._VSTConfig[0], (unsigned long)Preset._VSTConfig.size());

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);
            _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
            else
            {
                std::string last_error;

                if (_Player->GetLastError(last_error))
                    throw exception_io_data(last_error.c_str());
            }
        }
    }
#ifdef FLUIDSYNTHSUPPORT
    else
    if (_PlayerType == PlayerTypeFluidSynth || _PlayerType == PlayerTypeBASSMIDI)
    {
        /*HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, nullptr, LOAD_LIBRARY_AS_DATAFILE );
            if ( !fsmod )
            {
                throw exception_io_data("Failed to load FluidSynth.dll");
            }
            FreeLibrary( fsmod );*/

        delete midiPlayer;

        auto sfPlayer = new SFPlayer;

        midiPlayer = sfPlayer;

        sfPlayer->setSoundFont(thePreset.soundfont_path);

        if (file_soundfont.length())
            sfPlayer->setFileSoundFont(file_soundfont);

        sfPlayer->setSampleRate(srate);
        sfPlayer->setInterpolationMethod(_FluidSynthInterpolationMethod);
        sfPlayer->setDynamicLoading(cfg_soundfont_dynamic.get());
        sfPlayer->setEffects(thePreset.effects);
        sfPlayer->setVoiceCount(thePreset.voices);

        sfPlayer->SetFilter((MIDIPlayer::filter_mode) thePreset.MIDIStandard, !thePreset._UseMIDIEffects);

        unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

        if (_IsLooping)
            LoopMode |= MIDIPlayer::loop_mode_force;

        if (sfPlayer->Load(midi_file, p_subsong, LoopMode, _CleanFlags))
        {
            _IsEndOfContainer = false;
            _DontLoop = true;

            return;
        }
    }
#ifdef BASSMIDISUPPORT
    else
     if (_PlayerType == PlayerTypeBASSMIDI)
    #endif
#else
    else
    // BASS MIDI
    if (_PlayerType == PlayerTypeFluidSynth || _PlayerType == PlayerTypeBASSMIDI)
#endif
#ifdef BASSMIDISUPPORT
    {
        {
            delete _Player;

            {
                _BASSMIDIVoiceCount = 0;
                _BASSMIDIVoiceMax = 0;

                if (Preset._SoundFontFilePath.is_empty() && SoundFontFilePath.is_empty())
                {
                    console::print("No SoundFonts configured, and no file or directory SoundFont found");
                    throw exception_io_data();
                }
            }

            auto Player = new BMPlayer;

            Player->setSoundFont(Preset._SoundFontFilePath);

            if (SoundFontFilePath.length())
                Player->setFileSoundFont(SoundFontFilePath);

            Player->setInterpolation((int)_BASSMIDIResamplingMode);
            Player->setEffects(Preset._BASSMIDIEffects);
            Player->setVoices((int)Preset._BASSMIDIVoices);

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);
            _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
            else
            {
                std::string last_error;

                if (_Player->GetLastError(last_error))
                    throw exception_io_data(last_error.c_str());
            }
        }
    }
#endif
    else
    // DirectX
#ifdef DXISUPPORT
    if (_PlayerType == PlayerTypeDirectX)
    {
        pfc::array_t<t_uint8> serialized_midi_file;
        midi_file.serialize_as_standard_midi_file(serialized_midi_file);

        delete dxiProxy;
        dxiProxy = nullptr;

        dxiProxy = new DXiProxy;
        if (SUCCEEDED(dxiProxy->initialize()))
        {
            dxiProxy->setSampleRate(srate);
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
    }
    else
#endif
    // ADL
    if (_PlayerType == PlayerTypeADL)
    {
        {
            delete _Player;

            ADLPlayer * Player = new ADLPlayer;

            Player->setBank(Preset._ADLBankNumber);
            Player->setChipCount(Preset._ADLChipCount);
            Player->setFullPanning(Preset._ADLUsePanning);
            Player->set4OpCount(Preset._ADLChipCount * 4 /*cfg_adl_4op*/);
            Player->setCore(Preset._ADLEmulatorCore);
            Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // OPN
    if (_PlayerType == PlayerTypeOPN)
    {
        {
            delete _Player;

            OPNPlayer * Player = new OPNPlayer;

            Player->setBank(Preset._OPNBankNumber);
            Player->setChipCount(Preset._ADLChipCount);
            Player->setFullPanning(Preset._ADLUsePanning);
            Player->setCore(Preset._OPNEmulatorCore);

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);
            _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Munt (MT32)
    if (_PlayerType == PlayerTypeSuperMunt)
    {
        {
            delete _Player;

            bool IsMT32 = false;

            {
                midi_meta_data_item Item;

                IsMT32 = (MetaData.get_item("type", Item) && (Item.m_value == "MT-32"));
            }

            auto * Player = new MT32Player(!IsMT32, Preset._MuntGMSet);

            pfc::string8 BasePath = CfgMuntDirectoryPath;

            if (BasePath.is_empty())
            {
                console::print("No Munt base path configured, attempting to load ROMs from plugin install path.");

                BasePath = core_api::get_my_full_path();
                BasePath.truncate(BasePath.scan_filename());
            }

            Player->setBasePath(BasePath);
            Player->setAbortCallback(&abortHandler);

            if (!Player->isConfigValid())
            {
                console::print("The Munt driver needs to be configured with a valid MT-32 or CM32L ROM set.");
                throw exception_io_data();
            }

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Nuke
    if (_PlayerType == PlayerTypeNuke)
    {
        {
            delete _Player;

            auto * Player = new NukePlayer();

            Player->SetSynth(Preset._NukeSynth);
            Player->SetBank(Preset._NukeBank);
            Player->SetExtp(Preset._NukePanning);

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Secret Sauce
    if (_PlayerType == PlayerTypeSecretSauce)
    {
        {
            delete _Player;

            auto * Player = new SCPlayer();

            pfc::string8 PathName;

            AdvCfgSecretSauceDirectoryPath.get(PathName);

            if (PathName.is_empty())
            {
                console::print("Secret Sauce path not configured, yet somehow enabled, trying plugin directory...");

                PathName = core_api::get_my_full_path();
                PathName.truncate(PathName.scan_filename());
            }

            Player->set_sccore_path(PathName);

            _Player = Player;
        }

        {
            _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);
            _Player->setSampleRate(_SampleRate);

            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413))
    if (_PlayerType == PlayerTypeEmuDeMIDI)
    {
        {
            delete _Player;

            auto * Player = new EdMPlayer;

            _Player = Player;
        }

        {
            unsigned LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::LoopModeForced;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                {
                    insync(_Lock);

                    if (++_IsRunning == 1)
                        _CurrentSampleRate = _SampleRate;
                    else
                    if (_SampleRate != _CurrentSampleRate)
                        _SampleRate = _CurrentSampleRate;

                    _IsEmuDeMIDI = true;
                }

                _Player->setSampleRate(_SampleRate);

                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
        }
    }

    throw exception_io_data();
}

/// <summary>
/// 
/// </summary>
bool InputDecoder::decode_run(audio_chunk & audioChunk, abort_callback & abortHandler)
{
    abortHandler.check();

    if (_IsEndOfContainer)
        return false;

    bool Success = false;

    if (_PlayerType == PlayerTypeVSTi)
    {
        auto * Player = (VSTiPlayer *) _Player;

        const size_t SamplesTodo = 4096;
        const size_t ChannelCount = Player->getChannelCount();

        audioChunk.set_data_size(SamplesTodo * ChannelCount);

        audio_sample * Samples = audioChunk.get_data();

        size_t SamplesDone = Player->Play(Samples, (unsigned long)SamplesTodo);

        if (SamplesDone == 0)
            return false;

        audioChunk.set_srate(_SampleRate);
        audioChunk.set_channels((unsigned int)ChannelCount);
        audioChunk.set_sample_count(SamplesDone);

        Success = true;
    }
    else
    if (_PlayerType == PlayerTypeSuperMunt)
    {
        auto * mt32Player = (MT32Player *) _Player;

        const size_t SamplesToDo = 4096;
        const size_t ChannelCount = 2;

        audioChunk.set_data_size(SamplesToDo * ChannelCount);

        audio_sample * Samples = audioChunk.get_data();

        mt32Player->setAbortCallback(&abortHandler);

        size_t SamplesDone = mt32Player->Play(Samples, (unsigned long)SamplesToDo);

        if (SamplesDone == 0)
            return false;

        audioChunk.set_srate(_SampleRate);
        audioChunk.set_channels((unsigned int)ChannelCount);
        audioChunk.set_sample_count(SamplesDone);

        Success = true;
    }
    else
#ifdef DXISUPPORT
    if (_PlayerType == PlayerTypeDirectX)
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
        const size_t SamplesToDo = 4096;
        const size_t ChannelCount = 2;

        audioChunk.set_data_size(SamplesToDo * ChannelCount);

        audio_sample * Samples = audioChunk.get_data();

        size_t SamplesDone = _Player->Play(Samples, (unsigned long)SamplesToDo);

        if (SamplesDone == 0)
        {
            std::string LastError;

            if (_Player->GetLastError(LastError) != 0)
                throw exception_io_data(LastError.c_str());

            return false;
        }

        audioChunk.set_srate(_SampleRate);
        audioChunk.set_channels((unsigned int)ChannelCount);
        audioChunk.set_sample_count(SamplesDone);

        Success = true;
    }

    // Scale the samples if fading was requaested.
    if (Success && _FadeRange.IsSet())
    {
        unsigned int BeginOfChunk = _SamplesPlayed;
        unsigned int EndOfChunk   = _SamplesPlayed + (unsigned int)audioChunk.get_sample_count();

        _SamplesPlayed = EndOfChunk;

        if (EndOfChunk >= _FadeRange.Begin())
        {
            for (unsigned int i = std::max(_FadeRange.Begin(), BeginOfChunk), j = std::min(EndOfChunk, _FadeRange.End()); i < j; ++i)
            {
                audio_sample * Sample = audioChunk.get_data() + (i - BeginOfChunk) * 2;
                audio_sample Scale = (audio_sample) (_FadeRange.End() - i) / (audio_sample) _FadeRange.Size();

                Sample[0] *= Scale;
                Sample[1] *= Scale;
            }

            if (EndOfChunk > _FadeRange.End())
            {
                unsigned int SamplesRemaining = 0;

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
/// 
/// </summary>
void InputDecoder::decode_seek(double timeInSeconds, abort_callback&)
{
    unsigned int OffsetInSamples = (unsigned int)audio_math::time_to_samples(timeInSeconds, 1000);

    _SamplesPlayed = (unsigned int)((t_int64(OffsetInSamples) * t_int64(_SampleRate)) / 1000);
    _IsFirstChunk = true;
    _IsEndOfContainer = false;

    if (OffsetInSamples > _LoopInMs.End())
        OffsetInSamples = (OffsetInSamples - _LoopInMs.Begin()) % _LoopInMs.Size() + _LoopInMs.Begin();

    unsigned int PositionInSamples = (unsigned int)((t_int64(OffsetInSamples) * t_int64(_SampleRate)) / 1000);

    if ((_LengthInSamples != 0) && (PositionInSamples >= (_LengthInSamples - _SampleRate)))
    {
        _IsEndOfContainer = true;
        return;
    }

#ifdef DXISUPPORT
    if (_PlayerType == PlayerTypeDirectX)
    {
        dxiProxy->setPosition(seek_msec);

        samples_done = done;

        return;
    }
    else
#endif
    if (_Player)
    {
        _Player->Seek(PositionInSamples);
        return;
    }
}

/// <summary>
/// 
/// </summary>
bool InputDecoder::decode_get_dynamic_info(file_info & fileInfo, double & timestampDelta)
{
    bool Success = false;

    if (_IsFirstChunk)
    {
        fileInfo.info_set_int(TagSampleRate, _SampleRate);
        timestampDelta = 0.;

        _IsFirstChunk = false;

        Success = true;
    }

#ifdef BASSMIDISUPPORT
#ifdef FLUIDSYNTHSUPPORT
    if (_PlayerType == PlayerTypeBASSMIDI)
#else
    if (_PlayerType == PlayerTypeFluidSynth || _PlayerType == PlayerTypeBASSMIDI)
#endif
    {
        auto Player = (BMPlayer *)_Player;

        unsigned int VoiceCount = Player->getVoicesActive();

        if (VoiceCount != _BASSMIDIVoiceCount)
        {
            fileInfo.info_set_int(TagBASSMIDIVoiceCount, VoiceCount);

            _BASSMIDIVoiceCount = VoiceCount;

            Success = true;
        }

        if (VoiceCount > _BASSMIDIVoiceMax)
        {
            fileInfo.info_set_int(TagBASSMIDIVoicesMax, VoiceCount);

            _BASSMIDIVoiceMax = VoiceCount;

            Success = true;
        }

        if (Success)
            timestampDelta = _AudioChunkDuration;
    }
#endif

    return Success;
}

#pragma region("input_info_writer")
/// <summary>
/// Set the tags for the specified file.
/// </summary>
void InputDecoder::retag_set_info(t_uint32, const file_info& fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        throw exception_io_data("You cannot tag SysEx dumps");

    file_info_impl fi(fileInfo);

    {
        fi.meta_remove_field(TagMIDIPreset);

        const char * Preset = fi.info_get(TagMIDIPreset);

        if (Preset)
            fi.meta_set(TagMIDIPreset, Preset);
    }

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

            Tag.set_count(TagFile->get_size_ex(abortHandler));

            TagFile->read_object(Tag.get_ptr(), Tag.get_count(), abortHandler);

            static_api_ptr_t<metadb_index_manager>()->set_user_data(GUIDTagMIDIHash, _Hash, Tag.get_ptr(), Tag.get_count());
        }
    }
}
#pragma endregion

/// <summary>
/// Initialize the time parameters.
/// </summary>
double InputDecoder::InitializeTime(unsigned subsongIndex)
{
    _LengthInMS = _Container.get_timestamp_end(subsongIndex, true);

    double Length = _LengthInMS * .001;

    if (_LoopType == 1)
        Length += 1.;

    _LengthInTicks = _Container.get_timestamp_end(subsongIndex); // theSequence->m_tempoMap.Sample2Tick(len, 1000);
    _LengthInSamples = (unsigned int) (((__int64) _LengthInMS * (__int64) _SampleRate) / 1000);

    if (_LoopType == 1)
        _LengthInSamples += _SampleRate;

    _LoopRange.Set(_Container.get_timestamp_loop_start(subsongIndex), _Container.get_timestamp_loop_end(subsongIndex));
    _LoopInMs.Set(_Container.get_timestamp_loop_start(subsongIndex, true), _Container.get_timestamp_loop_end(subsongIndex, true));

    if ((_LoopType > 2) || !_LoopRange.IsEmpty())
    {
        if (!_LoopInMs.HasBegin())
            _LoopInMs.SetBegin(0);

        if (!_LoopInMs.HasEnd())
            _LoopInMs.SetEnd(_LengthInMS);

        Length = (double) (_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount) + _FadeDuration) * 0.001;
    }

    return Length;
}

/// <summary>
/// Gets the path name of the matching SoundFont file for the specified file, if any.
/// </summary>
bool InputDecoder::GetSoundFontFilePath(const pfc::string8 filePath, pfc::string8 & soundFontPath, abort_callback & abortHandler) noexcept
{
    static const char * Extensions[] =
    {
        "json",
        "sflist",
#ifdef SF2PACK
        "sf2pack",
        "sfogg",
#endif
        "sf2",
        "sf3"
    };

    soundFontPath = filePath;

    size_t length = soundFontPath.length();

    for (size_t i = 0; i < _countof(Extensions); ++i)
    {
        soundFontPath.truncate(length);
        soundFontPath += ".";
        soundFontPath += Extensions[i];

        if (filesystem::g_exists(soundFontPath, abortHandler))
            return true;
    }

    return false;
}

/// <summary>
/// Returns the number of bytes required to represent a Shift-JIS character.
/// </summary>
static size_t DecodeShiftJISChar(const uint8_t * data, size_t size)
{
    if (size == 0)
        return 0;

    if (size > 2)
        size = 2;

    if (data[0] < 0x80)
        return (size_t)((data[0] > 0) ? 1 : 0);

    if (size >= 1)
    {
        // TODO: definitely weak
        if (unsigned(data[0] - 0xA1) < 0x3F)
            return 1;
    }

    if (size >= 2)
    {
        // TODO: probably very weak
        if ((unsigned(data[0] - 0x81) < 0x1F || unsigned(data[0] - 0xE0) < 0x10) && unsigned(data[1] - 0x40) < 0xBD)
            return 2;
    }

    return 0;
}

/// <summary>
/// Is the data Shift-JIS string?
/// </summary>
bool IsValidShiftJIS(const char * data, size_t size)
{
    for (size_t i = 0; (i < size) && (data[i] != 0);)
    {
        size_t n = ::DecodeShiftJISChar((const uint8_t *)data + i, size - i);

        if (n == 0)
            return false;

        i += n;

        if (i > size)
            return false;
    }

    return true;
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

    pfc::string8 NewValue;

    if ((maxLength != 0) && value[maxLength - 1])
    {
        NewValue.set_string(value, maxLength);
        value = NewValue;
    }
    else
        maxLength = ::strlen(value);

    if (pfc::is_lower_ascii(value) || pfc::is_valid_utf8(value, maxLength))
        fileInfo.meta_add(name, value);
    else
    if (IsValidShiftJIS(value, maxLength))
        fileInfo.meta_add(name, pfc::stringcvt::string_utf8_from_codepage(932, value)); // Shift-JIS
    else
        fileInfo.meta_add(name, pfc::stringcvt::string_utf8_from_ansi(value));
}

void InputDecoder::InitializeIndexManager()
{
    try
    {       
        static_api_ptr_t<metadb_index_manager>()->add(new service_impl_t<FileHasher>, GUIDTagMIDIHash, system_time_periods::week * 4);
    }
    catch (...) { }
}

/*
#define XMIDI_CONTROLLER_FOR_LOOP 0x74 // For Loop
#define XMIDI_CONTROLLER_NEXT_BREAK 0x75 // Next/Break

#define EMIDI_CONTROLLER_TRACK_DESIGNATION 110 // Track Designation
#define EMIDI_CONTROLLER_TRACK_EXCLUSION 111 // Track Exclusion
#define EMIDI_CONTROLLER_LOOP_BEGIN XMIDI_CONTROLLER_FOR_LOOP
#define EMIDI_CONTROLLER_LOOP_END XMIDI_CONTROLLER_NEXT_BREAK
*/

static input_factory_t<InputDecoder> InputDecoderFactory;
