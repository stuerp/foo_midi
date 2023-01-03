
/** $VER: InputDecoder.h (2023.01.03) **/

#pragma once

#define NOMINMAX

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/input_impl.h>
#include <sdk/file_info_impl.h>
#include <sdk/tag_processor.h>

#include <shared/audio_math.h>

#include <pfc/synchro.h>

#include <midi_processing/midi_processor.h>

#include "Configuration.h"
#include "FileHasher.h"
#include "MIDIPreset.h"
#include "MIDISysExDumps.h"

/** Players **/

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "EdMPlayer.h"
#include "NukePlayer.h"
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

extern volatile int _IsRunning;
extern critical_section _Lock;
extern volatile unsigned int _CurrentSampleRate;

/// <summary>
/// Implements an input decoder.
/// </summary>
#pragma warning(disable: 4820) // x bytes padding added after data member
class InputDecoder : public input_stubs
{
public:
    InputDecoder() :
        _SampleRate((unsigned int)CfgSampleRate),
        _ResamplingMode((unsigned int)CfgResamplingMode),

        _LoopTypePlayback((unsigned int)CfgLoopTypePlayback),
        _LoopTypeOther((unsigned int)CfgLoopTypeOther),

        _UseXMILoops(!!cfg_xmiloopz),
        _UseFF7Loops(!!cfg_ff7loopz),
        _UseRPGMLoops(!!cfg_rpgmloopz),
        _UseThLoops(!!cfg_thloopz)
    {
        _IsEmuDeMIDI = false;

        _Player = nullptr;

        _LengthInSamples = 0;
        _LengthInTicks = 0;

        _CleanFlags = (unsigned int)(cfg_emidi_exclusion ? midi_container::clean_flag_emidi : 0) |
                                    (cfg_filter_instruments ? midi_container::clean_flag_instruments : 0) |
                                    (cfg_filter_banks ? midi_container::clean_flag_banks : 0);

        _LoopCount = (unsigned int)CfgLoopCount.get();
        _FadeTimeInMs = (unsigned int)CfgFadeTimeInMS.get();

    #ifdef FLUIDSYNTHSUPPORT
        fluid_interp_method(Cfg_FluidSynthInterpolationMethod),
    #endif

    #ifdef DXISUPPORT
        dxiProxy = nullptr;
    #endif
    }

    InputDecoder(const InputDecoder&) = delete;
    InputDecoder(const InputDecoder&&) = delete;
    InputDecoder& operator=(const InputDecoder&) = delete;
    InputDecoder& operator=(InputDecoder&&) = delete;

    ~InputDecoder()
    {
        delete _Player;

        if (_IsEmuDeMIDI)
        {
            insync(_Lock);
            _IsRunning--;
        }
    #ifdef DXISUPPORT
        if (dxiProxy)
            delete dxiProxy;
    #endif
    }

public:

    #pragma region("input_impl")
    void open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler);
    #pragma endregion

    #pragma region("input_info_reader")
    unsigned get_subsong_count()
    {
        return _IsSysExFile ? 1 : _Container.get_subsong_count();
    }

    t_uint32 get_subsong(unsigned subSongIndex)
    {
        return _IsSysExFile ? 0 : _Container.get_subsong(subSongIndex);
    }

    void get_info(t_uint32 subsongIndex, file_info & fileInfo, abort_callback & abortHandler);
    #pragma endregion

    #pragma region("input_info_reader_v2")
    t_filestats2 get_stats2(uint32_t, abort_callback&)
    {
        return _FileStats2;
    }

    t_filestats get_file_stats(abort_callback&)
    {
        return _FileStats;
    }
    #pragma endregion

    #pragma region("input_decoder")
    void decode_initialize(unsigned subsongIndex, unsigned flags, abort_callback & abortHandler);

    bool decode_run(audio_chunk & audioChunk, abort_callback & abortHandler);

    void decode_seek(double p_seconds, abort_callback&);

    bool decode_can_seek()
    {
        return true;
    }

    bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);

    bool decode_get_dynamic_info_track(file_info&, double&)
    {
        return false;
    }

    void decode_on_idle(abort_callback&)
    {
    }
    #pragma endregion

    #pragma region("input_info_writer")
    void retag_set_info(t_uint32, const file_info& fileInfo, abort_callback & abortHandler);

    void retag_commit(abort_callback&) { }

    void remove_tags(abort_callback&) { }
    #pragma endregion

    static bool g_is_our_content_type(const char * p_content_type)
    {
        return ::strcmp(p_content_type, "audio/midi") == 0;
    }

    static bool g_is_our_path(const char *, const char * p_extension)
    {
        return IsMIDIFileExtension(p_extension) || IsSysExFileExtension(p_extension);
    }

    static GUID g_get_guid()
    {
        static const GUID guid = { 0xae29c554, 0xee59, 0x4c1a, { 0x82, 0x11, 0x32, 0xf, 0x2a, 0x1a, 0x99, 0x2b } }; // {AE29C554-EE59-4C1A-8211-320F2A1A992B}

        return guid;
    }

    static const char * g_get_name()
    {
        return STR_COMPONENT_NAME;
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
    double InitializeTime(unsigned subsongIndex);

    void AddMetaData(file_info & fileInfo, const char * name, const char * value, t_size max);

    static bool GetSoundFontFilePath(const pfc::string8 filePath, pfc::string8 & soundFontPath, abort_callback & abortHandler) noexcept;

    #ifdef DXISUPPORT
    void set_loop()
    {
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
            _DontLoop = false;
    }
    #endif

private:
    MIDIPlayer * _Player;
    midi_container _Container;

    pfc::string8 _FilePath;

    t_filestats _FileStats;
    t_filestats2 _FileStats2;

    bool _IsSysExFile;

    metadb_index_hash _Hash;
    hasher_md5_result _FileHash;

    unsigned _PlayerType;
    unsigned _SampleRate;
    unsigned _ResamplingMode;

    unsigned _LoopType;
    unsigned _LoopTypePlayback;
    unsigned _LoopTypeOther;

    unsigned _CleanFlags;

    bool _UseXMILoops;
    bool _UseFF7Loops;
    bool _UseRPGMLoops;
    bool _UseThLoops;

    unsigned _TrackCount;

    unsigned _LengthInMS;
    unsigned _LengthInTicks;
    unsigned _LengthInSamples;
    unsigned _LoopBegin;
    unsigned _LoopBeginInMS;
    unsigned _LoopEnd;
    unsigned _LoopEndInMS;

    unsigned _LoopCount;
    unsigned _FadeTimeInMs;

    unsigned _SamplesPlayed;
    unsigned _SamplesDone;
    unsigned _SamplesFadeBegin;
    unsigned _SamplesFadeEnd;

    bool _IsEmuDeMIDI;
    bool doing_loop;
    bool eof;
    bool _DontLoop;
    bool _IsFirstBlock;

    double _AudioChunkDuration;

#ifdef BASSMIDISUPPORT
    unsigned bassmidi_voices, bassmidi_voices_max;
#endif

#ifdef FLUIDSYNTHSUPPORT
    unsigned fluid_interp_method;
#endif

#ifdef DXISUPPORT
    DXiProxy * dxiProxy;

#if audio_sample_size != 32
    pfc::array_t<float> sample_buffer;
#endif
#endif
};
#pragma warning(default: 4820) // x bytes padding added after data member
