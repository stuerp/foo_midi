
/** $VER: InputDecoder.h (2023.05.18) **/

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
/// Represents a range.
/// </summary>
class Range
{
public:
    Range() noexcept  { Clear(); }
    virtual ~Range() noexcept { }

    unsigned int Begin() const noexcept { return _Begin; }
    unsigned int End() const noexcept { return _End; }

    void Set(unsigned int begin, unsigned int end) noexcept { _Begin = begin; _End = end; }
    void SetBegin(unsigned int begin) noexcept { _Begin = begin; }
    void SetEnd(unsigned int end) noexcept { _End = end; }
    void Clear() { Set(pfc::infinite32, pfc::infinite32); }

    unsigned int Size() const noexcept { return _End - _Begin; }

    bool IsEmpty() const noexcept { return (_Begin == pfc::infinite32) && (_End == pfc::infinite32); }
    bool IsSet() const noexcept { return  (_Begin != pfc::infinite32) && (_End != pfc::infinite32); }
    bool HasBegin() const noexcept { return _Begin != pfc::infinite32; }
    bool HasEnd() const noexcept { return _End != pfc::infinite32; }

private:
    unsigned int _Begin;
    unsigned int _End;
};

/// <summary>
/// Implements an input decoder.
/// </summary>
#pragma warning(disable: 4820) // x bytes padding added after data member
class InputDecoder : public input_stubs
{
public:
    InputDecoder() :
        _IsSysExFile(false),
        _TrackCount(0),

        _DetectXMILoops(!!CfgDetectXMILoops),
        _DetectFF7Loops(!!CfgDetectFF7Loops),
        _DetectRPGMakerLoops(!!CfgDetectRPGMakerLoops),
        _DetectTouhouLoops(!!CfgDetectTouhouLoops),

        _LoopRange(),
        _LoopInMs(),

        _Player(nullptr),

        _SampleRate((unsigned int)CfgSampleRate),

        _LoopTypePlayback((unsigned int)CfgLoopTypePlayback),
        _LoopTypeOther((unsigned int)CfgLoopTypeOther),

        _BASSMIDIResamplingMode((unsigned int)CfgResamplingMode)
    {
        _FileStats = { 0 };
        _FileStats2 = { 0 };

        _IsEmuDeMIDI = false;

        _LengthInSamples = 0;
        _LengthInTicks = 0;

        _CleanFlags = (unsigned int)(CfgEmuDeMIDIExclusion ? midi_container::CleanFlagEMIDI : 0) |
                                    (CfgFilterInstruments ? midi_container::clean_flag_instruments : 0) |
                                    (CfgFilterBanks ? midi_container::clean_flag_banks : 0);

        _LoopCount    = (unsigned int)AdvCfgLoopCount.get();
        _FadeDuration = (unsigned int)AdvCfgFadeTimeInMS.get();

    #ifdef FLUIDSYNTHSUPPORT
        _FluidSynthInterpolationMethod(Cfg_FluidSynthInterpolationMethod),
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

    #pragma region("input_decoder")
    void decode_initialize(unsigned subsongIndex, unsigned flags, abort_callback & abortHandler);

    bool decode_run(audio_chunk & audioChunk, abort_callback & abortHandler);

    void decode_seek(double p_seconds, abort_callback &);

    bool decode_can_seek()
    {
        return true;
    }

    bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);

    bool decode_get_dynamic_info_track(file_info &, double &) noexcept { return false; }

    void decode_on_idle(abort_callback &) noexcept { }
    #pragma endregion

    #pragma region("input_info_reader")
    unsigned get_subsong_count()
    {
        return _IsSysExFile ? 1 : (unsigned)_Container.GetSubSongCount();
    }

    t_uint32 get_subsong(unsigned subSongIndex)
    {
        return _IsSysExFile ? 0 : (t_uint32)_Container.GetSubSong(subSongIndex);
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

    #pragma region("input_info_writer")
    void retag_set_info(t_uint32, const file_info& fileInfo, abort_callback & abortHandler);

    void retag_commit(abort_callback &) { }

    void remove_tags(abort_callback &) { }
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

    static void InitializeIndexManager();

private:
    void InitializeTime(size_t subsongIndex);

    void ConvertMetaDataToTags(size_t subSongIndex, file_info & fileInfo, abort_callback & abortHandler);
    void AddTag(file_info & fileInfo, const char * name, const char * value, t_size max);

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
    // File Properties
    pfc::string8 _FilePath;

    t_filestats _FileStats;
    t_filestats2 _FileStats2;

    midi_container _Container;

    bool _IsSysExFile;
    size_t _TrackCount;

    metadb_index_hash _Hash;
    hasher_md5_result _FileHash;

    bool _IsMT32;

    bool _DetectXMILoops;
    bool _DetectFF7Loops;
    bool _DetectRPGMakerLoops;
    bool _DetectTouhouLoops;

    Range _LoopRange;
    Range _LoopInMs;

    // Player Properties
    MIDIPlayer * _Player;

    uint32_t _PlayerType;
    uint32_t _SampleRate;

    uint32_t _LoopType;
    uint32_t _LoopTypePlayback;
    uint32_t _LoopTypeOther;

    uint32_t _CleanFlags;

    uint32_t _DurationInMS;
    uint32_t _LengthInTicks;
    uint32_t _LengthInSamples;

    uint32_t _LoopCount;

    uint32_t _SamplesPlayed;
    uint32_t _SamplesDone;

    uint32_t _FadeDuration; // in ms
    Range _FadeRange;

    bool _IsEmuDeMIDI;
    bool _IsLooping;
    bool _IsEndOfContainer;
    bool _DontLoop;
    bool _IsFirstChunk;

    double _AudioChunkDuration;

    uint32_t _BASSMIDIResamplingMode;
    uint32_t _BASSMIDIVoiceCount;
    uint32_t _BASSMIDIVoiceMax;

#ifdef FLUIDSYNTHSUPPORT
    unsigned int _FluidSynthInterpolationMethod;
#endif

#ifdef DXISUPPORT
    DXiProxy * dxiProxy;

#if audio_sample_size != 32
    pfc::array_t<float> sample_buffer;
#endif
#endif
};
#pragma warning(default: 4820) // x bytes padding added after data member
