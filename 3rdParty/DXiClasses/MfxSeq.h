// MfxSeq.h: interface for the CMfxSeq class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXSEQ_H__AC18B35A_0809_4C80_BB8B_328A7DFEEE6A__INCLUDED_)
#define AFX_MFXSEQ_H__AC18B35A_0809_4C80_BB8B_328A7DFEEE6A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MfxEvent.h"
#include "MfxTrack.h"
#include "MfxMarker.h"
#include "MfxMeterKeySigMap.h"
#include "MfxTempoMap.h"

//static const UINT SAMPLE_RATE = 44100;

class CMfxSeq  
{
public:
	CMfxSeq();
	virtual ~CMfxSeq();

	map<int,CMfxTrack>::iterator GetBeginTrack() { return m_tracks.begin(); }
	map<int,CMfxTrack>::iterator GetEndTrack() { return m_tracks.end(); }

	CMfxTrack* GetTrack( int nTrkNum, BOOL bCreate = FALSE );

	void SetPosition( LONG lTicks );

	void SetPPQ( int nPPQ );
	int GetPPQ() const { return m_nPPQ; }

	LONG UTick2Tick( LONGLONG llUTick ) const
	{
		double const dTick = static_cast<double>( llUTick ) / UTICK_UNITS;
		return static_cast<LONG>( dTick * GetPPQ() );
	}

	LONGLONG Tick2UTick( LONG lTick ) const
	{
		double const dTick = static_cast<double>( lTick ) / GetPPQ();
		return static_cast<LONGLONG>( dTick * UTICK_UNITS );
	}

public:

	vector<CMarker>		m_markers;
	CMfxMeterKeySigMap	m_meterKeySigMap;
	CMfxTempoMap			m_tempoMap;

private:

	map<int,CMfxTrack>	m_tracks;
	int						m_nPPQ;
};

#endif // !defined(AFX_MFXSEQ_H__AC18B35A_0809_4C80_BB8B_328A7DFEEE6A__INCLUDED_)
