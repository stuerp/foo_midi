 
/** $VER: InputDecoder.cpp (2023.01.03) **/

#pragma warning(disable: 5045 26481 26485)

#include "InputDecoder.h"

volatile int _IsRunning = 0;

critical_section _Lock;
volatile unsigned int _CurrentSampleRate;

/// <summary>
/// 
/// </summary>
void InputDecoder::open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler)
{
    if (file.is_empty())
        filesystem::g_open(file, filePath, filesystem::open_mode_read, abortHandler);

    _FilePath = filePath;

    _IsSysExFile = IsSysExFileExtension(pfc::string_extension(filePath));

    {
        _FileStats = file->get_stats(abortHandler);

        if (!_FileStats.m_size || _FileStats.m_size > (t_size)(1 << 30))
            throw exception_io_unsupported_format();

        _FileStats2 = file->get_stats2_((uint32_t)stats2_all, abortHandler);

        if (!_FileStats2.m_size || _FileStats2.m_size > (t_size)(1 << 30))
            throw exception_io_unsupported_format();
    }

    std::vector<uint8_t> Data;

    Data.resize(_FileStats.m_size);

    file->read_object(&Data[0], _FileStats.m_size, abortHandler);

    if (_IsSysExFile)
    {
        if (!midi_processor::process_syx_file(Data, _Container))
            throw exception_io_data("Invalid SysEx dump");

        return;
    }
    else
    if (!midi_processor::process_file(Data, pfc::string_extension(filePath), _Container))
        throw exception_io_data("Invalid MIDI file");

    {
        _TrackCount = _Container.get_track_count();

        if (_TrackCount == 0)
            throw exception_io_data("Invalid MIDI file");

        bool HasDuration = false;

        for (unsigned int i = 0; i < _TrackCount; ++i)
        {
            if (_Container.get_timestamp_end(i))
            {
                HasDuration = true;
                break;
            }
        }

        if (!HasDuration)
            throw exception_io_data("Invalid MIDI file");

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

    if (cfg_SkipToFirstNote)
        _Container.trim_start();

    _LoopBegin = 
    _LoopBeginInMS =
    _LoopEnd =
    _LoopEndInMS = (unsigned int)~0;
}

/// <summary>
/// Retrieves information about specified subsong.
/// </summary>
void InputDecoder::get_info(t_uint32 subsongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        return;

    // General tags
    fileInfo.info_set_int("channels", 2);
    fileInfo.info_set("encoding", "Synthesized");

    // Specific tags
    {
        pfc::string8 FileHashString;

        for (size_t i = 0; i < 16; ++i)
            FileHashString += pfc::format_uint((t_uint8)_FileHash.m_data[i], 2, 16);

        fileInfo.info_set(MetaDataHash, FileHashString);
    }

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

                AddMetaData(fileInfo, Name.c_str(), mdi.m_value.c_str(), 0);
            }
        }
    }

    fileInfo.info_set_int(MetaDataFormat,   _Container.get_format());
    fileInfo.info_set_int(MetaDataTracks,  (_Container.get_format() == 2) ? 1 : _Container.get_track_count());
    fileInfo.info_set_int(MetaDataChannels, _Container.get_channel_count(subsongIndex));
    fileInfo.info_set_int(MetaDataTicks,    _Container.get_timestamp_end(subsongIndex));

    if (MetaData.get_item("type", Item))
        fileInfo.info_set(MetaDataType, Item.m_value.c_str());

    {
        unsigned long LoopBegin = _Container.get_timestamp_loop_start(subsongIndex);
        unsigned long LoopEnd = _Container.get_timestamp_loop_end(subsongIndex);
        unsigned long LoopBeginInMS = _Container.get_timestamp_loop_start(subsongIndex, true);
        unsigned long LoopEndInMS = _Container.get_timestamp_loop_end(subsongIndex, true);

        if (LoopBegin != ~0) fileInfo.info_set_int(MetaDataLoopStart, LoopBegin);
        if (LoopEnd != ~0) fileInfo.info_set_int(MetaDataLoopEnd, LoopEnd);
        if (LoopBeginInMS != ~0) fileInfo.info_set_int(MetaDataLoopStartInMS, LoopBeginInMS);
        if (LoopEndInMS != ~0) fileInfo.info_set_int(MetaDataLoopEndInMS, LoopEndInMS);
    }

    {
        unsigned long LengthInMs = _Container.get_timestamp_end(subsongIndex, true);

        double LengthInSeconds = double(LengthInMs) * 0.001;

        if (_LoopTypeOther == 1)
            LengthInSeconds += 1.;

        if (_LoopBegin != ~0 || _LoopEnd != ~0 || (_LoopTypeOther > 2))
        {
            if (_LoopBeginInMS == ~0)
                _LoopBeginInMS = 0;

            if (_LoopEndInMS == ~0)
                _LoopEndInMS = LengthInMs;

            LengthInSeconds = (double) (_LoopBeginInMS + (_LoopEndInMS - _LoopBeginInMS) * _LoopCount + _FadeTimeInMs) * 0.001;
        }

        fileInfo.set_length(LengthInSeconds);
    }

    {
        service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<TrackHasher>;

        _Hash = IndexClient->transform(fileInfo, playable_location_impl(_FilePath, subsongIndex));
    }

    {
        pfc::array_t<t_uint8> Tag;

        static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTrackHasher, _Hash, Tag);

        if (Tag.get_count())
        {
            file::ptr File;

            filesystem::g_open_tempmem(File, abortHandler);

            File->write_object(Tag.get_ptr(), Tag.get_count(), abortHandler);

            fileInfo.meta_remove_all();

            tag_processor::read_trailing(File, fileInfo, abortHandler);

            fileInfo.info_set("tagtype", "apev2 db");

            {
                const char * MIDIPreset = fileInfo.meta_get(MetaDataPreset, 0);

                if (MIDIPreset)
                {
                    fileInfo.info_set(MetaDataPreset, MIDIPreset);
                    fileInfo.meta_remove_field(MetaDataPreset);
                }
            }

            {
                const char * MIDISysExDumps = fileInfo.meta_get(MetaDataSysExDumps, 0);

                if (MIDISysExDumps)
                {
                    fileInfo.info_set(MetaDataSysExDumps, MIDISysExDumps);
                    fileInfo.meta_remove_field(MetaDataSysExDumps);
                }
            }
        }
    }
}

/// <summary>
/// 
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
/// 
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
/// Adds the specified name and value to the meta data.
/// </summary>
void InputDecoder::AddMetaData(file_info & fileInfo, const char * name, const char * value, t_size maxLength)
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

/// <summary>
/// 
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

        FileInfo.info_set_int(MetaDataFormat, _Container.get_format());
        FileInfo.info_set_int(MetaDataTracks, _Container.get_format() == 2 ? 1 : _Container.get_track_count());
        FileInfo.info_set_int(MetaDataChannels, _Container.get_channel_count(subsongIndex));
        FileInfo.info_set_int(MetaDataTicks, _Container.get_timestamp_end(subsongIndex));

        if (MetaData.get_item("type", item))
            FileInfo.info_set(MetaDataType, item.m_value.c_str());

        {
            unsigned long LoopBegin = _Container.get_timestamp_loop_start(subsongIndex);
            unsigned long LoopEnd = _Container.get_timestamp_loop_end(subsongIndex);
            unsigned long LoopBeginInMS = _Container.get_timestamp_loop_start(subsongIndex, true);
            unsigned long LoopEndInMS = _Container.get_timestamp_loop_end(subsongIndex, true);

            if (LoopBegin != ~0) FileInfo.info_set_int(MetaDataLoopStart, LoopBegin);
            if (LoopEnd != ~0) FileInfo.info_set_int(MetaDataLoopEnd, LoopEnd);
            if (LoopBeginInMS != ~0) FileInfo.info_set_int(MetaDataLoopStartInMS, LoopBeginInMS);
            if (LoopEndInMS != ~0) FileInfo.info_set_int(MetaDataLoopEndInMS, LoopEndInMS);
        }

        {
            pfc::string8 FileHasString;

            for (size_t i = 0; i < 16; ++i)
                FileHasString += pfc::format_uint((t_uint8) _FileHash.m_data[i], 2, 16);

            FileInfo.info_set(MetaDataHash, FileHasString);
        }

        {
            service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<TrackHasher>;

            _Hash = IndexClient->transform(FileInfo, playable_location_impl(_FilePath, subsongIndex));
        }

        {
            pfc::array_t<t_uint8> Tag;

            static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTrackHasher, _Hash, Tag);

            if (Tag.get_count())
            {
                file::ptr File;

                filesystem::g_open_tempmem(File, abortHandler);

                File->write_object(Tag.get_ptr(), Tag.get_count(), abortHandler);

                FileInfo.meta_remove_all();

                tag_processor::read_trailing(File, FileInfo, abortHandler);

                FileInfo.info_set("tagtype", "apev2 db");
            }
        }

        {
            const char * MIDIPreset = FileInfo.meta_get(MetaDataPreset, 0);

            if (MIDIPreset)
                Preset.unserialize(MIDIPreset);
        }

        MIDISysExDumps SysExDumps;

        {
            const char * MIDISysExDumps = FileInfo.meta_get(MetaDataSysExDumps, 0);

            if (MIDISysExDumps)
                SysExDumps.unserialize(MIDISysExDumps, _FilePath);
        }

        _Container.set_track_count(_TrackCount);

        SysExDumps.Merge(_Container, abortHandler);
    }

    _PlugInId = Preset._PluginId;
    _IsFirstBlock = true;

    midi_meta_data MetaData;

    {
        _Container.get_meta_data(subsongIndex, MetaData);

        {
            midi_meta_data_item Item;

            if (MetaData.get_item("type", Item) && (strcmp(Item.m_value.c_str(), "MT-32") == 0))
                _PlugInId= SuperMUNTPlugInId;
        }
    }

    pfc::string8 SoundFontFilePath;

    // Gets a SoundFont file that has been configured for the file, if any.
    {
        pfc::string8_fast FilePath = _FilePath;
        pfc::string8_fast TempSoundFontFilePath;

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
            _PlugInId = BASSMIDIPlugInId;
        }
    }

    if (_PlugInId == SuperMUNTPlugInId)
        _SampleRate = (unsigned int)MT32Player::GetSampleRate();

    if ((flags & input_flag_no_looping) || (_LoopType < 4))
    {
        unsigned samples_length = _LengthInSamples;

        _SamplesFadeBegin = samples_length;
        _SamplesFadeEnd = samples_length;
        doing_loop = false;

        if (_LoopBegin != ~0 || _LoopEnd != ~0 || (_LoopType > 2))
        {
            _SamplesFadeBegin = (unsigned int)::MulDiv((int)(_LoopBeginInMS + (_LoopEndInMS - _LoopBeginInMS) * _LoopCount), (int)_SampleRate, 1000);
            _SamplesFadeEnd = _SamplesFadeBegin + _SampleRate * _FadeTimeInMs / 1000;
            doing_loop = true;
        }
    }
    else
    {
        if (_LoopType > 4 || _LoopBegin != ~0 || _LoopEnd != ~0)
        {
            _SamplesFadeBegin = (unsigned int)~0;
            _SamplesFadeEnd = (unsigned)~0;
            doing_loop = true;
        }
        else
        {
            unsigned samples_length = _LengthInSamples;

            _SamplesFadeBegin = samples_length;
            _SamplesFadeEnd = samples_length;
            doing_loop = false;
        }
    }

    // VSTi
    if (_PlugInId == VSTiPlugInId)
    {
        {
            delete _Player;

            if (Preset._VSTPathName.is_empty())
            {
                console::print("No VST instrument configured.");
                throw exception_io_data();
            }

            VSTiPlayer * Player = new VSTiPlayer;

            if (!Player->LoadVST(Preset._VSTPathName))
            {
                pfc::string8 Text = "Unable to load VSTi plugin: ";

                Text += Preset._VSTPathName;

                console::print(Text);

                return;
            }
            
            if (Preset._VSTConfig.size())
                Player->setChunk(&Preset._VSTConfig[0], (unsigned long)Preset._VSTConfig.size());

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);
            _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
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
    if (_PlugInId == FluidSynthPlugInId || _PlugInId == BASSMIDIPlugInId)
    {
        /*HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, nullptr, LOAD_LIBRARY_AS_DATAFILE );
            if ( !fsmod )
            {
                throw exception_io_data("Failed to load FluidSynth.dll");
            }
            FreeLibrary( fsmod );*/

        delete midiPlayer;

        SFPlayer * sfPlayer = new SFPlayer;
        midiPlayer = sfPlayer;

        sfPlayer->setSoundFont(thePreset.soundfont_path);

        if (file_soundfont.length())
            sfPlayer->setFileSoundFont(file_soundfont);

        sfPlayer->setSampleRate(srate);
        sfPlayer->setInterpolationMethod(fluid_interp_method);
        sfPlayer->setDynamicLoading(cfg_soundfont_dynamic.get());
        sfPlayer->setEffects(thePreset.effects);
        sfPlayer->setVoiceCount(thePreset.voices);

        sfPlayer->setFilterMode((MIDIPlayer::filter_mode) thePreset.MIDIStandard, !thePreset._UseMIDIEffects);

        unsigned LoopMode = 0;

        LoopMode = MIDIPlayer::loop_mode_enable;
        if (doing_loop) LoopMode |= MIDIPlayer::loop_mode_force;

        if (sfPlayer->Load(midi_file, p_subsong, LoopMode, _CleanFlags))
        {
            eof = false;
            _DontLoop = true;

            return;
        }
    }
#ifdef BASSMIDISUPPORT
    else
     if (_PlugInId == BASSMIDIPlugInId)
    #endif
#else
    else
    // BASS MIDI
    if (_PlugInId == FluidSynthPlugInId || _PlugInId == BASSMIDIPlugInId)
#endif
#ifdef BASSMIDISUPPORT
    {
        {
            delete _Player;

            {
                bassmidi_voices = 0;
                bassmidi_voices_max = 0;

                if (Preset._SoundFontPathName.is_empty() && SoundFontFilePath.is_empty())
                {
                    console::print("No SoundFonts configured, and no file or directory SoundFont found");
                    throw exception_io_data();
                }
            }

            BMPlayer * Player = new BMPlayer;

            Player->setSoundFont(Preset._SoundFontPathName);

            if (SoundFontFilePath.length())
                Player->setFileSoundFont(SoundFontFilePath);

            Player->setInterpolation((int)_ResamplingMode);
            Player->setEffects(Preset._BASSMIDIEffects);
            Player->setVoices((int)Preset._BASSMIDIVoices);

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);
            _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
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
#ifdef DXISUPPORT
    if (_PlugInId == DirectXPlugInId)
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
    if (_PlugInId == ADLPlugInId)
    {
        {
            delete _Player;

            ADLPlayer * Player = new ADLPlayer;

            Player->setBank(Preset._ADLBankNumber);
            Player->setChipCount(Preset._ADLChipCount);
            Player->setFullPanning(Preset._ADLUsePanning);
            Player->set4OpCount(Preset._ADLChipCount * 4 /*cfg_adl_4op*/);
            Player->setCore(Preset._ADLEmulatorCore);
            Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            _Player = Player;
        }

        {
            _Player->setSampleRate(_SampleRate);

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    if (_PlugInId == OPNPlugInId)
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
            _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // MUNT (MT32)
    if (_PlugInId == SuperMUNTPlugInId)
    {
        {
            delete _Player;

            bool IsMT32 = false;

            {
                midi_meta_data_item Item;

                IsMT32 = (MetaData.get_item("type", Item) && (::strcmp(Item.m_value.c_str(), "MT-32") == 0));

                if (CfgMUNTPath.is_empty())
                    console::print("No MUNT base path configured, attempting to load ROMs from plugin install path");
            }

            auto * Player = new MT32Player(!IsMT32, Preset._MuntGMSet);

            pfc::string8 BasePath = CfgMUNTPath;

            if (BasePath.is_empty())
            {
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

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Nuke
    if (_PlugInId == NukePlugInId)
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

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Secret Sauce
    if (_PlugInId == SecretSaucePlugInId)
    {
        {
            delete _Player;

            auto * Player = new SCPlayer();

            pfc::string8 PathName;

            CfgSecretSaucePath.get(PathName);

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
            _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);
            _Player->setSampleRate(_SampleRate);

            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (_Player->Load(_Container, subsongIndex, LoopMode, _CleanFlags))
            {
                eof = false;
                _DontLoop = true;

                return;
            }
        }
    }
    else
    // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413 ))
    if (_PlugInId == EmuDeMIDIPlugInId)
    {
        {
            delete _Player;

            auto * Player = new EdMPlayer;

            _Player = Player;
        }

        {
            unsigned LoopMode = MIDIPlayer::loop_mode_enable;

            if (doing_loop)
                LoopMode |= MIDIPlayer::loop_mode_force;

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

                eof = false;
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

    if (eof)
        return false;

    bool Success = false;

#ifdef DXISUPPORT
    if (_PlugInId == 5)
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
    if (_PlugInId == VSTiPlugInId)
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
    if (_PlugInId == SuperMUNTPlugInId)
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
    if (Success && (_SamplesFadeBegin != ~0) && (_SamplesFadeEnd != ~0))
    {
        unsigned int SamplesPlayedBegin = _SamplesPlayed;
        unsigned int SamplesPlayedEnd   = _SamplesPlayed + (unsigned int)audioChunk.get_sample_count();

        _SamplesPlayed = SamplesPlayedEnd;

        if (SamplesPlayedEnd >= _SamplesFadeBegin)
        {
            for (unsigned i = std::max(_SamplesFadeBegin, SamplesPlayedBegin), j = std::min(SamplesPlayedEnd, _SamplesFadeEnd); i < j; ++i)
            {
                audio_sample * Sample = audioChunk.get_data() + (i - SamplesPlayedBegin) * 2;
                audio_sample Scale = (audio_sample) (_SamplesFadeEnd - i) / (audio_sample) (_SamplesFadeEnd - _SamplesFadeBegin);

                Sample[0] *= Scale;
                Sample[1] *= Scale;
            }

            if (SamplesPlayedEnd > _SamplesFadeEnd)
            {
                unsigned SamplesRemaining = 0;

                if (_SamplesFadeEnd > SamplesPlayedBegin)
                    SamplesRemaining = _SamplesFadeEnd - SamplesPlayedBegin;

                audioChunk.set_sample_count(SamplesRemaining);

                eof = true;

                if (SamplesRemaining == 0)
                    return false;
            }
        }
    }

    _AudioChunkDuration = audioChunk.get_duration();

    return Success;
}

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
    _LengthInSamples = (unsigned) (((__int64) _LengthInMS * (__int64) _SampleRate) / 1000);

    if (_LoopType == 1)
        _LengthInSamples += _SampleRate;

    _LoopBegin = _Container.get_timestamp_loop_start(subsongIndex);
    _LoopEnd = _Container.get_timestamp_loop_end(subsongIndex);
    _LoopBeginInMS = _Container.get_timestamp_loop_start(subsongIndex, true);
    _LoopEndInMS = _Container.get_timestamp_loop_end(subsongIndex, true);

    if (_LoopBegin != ~0 || _LoopEnd != ~0 || (_LoopType > 2))
    {
        if (_LoopBeginInMS == ~0)
            _LoopBeginInMS = 0;

        if (_LoopEndInMS == ~0)
            _LoopEndInMS = _LengthInMS;

        Length = (double) (_LoopBeginInMS + (_LoopEndInMS - _LoopBeginInMS) * _LoopCount + _FadeTimeInMs) * 0.001;
    }

    return Length;
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
