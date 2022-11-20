// MfxMarker.h: interface for the CMarker class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXMARKER_H__70DA435B_1D8B_42A0_B1A3_86A702EA4F98__INCLUDED_)
#define AFX_MARKER_H__70DA435B_1D8B_42A0_B1A3_86A702EA4F98__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////////////////

class CMarker
{
public:

	CMarker()
	{
		m_szName[0] = 0;
		m_t = 0;
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
	}

	LONG GetTime() const
	{
		return m_t;
	}

	void SetTime( LONG t )
	{
		m_t = t;
	}

private:

	char	m_szName[ 64 ];
	LONG	m_t;
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_MFXMARKER_H__70DA435B_1D8B_42A0_B1A3_86A702EA4F98__INCLUDED_)
