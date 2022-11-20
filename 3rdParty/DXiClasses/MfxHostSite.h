// MfxHostSite.h: interface for the CMfxHostSite class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXHOSTSITE_H__596C3488_AF78_4305_B532_303B4256B930__INCLUDED_)
#define AFX_MFXHOSTSITE_H__596C3488_AF78_4305_B532_303B4256B930__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxSeq;

class CMfxHostSite :
	public IMfxTempoMap,
	public IMfxMeterMap,
	public IMfxKeySigMap,
	public IMfxMarkerMap,
	public IMfxInputPulse,
	public IMfxNotifyHost,
	public IMfxInputCallback,
	public IMfxTimeConverter
{
public:

	static HRESULT Create( CMfxHostSite** ppObj );

	CMfxSeq* GetSeq() { return m_pSeq; }
	void SetSeq( CMfxSeq* pSeq ) { m_pSeq = pSeq; }

	// IUnknown
	STDMETHODIMP_(HRESULT) QueryInterface( REFIID riid, LPVOID* ppvObj );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMfxTempoMap
	STDMETHODIMP_(LONG) TicksToMsecs( LONG lTicks );
	STDMETHODIMP_(LONG) MsecsToTicks( LONG lMsecs );
	STDMETHODIMP_(int) GetTicksPerQuarterNote();
	STDMETHODIMP_(int) GetTempoIndexForTime( LONG lTicks );
	STDMETHODIMP_(int) GetTempoCount();
	STDMETHODIMP_(HRESULT) GetTempoAt( int ix, LONG* plTicks, int* pnBPM100 );

	// IMfxTimeConverter
	STDMETHODIMP_(HRESULT) ConvertMfxTime( MFX_TIME* pTime, MFX_TIME_FORMAT newFormat );

	// IMfxMeterMap
	STDMETHODIMP_(HRESULT) TicksToMBT( LONG lTicks, int* pnMeasure, int* pnBeat, int* pnTicks );
	STDMETHODIMP_(LONG) MBTToTicks( int nMeasure, int nBeat, int nTicks );
	STDMETHODIMP_(int) GetMeterIndexForTime( LONG lTicks );
	STDMETHODIMP_(int) GetMeterCount();
	STDMETHODIMP_(HRESULT) GetMeterAt( int ix, int* pnMeasure, int* pnTop, EBeatValue* peBottom );

	// IMfxKeySigMap
	STDMETHODIMP_(int) GetKeySigIndexForTime( LONG lTicks );
	STDMETHODIMP_(int) GetKeySigCount();
	STDMETHODIMP_(HRESULT) GetKeySigAt( int ix, int* pnMeasure, int* pnKeySig );

	// IMfxMarkerMap
 	STDMETHODIMP_(int) GetMarkerIndexForTime( LONG lTicks );
	STDMETHODIMP_(int) GetMarkerCount();
	STDMETHODIMP_(HRESULT) GetMarkerAt( int ix, LONG* plTime, IMfxMarkerMap::EUnits* peUnits, wchar_t** ppwszName );

	// IMfxInputPulse
	enum { PULSE_INTERVAL_MSEC = 100 };
	STDMETHODIMP GetPulseInterval( LONG* plIntervalMsec );
	STDMETHODIMP BeginPulse();
	STDMETHODIMP EndPulse();

	// IMfxNotifyHost
	STDMETHODIMP OnMfxNotifyHost( UINT uMsg, IUnknown* pUnkFrom, LPARAM lParam );

	// IMfxInputCallback
	STDMETHODIMP OnEvent( IMfxInputPort* pPortFrom, const MfxEvent& mfxEvent );


	STDMETHODIMP_(unsigned) GetSampleRate() { return m_uSampleRate; }
	STDMETHODIMP SetSampleRate(unsigned rate) { m_uSampleRate = rate; return S_OK; }

private:

	LONG		m_cRef;
	int		m_nPulseRequests;
	CMfxSeq*	m_pSeq;

	unsigned m_uSampleRate;

private:

	// Ctor/dtor private, to allow COM AddRef/Release.
	CMfxHostSite();
	~CMfxHostSite();
};

#endif // !defined(AFX_MFXHOSTSITE_H__596C3488_AF78_4305_B532_303B4256B930__INCLUDED_)
