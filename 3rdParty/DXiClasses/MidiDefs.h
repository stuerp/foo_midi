// MidiDefs.h: various MIDI related constants and macros.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _MIDIDEFS_H_
#define _MIDIDEFS_H_
#pragma once

/////////////////////////////////////////////////////////////////////////////
// Useful typedefs and macros

typedef unsigned __int64 QWORD;

/////////////////////////////////////////////////////////////////////////////
// MIDI constants and utility functions

// MIDI Status bytes

const BYTE	NOTE_OFF	= 0x80;	// note-off
const BYTE	NOTE_ON	= 0x90;	// note-on
const BYTE	KEYAFT	= 0xA0;	// polyphonic key aftertouch
const BYTE	CONTROL	= 0xB0;	// control change
const BYTE	PROGRAM	= 0xC0;	// program change
const BYTE	PATCH		= 0xC0;	// patch change (synonym)
const BYTE	CHANAFT	= 0xD0;	// monophonic channel aftertouch
const BYTE	WHEEL		= 0xE0;	// pitch wheel
const BYTE	SYSX		= 0xF0;	// system exclusive
const BYTE	MTC		= 0xF1;	// MTC quarter-frame
const BYTE	SONGPP	= 0xF2;	// song position pointer
const BYTE	SONGSEL	= 0xF3;	// select song number
const BYTE	TUNE		= 0xF6;	// tune request
const BYTE	EOX		= 0xF7;	// end of system exclusive
const BYTE	CLOCK		= 0xF8;	// Realtime: Clock
const BYTE	START		= 0xFA;	// Realtime: Start
const BYTE	CONTINUE	= 0xFB;	// Realtime: Continue
const BYTE	STOP		= 0xFC;	// Realtime: Stop
const BYTE	ACTIVE	= 0xFE;	// active sensing
const BYTE	RESET		= 0xFF;

inline BYTE KIND( BYTE by )
{ 
	return (BYTE)(by & (BYTE)0xF0);
}
inline BYTE CHAN( BYTE by )
{
	return (BYTE)(by & (BYTE)0x0F);
}

// Some common controller numbers
const BYTE	CTL_MODULATION	= 1;	// modulation wheel
const BYTE	CTL_VOLUME		= 7;	// volume
const BYTE	CTL_PAN			= 10;	// pan
const BYTE	CTL_SUSTAIN		= 64;	// sustain pedal
const BYTE	CTL_RESETALL	= 0x79;	// "reset all continuous controllers"

// Pitch wheel value constants.  Represented in MIDI in two 7-bit
// bytes, for a combined 14-bit unsigned value with the wheel
// range 0..16383, center = 8192.  Constants are also defined for
// the conceptually clearer signed equivalent, -8192..+8191, with
// the center = 0.  Signed rep = unsigned rep - 8192.

const int	WHEEL_MAX			= 16383;	// (2 << 14) - 1
const int	WHEEL_CENTER		=  8192;	// 0x2000 or (2 <<14) / 2
const int	WHEEL_MIN			=     0;
const int	SIGNED_WHEEL_MAX	=  8191;
const int	SIGNED_WHEEL_MIN	= -8192;

inline WORD BYTES_WORD14( BYTE byLSB, BYTE byMSB )
{
	return (WORD) ((((WORD)byMSB) << 7) | (WORD)byLSB);
}

inline void WORD14_BYTES( WORD w, BYTE* pbyLSB, BYTE* pbyMSB )
{
	*pbyLSB = (BYTE)(w & 0x007F);
	*pbyMSB = (BYTE)((w & 0x3F80) >> 7);
}

const int BANK_NONE = -1;
const int MIN_BANK = -1;
const int MAX_BANK = 16383;

const int PATCH_NONE = -1;
const int MIN_PATCH = 0;
const int MAX_PATCH = 127;

const int CHAN_NONE = -1;
const int MIN_CHAN = 0;
const int MAX_CHAN = 15;

const int MAX_SYSXBANK = 8192;

enum BankSelMethod
{
	Normal, Ctrl0, Ctrl32, Patch100, MAX_BANKSELMETHOD
};

static const DWORD UTICK_PPQ = 960;
static const DWORD UTICK_SHIFT = 16;
static const DWORD UTICK_UNITS = (UTICK_PPQ << UTICK_SHIFT);
static const DWORD UTICK_ROUND = (1 << (UTICK_SHIFT-1));

////////////////////////////////////////////////////////////////////////////////

#endif //_MIDIDEFS_H_