// DXiPlayer.cpp: implementation of the CDXiPlayer class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <DxFilterGraph.h>
#include <MfxSeq.h>
#include <MfxHostSite.h>
#include <MfxEventQueue.h>
#include <MfxDataQueue.h>

#include "DXiPlayer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

CDXiPlayer::CDXiPlayer()
{
	m_pFilterGraph = NULL;
	m_pFilter = NULL;
	m_pSeq = NULL;
	m_pMfxSynth = NULL;
	m_pMfxHostSite = NULL;
	m_bPlaying = FALSE;
	m_bLooping = FALSE;
	m_bRunning = FALSE;
	m_tLoopStart = 0;
	m_tLoopEnd = 0;
	m_llSampLoopStart = 0;
	m_llSampLoopEnd = 0;
	m_tNow = 0;
	m_nCurrentTrack = -1;
	m_uSampleRate = 44100;
}

CDXiPlayer::~CDXiPlayer()
{
	Terminate();
}

//////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::Initialize()
{
	if (m_pFilterGraph)
		CHECK(E_FAIL); // already initialized

	HRESULT hr = S_OK;
	m_pFilterGraph = new CDxFilterGraph( &hr );
	CHECK( hr );
	if (NULL == m_pFilterGraph)
		CHECK(E_OUTOFMEMORY);

	CHECK( CMfxHostSite::Create( &m_pMfxHostSite ) );

	return S_OK;
}

///////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::Terminate()
{
	if (GetFilterGraph())
	{
		CHECK( SetFilter( NULL ) );
		SAFE_DELETE( m_pFilterGraph );
	}

	SAFE_RELEASE( m_pMfxHostSite );

	return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::SetSeq( CMfxSeq* pSeq )
{
	// We can't change CMfxSeq's while playing
	CHECK( Stop() );

	// Disconnect the old DXi before changing the seq
	if (m_pMfxSynth)
		m_pMfxSynth->Disconnect();
	
	// Set the new seq
	m_pSeq = pSeq;

	// Do we have something to reconnect?
	if (GetSeq() && m_pMfxSynth)
	{
		// Reconnect the DXi
		GetHostSite()->SetSeq( GetSeq() );
		GetHostSite()->AddRef();
		CHECK( m_pMfxSynth->Connect( reinterpret_cast<IMfxTempoMap*>( GetHostSite() ) ) );

		// Restart live thruing of MIDI
		CHECK( SetPosition( 0 ) );
		CHECK( Play( FALSE ) );
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::SetFilter( IBaseFilter* pFilter )
{
	if (pFilter == m_pFilter)
		return S_OK; // nothing to do

	// We can't change filters while playing
	CHECK( Stop() );

	// Disconnect and release the DXi first
	if (m_pMfxSynth)
	{
		CHECK( m_pMfxSynth->Disconnect() );
		SAFE_RELEASE( m_pMfxSynth );
	}

	// Set the new filter in its graph
	CHECK( GetFilterGraph()->SetFilter( pFilter ) );

	// Store the filter pointer
	if (m_pFilter)
		m_pFilter->Release();
	m_pFilter = pFilter;
	if (m_pFilter)
		m_pFilter->AddRef();

	// Query for a DXi
	if (m_pFilter && SUCCEEDED( m_pFilter->QueryInterface( IID_IMfxSoftSynth, (void**)&m_pMfxSynth ) ))
	{
		ASSERT( GetSeq() );
		if (NULL == GetSeq())
			CHECK(E_FAIL);

		// Connect the DXi if it was found
		GetHostSite()->SetSeq( GetSeq() );
		GetHostSite()->AddRef();
		CHECK( m_pMfxSynth->Connect( reinterpret_cast<IMfxTempoMap*>( GetHostSite() ) ) );

		// Restart live thruing of MIDI
		CHECK( Play( FALSE ) );
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::Play( BOOL bPlaySeq )
{
	if (NULL == m_pFilterGraph)
		CHECK(E_FAIL); // not initialized
	if (m_bRunning)
		CHECK(E_FAIL); // already playing

	// Make sure our loop sample times are up to date
	CHECK( updateLoopSampleTimes() );

	// Set up the host site
	CHECK( GetHostSite()->SetSampleRate( GetSampleRate() ) );

	// Initialize the filter graph
	CHECK( GetFilterGraph()->SetSampleRate( GetSampleRate() ) );
	CHECK( GetFilterGraph()->SetBufferSize( 576 ) );

	// Position the graph et al
	CHECK( SetPosition( m_tNow ) );

	// Start the DXi
	if (m_pMfxSynth)
		CHECK( m_pMfxSynth->OnStart( m_tNow ) );

	// Start the filter graph
	CHECK( GetFilterGraph()->Pause() );
	CHECK( GetFilterGraph()->Play() );

	m_bRunning = TRUE;
	m_bPlaying = bPlaySeq;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::Stop()
{
	if (NULL == m_pFilterGraph)
		CHECK(E_FAIL); // not initialized
	if (!m_bRunning)
		return S_OK; // already stopped

	// Stop the filter graph
	CHECK( GetFilterGraph()->Stop() );

	// Remember where we stopped
	if (m_bPlaying)
		m_tNow = GetPosition();

	// Stop the DXi
	if (m_pMfxSynth)
		m_pMfxSynth->OnStop( m_tNow );

	m_bPlaying = m_bRunning = FALSE;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::Rewind()
{
	CHECK( Stop() );
	CHECK( SetPosition( 0 ) );
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

LONG CDXiPlayer::GetPosition()
{
	if (NULL != GetSeq())
	{
		MFX_TIME t;
		t.timeFormat = TF_SAMPLES;
		t.llSamples = GetFilterGraph()->GetPosition();
		GetHostSite()->ConvertMfxTime( &t, TF_TICKS );
		const_cast<CDXiPlayer*>(this)->m_tNow = t.lTicks;
	}
	return m_tNow;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::SetPosition( LONG lTicks )
{
	if (NULL == GetSeq())
		return E_FAIL;

	// Convert tick time to samples
	MFX_TIME t;
	t.timeFormat = TF_TICKS;
	t.lTicks = lTicks;
	CHECK( GetHostSite()->ConvertMfxTime( &t, TF_SAMPLES ) );

	// Reposition the graph
	CHECK( GetFilterGraph()->SetPosition( t.llSamples ) );

	// Reposition each track in the sequence
	GetSeq()->SetPosition( lTicks );

	// Remember our position
	m_tNow = lTicks;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::SetLoopStart( LONG t )
{
	m_tLoopStart = t;
	if (m_tLoopStart > m_tLoopEnd)
		m_tLoopEnd = m_tLoopStart;

	CHECK( updateLoopSampleTimes() );
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::SetLoopEnd( LONG t )
{
	m_tLoopEnd = t;
	if (m_tLoopStart > m_tLoopEnd)
		m_tLoopEnd = m_tLoopStart;

	CHECK( updateLoopSampleTimes() );
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

void CDXiPlayer::SetLooping( BOOL b )
{
	if (b == m_bLooping)
		return; // nothing to do
	
	m_bLooping = b;

	if (IsLooping())
	{
		// Turn zero length loops into something reasonable
		if (m_tLoopStart == m_tLoopEnd)
			m_tLoopEnd = m_tLoopStart + (GetSeq() ? GetSeq()->m_tempoMap.GetPPQ() : 120) * 4;

		// Make sure the sample times are in sync
		updateLoopSampleTimes();
		
		// If they've enabled looping and we're already past the loop end time,
		// reposition to the start of the loop
		if (GetPosition() > m_tLoopEnd && IsPlaying())
		{
			Stop();
			SetPosition( GetLoopStart() );
			Play( TRUE );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::updateLoopSampleTimes()
{
	if (NULL == GetSeq())
		CHECK(E_FAIL);

	MFX_TIME t;

	t.timeFormat = TF_TICKS;
	t.lTicks = GetLoopStart();
	CHECK( GetHostSite()->ConvertMfxTime( &t, TF_SAMPLES ) );
	m_llSampLoopStart = t.llSamples;

	t.timeFormat = TF_TICKS;
	t.lTicks = GetLoopEnd();
	CHECK( GetHostSite()->ConvertMfxTime( &t, TF_SAMPLES ) );
	m_llSampLoopEnd = t.llSamples;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CDXiPlayer::FillBuffer( void* pvData, DWORD cbSamples )
{	
	if (NULL == GetFilterGraph() ||
		 NULL == GetSoftSynth() ||
		 NULL == GetHostSite() ||
		 NULL == GetSeq())
		return S_OK; // nothing to do

	float * pnOutput = (float*)pvData;

	// Determine loop times
	LONGLONG	llLoopFrom = IsLooping() ? m_llSampLoopStart : _I64_MIN;
	LONGLONG llLoopThru = IsLooping() ? m_llSampLoopEnd : _I64_MAX;

	// Loopback may require repeated MIDI/audio streaming
	LONG cSampToGo = cbSamples; //GetFilterGraph()->GetBufferSize();
	while (cSampToGo > 0)
	{
		LONGLONG llPosition = GetFilterGraph()->GetPosition();

		// See if we need to loop back
		BOOL bLoopedBack = FALSE;
		if (llPosition >= llLoopThru)
		{
			llPosition = llLoopFrom;
			bLoopedBack = TRUE;
		}
	
		// Determine how many samples to process this time
		LONG cSampProcess = cSampToGo;
		if (llPosition + cSampProcess > llLoopThru)
			cSampProcess = static_cast<LONG>( llLoopThru - llPosition );

		// Convert audio buffer times to MIDI ticks
		MFX_TIME t;
		t.timeFormat = TF_SAMPLES;
		t.llSamples = llPosition;
		CHECK( GetHostSite()->ConvertMfxTime( &t, TF_TICKS ) );
		LONG const lTickFrom = t.lTicks;
		t.timeFormat = TF_SAMPLES;
		t.llSamples = llPosition + cSampProcess;
		CHECK( GetHostSite()->ConvertMfxTime( &t, TF_TICKS ) );
		LONG const lTickUpTo = t.lTicks + 1;
		
		// Deal with loop back
		if (bLoopedBack)
		{
			// Tell the DXi about it
			CHECK( m_pMfxSynth->OnLoop( GetLoopStart(), GetLoopEnd() ) );
				
			// Reposition the playing seq
			GetSeq()->SetPosition( GetLoopStart() );
		}

		if (m_bPlaying)
		{
			// Feed data from each track
			map<int,CMfxTrack>::iterator it;
			for (it = GetSeq()->GetBeginTrack(); it != GetSeq()->GetEndTrack(); it++)
			{
				int		nTrack = it->first;
				CMfxTrack&	trk = it->second;

				// Get the index of the next event to be fed to the DXi
				int ixFrom = trk.GetPlayIndex();
				int ixUpTo = ixFrom;
				
				if (ixFrom < trk.size())
				{
					// Do we have an event to feed?
					LONG const tCurr = trk[ ixFrom ].GetTime();
					if (tCurr < lTickUpTo)
					{
						for (ixUpTo = ixFrom + 1; ixUpTo < trk.size(); ixUpTo++)
						{
							if (trk[ ixUpTo ].GetTime() > lTickUpTo)
								break;
						}
					}
				}
				
				// Create an IMfxEventQueue for the next range of events to feed
				if (ixUpTo > ixFrom)
				{
					CMfxEventQueue queue( trk, ixFrom, ixUpTo );

					// Stream events to the DXi
					CHECK( m_pMfxSynth->OnEvents( lTickFrom, lTickUpTo, nTrack, &queue ) );
					
					// Update the track's play index
					trk.SetPlayIndex( ixUpTo );
				}
			}
		}
		
		// Loop back the filter graph if necessary
		if (bLoopedBack)
			CHECK( GetFilterGraph()->SetPosition( llPosition ) );

		// Pump another buffer through the filter graph
		float* pfOutput = NULL;
		CHECK( GetFilterGraph()->Process( &pfOutput, cSampProcess ) );
		
		// Fill the data.  Note we assume stereo!
		memcpy(pnOutput, pfOutput, cSampProcess * 2 * sizeof(float));
		pnOutput += cSampProcess * 2;

		// Tally up what we processed so far
		cSampToGo -= cSampProcess;
	}

	return S_OK;
}
