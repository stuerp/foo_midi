// MfxMeterKeySigMap.cpp: implementation of the CMfxMeterKeySigMap class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <malloc.h>
#include <stdio.h>

#include "MfxMeterKeySigMap.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////

CMfxMeterKeySigMap::CMfxMeterKeySigMap() : m_nPPQ(120)
{
}

//////////////////////////////////////////////////////////////////////

int CMfxMeterKeySigMap::Tick2Meas( const LONG& t ) const
{
	ASSERT( !empty() );

	const CMeterKey&	m		= (*this)[ IndexForTime( t ) ];
	LONG const			tSince= t - m.GetTime();

	return m.GetMeasure() + (tSince / m.GetTicksPerMeasure());
}

/////////////////////////////////////////////////////////////////////////////

LONG CMfxMeterKeySigMap::Mbt2Tick( int nMeas, int nBeat, int nTick ) const
{
	const CMeterKey&	m = (*this)[ IndexForMeasure( nMeas ) ];

	return	m.GetTime() +
				((nMeas - m.GetMeasure()) * m.GetTicksPerMeasure()) +
				((nBeat - 1) * m.GetTicksPerBeat()) +
				nTick;
}

/////////////////////////////////////////////////////////////////////////////

void CMfxMeterKeySigMap::Tick2Mbt( const LONG t, int* pnMeas, short* pnBeat, short* pnTick ) const
{
	ASSERT( pnMeas );
	ASSERT( pnBeat );
	ASSERT( pnTick );

	const CMeterKey&	m = (*this)[ IndexForTime( t ) ];
	LONG const			tSince = t - m.GetTime();

	*pnMeas = m.GetMeasure() + (tSince / m.GetTicksPerMeasure());
	*pnBeat = static_cast<short>( (tSince % m.GetTicksPerMeasure()) / m.GetTicksPerBeat() + 1 );
	*pnTick = static_cast<short>( (tSince % m.GetTicksPerMeasure()) % m.GetTicksPerBeat() );
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxMeterKeySigMap::Tick2MbtStr( const LONG t, char* psz, BOOL bLeadZero /* = FALSE */ ) const
{
	if (IsBadWritePtr( psz, 13 ))
		CHECK(E_POINTER);

	int	nMeas;
	short	nBeat, nTick;
	Tick2Mbt( t, &nMeas, &nBeat, &nTick );

	if (bLeadZero)
		sprintf( psz, "%u:%02u:%03u", nMeas, nBeat, nTick );
	else
		sprintf( psz, "%u:%u:%03u", nMeas, nBeat, nTick );

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxMeterKeySigMap::MbtStr2Tick( const char* pcsz, LONG* plTick ) const
{
	if (IsBadReadPtr( pcsz, 1 ))
		CHECK(E_POINTER);

	// Parses a string of the form:
	//
	//		[<measure number>:[<beat number>:[<tick number>]]]
	//
	// where ':' is at least one non-digit separator.
	//
	// The default time of 1:1:0 is returned if an invalid string was
	// entered.  If the user supplied only the measure, or only the measure
	// and the beat, then the same defaults are used: beat 1, tick 0.
	//
	// NOTE: Modifies the string pointed to.

	static const char szToks[] = ": |.";

	*plTick = 0;

	int nMeas = 1;
	int nBeat = 1;
	int nTick = 0;

	// Get a non-const string for use by strtok()
	char* psz = (char*) _alloca( strlen(pcsz) + 1 );
	strcpy( psz, pcsz );

	char* p = strtok( psz, szToks );
	if (p)
	{
		if (strlen( p ) < 6)
		{
			nMeas = atoi( p );
			if (0 == nMeas)
				nMeas = 1;
		}

		p = strtok( NULL, szToks );
		if (p)
		{
			if (strlen( p ) < 6)
			{
				nBeat = atoi( p );
				if (0 == nBeat)
					nBeat = 1;
			}

			p = strtok( NULL, szToks );
			if (p)
			{
				if (strlen( p ) < 6)
				{
					nTick = atoi( p );
				}
			}
		}
	}

	*plTick = Mbt2Tick( nMeas, nBeat, nTick );

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

void CMfxMeterKeySigMap::Recalc( int ix /* = 0 */ )
{
	// Recalculates ticks for each entry in the map from entry 'ix'
	// onward.  If ix==0, ensures that LONG==0 and measure==1, just in case
	// the recalculation is part of cleaning up after deleting the first
	// entry in the map.
	//
	// The measure field for CMeterKey is the key value from which the
	// ticks value is derived.  So caller must have set .wMeas correctly.
	//
	// Also recalculates the ticks per beat and ticks per measure fields.
	// Done so this can validate map after doing file-loading, where these
	// fields may be set to zero by external programs creating Cakewalk files.

	// Ensure that we always have at least one meter/key entry
	if (empty())
	{
		CMeterKey mk;
		push_back( mk );
	}

	if (ix == 0)
	{
		// Special treatment for 1st entry.
		CMeterKey& m = (*this)[ 0 ];

		m.SetTime( 0 );
		m.SetMeasure( 1 );

		m.SetTicksPerBeat( (GetPPQ() * 4) / (1 << m.GetBottom()) );
		m.SetTicksPerMeasure( m.GetTicksPerBeat() * m.GetTop() );

		++ix;
	}

	for ( ; ix < size(); ++ix)
	{
		CMeterKey& m		= (*this)[ ix ];
		CMeterKey& mPrev	= (*this)[ ix - 1 ];

		m.SetTicksPerBeat( (GetPPQ() * 4) / (1 << m.GetBottom()) );
		m.SetTicksPerMeasure( m.GetTicksPerBeat() * m.GetTop() );

		m.SetTime( mPrev.GetTime() + ((m.GetMeasure() - mPrev.GetMeasure()) * mPrev.GetTicksPerMeasure()) );
	}
}

////////////////////////////////////////////////////////////////////////////////

int CMfxMeterKeySigMap::IndexForTime( LONG t ) const
{
	ASSERT( !empty() );

	GetTime<CMeterKey> getFn;
	return bsearch_for_value( *this, t, getFn );
}

/////////////////////////////////////////////////////////////////////////////

template<class T>
class GetMeasure
{
public:
	int operator()( const T& t ) const { return t.GetMeasure(); }
};

int CMfxMeterKeySigMap::IndexForMeasure( int nMeas ) const
{
	ASSERT( !empty() );

	GetMeasure<CMeterKey> getFn;
	return bsearch_for_value( *this, nMeas, getFn );
}

/////////////////////////////////////////////////////////////////////////////
