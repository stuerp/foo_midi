/*
 * Copyright (с) 2004 Mitsutaka Okazaki
 * Copyright (с) 2010-2018 Chris Moeller <kode54@gmail.com>
 * Copyright (с) 2021-2025 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef EMUDEMIDI_H
#define EMUDEMIDI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDMIDI_VERSION_MAJOR       1
#define EDMIDI_VERSION_MINOR       0
#define EDMIDI_VERSION_PATCHLEVEL  0

#define EDMIDI_TOSTR_I(s) #s
#define EDMIDI_TOSTR(s) EDMIDI_TOSTR_I(s)
#define EDMIDI_VERSION \
        EDMIDI_TOSTR(EDMIDI_VERSION_MAJOR) "." \
        EDMIDI_TOSTR(EDMIDI_VERSION_MINOR) "." \
        EDMIDI_TOSTR(EDMIDI_VERSION_PATCHLEVEL)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#include <stdint.h>
typedef uint8_t         EDMIDI_UInt8;
typedef uint16_t        EDMIDI_UInt16;
typedef int8_t          EDMIDI_SInt8;
typedef int16_t         EDMIDI_SInt16;
#else
typedef unsigned char   EDMIDI_UInt8;
typedef unsigned short  EDMIDI_UInt16;
typedef char            EDMIDI_SInt8;
typedef short           EDMIDI_SInt16;
#endif

/* == Deprecated function markers == */

#if defined(_MSC_VER) /* MSVC */
#   if _MSC_VER >= 1500 /* MSVC 2008 */
                     /*! Indicates that the following function is deprecated. */
#       define EDMIDI_DEPRECATED(message) __declspec(deprecated(message))
#   endif
#endif /* defined(_MSC_VER) */

#ifdef __clang__
#   if __has_extension(attribute_deprecated_with_message)
#       define EDMIDI_DEPRECATED(message) __attribute__((deprecated(message)))
#   endif
#elif defined __GNUC__ /* not clang (gcc comes later since clang emulates gcc) */
#   if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#       define EDMIDI_DEPRECATED(message) __attribute__((deprecated(message)))
#   elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#       define EDMIDI_DEPRECATED(message) __attribute__((__deprecated__))
#   endif /* GNUC version */
#endif /* __clang__ || __GNUC__ */

#if !defined(EDMIDI_DEPRECATED)
#   define EDMIDI_DEPRECATED(message)
#endif /* if !defined(EDMIDI_DEPRECATED) */


#ifdef EDMIDI_BUILD
#   ifndef EDMIDI_DECLSPEC
#       if defined (_WIN32) && defined(EDMIDI_BUILD_DLL)
#           define EDMIDI_DECLSPEC __declspec(dllexport)
#       else
#           define EDMIDI_DECLSPEC
#       endif
#   endif
#else
#   define EDMIDI_DECLSPEC
#endif



/**
 * @brief Library version context
 */
typedef struct {
    EDMIDI_UInt16 major;
    EDMIDI_UInt16 minor;
    EDMIDI_UInt16 patch;
} EDMIDI_Version;


/**
 * @brief Returns string which contains a version number
 * @return String which contains a version of the library
 */
extern EDMIDI_DECLSPEC const char *edmidi_linkedLibraryVersion();

/**
 * @brief Returns structure which contains a version number of library
 * @return Library version context structure which contains version number of the library
 */
extern EDMIDI_DECLSPEC const EDMIDI_Version *edmidi_linkedVersion();



/**
 * @brief Sound output format
 */
enum EDMIDI_SampleType
{
    /*! signed PCM 16-bit */
    EDMIDI_SampleType_S16 = 0,
    /*! signed PCM 8-bit */
    EDMIDI_SampleType_S8,
    /*! float 32-bit */
    EDMIDI_SampleType_F32,
    /*! float 64-bit */
    EDMIDI_SampleType_F64,
    /*! signed PCM 24-bit */
    EDMIDI_SampleType_S24,
    /*! signed PCM 32-bit */
    EDMIDI_SampleType_S32,
    /*! unsigned PCM 8-bit */
    EDMIDI_SampleType_U8,
    /*! unsigned PCM 16-bit */
    EDMIDI_SampleType_U16,
    /*! unsigned PCM 24-bit */
    EDMIDI_SampleType_U24,
    /*! unsigned PCM 32-bit */
    EDMIDI_SampleType_U32,
    /*! Count of available sample format types */
    EDMIDI_SampleType_Count
};

/**
 * @brief Sound output format context
 */
struct EDMIDI_AudioFormat
{
    /*! type of sample */
    enum EDMIDI_SampleType type;
    /*! size in bytes of the storage type */
    unsigned containerSize;
    /*! distance in bytes between consecutive samples */
    unsigned sampleOffset;
};

/**
 * @brief Instance of the library
 */
struct EDMIDIPlayer
{
    /*! Private context descriptor */
    void *edmidiPlayer;
};


/* ======== Error Info ======== */

/**
 * @brief Returns string which contains last error message of initialization
 *
 * Don't use this function to get info on any function except of `edmidi_init`!
 * Use `edmidi_errorInfo()` to get error information while workflow
 *
 * @return String with error message related to library initialization
 */
extern EDMIDI_DECLSPEC const char *edmidi_errorString();

/**
 * @brief Returns string which contains last error message on specific device
 * @param device Instance of the library
 * @return String with error message related to last function call returned non-zero value.
 */
extern EDMIDI_DECLSPEC const char *edmidi_errorInfo(struct EDMIDIPlayer *device);



/* ======== Initialization ======== */

/**
 * @brief Initialize Emu De Midi Player device
 *
 * Tip 1: You can initialize multiple instances and run them in parallel
 * Tip 2: Library is NOT thread-safe, therefore don't use same instance in different threads or use mutexes
 * Tip 3: Changing of sample rate on the fly is not supported. Re-create the instance again.
 * Top 4: To generate output in OPL chip native sample rate, please initialize it with sample rate value as `edmidi_CHIP_SAMPLE_RATE`
 *
 * @param sample_rate Output sample rate
 * @return Instance of the library. If NULL was returned, check the `edmidi_errorString` message for more info.
 */
extern EDMIDI_DECLSPEC struct EDMIDIPlayer *edmidi_init(long sample_rate);


extern EDMIDI_DECLSPEC struct EDMIDIPlayer *edmidi_initEx(long sample_rate, int modules);

/**
 * @brief Close and delete Emu De Midi device
 * @param device Instance of the library
 */
extern EDMIDI_DECLSPEC void edmidi_close(struct EDMIDIPlayer *device);


/* ======== MIDI Sequencer ======== */

/**
 * @brief Load MIDI (or any other supported format) file from File System
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @param filePath Absolute or relative path to the music file. UTF8 encoding is required, even on Windows.
 * @return 0 on success, <0 when any error has occurred
 */
extern EDMIDI_DECLSPEC int edmidi_openFile(struct EDMIDIPlayer *device, const char *filePath);

/**
 * @brief Load MIDI (or any other supported format) file from memory data
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @param mem Pointer to memory block where is raw data of music file is stored
 * @param size Size of given memory block
 * @return 0 on success, <0 when any error has occurred
 */
extern EDMIDI_DECLSPEC int edmidi_openData(struct EDMIDIPlayer *device, const void *mem, unsigned long size);

/**
 * @brief Switch another song if multi-song file is playing (for example, XMI)
 *
 * Note: to set the initial song to load, you should call this function
 * BBEFORE calling `adl_openFile` or `adl_openData`.  When loaded file has more than
 * one built-in songs (Usually XMIformat), it will be started from the selected number.
 * You may call this function to switch another song.
 *
 * @param device Instance of the library
 * @param songNumber Identifier of the track to load (or -1 to mix all tracks as one song)
 * @return
 */
extern EDMIDI_DECLSPEC void edmidi_selectSongNum(struct EDMIDIPlayer *device, int songNumber);

/**
 * @brief Retrive the number of songs in a currently opened file
 * @param device Instance of the library
 * @return Number of songs in the file. If 1 or less, means, the file has only one song inside.
 */
extern EDMIDI_DECLSPEC int edmidi_getSongsCount(struct EDMIDIPlayer *device);

/**
 * @brief Resets MIDI player (per-channel setup) into initial state
 * @param device Instance of the library
 */
extern EDMIDI_DECLSPEC void edmidi_reset(struct EDMIDIPlayer *device);

/**
 * @brief Get total time length of current song
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @return Total song length in seconds
 */
extern EDMIDI_DECLSPEC double edmidi_totalTimeLength(struct EDMIDIPlayer *device);

/**
 * @brief Get loop start time if presented.
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @return Time position in seconds of loop start point, or -1 when file has no loop points
 */
extern EDMIDI_DECLSPEC double edmidi_loopStartTime(struct EDMIDIPlayer *device);

/**
 * @brief Get loop endtime if presented.
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @return Time position in seconds of loop end point, or -1 when file has no loop points
 */
extern EDMIDI_DECLSPEC double edmidi_loopEndTime(struct EDMIDIPlayer *device);

/**
 * @brief Get current time position in seconds
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @return Current time position in seconds
 */
extern EDMIDI_DECLSPEC double edmidi_positionTell(struct EDMIDIPlayer *device);

/**
 * @brief Jump to absolute time position in seconds
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @param seconds Destination time position in seconds to seek
 */
extern EDMIDI_DECLSPEC void edmidi_positionSeek(struct EDMIDIPlayer *device, double seconds);

/**
 * @brief Reset MIDI track position to begin
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 */
extern EDMIDI_DECLSPEC void edmidi_positionRewind(struct EDMIDIPlayer *device);

/**
 * @brief Set tempo multiplier
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @param tempo Tempo multiplier value: 1.0 - original tempo, >1 - play faster, <1 - play slower
 */
extern EDMIDI_DECLSPEC void edmidi_setTempo(struct EDMIDIPlayer *device, double tempo);

/**
 * @brief Returns 1 if music position has reached end
 * @param device Instance of the library
 * @return 1 when end of sing has been reached, otherwise, 0 will be returned. <0 is returned on any error
 */
extern EDMIDI_DECLSPEC int edmidi_atEnd(struct EDMIDIPlayer *device);

/**
 * @brief Returns the number of tracks of the current sequence
 * @param device Instance of the library
 * @return Count of tracks in the current sequence
 */
extern EDMIDI_DECLSPEC size_t edmidi_trackCount(struct EDMIDIPlayer *device);

/**
 * @brief Sets the channel of the current sequence enable state
 * @param device Instance of the library
 * @param channelNumber Number of the channel (from 0 to 15)
 * @param enabled 1 to enable and 0 to disable
 * @return 0 on success, <0 when any error has occurred
 */
extern EDMIDI_DECLSPEC int edmidi_setTrackEnabled(struct EDMIDIPlayer *device, size_t trackNumber, int enabled);

/**
 * @brief Sets the channel of the current sequence enable state
 * @param device Instance of the library
 * @param channelNumber Number of the channel (from 0 to 15)
 * @param enabled 1 to enable and 0 to disable
 * @return 0 on success, <0 when any error has occurred
 */
extern EDMIDI_DECLSPEC int edmidi_setChannelEnabled(struct EDMIDIPlayer *device, size_t channelNumber, int enabled);

/**
 * @brief Handler of callback trigger events
 * @param userData Pointer to user data (usually, context of something)
 * @param trigger Value of the event which triggered this callback.
 * @param track Identifier of the track which triggered this callback.
 */
typedef void (*EDMIDI_TriggerHandler)(void *userData, unsigned trigger, size_t track);

/**
 * @brief Defines a handler for callback trigger events
 * @param device Instance of the library
 * @param handler Handler to invoke from the sequencer when triggered, or NULL.
 * @param userData Instance of the library
 * @return 0 on success, <0 when any error has occurred
 */
extern EDMIDI_DECLSPEC int edmidi_setTriggerHandler(struct EDMIDIPlayer *device, EDMIDI_TriggerHandler handler, void *userData);




/* ======== Meta-Tags ======== */

/**
 * @brief Returns string which contains a music title
 * @param device Instance of the library
 * @return A string that contains music title
 */
extern EDMIDI_DECLSPEC const char *edmidi_metaMusicTitle(struct EDMIDIPlayer *device);

/**
 * @brief Returns string which contains a copyright string*
 * @param device Instance of the library
 * @return A string that contains copyright notice, otherwise NULL
 */
extern EDMIDI_DECLSPEC const char *edmidi_metaMusicCopyright(struct EDMIDIPlayer *device);

/**
 * @brief Returns count of available track titles
 *
 * NOTE: There are CAN'T be associated with channel in any of event or note hooks
 *
 * @param device Instance of the library
 * @return Count of available MIDI tracks, otherwise NULL
 */
extern EDMIDI_DECLSPEC size_t edmidi_metaTrackTitleCount(struct EDMIDIPlayer *device);

/**
 * @brief Get track title by index
 * @param device Instance of the library
 * @param index Index of the track to retreive the title
 * @return A string that contains track title, otherwise NULL.
 */
extern EDMIDI_DECLSPEC const char *edmidi_metaTrackTitle(struct EDMIDIPlayer *device, size_t index);

/**
 * @brief MIDI Marker structure
 */
struct EdMidi_MarkerEntry
{
    /*! MIDI Marker title */
    const char      *label;
    /*! Absolute time position of the marker in seconds */
    double          pos_time;
    /*! Absolute time position of the marker in MIDI ticks */
    unsigned long   pos_ticks;
};

/**
 * @brief Returns count of available markers
 * @param device Instance of the library
 * @return Count of available MIDI markers
 */
extern EDMIDI_DECLSPEC size_t edmidi_metaMarkerCount(struct EDMIDIPlayer *device);

/**
 * @brief Returns the marker entry
 * @param device Instance of the library
 * @param index Index of the marker to retreive it.
 * @return MIDI Marker description structure.
 */
extern EDMIDI_DECLSPEC struct EdMidi_MarkerEntry edmidi_metaMarker(struct EDMIDIPlayer *device, size_t index);


/* ======== Audio output Generation ======== */

/**
 * @brief Generate PCM signed 16-bit stereo audio output and iterate MIDI timers
 *
 * Use this function when you are playing MIDI file loaded by `edmidi_openFile` or by `edmidi_openData`
 * with using of built-in MIDI sequencer.
 *
 * Don't use count of frames, use instead count of samples. One frame is two samples.
 * So, for example, if you want to take 10 frames, you must to request amount of 20 samples!
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @param sampleCount Count of samples (not frames!)
 * @param out Pointer to output with 16-bit stereo PCM output
 * @return Count of given samples, otherwise, 0 or when catching an error while playing
 */
extern EDMIDI_DECLSPEC int  edmidi_play(struct EDMIDIPlayer *device, int sampleCount, short *out);
extern EDMIDI_DECLSPEC int  edmidi_playF32(struct EDMIDIPlayer *device, int sampleCount, float *out);

/**
 * @brief Generate PCM stereo audio output in sample format declared by given context and iterate MIDI timers
 *
 * Use this function when you are playing MIDI file loaded by `adl_openFile` or by `adl_openData`
 * with using of built-in MIDI sequencer.
 *
 * Don't use count of frames, use instead count of samples. One frame is two samples.
 * So, for example, if you want to take 10 frames, you must to request amount of 20 samples!
 *
 * Available when library is built with built-in MIDI Sequencer support.
 *
 * @param device Instance of the library
 * @param sampleCount Count of samples (not frames!)
 * @param left Left channel buffer output (Must be casted into bytes array)
 * @param right Right channel buffer output (Must be casted into bytes array)
 * @param format Destination PCM format format context
 * @return Count of given samples, otherwise, 0 or when catching an error while playing
 */
extern EDMIDI_DECLSPEC int  edmidi_playFormat(struct EDMIDIPlayer *device, int sampleCount, EDMIDI_UInt8 *left, EDMIDI_UInt8 *right, const struct EDMIDI_AudioFormat *format);



/* ======== Hooks and debugging ======== */

/**
 * @brief Debug messages callback
 * @param userdata Pointer to user data (usually, context of someting)
 * @param fmt Format strign output (in context of `printf()` standard function)
 */
typedef void (*EDMIDI_DebugMessageHook)(void *userdata, const char *fmt, ...);

/**
 * @brief Loop start/end point reach hook
 * @param userdata Pointer to user data (usually, context of someting)
 */
typedef void (*EDMIDI_LoopPointHook)(void *userdata);

/**
 * @brief Set debug message hook
 *
 * CAUTION: Don't call any libEDMIDI API functions from off this hook directly!
 * Suggestion: Use boolean variables to mark the fact this hook got been called, and then,
 * apply your action outside of this hook, for example, in the next after audio output call.
 *
 * @param device Instance of the library
 * @param debugMessageHook Pointer to the callback function which will be called on every debug message
 * @param userData Pointer to user data which will be passed through the callback.
 */
extern EDMIDI_DECLSPEC void edmidi_setDebugMessageHook(struct EDMIDIPlayer *device, EDMIDI_DebugMessageHook debugMessageHook, void *userData);

/**
 * @brief Set the look start point hook
 *
 * CAUTION: Don't call any libEDMIDI API functions from off this hook directly!
 * Suggestion: Use boolean variables to mark the fact this hook got been called, and then,
 * apply your action outside of this hook, for example, in the next after audio output call.
 *
 * @param device Instance of the library
 * @param loopStartHook Pointer to the callback function which will be called on every loop start point passing
 * @param userData Pointer to user data which will be passed through the callback.
 */
extern EDMIDI_DECLSPEC void edmidi_setLoopStartHook(struct EDMIDIPlayer *device, EDMIDI_LoopPointHook loopStartHook, void *userData);

/**
 * @brief Set the look start point hook
 *
 * CAUTION: Don't call any libEDMIDI API functions from off this hook directly!
 * Suggestion: Use boolean variables to mark the fact this hook got been called, and then,
 * apply your action outside of this hook, for example, in the next after audio output call.
 *
 * If you want to switch the song after calling this hook, suggested to call the function
 * adl_setLoopHooksOnly(device, 1) to immediately stop the song on reaching the loop point
 *
 * @param device Instance of the library
 * @param loopStartHook Pointer to the callback function which will be called on every loop start point passing
 * @param userData Pointer to user data which will be passed through the callback.
 */
extern EDMIDI_DECLSPEC void edmidi_setLoopEndHook(struct EDMIDIPlayer *device, EDMIDI_LoopPointHook loopEndHook, void *userData);




/* ======== Setup ======== */

/**
 * @brief Enable or disable built-in loop (built-in loop supports 'loopStart' and 'loopEnd' tags to loop specific part)
 * @param device Instance of the library
 * @param loopEn 0 - disabled, 1 - enabled
 */
extern EDMIDI_DECLSPEC void edmidi_setLoopEnabled(struct EDMIDIPlayer *device, int loopEn);

/**
 * @brief Set how many times loop will be played
 *
 * Note: The song will be played once if loop has been disabled with no matter which value of loop count was set
 *
 * @param device Instance of the library
 * @param loopCount Number of loops or -1 to loop infinitely
 */
extern EDMIDI_DECLSPEC void edmidi_setLoopCount(struct EDMIDIPlayer *device, int loopCount);

/**
 * @brief Make song immediately stop on reaching a loop end point
 * @param device Instance of the library
 * @param loopHooksOnly 0 - disabled, 1 - enabled
 */
extern EDMIDI_DECLSPEC void edmidi_setLoopHooksOnly(struct EDMIDIPlayer *device, int loopHooksOnly);

#ifdef __cplusplus
}
#endif

#endif /* EMUDEMIDI_H */
