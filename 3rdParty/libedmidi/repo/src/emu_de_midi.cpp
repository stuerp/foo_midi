/*
 * Copyright (с) 2004 Mitsutaka Okazaki
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

#include "../include/emu_de_midi.h"
#include "CSMFPlay.hpp"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


// Setup compiler defines useful for exporting required public API symbols in gme.cpp
#ifndef EDMIDI_EXPORT
#   if defined (_WIN32) && defined(EDMIDI_BUILD_DLL)
#       define EDMIDI_EXPORT __declspec(dllexport)
#   elif defined (LIBEDMIDI_VISIBILITY) && defined (__GNUC__)
#       define EDMIDI_EXPORT __attribute__((visibility ("default")))
#   else
#       define EDMIDI_EXPORT
#   endif
#endif

/* Unify MIDI player casting and interface between ADLMIDI and OPNMIDI */
#define GET_MIDI_PLAYER(device) reinterpret_cast<dsa::CSMFPlay *>((device)->edmidiPlayer)
typedef dsa::CSMFPlay MidiPlayer;

static char EDMIDI_ErrorString[2048] = {0};
static EDMIDI_Version edmidi_version = {
    EDMIDI_VERSION_MAJOR,
    EDMIDI_VERSION_MINOR,
    EDMIDI_VERSION_PATCHLEVEL
};

EDMIDI_EXPORT const char *edmidi_linkedLibraryVersion()
{
    return EDMIDI_VERSION;
}

EDMIDI_EXPORT const EDMIDI_Version *edmidi_linkedVersion()
{
    return &edmidi_version;
}

EDMIDI_EXPORT const char *edmidi_errorString()
{
    return EDMIDI_ErrorString;
}

EDMIDI_EXPORT const char *edmidi_errorInfo(struct EDMIDIPlayer *device)
{
    if(!device)
        return edmidi_errorString();
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    if(!play)
        return edmidi_errorString();
    return play->getErrorString().c_str();
}

EDMIDI_EXPORT struct EDMIDIPlayer *edmidi_init(long sample_rate)
{
    return edmidi_initEx(sample_rate, 8);
}

EDMIDI_EXPORT EDMIDIPlayer *edmidi_initEx(long sample_rate, int modules)
{
    EDMIDIPlayer *midi_device;
    memset(EDMIDI_ErrorString, 0, sizeof(EDMIDI_ErrorString));

    if(modules < 2)
    {
        sprintf(EDMIDI_ErrorString, "Can't initialize Emu De MIDI: modules less than minimum allowed (2)!");
        return NULL;
    }

    if(modules & 1)
    {
        sprintf(EDMIDI_ErrorString, "Can't initialize Emu De MIDI: modules number must be an even number!");
        return NULL;
    }

    midi_device = (EDMIDIPlayer *)malloc(sizeof(EDMIDIPlayer));
    if(!midi_device)
    {
        sprintf(EDMIDI_ErrorString, "Can't initialize Emu De MIDI: out of memory!");
        return NULL;
    }

    MidiPlayer *player = new(std::nothrow) MidiPlayer(static_cast<unsigned long>(sample_rate), modules);
    if(!player)
    {
        free(midi_device);
        sprintf(EDMIDI_ErrorString, "Can't initialize Emu De MIDI: out of memory!");
        return NULL;
    }

    midi_device->edmidiPlayer = player;

    return midi_device;
}

EDMIDI_EXPORT void edmidi_close(struct EDMIDIPlayer *device)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    delete play;
    device->edmidiPlayer = NULL;
    free(device);
    device = NULL;
}

EDMIDI_EXPORT int edmidi_openFile(struct EDMIDIPlayer *device, const char *filePath)
{
    if(device)
    {
        MidiPlayer *play = GET_MIDI_PLAYER(device);
        assert(play);
        if(!play->Open(filePath))
        {
            std::string err = play->getErrorString();
            if(err.empty())
                play->setErrorString("Emu De MIDI: Can't load file");
            return -1;
        }
        else return 0;
    }

    sprintf(EDMIDI_ErrorString, "Can't load file: Emu De MIDI is not initialized");
    return -1;
}

EDMIDI_EXPORT int edmidi_openData(struct EDMIDIPlayer *device, const void *mem, unsigned long size)
{
    if(device)
    {
        MidiPlayer *play = GET_MIDI_PLAYER(device);
        assert(play);
        if(!play->Load(mem, static_cast<int>(size)))
        {
            std::string err = play->getErrorString();
            if(err.empty())
                play->setErrorString("Emu De MIDI: Can't load data from memory");
            return -1;
        }
        else return 0;
    }

    sprintf(EDMIDI_ErrorString, "Can't load file: Emu De MIDI is not initialized");
    return -1;
}

EDMIDI_EXPORT void edmidi_selectSongNum(struct EDMIDIPlayer *device, int songNumber)
{
    if(!device)
        return;

    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->setSongNum(songNumber);
}

EDMIDI_EXPORT int edmidi_getSongsCount(struct EDMIDIPlayer *device)
{
    if(!device)
        return 0;

    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->getSongsCount();
}

EDMIDI_EXPORT void edmidi_reset(struct EDMIDIPlayer *device)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->Reset();
}

EDMIDI_EXPORT double edmidi_totalTimeLength(struct EDMIDIPlayer *device)
{
    if(!device)
        return -1.0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->Duration();
}

EDMIDI_EXPORT double edmidi_loopStartTime(struct EDMIDIPlayer *device)
{
    if(!device)
        return -1.0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->loopStart();
}

EDMIDI_EXPORT double edmidi_loopEndTime(struct EDMIDIPlayer *device)
{
    if(!device)
        return -1.0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->loopEnd();
}

EDMIDI_EXPORT double edmidi_positionTell(struct EDMIDIPlayer *device)
{
    if(!device)
        return -1.0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->Tell();
}

EDMIDI_EXPORT void edmidi_positionSeek(struct EDMIDIPlayer *device, double seconds)
{
    if(seconds < 0.0)
        return;//Seeking negative position is forbidden! :-P
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->Panic();
    play->Seek(seconds);
}

EDMIDI_EXPORT void edmidi_positionRewind(struct EDMIDIPlayer *device)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->Panic();
    play->Rewind();
}

EDMIDI_EXPORT void edmidi_setTempo(struct EDMIDIPlayer *device, double tempo)
{
    if(!device || (tempo <= 0.0))
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->setTempo(tempo);
}

EDMIDI_EXPORT int edmidi_atEnd(struct EDMIDIPlayer *device)
{
    if(!device)
        return 1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return (int)play->SeqEof();
}

EDMIDI_EXPORT size_t edmidi_trackCount(struct EDMIDIPlayer *device)
{
    if(!device)
        return 0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->tracksCount();
}

EDMIDI_EXPORT int edmidi_setTrackEnabled(struct EDMIDIPlayer *device, size_t trackNumber, int enabled)
{
    if(!device)
        return -1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    if(!play->setTrackEnabled(trackNumber, (bool)enabled))
        return -1;
    return 0;
}

EDMIDI_EXPORT int edmidi_setChannelEnabled(struct EDMIDIPlayer *device, size_t channelNumber, int enabled)
{
    if(!device)
        return -1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    if(!play->setChannelEnabled(channelNumber, (bool)enabled))
        return -1;
    return 0;
}

EDMIDI_EXPORT int edmidi_setTriggerHandler(struct EDMIDIPlayer *device, EDMIDI_TriggerHandler handler, void *userData)
{
    if(!device)
        return -1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->setTriggerHandler(handler, userData);
    return 0;
}

EDMIDI_EXPORT const char *edmidi_metaMusicTitle(struct EDMIDIPlayer *device)
{
    if(!device)
        return "";
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->getMusicTitle().c_str();
}

EDMIDI_EXPORT const char *edmidi_metaMusicCopyright(struct EDMIDIPlayer *device)
{
    if(!device)
        return "";
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->getMusicCopyright().c_str();
}

EDMIDI_EXPORT size_t edmidi_metaTrackTitleCount(struct EDMIDIPlayer *device)
{
    if(!device)
        return 0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->getTrackTitles().size();
}

EDMIDI_EXPORT const char *edmidi_metaTrackTitle(struct EDMIDIPlayer *device, size_t index)
{
    if(!device)
        return "";
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    const std::vector<std::string> &titles = play->getTrackTitles();
    if(index >= titles.size())
        return "INVALID";
    return titles[index].c_str();
}

EDMIDI_EXPORT size_t edmidi_metaMarkerCount(struct EDMIDIPlayer *device)
{
    if(!device)
        return 0;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->getMarkersCount();
}

EDMIDI_EXPORT EdMidi_MarkerEntry edmidi_metaMarker(struct EDMIDIPlayer *device, size_t index)
{
    struct EdMidi_MarkerEntry marker;
    if(!device)
    {
        marker.label = "INVALID";
        marker.pos_time = 0.0;
        marker.pos_ticks = 0;
        return marker;
    }

    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);

    return play->getMarker(index);
}

EDMIDI_EXPORT int edmidi_play(struct EDMIDIPlayer *device, int sampleCount, short *out)
{
    if(!device)
        return -1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->RenderS16(out, sampleCount >> 1);
}

EDMIDI_EXPORT int edmidi_playF32(EDMIDIPlayer *device, int sampleCount, float *out)
{
    if(!device)
        return -1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->RenderF32(out, sampleCount >> 1);
}

EDMIDI_EXPORT int edmidi_playFormat(EDMIDIPlayer *device, int sampleCount,
                                 EDMIDI_UInt8 *left, EDMIDI_UInt8 *right,
                                 const EDMIDI_AudioFormat *format)
{
    if(!device)
        return -1;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    return play->RenderFormat(sampleCount, left, right, format);
}

EDMIDI_EXPORT void edmidi_setDebugMessageHook(struct EDMIDIPlayer *device, EDMIDI_DebugMessageHook debugMessageHook, void *userData)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->setDebugMessageHook(debugMessageHook, userData);
}

/* Set loop start hook */
EDMIDI_EXPORT void adl_setLoopStartHook(struct EDMIDIPlayer *device, EDMIDI_LoopPointHook loopStartHook, void *userData)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->adl_setLoopStartHook(loopStartHook, userData);
}

/* Set loop end hook */
EDMIDI_EXPORT void adl_setLoopEndHook(struct EDMIDIPlayer *device, EDMIDI_LoopPointHook loopEndHook, void *userData)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->adl_setLoopEndHook(loopEndHook, userData);
}

EDMIDI_EXPORT void edmidi_setLoopEnabled(EDMIDIPlayer *device, int loopEn)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->SetLoop(loopEn != 0);
}

EDMIDI_EXPORT void edmidi_setLoopCount(EDMIDIPlayer *device, int loopCount)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->SetLoopsNumber(loopCount);
}

EDMIDI_EXPORT void adl_setLoopHooksOnly(EDMIDIPlayer *device, int loopHooksOnly)
{
    if(!device)
        return;
    MidiPlayer *play = GET_MIDI_PLAYER(device);
    assert(play);
    play->setLoopHooksOnly(loopHooksOnly);
}
