
/** $VER: InputDecoder.h (2025.07.13) **/

#pragma once

#define NOMINMAX

#include <sdk/foobar2000-lite.h>
#include <sdk/input_impl.h>
#include <sdk/file_info_impl.h>
#include <sdk/tag_processor.h>

#include <shared/audio_math.h>

#include <pfc/synchro.h>

#include "Configuration.h"
#include "FileHasher.h"
#include "Preset.h"
#include "MIDISysExDumps.h"
#include "SoundFont.h"

/** Players **/

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "CLAPPlayer.h"
#include "EdMPlayer.h"
#include "FMMPlayer.h"
#include "FSPlayer.h"
#include "MCIPlayer.h"
#include "MT32Player/MT32Player.h"
#include "NukedOPL3Player.h"
#include "NukedSC-55Player.h"
#include "OPNPlayer/OPNPlayer.h"
#include "SCPlayer.h"
#include "VSTiPlayer.h"

#include "Resource.h"

/* KEEP? 06/07/25
extern volatile int _IsRunning;
extern critical_section _Lock;
extern volatile uint32_t _CurrentSampleRate;
*/

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements an input decoder.
/// </summary>
class InputDecoder : public input_stubs
{
public:
    InputDecoder() noexcept;

    InputDecoder(const InputDecoder &) = delete;
    InputDecoder(InputDecoder &&) = delete;
    InputDecoder & operator=(const InputDecoder &) = delete;
    InputDecoder & operator=(InputDecoder &&) = delete;

    virtual ~InputDecoder() noexcept;

public:
    #pragma region input_impl

    void open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler);

    static bool g_is_our_content_type(const char * contentType)
    {
        return (contentType != nullptr) && ::strcmp(contentType, "audio/midi") == 0;
    }

    static bool g_is_our_path(const char *, const char * fileExtension)
    {
        return (fileExtension != nullptr) && (IsMIDIFileExtension(fileExtension) || IsSysExFileExtension(fileExtension));
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

    // Returns the GUID of the Preferences page (if there is one.)
    static GUID g_get_preferences_guid()
    {
        return PreferencesPageGUID;
    }

    static bool g_is_low_merit()
    {
        return false;
    }

    #pragma endregion

    #pragma region input_decoder

    void decode_initialize(unsigned subsongIndex, unsigned flags, abort_callback & abortHandler);

    bool decode_run(audio_chunk & audioChunk, abort_callback & abortHandler);

    void decode_seek(double seconds, abort_callback &);

    bool decode_can_seek()
    {
        return true;
    }

    bool decode_get_dynamic_info(file_info & fi, double & timestampDelta);

    bool decode_get_dynamic_info_track(file_info &, double &) noexcept { return false; }

    void decode_on_idle(abort_callback &) noexcept { }

    #pragma endregion

    #pragma region input_info_reader

    unsigned get_subsong_count()
    {
        return _IsSysExFile ? 1 : (unsigned) _Container.GetSubSongCount();
    }

    t_uint32 get_subsong(unsigned subSongIndex)
    {
        return _IsSysExFile ? 0 : (unsigned) _Container.GetSubSong(subSongIndex);
    }

    void get_info(t_uint32 subsongIndex, file_info & fileInfo, abort_callback & abortHandler);

    #pragma endregion

    #pragma region input_info_reader_v2

    t_filestats2 get_stats2(uint32_t, abort_callback &) const
    {
        return _FileStats2;
    }

    t_filestats get_file_stats(abort_callback &) const
    {
        return _FileStats;
    }

    #pragma endregion

    #pragma region input_info_writer

    void retag_set_info(t_uint32, const file_info & fileInfo, abort_callback & abortHandler) const;

    void retag_commit(abort_callback &) { }

    void remove_tags(abort_callback &) { }

    #pragma endregion

    static void InitializeIndexManager();

private:
    uint32_t GetDuration(size_t subSongIndex) noexcept;
    void InitializeFade() noexcept;
    void SelectPlayer(preset_t & preset, size_t subSongIndex, abort_callback & abortHandler) noexcept;
    void GetSoundfonts(const pfc::string & defaultSoundFontPath, abort_callback & abortHandler);

    void ConvertMetaDataToTags(size_t subSongIndex, file_info & fileInfo, abort_callback & abortHandler);
    void AddTag(file_info & fileInfo, const char * name, const char * value, t_size max);

private:
    unsigned _DecoderFlags;

    // File Properties
    pfc::string _FilePath;

    t_filestats _FileStats;
    t_filestats2 _FileStats2;

    midi::container_t _Container;

    std::vector<soundfont_t> _Soundfonts;

    bool _IsSysExFile;

    metadb_index_hash _Hash;
    hasher_md5_result _FileHash;

    bool _IsMT32;
    bool _IsXG;

    bool _DetectRPGMakerLoops;
    bool _DetectLeapFrogLoops;
    bool _DetectXMILoops;
    bool _DetectTouhouLoops;
    bool _DetectFF7Loops;

    // Player Properties
    player_t * _Player;
    CLAP::Host * _Host;

    PlayerTypes _PlayerType;
    bool _IsPlayerTypeOverriden;
    std::string _PlugInName;

    // Sample rate dependent
    uint32_t _RequestedSampleRate;  // in Hz
    uint32_t _ActualSampleRate;     // in Hz

    uint32_t _Time;                 // in frames
    uint32_t _TotalTime;            // in frames

    LoopTypes _LoopType;
    range_t _LoopRange;             // in ms
    range_t _FadeRange;             // in ms

    // Flags
    uint32_t _ExtraPercussionChannel;
    uint32_t _CleanFlags;

    float _BASSMIDIVolume;
    uint32_t _BASSMIDIInterpolationMode;

    uint32_t _FluidSynthInterpolationMethod;

    bool _IsEndOfContainer;
    bool _IsFirstBlock;

    double _AudioChunkDuration;     // in ms

    uint32_t _ActiveVoiceCount;
    uint32_t _PeakVoiceCount;

    static constexpr GUID GUIDTagMIDIHash = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };
    static const char * PlayerTypeNames[15];
};

#pragma warning(default: 4820) // x bytes padding added after data member
