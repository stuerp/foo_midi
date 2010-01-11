// MfxHostSite.cpp: implementation of the CMfxHostSite class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxSeq.h"
#include "MfxHostSite.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::Create( CMfxHostSite** ppObj )
{
	if (NULL == ppObj)
		return E_POINTER;

	CMfxHostSite* pObjNew = new CMfxHostSite;
	if (NULL == pObjNew)
		return E_OUTOFMEMORY;

	pObjNew->AddRef();
	*ppObj = pObjNew;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////

CMfxHostSite::CMfxHostSite() : m_cRef(0), m_pSeq(NULL), m_nPulseRequests(0), m_uSampleRate(44100)
{
}

CMfxHostSite::~CMfxHostSite()
{
	ASSERT( 0 == m_cRef );
}

/////////////////////////////////////////////////////////////////////////////
// IUnknown

HRESULT CMfxHostSite::QueryInterface( REFIID riid, LPVOID* ppv )
{
	if (!ppv || IsBadWritePtr( ppv, sizeof(LPVOID) ))
		return E_POINTER;

	if (IID_IUnknown == riid)
		*ppv = static_cast<IMfxTempoMap*>(this);
	else if (IID_IMfxTempoMap == riid)
		*ppv = static_cast<IMfxTempoMap*>(this);
	else if (IID_IMfxTimeConverter == riid)
		*ppv = static_cast<IMfxTimeConverter*>(this);
	else if (IID_IMfxMeterMap == riid)
		*ppv = static_cast<IMfxMeterMap*>(this);
	else if (IID_IMfxKeySigMap == riid)
		*ppv = static_cast<IMfxKeySigMap*>(this);
	else if (IID_IMfxMarkerMap == riid)
		*ppv = static_cast<IMfxMarkerMap*>(this);
	else if (IID_IMfxNotifyHost == riid)
		*ppv = static_cast<IMfxNotifyHost*>(this);
	else if (IID_IMfxInputCallback == riid)
		*ppv = static_cast<IMfxInputCallback*>(this);
	else if (IID_IMfxInputPulse == riid)
	{
		ASSERT(FALSE); // MFX input pulse is not implemented yet
		*ppv = static_cast<IMfxInputPulse*>(this);
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

ULONG CMfxHostSite::AddRef()
{
	return InterlockedIncrement( &m_cRef );
}

ULONG CMfxHostSite::Release()
{
	ASSERT( 0 < m_cRef );
	if (InterlockedDecrement( &m_cRef ) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxTempoMap

LONG CMfxHostSite::TicksToMsecs( LONG lTicks )
{
	ASSERT( GetSeq() );

	// Use CMfxTempoMap::Tick2Sample() with a "sample rate" of 1000 (1000 msec per sec)
	return GetSeq()->m_tempoMap.Tick2Sample( lTicks, 1000 );
}

/////////////////////////////////////////////////////////////////////////////

LONG CMfxHostSite::MsecsToTicks( LONG lMsecs )
{
	ASSERT( GetSeq() );

	// Use CMfxTempoMap::Sample2Tick() with a "sample rate" of 1000 (1000 msec per sec)
	return GetSeq()->m_tempoMap.Sample2Tick( lMsecs, 1000 );
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetTicksPerQuarterNote()
{
	ASSERT( GetSeq() );
	return GetSeq()->GetPPQ();
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetTempoIndexForTime( LONG lTicks )
{
	ASSERT( GetSeq() );
	return GetSeq()->m_tempoMap.IndexForTime( lTicks );
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetTempoCount()
{
	ASSERT( GetSeq() );
	return GetSeq()->m_tempoMap.size();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::GetTempoAt( int ix, LONG* plTicks, int* pnBPM100 )
{
	ASSERT( GetSeq() );

	if (!( 0 <= ix && ix < GetSeq()->m_tempoMap.size() ))
		return E_INVALIDARG;
	if (!plTicks || IsBadWritePtr( plTicks, sizeof(LONG) ))
		return E_POINTER;
	if (!pnBPM100 || IsBadWritePtr( pnBPM100, sizeof(int) ))
		return E_POINTER;

	const CTempo& tempo = GetSeq()->m_tempoMap[ ix ];
	*plTicks = tempo.GetTime();
	*pnBPM100 = tempo.GetBPM();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxTimeConverter (and helpers)

static BOOL IsMusicalTime( MFX_TIME_FORMAT fmt )
{
	return
		TF_TICKS == fmt ||
		TF_UTICKS == fmt ||
		TF_MBT == fmt;
}

/////////////////////////////////////////////////////////////////////////////

static BOOL IsSmpteTime( MFX_TIME_FORMAT fmt )
{
	return
		TF_FRAMES == fmt ||
		TF_FRAMES_REL == fmt ||
		TF_SMPTE == fmt ||
		TF_SMPTE_REL == fmt;
};

/////////////////////////////////////////////////////////////////////////////

static LONGLONG GetUTicks( const CMfxSeq* pSeq, MFX_TIME* pTime )
{
	ASSERT( pSeq );
	
	if (TF_UTICKS == pTime->timeFormat)
		return pTime->llUTicks;
	else
	{
		LONG lTicks = 0;
		if (TF_TICKS == pTime->timeFormat)
			lTicks = pTime->lTicks;
		else if (TF_MBT == pTime->timeFormat)
			lTicks = pSeq->m_meterKeySigMap.Mbt2Tick( pTime->mbt.nMeas, pTime->mbt.nBeat, pTime->mbt.nTick );
		else
		{
			ASSERT(FALSE);
			return 0;
		}
		return pSeq->Tick2UTick( lTicks );
	}
}

/////////////////////////////////////////////////////////////////////////////

static LONGLONG GetSamples( MFX_TIME* pTime , unsigned uSampleRate )
{
	if (TF_SAMPLES == pTime->timeFormat)
		return pTime->llSamples;
	else if (TF_SECONDS == pTime->timeFormat)
		return static_cast<LONGLONG>( pTime->dSeconds * uSampleRate );
	else
	{
		ASSERT(FALSE);
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::ConvertMfxTime( MFX_TIME* pTime, MFX_TIME_FORMAT newFormat )
{
	ASSERT( GetSeq() );

	if (pTime->timeFormat == newFormat)
		return S_OK; // nothing to do

	// SMPTE is not supported by this demo application!
	if (IsSmpteTime( pTime->timeFormat ) || IsSmpteTime( newFormat ))
		CHECK(E_NOTIMPL);

	BOOL const bInMusical = IsMusicalTime( pTime->timeFormat );
	BOOL const bOutMusical = IsMusicalTime( newFormat );

	if (bInMusical == bOutMusical)
	{
		if (bInMusical)
		{
			// Convert musical to musical
			LONGLONG llUTicks = GetUTicks( GetSeq(), pTime );
			if (TF_UTICKS == newFormat)
				pTime->llUTicks = llUTicks;
			else
			{
				LONG lTicks = GetSeq()->UTick2Tick( llUTicks );
				if (TF_TICKS == newFormat)
					pTime->lTicks = lTicks;
				else
					GetSeq()->m_meterKeySigMap.Tick2Mbt( lTicks, &pTime->mbt.nMeas, &pTime->mbt.nBeat, &pTime->mbt.nTick );
			}
		}
		else
		{
			// Convert absolute to absolute
			LONGLONG llSamples = GetSamples( pTime , GetSampleRate() );
			if (TF_SECONDS == newFormat)
				pTime->dSeconds = static_cast<double>( llSamples ) / GetSampleRate();
			else
				pTime->llSamples = llSamples;
		}
	}
	else
	{
		if (bInMusical)
		{
			// Convert musical to absolute
			LONGLONG llUTicks = GetUTicks( GetSeq(), pTime );
			LONGLONG llSample = GetSeq()->m_tempoMap.UTick2Sample( llUTicks, GetSampleRate() );
			if (TF_SAMPLES == newFormat)
				pTime->llSamples = llSample;
			else
				pTime->dSeconds = static_cast<double>( llSample ) / GetSampleRate();
		}
		else
		{
			// Convert absolute to musical
			LONGLONG llSample = GetSamples( pTime , GetSampleRate() );
			LONGLONG llUTicks = GetSeq()->m_tempoMap.Sample2UTick( llSample, GetSampleRate() );
			if (TF_UTICKS == newFormat)
				pTime->llUTicks = llUTicks;
			else
			{
				LONG lTicks = static_cast<LONG>( llUTicks >> 16 );
				if (TF_TICKS == newFormat)
					pTime->lTicks = lTicks;
				else
					GetSeq()->m_meterKeySigMap.Tick2Mbt( lTicks, &pTime->mbt.nMeas, &pTime->mbt.nBeat, &pTime->mbt.nTick );
			}
		}
	}
	
	pTime->timeFormat = newFormat;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxMeterMap

HRESULT CMfxHostSite::TicksToMBT( LONG lTicks, int* pnMeasure, int* pnBeat, int* pnTicks )
{
	ASSERT( GetSeq() );

	if (!pnMeasure || IsBadWritePtr( pnMeasure, sizeof(int) ))
		return E_POINTER;
	if (!pnBeat || IsBadWritePtr( pnBeat, sizeof(int) ))
		return E_POINTER;
	if (!pnTicks || IsBadWritePtr( pnTicks, sizeof(int) ))
		return E_POINTER;

	short nBeat, nTick;
	GetSeq()->m_meterKeySigMap.Tick2Mbt( lTicks, pnMeasure, &nBeat, &nTick );
	*pnBeat = nBeat;
	*pnTicks = nTick;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

LONG CMfxHostSite::MBTToTicks( int nMeasure, int nBeat, int nTicks )
{
	ASSERT( GetSeq() );

	return GetSeq()->m_meterKeySigMap.Mbt2Tick( nMeasure, nBeat, nTicks );
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetMeterIndexForTime( LONG lTicks )
{
	ASSERT( GetSeq() );

	return GetSeq()->m_meterKeySigMap.IndexForTime( lTicks );
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetMeterCount()
{
	ASSERT( GetSeq() );

	return GetSeq()->m_meterKeySigMap.size();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::GetMeterAt( int ix, int* pnMeasure, int* pnTop, EBeatValue* peBottom )
{
	ASSERT( GetSeq() );

	if (!(0 <= ix && ix < GetSeq()->m_meterKeySigMap.size()))
		return E_INVALIDARG;
	if (!pnMeasure || IsBadWritePtr( pnMeasure, sizeof(int) ))
		return E_POINTER;
	if (!pnTop || IsBadWritePtr( pnTop, sizeof(int) ))
		return E_POINTER;
	if (!peBottom || IsBadWritePtr( peBottom, sizeof(EBeatValue) ))
		return E_POINTER;

	const CMeterKey& meter = GetSeq()->m_meterKeySigMap[ ix ];
	*pnMeasure = meter.GetTime();
	*pnTop = meter.GetTop();
	*peBottom = static_cast<IMfxMeterMap::EBeatValue>(meter.GetBottom());
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxKeySigMap

int CMfxHostSite::GetKeySigIndexForTime( LONG lTicks )
{
	ASSERT( GetSeq() );

	return GetSeq()->m_meterKeySigMap.IndexForTime( lTicks );
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetKeySigCount()
{
	ASSERT( GetSeq() );

	return GetSeq()->m_meterKeySigMap.size();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::GetKeySigAt( int ix, int* pnMeasure, int* pnKeySig )
{
	ASSERT( GetSeq() );

	if (!(0 <= ix && ix < GetSeq()->m_meterKeySigMap.size()))
		return E_INVALIDARG;
	if (!pnMeasure || IsBadWritePtr( pnMeasure, sizeof(int) ))
		return E_POINTER;
	if (!pnKeySig || IsBadWritePtr( pnKeySig, sizeof(int) ))
		return E_POINTER;

	const CMeterKey& meter = GetSeq()->m_meterKeySigMap[ ix ];
	*pnMeasure = meter.GetTime();
	*pnKeySig = meter.GetKeySig();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxMarkerMap

int CMfxHostSite::GetMarkerIndexForTime( LONG lTicks )
{
	ASSERT( GetSeq() );

	if (GetSeq()->m_markers.empty())
		return -1; // not found

	GetTime<CMarker> getFn;
	return bsearch_for_value( GetSeq()->m_markers, lTicks, getFn );
}

/////////////////////////////////////////////////////////////////////////////

int CMfxHostSite::GetMarkerCount()
{
	ASSERT( GetSeq() );

	return GetSeq()->m_markers.size();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::GetMarkerAt( int ix, LONG* plTime, IMfxMarkerMap::EUnits* peUnits, wchar_t** ppwszName )
{
	ASSERT( GetSeq() );

	if (!(0 <= ix && ix < GetSeq()->m_markers.size()))
		return E_INVALIDARG;
	if (!plTime || IsBadWritePtr( plTime, sizeof(LONG) ))
		return E_POINTER;
	if (!ppwszName || IsBadWritePtr( ppwszName, sizeof(wchar_t*) ))
		return E_POINTER;
	if (!peUnits || IsBadWritePtr( peUnits, sizeof(EUnits) ))
		return E_POINTER;

	const CMarker& marker = GetSeq()->m_markers[ ix ];

	*plTime = marker.GetTime();
	*peUnits = Ticks;

	int const nLen = MultiByteToWideChar( CP_ACP, 0, marker.GetName(), -1, NULL, 0 );
	int const cb = (nLen + 1) * sizeof(wchar_t);

	*ppwszName = static_cast<wchar_t*>( CoTaskMemAlloc( cb ) );
	if (*ppwszName)
	{
		if (0 == MultiByteToWideChar( CP_ACP, 0, marker.GetName(), -1, *ppwszName, cb ))
		{
			CoTaskMemFree( *ppwszName );
			*ppwszName = NULL;
			return E_FAIL;
		}
	
	}
	else
		return E_OUTOFMEMORY;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxInputPulse

HRESULT CMfxHostSite::GetPulseInterval( LONG* plIntervalMsec )
{
	if (NULL == plIntervalMsec || IsBadWritePtr( plIntervalMsec, sizeof(LONG) ))
		return E_POINTER;
	
	*plIntervalMsec = PULSE_INTERVAL_MSEC;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::BeginPulse()
{
	++m_nPulseRequests;

	// TODO: On the first pulse requester, set up a timer with a period equal to
	// PULSE_INTERVAL_MSEC, and call OnInput on the DXi on every timer interval.

	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMfxHostSite::EndPulse()
{
	--m_nPulseRequests;
	
	// TODO: Shut off our time on the last pulse requester.

	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// IMfxNotifyHost

HRESULT CMfxHostSite::OnMfxNotifyHost( UINT uMsg, IUnknown* pUnkFrom, LPARAM lParam )
{
	// NOTE: OnMfxNotifyHost is called from an MFX plugin that may be running under 
	// a different static module state than the application. 
	// Since this function potentially makes calls to MFC, we must reset the 
	// module state to the applications module state for the entire scope of this 
	// function, restoring it prior to returning. Failure to do this
	// will cause application windows to be not found in MFC's static handle map.
	// See TN058: MFC Module State Implementation
	
//	AFX_MANAGE_STATE( AfxGetAppModuleState() );

	HRESULT hr = E_FAIL;

	switch (uMsg)
	{
		case MH_SYNTH_AUDIO_PORTS_CHANGE_BEGIN:
		{
			// TODO: see if we are in an acceptable state to change output pins
			hr = S_OK;
			break;
		}		

		case MH_SYNTH_AUDIO_PORTS_CHANGE_END:
		{
			// TODO: see if we are in an acceptable state to change output pins
			hr = S_OK;
			break;
		}		

		default:
			// Unhandled message
			hr = E_FAIL;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IMfxInputCallback

HRESULT CMfxHostSite::OnEvent( IMfxInputPort* pPortFrom, const MfxEvent& mfxEvent )
{
	// NOTE: OnEvent is called from an MFX plugin that may be running under 
	// a different static module state than the application. 
	// Since this function potentially makes calls to MFC, we must reset the 
	// module state to the applications module state for the entire scope of this 
	// function, restoring it prior to returning. Failure to do this
	// will cause application windows to be not found in MFC's static handle map.
	// See TN058: MFC Module State Implementation
	
//	AFX_MANAGE_STATE( AfxGetAppModuleState() );

	// TODO: Record the incoming MfxEvent, since it contains automation data.

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
