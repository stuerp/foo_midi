// MfxTrack.h: interface for the CMfxTrack class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXTRACK_H__DC0A32AB_959C_47D3_8AEA_823F7C44A1D6__INCLUDED_)
#define AFX_MFXTRACK_H__DC0A32AB_959C_47D3_8AEA_823F7C44A1D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MfxEvent.h"

////////////////////////////////////////////////////////////////////////////////

class CMfxTrack : public vector<CMfxEvent>
{
public:

	CMfxTrack();
	CMfxTrack( const CMfxTrack& );
	CMfxTrack& operator=( const CMfxTrack& );
	virtual ~CMfxTrack();

	MFX_CHANNEL GetMfxChannel() const
	{
		return m_mfxChannel;
	}

	void SetMfxChannel( MFX_CHANNEL mfxChannel )
	{
		m_mfxChannel = mfxChannel;
	}

	int GetPort() const
	{
		return m_nPort;
	}

	void SetPort( int n )
	{
		m_nPort = n;
	}

	int GetChannel() const
	{
		return m_nChan;
	}

	void SetChannel( int nChan )
	{
		ASSERT( nChan >= -1 && nChan <= 15 );
		m_nChan = nChan;
	}

	short GetBank() const
	{
		return m_nBank; 
	}
	
	void SetBank( short n )
	{
		ASSERT( MIN_BANK <= n && n <= MAX_BANK );
		m_nBank = n;
	}
	
	int GetPatch() const
	{
		return m_nPatch; 
	}
	
	void SetPatch( int n )
	{
		ASSERT( n < 128 );
		m_nPatch = n;
	}
	
	BYTE GetBankSelectMethod() const
	{
		return m_byBankSelMethod; 
	}
	
	void SetBankSelectMethod( BYTE by )
	{
		ASSERT( by < 128 );
		m_byBankSelMethod = by; 
	}

	const char* GetName() const
	{
		return m_szName;
	}

	void SetName( const char* psz )
	{
		ASSERT( psz );
		if (psz)
		{
			strncpy( m_szName, psz, sizeof(m_szName) );
			m_szName[ sizeof(m_szName) - 1 ] = 0;
		}
		else
			*m_szName = 0;
	}

	MfxMuteMask GetMuteState() const
	{
		return m_mute;
	}

	void SetMuteState( MfxMuteMask muteSetBits, MfxMuteMask muteClearBits )
	{
		m_mute = (m_mute.byte | muteSetBits) & ~muteClearBits;
	}

	int GetKeyOfs() const
	{
		return m_nKeyOfs;
	}

	void SetKeyOfs( int nKeyOfs )
	{
		m_nKeyOfs = nKeyOfs;
	}

	int GetVelOfs() const
	{
		return m_nVelOfs;
	}

	void SetVelOfs( int nVelOfs )
	{
		m_nVelOfs = nVelOfs; 
	}

	int IndexForTimeGE( LONG lTime ) const;

	int GetPlayIndex() const { return m_ixPlay; }
	void SetPlayIndex( int ix ) { m_ixPlay = ix; }

	void ApplyTrackProperties( DWORD* pdwShortMsg );

private:

	MFX_CHANNEL		m_mfxChannel;
	int				m_nPort;
	int				m_nChan;
	short				m_nBank;
	int				m_nPatch;
	BYTE				m_byBankSelMethod;
	char				m_szName[ 128 ];
	MfxMuteMask		m_mute;
	int				m_nKeyOfs;
	int				m_nVelOfs;
	int				m_ixPlay;

private:

	void copy( const CMfxTrack& rhs );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_MFXTRACK_H__DC0A32AB_959C_47D3_8AEA_823F7C44A1D6__INCLUDED_)
