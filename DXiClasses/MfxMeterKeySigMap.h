// MfxMeterKeySigMap.h: interface for the CMfxMeterKeySigMap class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXMETERKEYSIGMAP_H__2B33AD86_5E72_4F75_98CD_0C9588123FE0__INCLUDED_)
#define AFX_MFXMETERKEYSIGMAP_H__2B33AD86_5E72_4F75_98CD_0C9588123FE0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxHostSite; // forward

////////////////////////////////////////////////////////////////////////////////

class CMeterKey
{
public:

	CMeterKey( int nMeas = 1, int nTop = 4, int nBottom = 4, int nKey = 0 )
	{
		SetMeasure( nMeas );
		SetTop( nTop );
		SetBottom( nBottom );
		SetKeySig( nKey );

		// These are meaningless until CMfxMeterKeySigMap calcs them...
		m_tTick = 0;
		m_nTicksPerBeat = 0;
		m_nTicksPerMeasure = 0;
	}

	int GetMeasure() const { return m_nMeas; }
	void SetMeasure( int nMeas ) { ASSERT( 1 <= nMeas ); m_nMeas = nMeas; }

	int GetTop() const { return m_nTop; }
	void SetTop( int nTop ) { ASSERT( 1 <= nTop && nTop <= 99 ); m_nTop = nTop; }

	int GetBottom() const { return m_nBot; }
	void SetBottom( int nBot ) { m_nBot = nBot; }

	int GetKeySig() const { return m_nKeySig; }
	void SetKeySig( int nKeySig ) { ASSERT( -7 <= nKeySig && nKeySig <= 7 ); m_nKeySig = nKeySig; } 

	void SetTime( LONG l ) { m_tTick = l; }
	LONG GetTime() const { return m_tTick; }

	void SetTicksPerBeat( int n ) { m_nTicksPerBeat = n; }
	int GetTicksPerBeat() const { return m_nTicksPerBeat; }

	void SetTicksPerMeasure( int n ) { m_nTicksPerMeasure = n; }
	int GetTicksPerMeasure() const { return m_nTicksPerMeasure; }

private:

	int	m_nMeas;		// number of first measure in this meter
	int	m_nTop;		// beats per measure
	int	m_nBot;		// den << 2 == 1,2,4, ... == whole, 1/2, 1/4,...
	int	m_nKeySig;	// -7..+7

	LONG	m_tTick;					// tick time of meter change
	int	m_nTicksPerBeat;		// (beat == denominator, not 1/4-note)
	int	m_nTicksPerMeasure;	// ticks per measure
};

////////////////////////////////////////////////////////////////////////////////

class CMfxMeterKeySigMap : public vector<CMeterKey>
{
public:

	CMfxMeterKeySigMap();

	int GetPPQ() const { return m_nPPQ; }

	void SetPPQ( int nPPQ )
	{
		m_nPPQ = nPPQ;
		Recalc();
	}

	int Tick2Meas( const LONG& t ) const;
	void Recalc( int ix = 0 );

	int IndexForTime( LONG lTicks ) const;
	int IndexForMeasure( int nMeasure ) const;

	void Tick2Mbt( LONG lTick, int* pnMeas, short* pnBeat, short* pnTick ) const;
	LONG Mbt2Tick( int nMeas, int nBeat, int nTick ) const;

	HRESULT Tick2MbtStr( const LONG t, char* psz, BOOL bLeadZero = FALSE ) const;
	HRESULT MbtStr2Tick( const char* psz, LONG* lTick ) const;

private:

	int m_nPPQ;
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_MFXMETERKEYSIGMAP_H__2B33AD86_5E72_4F75_98CD_0C9588123FE0__INCLUDED_)
