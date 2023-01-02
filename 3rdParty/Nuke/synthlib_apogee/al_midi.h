/*
Copyright (C) 1994-1995 Apogee Software, Ltd.
Copyright (C) 2013-2016 Alexey Khokholov.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __AL_MIDI_H
#define __AL_MIDI_H

#include "_al_midi.h"
#include "..\interface.h"

enum AL_Errors
   {
   AL_Warning  = -2,
   AL_Error    = -1,
   AL_Ok       = 0,
   };
#ifndef max
#define max(a,b) ((a>b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a<b)?(a):(b))
#endif

#define AL_MaxVolume             127
#define AL_DefaultChannelVolume  90
//#define AL_DefaultPitchBendRange 2
#define AL_DefaultPitchBendRange 200

class ApogeeOPL : public nomidisynth
{
private:
    bool opl_extp;
    fm_chip* chip;
    TIMBRE ADLIB_TimbreBank[ 256 ];
    unsigned NoteMod12[ MAX_NOTE + 1 ];
    unsigned NoteDiv12[ MAX_NOTE + 1 ];
    int VoiceLevel[ NumChipSlots ][ 2 ];
    int VoiceKsl[ NumChipSlots ][ 2 ];
    int VoiceReserved[ NUM_VOICES * 2 ];
    VOICE     Voice[ NUM_VOICES * 2 ];
    VOICELIST Voice_Pool;
    CHANNEL   Channel[ NUM_CHANNELS ];
    void AL_SendOutputToPort( int port, int reg, int data );
	void AL_SendOutputToPanPort( int port, int reg, int data );
    void AL_SendOutput( int  voice, int reg, int data );
    void AL_SetVoiceTimbre( int voice );
    void AL_SetVoiceVolume( int voice );
    void AL_SetVoicePan( int voice );
    int  AL_AllocVoice( void );
    int  AL_GetVoice( int channel, int key );
    void AL_SetVoicePitch( int voice );
    void AL_SetChannelVolume( int channel, int volume );
    void AL_SetChannelPan( int channel, int pan );
    void AL_SetChannelDetune( int channel, int detune );
    void AL_ResetVoices( void );
    void AL_CalcPitchInfo( void );
    void AL_FlushCard ( int port );
    void AL_StereoOn( void );
    void AL_StereoOff( void );
    int  AL_ReserveVoice( int voice );
    int  AL_ReleaseVoice( int voice );
    void AL_SetMaxMidiChannel( int channel );
    void AL_Reset( void );
    void AL_NoteOff( int channel, int key, int velocity );
    void AL_NoteOn( int channel, int key, int vel );
    void AL_AllNotesOff( int channel );
    void AL_ControlChange( int channel, int type, int data );
    void AL_ProgramChange( int channel, int patch );
    void AL_SetPitchBend( int channel, int lsb, int msb );
    void AL_RegisterTimbreBank( const unsigned char *timbres );
public:
    const char *midi_synth_name(void);
    unsigned int midi_bank_count(void);
    const char * midi_bank_name(unsigned int bank);
    int midi_init(unsigned int rate, unsigned int bank, unsigned int extp);
    void midi_write(unsigned int data);
    void midi_generate(signed short *buffer, unsigned int length);
};

void AL_RemoveNode ( char *item, char **head, char **tail, int next, int prev );
void AL_AddNode ( char *item, char **head, char **tail, int next, int prev );
#endif
