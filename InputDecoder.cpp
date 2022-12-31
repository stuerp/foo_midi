
/** $VER: InputDecoder.cpp (2022.12.30) **/

#pragma warning(disable: 4310 5045 6326)

#define NOMINMAX

#include <sdk/foobar2000-lite.h>
#include<sdk/input_impl.h>

#include <midi_processing/midi_processor.h>

#include "Configuration.h"
#include "TrackHasher.h"
#include "MIDIPreset.h"
#include "MIDISysExDumps.h"

/** Players **/

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "EMIDIPlayer.h"
#include "MSPlayer.h"
#include <MT32Player/MT32Player.h>
#include <OPNPlayer/OPNPlayer.h>
#include "SCPlayer.h"
#include "VSTiPlayer.h"

#ifdef FLUIDSYNTHSUPPORT
#include "SFPlayer.h"
#endif

#ifdef DXISUPPORT
#include "DXiProxy.h"
#include "PlugInInventory.h"
#pragma comment(lib, "strmiids.lib")
#endif

#include "resource.h"

volatile int g_running = 0;

static critical_section sync;
static volatile int g_srate;

bool IsValidShiftJIS(const char * text, t_size size= ~0);

/// <summary>
/// Implements an input decoder.
/// </summary>
#pragma warning(disable: 4820) // x bytes padding added after data member
class input_midi : public input_stubs
{
public:
    input_midi() : _SampleRate((unsigned int)CfgSampleRate), _ResamplingMode((unsigned int)CfgResamplingMode),
    #ifdef FLUIDSYNTHSUPPORT
        fluid_interp_method(Cfg_FluidSynthInterpolationMethod),
    #endif
        loop_type_playback((unsigned int)cfg_loop_type),
        loop_type_other((unsigned int)cfg_loop_type_other),

        b_thloopz(!!cfg_thloopz),
        b_rpgmloopz(!!cfg_rpgmloopz),
        b_xmiloopz(!!cfg_xmiloopz),
        b_ff7loopz(!!cfg_ff7loopz) //, b_gm2(!!cfg_gm2)
    {
    #ifdef DXISUPPORT
        dxiProxy = NULL;
    #endif

        is_emidi = false;

        _Player = NULL;

        length_samples = 0;
        length_ticks = 0;
/*
        external_decoder = 0;
        mem_reader = 0;
*/
        clean_flags = (cfg_emidi_exclusion ? midi_container::clean_flag_emidi : 0) | (cfg_filter_instruments ? midi_container::clean_flag_instruments : 0) | (cfg_filter_banks ? midi_container::clean_flag_banks : 0);

        loop_count = (unsigned int)cfg_midi_loop_count.get();
        fade_ms = (unsigned int)cfg_midi_fade_time.get();

    #ifdef BASSMIDISUPPORT
        if (!_HASSSE2 && _ResamplingMode > 1)
            _ResamplingMode = 1;
    #endif
    }

    input_midi(const input_midi&) = delete;
    input_midi(const input_midi&&) = delete;
    input_midi& operator=(const input_midi&) = delete;
    input_midi& operator=(input_midi&&) = delete;

    ~input_midi()
    {
/*      if (external_decoder) external_decoder->service_release();
        if (mem_reader) mem_reader->reader_release();*/
        delete _Player;

        if (is_emidi)
        {
            insync(sync);
            g_running--;
        }
    #ifdef DXISUPPORT
        if (dxiProxy)
            delete dxiProxy;
    #endif
    }

public:
    void open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & p_abort)
    {
        if (file.is_empty())
            filesystem::g_open(file, filePath, filesystem::open_mode_read, p_abort);

        _FilePath = filePath;

        _IsSyxFile = g_test_extension_syx(pfc::string_extension(filePath));

        {
            _FileStats = file->get_stats(p_abort);

            if (!_FileStats.m_size || _FileStats.m_size > (t_size)(1 << 30))
                throw exception_io_unsupported_format();

            _FileStats2 = file->get_stats2_((uint32_t)stats2_all, p_abort);

            if (!_FileStats2.m_size || _FileStats2.m_size > (t_size)(1 << 30))
                throw exception_io_unsupported_format();
        }

        std::vector<uint8_t> Data;

        Data.resize(_FileStats.m_size);

        file->read_object(&Data[0], _FileStats.m_size, p_abort);

        if (_IsSyxFile)
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

            for (unsigned i = 0; i < _TrackCount; ++i)
            {
                if (_Container.get_timestamp_end(i))
                {
                    HasDuration = true;
                    break;
                }
            }

            if (!HasDuration)
                throw exception_io_data("Invalid MIDI file");

            _Container.scan_for_loops(b_xmiloopz, b_ff7loopz, b_rpgmloopz, b_thloopz);
        }

        {
            Data.resize(0);

            _Container.serialize_as_standard_midi_file(Data);

            hasher_md5_state hasher_state;
            static_api_ptr_t<hasher_md5> hasher;

            hasher->initialize(hasher_state);
            hasher->process(hasher_state, &Data[0], Data.size());

            _FileHash = hasher->get_result(hasher_state);
        }

        if (cfg_SkipToFirstNote)
            _Container.trim_start();
    }

    unsigned get_subsong_count()
    {
        return _IsSyxFile ? 1 : _Container.get_subsong_count();
    }

    t_uint32 get_subsong(unsigned subSongIndex)
    {
        return _IsSyxFile ? 0 : _Container.get_subsong(subSongIndex);
    }

    void get_info(t_uint32 subsongIndex, file_info & fileInfo, abort_callback & abortHandler)
    {
        if (_IsSyxFile)
            return;

        midi_meta_data MetaData;

        _Container.get_meta_data(subsongIndex, MetaData);

        midi_meta_data_item Item;

        {
            bool remap_display_name = !MetaData.get_item("title", Item);

            for (t_size i = 0; i < MetaData.get_count(); ++i)
            {
                const midi_meta_data_item& mdi = MetaData[i];

                if (pfc::stricmp_ascii(mdi.m_name.c_str(), "type"))
                {
                    std::string Name = mdi.m_name;

                    if (remap_display_name && !pfc::stricmp_ascii(Name.c_str(), "display_name"))
                        Name = "title";

                    meta_add(fileInfo, Name.c_str(), mdi.m_value.c_str(), 0);
                }
            }
        }

        fileInfo.info_set_int(field_format, _Container.get_format());
        fileInfo.info_set_int(field_tracks, _Container.get_format() == 2 ? 1 : _Container.get_track_count());
        fileInfo.info_set_int(field_channels, _Container.get_channel_count(subsongIndex));
        fileInfo.info_set_int(field_ticks, _Container.get_timestamp_end(subsongIndex));

        if (MetaData.get_item("type", Item))
            fileInfo.info_set(field_type, Item.m_value.c_str());

        unsigned loop_begin = _Container.get_timestamp_loop_start(subsongIndex);
        unsigned loop_end = _Container.get_timestamp_loop_end(subsongIndex);
        unsigned loop_begin_ms = _Container.get_timestamp_loop_start(subsongIndex, true);
        unsigned loop_end_ms = _Container.get_timestamp_loop_end(subsongIndex, true);

        if (loop_begin != ~0) fileInfo.info_set_int(field_loop_start, loop_begin);
        if (loop_end != ~0) fileInfo.info_set_int(field_loop_end, loop_end);
        if (loop_begin_ms != ~0) fileInfo.info_set_int(field_loop_start_ms, loop_begin_ms);
        if (loop_end_ms != ~0) fileInfo.info_set_int(field_loop_end_ms, loop_end_ms);

        // p_info.info_set_int("samplerate", srate);

        {
            unsigned long LengthInMs = _Container.get_timestamp_end(subsongIndex, true);

            double Length = double(LengthInMs) * 0.001;

            if (loop_type_other == 1)
                Length += 1.;

            if (loop_begin != ~0 || loop_end != ~0 || loop_type_other > 2)
            {
                if (loop_begin_ms == ~0)
                    loop_begin_ms = 0;

                if (loop_end_ms == ~0)
                    loop_end_ms = LengthInMs;

                Length = (double) (loop_begin_ms + (loop_end_ms - loop_begin_ms) * loop_count + fade_ms) * 0.001;
            }

            fileInfo.set_length(Length);
        }

        fileInfo.info_set_int("channels", 2);
        fileInfo.info_set("encoding", "Synthesized");

        pfc::string8 hash_string;

        for (unsigned i = 0; i < 16; ++i)
            hash_string += pfc::format_uint((t_uint8) _FileHash.m_data[i], 2, 16);

        fileInfo.info_set(field_hash, hash_string);

        service_ptr_t<metadb_index_client> index_client = new service_impl_t<TrackHasher>;

        m_index_hash = index_client->transform(fileInfo, playable_location_impl(_FilePath, subsongIndex));

        pfc::array_t<t_uint8> tag;

        static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTrackHasher, m_index_hash, tag);

        if (tag.get_count())
        {
            file::ptr tag_file;

            filesystem::g_open_tempmem(tag_file, abortHandler);

            tag_file->write_object(tag.get_ptr(), tag.get_count(), abortHandler);

            fileInfo.meta_remove_all();

            tag_processor::read_trailing(tag_file, fileInfo, abortHandler);

            fileInfo.info_set("tagtype", "apev2 db");

            const char * midi_preset = fileInfo.meta_get(field_preset, 0);

            if (midi_preset)
            {
                fileInfo.info_set(field_preset, midi_preset);
                fileInfo.meta_remove_field(field_preset);
            }

            const char * midi_syx = fileInfo.meta_get(field_syx, 0);

            if (midi_syx)
            {
                fileInfo.info_set(field_syx, midi_syx);
                fileInfo.meta_remove_field(field_syx);
            }
        }
    }

    t_filestats2 get_stats2(uint32_t, abort_callback&)
    {
        return _FileStats2;
    }

    t_filestats get_file_stats(abort_callback&)
    {
        return _FileStats;
    }

    static bool test_soundfont_extension(const char * filePath, pfc::string_base & soundFontPath, abort_callback & p_abort)
    {
        static const char * extensions[] =
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

        for (int i = 0; i < _countof(extensions); ++i)
        {
            soundFontPath.truncate(length);
            soundFontPath += ".";
            soundFontPath += extensions[i];

            if (filesystem::g_exists(soundFontPath, p_abort))
                return true;
        }

        return false;
    }

    void decode_initialize(unsigned subsongIndex, unsigned flags, abort_callback & abortHandler)
    {
        if (_IsSyxFile)
            throw exception_io_data("You cannot play SysEx dumps");

        _LoopType = (flags & input_flag_playback) ? loop_type_playback : loop_type_other;

        MIDIPreset Preset;
        MIDISysExDumps theDumps;

        {
            file_info_impl FileInfo;

            midi_meta_data MetaData;

            _Container.get_meta_data(subsongIndex, MetaData);

            midi_meta_data_item item;

            FileInfo.info_set_int(field_format, _Container.get_format());
            FileInfo.info_set_int(field_tracks, _Container.get_format() == 2 ? 1 : _Container.get_track_count());
            FileInfo.info_set_int(field_channels, _Container.get_channel_count(subsongIndex));
            FileInfo.info_set_int(field_ticks, _Container.get_timestamp_end(subsongIndex));

            if (MetaData.get_item("type", item))
                FileInfo.info_set(field_type, item.m_value.c_str());

            unsigned loop_begin = _Container.get_timestamp_loop_start(subsongIndex);
            unsigned loop_end = _Container.get_timestamp_loop_end(subsongIndex);
            unsigned loop_begin_ms = _Container.get_timestamp_loop_start(subsongIndex, true);
            unsigned loop_end_ms = _Container.get_timestamp_loop_end(subsongIndex, true);

            if (loop_begin != ~0) FileInfo.info_set_int(field_loop_start, loop_begin);
            if (loop_end != ~0) FileInfo.info_set_int(field_loop_end, loop_end);
            if (loop_begin_ms != ~0) FileInfo.info_set_int(field_loop_start_ms, loop_begin_ms);
            if (loop_end_ms != ~0) FileInfo.info_set_int(field_loop_end_ms, loop_end_ms);

            pfc::string8 hash_string;

            for (unsigned i = 0; i < 16; ++i)
                hash_string += pfc::format_uint((t_uint8) _FileHash.m_data[i], 2, 16);

            FileInfo.info_set(field_hash, hash_string);

            service_ptr_t<metadb_index_client> index_client = new service_impl_t<TrackHasher>;
            m_index_hash = index_client->transform(FileInfo, playable_location_impl(_FilePath, subsongIndex));

            pfc::array_t<t_uint8> tag;
            static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTrackHasher, m_index_hash, tag);

            if (tag.get_count())
            {
                file::ptr tag_file;
                filesystem::g_open_tempmem(tag_file, abortHandler);
                tag_file->write_object(tag.get_ptr(), tag.get_count(), abortHandler);

                FileInfo.meta_remove_all();

                tag_processor::read_trailing(tag_file, FileInfo, abortHandler);
                FileInfo.info_set("tagtype", "apev2 db");
            }

            {
                const char * midi_preset = FileInfo.meta_get(field_preset, 0);

                if (midi_preset)
                    Preset.unserialize(midi_preset);
            }

            {
                const char * midi_syx = FileInfo.meta_get(field_syx, 0);

                if (midi_syx)
                    theDumps.unserialize(midi_syx, _FilePath);
            }
        }

        _Container.set_track_count(_TrackCount);

        theDumps.Merge(_Container, abortHandler);

        _PluginID = Preset._PluginId;

        first_block = true;

        midi_meta_data MetaData;

        {
            _Container.get_meta_data(subsongIndex, MetaData);

            {
                midi_meta_data_item Item;

                if (MetaData.get_item("type", Item) && (strcmp(Item.m_value.c_str(), "MT-32") == 0))
                    _PluginID= 3;
            }
        }

        pfc::string8 file_soundfont;

        /*if (_SelectedPluginIndex == 2 || _SelectedPluginIndex == 4 )*/
        {
            pfc::string8_fast temp = _FilePath, temp_out;

            bool exists = test_soundfont_extension(temp, temp_out, abortHandler);

            if (!exists)
            {
                size_t dot = temp.find_last('.');

                if (dot > temp.scan_filename())
                {
                    temp.truncate(dot);

                    exists = test_soundfont_extension(temp, temp_out, abortHandler);
                }

                if (!exists)
                {
                    // Bah. The things needed to keep the last path separator.
                    temp.truncate(temp.scan_filename());
                    temp_out = "";
                    temp_out.add_byte(temp[temp.length() - 1]);
                    temp.truncate(temp.length() - 1);

                    size_t pos = temp.scan_filename();

                    if (pos != pfc::infinite_size)
                    {
                        temp += temp_out;
                        temp.add_string(&temp[pos], temp.length() - pos - 1);

                        exists = test_soundfont_extension(temp, temp_out, abortHandler);
                    }
                }
            }

            if (exists)
            {
                file_soundfont = temp_out;
                _PluginID = 4;
            }
        }

        if (_PluginID == 3)
            _SampleRate = MT32Player::getSampleRate();

        get_length(subsongIndex);

        samples_played = 0;

        if ((flags & input_flag_no_looping) || _LoopType < 4)
        {
            unsigned samples_length = length_samples;

            samples_fade_begin = samples_length;
            samples_fade_end = samples_length;
            doing_loop = false;

            if (loop_begin != ~0 || loop_end != ~0 || _LoopType > 2)
            {
                samples_fade_begin = MulDiv(loop_begin_ms + (loop_end_ms - loop_begin_ms) * loop_count, _SampleRate, 1000);
                samples_fade_end = samples_fade_begin + _SampleRate * fade_ms / 1000;
                doing_loop = true;
            }
        }
        else
        {
            if (_LoopType > 4 || loop_begin != ~0 || loop_end != ~0)
            {
                samples_fade_begin = (unsigned int)~0;
                samples_fade_end = (unsigned)~0;
                doing_loop = true;
            }
            else
            {
                unsigned samples_length = length_samples;

                samples_fade_begin = samples_length;
                samples_fade_end = samples_length;
                doing_loop = false;
            }
        }

    #ifdef DXISUPPORT
        if (_SelectedPluginIndex == 5)
        {
            pfc::array_t<t_uint8> serialized_midi_file;
            midi_file.serialize_as_standard_midi_file(serialized_midi_file);

            delete dxiProxy;
            dxiProxy = NULL;

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
                        dont_loop = true;

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
            // VSTi
            if (_PluginID == 1)
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
            
                    if (Preset.vst_config.size())
                        Player->setChunk(&Preset.vst_config[0], Preset.vst_config.size());

                    _Player = Player;
                }

                {
                    _Player->setSampleRate(_SampleRate);
                    _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                    unsigned LoopMode = MIDIPlayer::loop_mode_enable;

                    if (doing_loop)
                        LoopMode |= MIDIPlayer::loop_mode_force;

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

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
            if (_SelectedPluginIndex == 2 || _SelectedPluginIndex == 4)
            {
                /*HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE );
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

                if (sfPlayer->Load(midi_file, p_subsong, LoopMode, clean_flags))
                {
                    eof = false;
                    dont_loop = true;

                    return;
                }
            }
    #ifdef BASSMIDISUPPORT
            else if (_SelectedPluginIndex == 4)
            #endif
            #else
            else
            // BASS MIDI
            if (_PluginID == 2 || _PluginID == 4)
    #endif
    #ifdef BASSMIDISUPPORT
            {
                {
                    delete _Player;

                    {
                        bassmidi_voices = 0;
                        bassmidi_voices_max = 0;

                        if (Preset._SoundFontPathName.is_empty() && file_soundfont.is_empty())
                        {
                            console::print("No SoundFonts configured, and no file or directory SoundFont found");
                            throw exception_io_data();
                        }
                    }

                    BMPlayer * Player = new BMPlayer;

                    Player->setSoundFont(Preset._SoundFontPathName);

                    if (file_soundfont.length())
                        Player->setFileSoundFont(file_soundfont);

                    Player->setInterpolation(_ResamplingMode);
                    Player->setEffects(Preset._BASSMIDIEffects);
                    Player->setVoices(Preset._BASSMIDIVoices);

                    _Player = Player;
                }

                {
                    _Player->setSampleRate(_SampleRate);
                    _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                    unsigned LoopMode = MIDIPlayer::loop_mode_enable;

                    if (doing_loop)
                        LoopMode |= MIDIPlayer::loop_mode_force;

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

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
            // libADLMIDI
            if (_PluginID == 6)
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

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

                        return;
                    }
                }
            }
            else
            // libOPNMIDI
            if (_PluginID == 7)
            {
                {
                    delete _Player;

                    OPNPlayer * Player = new OPNPlayer;

                    Player->setBank(Preset.opn_bank);
                    Player->setChipCount(Preset._ADLChipCount);
                    Player->setFullPanning(Preset._ADLUsePanning);
                    Player->setCore(Preset.opn_emu_core);

                    _Player = Player;
                }

                {
                    _Player->setSampleRate(_SampleRate);
                    _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                    unsigned LoopMode = MIDIPlayer::loop_mode_enable;

                    if (doing_loop)
                        LoopMode |= MIDIPlayer::loop_mode_force;

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

                        return;
                    }
                }
            }
            else
            // MUNT (MT32)
            if (_PluginID == 3)
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

                    MT32Player * Player = new MT32Player(!IsMT32, Preset.munt_gm_set);

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

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

                        return;
                    }
                }
            }
            else
            // Nuclear Option
            if (_PluginID == 9)
            {
                {
                    delete _Player;

                    MSPlayer * Player = new MSPlayer();

                    Player->SetSynth(Preset.ms_synth);
                    Player->SetBank(Preset.ms_bank);
                    Player->SetExtp(Preset.ms_panning);

                    _Player = Player;
                }

                {
                    _Player->setSampleRate(_SampleRate);

                    unsigned LoopMode = MIDIPlayer::loop_mode_enable;

                    if (doing_loop)
                        LoopMode |= MIDIPlayer::loop_mode_force;

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

                        return;
                    }
                }
            }
            else
            // Secret Sauce
            if (_PluginID == 10)
            {
                {
                    delete _Player;

                    SCPlayer * Player = new SCPlayer();

                    pfc::string8 p_path;

                    CfgSecretSaucePath.get(p_path);

                    if (p_path.is_empty())
                    {
                        console::print("Secret Sauce path not configured, yet somehow enabled, trying plugin directory");
                        p_path = core_api::get_my_full_path();
                        p_path.truncate(p_path.scan_filename());
                    }

                    Player->set_sccore_path(p_path);

                    _Player = Player;
                }

                {
                    _Player->setFilterMode((MIDIPlayer::filter_mode) Preset._MIDIStandard, !Preset._UseMIDIEffects);
                    _Player->setSampleRate(_SampleRate);

                    unsigned LoopMode = MIDIPlayer::loop_mode_enable;

                    if (doing_loop)
                        LoopMode |= MIDIPlayer::loop_mode_force;

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        eof = false;
                        dont_loop = true;

                        return;
                    }
                }
            }
            else
            // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413 ))
            if (_PluginID == 0)
            {
                {
                    delete _Player;

                    EMIDIPlayer * Player = new EMIDIPlayer;

                    _Player = Player;
                }

                {
                    unsigned LoopMode = MIDIPlayer::loop_mode_enable;

                    if (doing_loop)
                        LoopMode |= MIDIPlayer::loop_mode_force;

                    if (_Player->Load(_Container, subsongIndex, LoopMode, clean_flags))
                    {
                        {
                            insync(sync);

                            if (++g_running == 1)
                                g_srate = _SampleRate;
                            else
                            if (_SampleRate != (unsigned int)g_srate)
                                _SampleRate = (unsigned int)g_srate;

                            is_emidi = true;
                        }

                        _Player->setSampleRate(_SampleRate);

                        eof = false;
                        dont_loop = true;

                        return;
                    }
                }
            }

        throw exception_io_data();
    }

    bool decode_run(audio_chunk & p_chunk, abort_callback & p_abort)
    {
        p_abort.check();

        if (eof)
            return false;

        bool rv = false;

    #ifdef DXISUPPORT
        if (_SelectedPluginIndex == 5)
        {
            unsigned todo = 4096;

            if (dont_loop)
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

            p_chunk.set_data_32(ptr, todo, 2, srate);
        #else
            p_chunk.set_data_size(todo * 2);
            float * ptr = p_chunk.get_data();
            dxiProxy->fillBuffer(ptr, todo);
            p_chunk.set_srate(srate);
            p_chunk.set_channels(2);
            p_chunk.set_sample_count(todo);
        #endif

            samples_done += todo;

            rv = true;
        }
        else
    #endif
        if (_PluginID == 1)
        {
            VSTiPlayer * vstPlayer = (VSTiPlayer *) _Player;

            size_t todo = 4096;
            unsigned nch = vstPlayer->getChannelCount();

            p_chunk.set_data_size(todo * nch);

            audio_sample * out = p_chunk.get_data();

            unsigned done = vstPlayer->Play(out, todo);

            if (!done) return false;

            p_chunk.set_srate(_SampleRate);
            p_chunk.set_channels(nch);
            p_chunk.set_sample_count(done);

            rv = true;
        }
        else
        if (_PluginID == 3)
        {
            MT32Player * mt32Player = (MT32Player *) _Player;

            size_t todo = 4096;

            p_chunk.set_data_size(todo * 2);

            audio_sample * out = p_chunk.get_data();

            mt32Player->setAbortCallback(&p_abort);

            unsigned done = mt32Player->Play(out, todo);

            if (!done)
                return false;

            p_chunk.set_srate(_SampleRate);
            p_chunk.set_channels(2);
            p_chunk.set_sample_count(done);

            rv = true;
        }
        else
        if (_Player)
        {
            size_t todo = 4096;

            p_chunk.set_data_size(todo * 2);

            audio_sample * out = p_chunk.get_data();

            unsigned done = _Player->Play(out, todo);

            if (!done)
            {
                std::string last_error;

                if (_Player->GetLastError(last_error))
                    throw exception_io_data(last_error.c_str());

                return false;
            }

            p_chunk.set_srate(_SampleRate);
            p_chunk.set_channels(2);
            p_chunk.set_sample_count(done);

            rv = true;
        }

        if (rv && samples_fade_begin != ~0 && samples_fade_end != ~0)
        {
            unsigned samples_played_start = samples_played;
            unsigned samples_played_end = samples_played + p_chunk.get_sample_count();

            samples_played = samples_played_end;

            if (samples_played_end >= samples_fade_begin)
            {
                for (unsigned i = std::max(samples_fade_begin, samples_played_start), j = std::min(samples_played_end, samples_fade_end); i < j; ++i)
                {
                    audio_sample * sample = p_chunk.get_data() + (i - samples_played_start) * 2;
                    audio_sample scale = (audio_sample) (samples_fade_end - i) / (audio_sample) (samples_fade_end - samples_fade_begin);

                    sample[0] *= scale;
                    sample[1] *= scale;
                }

                if (samples_played_end > samples_fade_end)
                {
                    unsigned samples_remain = 0;

                    if (samples_fade_end > samples_played_start)
                        samples_remain = samples_fade_end - samples_played_start;

                    p_chunk.set_sample_count(samples_remain);

                    eof = true;

                    if (!samples_remain)
                        return false;
                }
            }
        }

        dynamic_time = p_chunk.get_duration();

        return rv;
    }

    void decode_seek(double p_seconds, abort_callback&)
    {
        unsigned seek_msec = unsigned(audio_math::time_to_samples(p_seconds, 1000));

        // This value should not be wrapped to within the loop range
        samples_played = unsigned((t_int64(seek_msec) * t_int64(_SampleRate)) / 1000);

        if (seek_msec > loop_end_ms)
        {
            seek_msec = (seek_msec - loop_begin_ms) % (loop_end_ms - loop_begin_ms) + loop_begin_ms;
        }

        first_block = true;
        eof = false;

        unsigned done = unsigned((t_int64(seek_msec) * t_int64(_SampleRate)) / 1000);
        if (length_samples && done >= (length_samples - _SampleRate))
        {
            eof = true;
            return;
        }

    #ifdef DXISUPPORT
        if (_SelectedPluginIndex == 5)
        {
            dxiProxy->setPosition(seek_msec);

            samples_done = done;

            return;
        }
        else
    #endif
        if (_Player)
        {
            _Player->Seek(done);
            return;
        }
    }

    bool decode_can_seek()
    {
        return true;
    }

    bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
    {
        int ret = false;

        if (first_block)
        {
            p_out.info_set_int("samplerate", _SampleRate);
            p_timestamp_delta = 0.;
            first_block = false;
            ret = true;
        }

    #ifdef BASSMIDISUPPORT
    #ifdef FLUIDSYNTHSUPPORT
        if (_SelectedPluginIndex == 4)
        #else
        if (_PluginID == 2 || _PluginID == 4)
        #endif
        {
            BMPlayer * bmPlayer = (BMPlayer *) _Player;
            unsigned voices = bmPlayer->getVoicesActive();

            if (voices != bassmidi_voices)
            {
                p_out.info_set_int("bassmidi_voices", voices);
                bassmidi_voices = voices;
                ret = true;
            }

            if (voices > bassmidi_voices_max)
            {
                p_out.info_set_int("bassmidi_voices_max", voices);
                bassmidi_voices_max = voices;
                ret = true;
            }

            if (ret)
                p_timestamp_delta = dynamic_time;
        }
    #endif

        return (bool)ret;
    }

    bool decode_get_dynamic_info_track(file_info&, double&)
    {
        return false;
    }

    void decode_on_idle(abort_callback&)
    {
    }

    void retag_set_info(t_uint32, const file_info& p_info, abort_callback & p_abort)
    {
        if (_IsSyxFile)
            throw exception_io_data("You cannot tag SysEx dumps");

        file_info_impl m_info(p_info);

        m_info.meta_remove_field(field_preset);

        const char * midi_preset = m_info.info_get(field_preset);

        if (midi_preset)
            m_info.meta_set(field_preset, midi_preset);

        m_info.meta_remove_field(field_syx);

        const char * midi_syx = m_info.info_get(field_syx);

        if (midi_syx)
        {
            m_info.meta_set(field_syx, midi_syx);
        }

        file::ptr tag_file;
        filesystem::g_open_tempmem(tag_file, p_abort);

        tag_processor::write_apev2(tag_file, m_info, p_abort);

        pfc::array_t<t_uint8> tag;

        tag_file->seek(0, p_abort);
        tag.set_count(tag_file->get_size_ex(p_abort));
        tag_file->read_object(tag.get_ptr(), tag.get_count(), p_abort);

        static_api_ptr_t<metadb_index_manager>()->set_user_data(GUIDTrackHasher, m_index_hash, tag.get_ptr(), tag.get_count());
    }

    void retag_commit(abort_callback&)
    {
    }

    void remove_tags(abort_callback&)
    {
    }

    static bool g_is_our_content_type(const char * p_content_type)
    {
        return !strcmp(p_content_type, "audio/midi");
    }

    static bool g_is_our_path(const char *, const char * p_extension)
    {
        return IsFileExtensionSupported(p_extension) || g_test_extension_syx(p_extension);
    }

    static GUID g_get_guid()
    {
        static const GUID guid = { 0xae29c554, 0xee59, 0x4c1a, { 0x82, 0x11, 0x32, 0xf, 0x2a, 0x1a, 0x99, 0x2b } }; // {AE29C554-EE59-4C1A-8211-320F2A1A992B}

        return guid;
    }

    static const char * g_get_name()
    {
        return COMPONENT_NAME;
    }

    static GUID g_get_preferences_guid()
    {
        static const GUID guid = COMPONENT_GUID;

        return guid;
    }

    static bool g_is_low_merit()
    {
        return false;
    }

private:
    double get_length(unsigned p_subsong)
    {
        length_ms = _Container.get_timestamp_end(p_subsong, true);

        double length = length_ms * .001;

        if (_LoopType == 1)
            length += 1.;

        length_ticks = _Container.get_timestamp_end(p_subsong); // theSequence->m_tempoMap.Sample2Tick(len, 1000);
        length_samples = (unsigned) (((__int64) length_ms * (__int64) _SampleRate) / 1000);

        if (_LoopType == 1)
            length_samples += _SampleRate;

        loop_begin = _Container.get_timestamp_loop_start(p_subsong);
        loop_end = _Container.get_timestamp_loop_end(p_subsong);
        loop_begin_ms = _Container.get_timestamp_loop_start(p_subsong, true);
        loop_end_ms = _Container.get_timestamp_loop_end(p_subsong, true);

        if (loop_begin != ~0 || loop_end != ~0 || _LoopType > 2)
        {
            if (loop_begin_ms == ~0) loop_begin_ms = 0;
            if (loop_end_ms == ~0) loop_end_ms = length_ms;
            length = (double) (loop_begin_ms + (loop_end_ms - loop_begin_ms) * loop_count + fade_ms) * 0.001;
        }

        return length;
    }

    void set_loop()
    {
    #ifdef DXISUPPORT
        if (_SelectedPluginIndex == 5 && dxiProxy)
        {
            dxiProxy->setLoop(loop_begin != ~0 ? loop_begin : 0, loop_end != ~0 ? loop_end : length_ticks);
        }
        /*else
        {
            sample_loop_start = theSequence->m_tempoMap.Tick2Sample(loop_begin != -1 ? loop_begin : 0, srate);
            sample_loop_end = theSequence->m_tempoMap.Tick2Sample((loop_end != -1 ? loop_end : length_ticks) + 1, srate);
        }*/
        else
        #endif
            dont_loop = false;
    }

    void meta_add(file_info & p_info, const char * name, const char * value, t_size max)
    {
        if (value[0])
        {
            pfc::string8 t;

            if (max && value[max - 1])
            {
                // TODO: moo
                t.set_string(value, max);
                value = t;
            }
            else
                max = strlen(value);

            if (pfc::is_lower_ascii(value) || pfc::is_valid_utf8(value, max))
                p_info.meta_add(name, value);
            else
            if (IsValidShiftJIS(value, max))
                p_info.meta_add(name, pfc::stringcvt::string_utf8_from_codepage(932, value)); // Shift-JIS
            else
                p_info.meta_add(name, pfc::stringcvt::string_utf8_from_ansi(value));
        }
    }

private:
    MIDIPlayer * _Player;
    midi_container _Container;

    pfc::string8 _FilePath;

    t_filestats _FileStats;
    t_filestats2 _FileStats2;

    bool _IsSyxFile;

    metadb_index_hash m_index_hash;
    hasher_md5_result _FileHash;

    unsigned _TrackCount;

    unsigned _PluginID;
    unsigned _SampleRate;
    unsigned _ResamplingMode;

    bool is_emidi;

    bool b_thloopz;
    bool b_rpgmloopz;
    bool b_xmiloopz;
    bool b_ff7loopz;

    bool doing_loop;

    unsigned _LoopType;
    unsigned loop_type_playback;
    unsigned loop_type_other;
    unsigned clean_flags;

    unsigned length_ms;
    unsigned length_samples;
    unsigned length_ticks;
    unsigned samples_done;

    unsigned loop_begin, loop_begin_ms;
    unsigned loop_end, loop_end_ms;

    unsigned loop_count, fade_ms;

    unsigned samples_played;
    unsigned samples_fade_begin;
    unsigned samples_fade_end;

    bool eof;
    bool dont_loop;

    bool first_block;

#ifdef BASSMIDISUPPORT
    unsigned bassmidi_voices, bassmidi_voices_max;
#endif
    double dynamic_time;

#ifdef DXISUPPORT
    DXiProxy * dxiProxy;

#if audio_sample_size != 32
    pfc::array_t<float> sample_buffer;
#endif
#endif
#ifdef FLUIDSYNTHSUPPORT
    unsigned fluid_interp_method;
#endif
};
#pragma warning(default: 4820) // x bytes padding added after data member

static input_factory_t<input_midi> g_input_midi_factory;

static size_t DecodeShiftJISChar(const uint8_t * data, size_t size)
{
    if (size == 0)
        return 0;

    if (size > 2)
        size = 2;

    if (data[0] < 0x80)
        return data[0] > 0 ? 1 : 0;

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

static bool IsValidShiftJIS(const char * data, size_t size)
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

/*
#define XMIDI_CONTROLLER_FOR_LOOP 0x74 // For Loop
#define XMIDI_CONTROLLER_NEXT_BREAK 0x75 // Next/Break

#define EMIDI_CONTROLLER_TRACK_DESIGNATION 110 // Track Designation
#define EMIDI_CONTROLLER_TRACK_EXCLUSION 111 // Track Exclusion
#define EMIDI_CONTROLLER_LOOP_BEGIN XMIDI_CONTROLLER_FOR_LOOP
#define EMIDI_CONTROLLER_LOOP_END XMIDI_CONTROLLER_NEXT_BREAK
*/
