// DxNullSource.h: interface for the CDxNullSourceXxx classes.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DXNULLSOURCE_H_
#define _DXNULLSOURCE_H_

//----------------------------------------------------------------------------
 
class CDxNullSource;

extern "C" const GUID CLSID_NullSource;

//----------------------------------------------------------------------------

class CDxNullSourceStream : public CBaseOutputPin
{
public:

	CDxNullSourceStream( HRESULT* phr, CDxNullSource* pParent, LPWAVEFORMATEX pwfx, long cbSrcBuf );

// CBaseOutputPin overrides
public:

	// Ask for buffers of the size appropriate to the agreed media type.
	HRESULT DecideBufferSize( IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties );

	// Set the agreed media type, and set up the necessary parameters
	// that depend on the media type.
	HRESULT CheckMediaType( const CMediaType* pmt );

	// Get the media type supplied by this source filter
	HRESULT GetMediaType( int iPosition, CMediaType* pmt );

	// Break our current connection
	HRESULT BreakConnect();

	// Overrides to use our custom allocator
	HRESULT DecideAllocator( IMemInputPin* pPin, IMemAllocator** ppAlloc );
	HRESULT InitAllocator( IMemAllocator** ppAlloc );

public:

	// Send a buffer downstream
	HRESULT PushBuffer( REFERENCE_TIME rt, long lSamples, BOOL bIsDiscontinuity );

	// Get the wave format currently being processed
	LPCWAVEFORMATEX GetWaveFormat() { return &m_wfx; }
	void SetWaveFormat( LPCWAVEFORMATEX pwfx ) { m_wfx = *pwfx; }

	// Let us know if we're fully and safely connected
	BOOL IsConnected() const { return m_bConnected; }
	void SetConnected( BOOL bConn ) { m_bConnected = TRUE; }

	CDxNullSource* GetNullSource() { return m_pNullSource; }

private:

	// Access to this state information should be serialized with the filters
	// critical section (m_pFilter->pStateLock())

	CCritSec				m_cSharedState;
	WAVEFORMATEX		m_wfx;				// Our current wave format
	long					m_cbSrcBuf;			// Desired source buffer length (bytes)
	BOOL					m_bConnected;		// TRUE when fully and safely connected
	CDxNullSource*		m_pNullSource;
};

//----------------------------------------------------------------------------

class CDxNullSource : public CBaseFilter
{
public:

	CDxNullSource( LPUNKNOWN lpunk, HRESULT* phr, LPWAVEFORMATEX pwfx, long cbSrcBuf );
	~CDxNullSource();
	
   CDxNullSourceStream* GetStream() { return m_pSrcStream; }
	CCritSec*			 	GetStateLock() { return &m_cStateLock; }

	DECLARE_IUNKNOWN;

	HRESULT SetAllocator( IMemAllocator* pma );
	HRESULT GetAllocator( IMemAllocator** ppma );

	HRESULT PushBuffer( REFERENCE_TIME rt, long lSamples, BOOL bIsDiscontinuity );

public:

	// CBaseFilter overrides
	int GetPinCount(void) { return 1; }
	CBasePin* GetPin( int n ) { return (0 == n) ? m_pSrcStream : NULL; }

private:

	CDxNullSourceStream*	m_pSrcStream;
   CCritSec					m_cStateLock;
	IMemAllocator*			m_pMemAllocator;
};

//----------------------------------------------------------------------------

#endif // _DXNULLSOURCE_H_
