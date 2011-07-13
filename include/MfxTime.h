#ifndef _MFXTIME_H_
#define _MFXTIME_H_
#pragma once

// MfxTime.h : Declarations of time formats for use by MFX, DXi, etc.
// Copyright (c) 2001- Twelve Tone Systems, Inc.  All rights reserved.

typedef enum MFX_TIME_FORMAT
{
	TF_NULL,				// no time value (used to clear a time value)
	TF_SECONDS,			// time in absolute seconds from song top (double)
	TF_SAMPLES,			// time in absolute samples from song top (LONGLONG)
	TF_TICKS,			// time in MIDI ticks @ 960 ppqn, from song top (LONGLONG)
	TF_UTICKS,			// time in hi-res MIDI ticks @ 960*(2^16) ppqn, from song top (LONGLONG)
	TF_MBT,				// time in measure:beat:ticks, from song top
	TF_FRAMES,			// time in SMPTE frames, including the projects SMPTE offset
	TF_FRAMES_REL,		// time in SMPTE frames, relative to song top (no offset applied)
	TF_SMPTE,			// time in SMPTE HMSF format, including the projects SMPTE offset
	TF_SMPTE_REL		// time in SMPTE HMSF format, relative to song top (no offset applied)
}
MFX_TIME_FORMAT;

typedef enum MFX_FPS
{
	FPS_24,				// 24 fps
	FPS_25,				// 25 fps
	FPS_2997,			// 29.97 fps
	FPS_2997_DROP,		// 29.97 fps, drop-frame
	FPS_30,				// 30 fps
	FPS_30_DROP			// 30 fps, drop-frame
}
MFX_FPS;

typedef struct MFX_TIME
{
	MFX_TIME_FORMAT	timeFormat;
	union
	{
		double		dSeconds;
		LONGLONG		llSamples;
		LONG			lTicks;
		LONGLONG		llUTicks;
		struct
		{
			int		nMeas;
			short		nBeat;
			short		nTick;
		}
		mbt;
		struct
		{
			short			fps;		// uses values from the enum MFX_FPS
			short			nSub400;	// subframe in 400ths of a frame
			char			nFrame;
			char			nSec;
			char			nMin;
			char			nHour;
		}
		smpte;
		struct
		{
			short		fps;
			LONG		lFrame;
		}
		frames;
	};
}
MFX_TIME;

#endif //_MFXTIME_H_
