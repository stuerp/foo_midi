// MfxSeq.cpp: implementation of the CMfxSeq class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxSeq.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////

CMfxSeq::CMfxSeq()
{
	SetPPQ( 120 );

	// Always construct with 1 empty track, for playing through live
	CMfxTrack trk;
	trk.SetName( "[Play Through]" );
	trk.SetChannel( 0 );
	trk.SetMfxChannel( -1 );
	m_tracks[ -1 ] = trk;
}

CMfxSeq::~CMfxSeq()
{

}
/////////////////////////////////////////////////////////////////////////////

void CMfxSeq::SetPPQ( int nPPQ )
{
	m_nPPQ = nPPQ;
	m_meterKeySigMap.SetPPQ( nPPQ );
	m_tempoMap.SetPPQ( nPPQ );
}

/////////////////////////////////////////////////////////////////////////////

CMfxTrack* CMfxSeq::GetTrack( int nTrkNum, BOOL bCreate /* = FALSE */ )
{
	if (!bCreate && m_tracks.end() == m_tracks.find( nTrkNum ))
		return NULL;
	else
	{
		CMfxTrack& trk = m_tracks[ nTrkNum ];
		trk.SetMfxChannel( nTrkNum );
		return &trk;
	}
}

/////////////////////////////////////////////////////////////////////////////

void CMfxSeq::SetPosition( LONG lTicks )
{
	map<int,CMfxTrack>::iterator it;
	for (it = m_tracks.begin(); it != m_tracks.end(); it++)
	{
		CMfxTrack& trk = it->second;
		if (!trk.empty())
		{
			int ix = trk.IndexForTimeGE( lTicks );
			while (ix > 0 && trk[ix].GetTime() == lTicks)
				--ix;
			trk.SetPlayIndex( ix );
		}
	}
}

