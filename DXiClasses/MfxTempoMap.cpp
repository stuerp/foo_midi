// MfxTempoMap.cpp: implementation of the CMfxTempoMap class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxTempoMap.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

static DWORD tempoToUsecsPerQN( int nTempo100 )
{
	const QWORD qwUsecsPerMinX100 = 6000000000;	// (60 Sec/Min) * (1000000 Usec/Sec) * 100

	return static_cast<DWORD>( (qwUsecsPerMinX100 + (nTempo100 >> 1)) / nTempo100 );
}

//////////////////////////////////////////////////////////////////////

CMfxTempoMap::CMfxTempoMap() : m_nPPQ(120)
{
}

CMfxTempoMap::~CMfxTempoMap()
{
}

/////////////////////////////////////////////////////////////////////////////

void CMfxTempoMap::Recalc( int ix /* = 0 */ )
{
	// Make sure we always have at least one tempo
	if (empty())
	{
		CTempo tempo;
		push_back( tempo );
	}

	// Recalculates the frame member of each entry in the map from entry 'ix'
	// onward.  If ix==0, it ensures that GetTime() == 0, just in case the
	// recalculation is part of cleaning up after deleting the first entry
	// in the map.
	//
	// Also recalcs .m_qwCnt for each field, so that this can be called after
	// file-loading to make sure all calculated fields are correct.  Caller
	// must have done CTempo::SetTime() correctly.

	if (ix == 0)
	{
		(*this)[ 0 ].SetTime( 0 );
		(*this)[ 0 ].m_qwCnt = 0;
		++ix;
	}

	int const nLen = size();
	if (ix < nLen)
	{
		QWORD qwCnt = (*this)[ ix - 1 ].m_qwCnt;
		for ( ; ix < nLen; ++ix)
		{
			CTempo&			curr = (*this)[ ix ];
			const CTempo&	prev = (*this)[ ix - 1 ];
	
			ASSERT( prev.GetTime() <= curr.GetTime() );

			// qwCnt += #ticks * usec/tick
			QWORD qw = curr.GetTime() - prev.GetTime();
			qw *= tempoToUsecsPerQN( prev.GetBPM() );
			qwCnt += qw;
			curr.m_qwCnt = qwCnt;	// save start usec for each tempo range
		}
	}

}

/////////////////////////////////////////////////////////////////////////////

int CMfxTempoMap::IndexForTime( const LONG t ) const
{
	ASSERT( !empty() );

	GetTime<CTempo> getFn;
	return bsearch_for_value( *this, t, getFn );
}

/////////////////////////////////////////////////////////////////////////////

template<class T>
class GetCount
{
public:
	QWORD operator()( const T& t ) const { return t.GetCount(); }
};

int CMfxTempoMap::IndexForSeconds( double dSeconds ) const
{
	ASSERT( !empty() );

	QWORD qwCnt = static_cast<QWORD>( dSeconds * 1000000 * GetPPQ() + 0.5 );

	GetCount<CTempo> getFn;
	return bsearch_for_value( *this, qwCnt, getFn );
}

/////////////////////////////////////////////////////////////////////////////

long CMfxTempoMap::Tick2Sample( const LONG tTick, long lSampleRate ) const
{
	ASSERT( !empty() );
	ASSERT( 0 <= lSampleRate );

	const CTempo& tempo = (*this)[ IndexForTime( tTick ) ];

	// First convert from ticks to usecs since beg of range, then add in
	// the #usecs since beg of song.  Now convert that to samples.

	QWORD qwDen = 1000000L;
	qwDen *= GetPPQ();								// qwDen = usecs/100secs

	QWORD qw = tTick - tempo.GetTime();			// qw = #ticks in range
	qw *= tempoToUsecsPerQN( tempo.GetBPM() );// qw = #usecs in range
	qw += tempo.GetCount();							// qw = #usecs since song beg
	qw *= lSampleRate;								// qw *= samples per second
	qw  = (qw + (qwDen >> 1)) / qwDen;			// qw /= qwDen (rndoff) => #samples

	if (qw > LONG_MAX)
		return LONG_MAX;								// too big, truncate
	else
		return static_cast<long>(qw);		      // (small enough, ignore hi DWORD)
}

/////////////////////////////////////////////////////////////////////////////

LONG CMfxTempoMap::Sample2Tick( long lSample, long lSampleRate ) const
{
	// Converts from samples to ticks, rounding

	ASSERT( !empty() );
	ASSERT( 0 <= lSampleRate );

	const CTempo& tempo = (*this)[ IndexForSeconds( static_cast<double>( lSample ) / lSampleRate ) ];
	
	QWORD qwDen = tempoToUsecsPerQN( tempo.GetBPM() );
	qwDen *= lSampleRate;

	QWORD qw = lSample;
	qw *= 1000000L;
	qw *= GetPPQ();

	QWORD qw1( tempo.GetCount() );
	qw1 *= lSampleRate;
	qw -= qw1;
	qw  = (qw + (qwDen >> 1)) / qwDen;		// round:   qw /= qwDen (rndoff)
	
	return static_cast<long>(qw) + tempo.GetTime();		// (small now, can ignore hi DWORD)
}

/////////////////////////////////////////////////////////////////////////////

inline LONGLONG TIME2UTicks( const long& t )
{
	LONGLONG	ll = t < 0 ? 0xffffffffffff0000 : 0;
	short*	pn = (short*)&ll;
	long*		pl = (long*)(pn + 1);
	*pl = t;

	return ll;
}

inline long UTicks2TIME( LONGLONG ticks )
{
	short*	pn = (short*) &ticks;
	long*		pl = (long*)( pn + 1 );

	return *pl + ((static_cast<DWORD>(ticks) & 0x8000) >> 15);
}

static const LONG UTIME_PPQ = (960 << 16);
static const DWORD PPQ_10_6 = 960 * 1000000;

/////////////////////////////////////////////////////////////////////////////

LONGLONG CMfxTempoMap::Sample2UTick( LONGLONG llSample, long lSampleRate ) const
{
	double dSecs = double(llSample) / lSampleRate;

	const CTempo& tempo = (*this)[ IndexForSeconds( dSecs ) ];
	
	// Time (in UTICKS) at the start of the tempo change
	LONGLONG llTicks = TIME2UTicks( tempo.GetTime() );

	// How far we are into the tempo change (in Usec)
	double dUsecFromStart = static_cast<double>( static_cast<LONGLONG>( tempo.GetCount() ) ) / 960;
	double dUsec = (dSecs * 1000000) - dUsecFromStart;

	// Divide by Usec/QN to get fraction of QN, multiply by PPQ to get UTICKS
	double dUsecPerQN = tempoToUsecsPerQN( tempo.GetBPM() );
	if (dUsec > 0.0)
		llTicks += static_cast<LONGLONG>( UTIME_PPQ * (dUsec / dUsecPerQN) + 0.5 );
	else
		llTicks += static_cast<LONGLONG>( UTIME_PPQ * (dUsec / dUsecPerQN) - 0.5 );

	return llTicks;
}

////////////////////////////////////////////////////////////////////////////////

LONGLONG CMfxTempoMap::UTick2Sample( LONGLONG llTicks, LONG lSampleRate ) const
{
	const CTempo& tempo = (*this)[ IndexForTime( UTicks2TIME( llTicks ) ) ];

	double dSecsFromStart = static_cast<double>( static_cast<LONGLONG>( tempo.GetCount() ) ) / PPQ_10_6;
	double dSecsPerQN = static_cast<double>( tempoToUsecsPerQN( tempo.GetBPM() ) ) / 1000000;
	LONGLONG utWithinRange = llTicks - TIME2UTicks( tempo.GetTime() );
	double dSecs = dSecsFromStart + ((dSecsPerQN * utWithinRange) / UTIME_PPQ);
	LONGLONG llSample = static_cast<LONGLONG>( dSecs * lSampleRate + 0.5 );

	return llSample;
}

////////////////////////////////////////////////////////////////////////////////
