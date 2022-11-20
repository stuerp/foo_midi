// MfxTrack.cpp: implementation of the CMfxTrack class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxTrack.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////

CMfxTrack::CMfxTrack()
{
	m_mfxChannel = 0;
	m_nPort = 0;
	m_nChan = -1;
	m_nBank = BANK_NONE;
	m_nPatch = PATCH_NONE;
	m_byBankSelMethod = Normal;
	m_szName[0] = 0;
	m_mute = 0;
	m_nKeyOfs = 0;
	m_nVelOfs = 0;
	m_ixPlay = 0;
}

void CMfxTrack::copy( const CMfxTrack& rhs )
{
	m_mfxChannel = rhs.m_mfxChannel;
	m_nPort = rhs.m_nPort;
	m_nChan = rhs.m_nChan;
	m_nBank = rhs.m_nBank;
	m_nPatch = rhs.m_nPatch;
	m_byBankSelMethod = rhs.m_byBankSelMethod;
	strcpy( m_szName, rhs.m_szName );
	m_mute = rhs.m_mute;
	m_nKeyOfs = rhs.m_nKeyOfs;
	m_nVelOfs = rhs.m_nVelOfs;
	m_ixPlay = rhs.m_ixPlay;
}

CMfxTrack::CMfxTrack( const CMfxTrack& rhs )
{
	copy( rhs );
}

CMfxTrack& CMfxTrack::operator=( const CMfxTrack& rhs )
{
	if (this != &rhs)
		copy( rhs );
	return *this;
}

CMfxTrack::~CMfxTrack()
{
}

/////////////////////////////////////////////////////////////////////

int CMfxTrack::IndexForTimeGE( LONG lTime ) const
{
	if (empty())
		return -1; // nothing in the track

	GetTime<CMfxEvent> getFn;
	return bsearch_for_value( *this, lTime, getFn );
}

//////////////////////////////////////////////////////////////////////

void CMfxTrack::ApplyTrackProperties( DWORD* pdwShortMsg )
{
	BYTE* pbyData = (BYTE*) pdwShortMsg;

	// Apply forced channel
	if (-1 != GetChannel() && ((pbyData[0] & 0xF0) != 0xF0))
		pbyData[0] = (pbyData[0] & 0xF0) | GetChannel();

	if (NOTE_ON == (pbyData[0] & 0xF0) ||
		 NOTE_OFF == (pbyData[0] & 0xF0))
	{
		int nKey = GetKeyOfs() + (pbyData[1] & 0x7F);
		int nVel = GetVelOfs() + (pbyData[2] & 0x7F);
		nKey = max( 0, min( 127, nKey ) );
		nVel = max( 0, min( 127, nVel ) );
		pbyData[1] = nKey;
		pbyData[2] = nVel;
	}
}

//////////////////////////////////////////////////////////////////////
