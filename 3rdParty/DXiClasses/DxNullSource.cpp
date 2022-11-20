// DxNullSource.cpp - Implementation of CDxNullSource filter and pin
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "DxNullSource.h"
#include "DxAllocator.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


EXTERN_C const GUID CLSID_NullSource =
	{ 0xD8b2a4c4, 0xd817, 0x489e, 0xa6, 0xb0, 0xe4, 0xae, 0x8b, 0xdb, 0xf5, 0xb3 };


//****************************************************************************
// CDxNullSource - A filter that streams null (silent) media samples.
//****************************************************************************

CDxNullSource::CDxNullSource( LPUNKNOWN lpunk, HRESULT* phr, LPWAVEFORMATEX pwfx, long cbSrcBuf ) :
	CBaseFilter( "CDxNullSource", lpunk, &m_cStateLock, CLSID_NullSource, phr ),
	m_pMemAllocator(NULL)
{
	m_pSrcStream = new CDxNullSourceStream( phr, this, pwfx, cbSrcBuf );
	if (NULL == m_pSrcStream)
		*phr = E_OUTOFMEMORY;
}

CDxNullSource::~CDxNullSource()
{
	SAFE_DELETE( m_pSrcStream );
	SAFE_RELEASE( m_pMemAllocator );
}

//------------------------------------------------------------------------------

HRESULT CDxNullSource::PushBuffer( REFERENCE_TIME rt, long lSamples, BOOL bIsDiscontinuity )
{
	return m_pSrcStream->PushBuffer( rt, lSamples, bIsDiscontinuity );
}

//------------------------------------------------------------------------------

HRESULT CDxNullSource::SetAllocator( IMemAllocator* pma )
{
	IDeferZeroFillAllocator* pama = NULL;
	if (S_OK == pma->QueryInterface( IID_IDeferZeroFillAllocator, (void**)&pama ))
	{
		if (m_pMemAllocator)
			m_pMemAllocator->Release();
		m_pMemAllocator = pma;
		return S_OK;
	}
	else
		return S_FALSE;
}

HRESULT CDxNullSource::GetAllocator( IMemAllocator** ppma )
{
	if (NULL == ppma)
		return E_POINTER;

	if (m_pMemAllocator)
		m_pMemAllocator->AddRef();
	*ppma = m_pMemAllocator;
	return S_OK;
}

//****************************************************************************
// CDxNullSourceStream - The output pin (or "source stream" in DX lingo) that
//		actually spews out the samples.
//****************************************************************************

CDxNullSourceStream::CDxNullSourceStream( HRESULT* phr, CDxNullSource* pParent, LPWAVEFORMATEX pwfx, long cbSrcBuf ) :
	CBaseOutputPin( "CDxNullSourceStream", pParent, &m_cSharedState, phr, L"Plug-in src" ),
	m_wfx(*pwfx),
	m_bConnected(FALSE),
	m_cbSrcBuf(cbSrcBuf),
	m_pNullSource(pParent)
{
	ASSERT( 32 == m_wfx.wBitsPerSample );
	if (32 != m_wfx.wBitsPerSample)
		*phr = E_INVALIDARG;
}


//----------------------------------------------------------------------------
// HACK ALERT: ActiveMovie seems to have a bug where it keeps one extra 
// reference count on allocators it creates itself.  Work around this bug
// by explicitly releasing our allocator before we disconnect.

HRESULT CDxNullSourceStream::BreakConnect()
{
	// Do the extra release first, if necessary
	if (IsConnected() && m_pAllocator)
	{
		int nCount = m_pAllocator->AddRef();
		if (nCount > 2) // if there was no bug, count would be 1 here
			m_pAllocator->Release();
		m_pAllocator->Release();
	}

	// Let the base class do the _real_ work
	return CBaseOutputPin::BreakConnect();
}


//----------------------------------------------------------------------------
// Check if allow a given media type.

HRESULT CDxNullSourceStream::CheckMediaType( const CMediaType* pmt )
{
	if (pmt == NULL)
		return E_INVALIDARG;

	LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX) pmt->Format();

	if (pwfx == NULL)
		return E_INVALIDARG;

	if (*(pmt->Type()) != MEDIATYPE_Audio)	// must be audio
		return E_INVALIDARG;
	if (pmt->IsTemporalCompressed())			// must not be compressed
		return E_INVALIDARG;
	if (!pmt->IsFixedSize())					// must be fixed size
		return E_INVALIDARG;

	// Must be 32-bit float, mono or stereo
	if (!(WAVE_FORMAT_IEEE_FLOAT == pwfx->wFormatTag && 32 == pwfx->wBitsPerSample && (2 == pwfx->nChannels || 1 == pwfx->nChannels)))
		return E_INVALIDARG;

	return S_OK;
}


//----------------------------------------------------------------------------
// Send a data buffer downstream, right away.

HRESULT CDxNullSourceStream::PushBuffer( REFERENCE_TIME rt, long lSamples, BOOL bIsDiscontinuity )
{
	CAutoLock	lShared(&m_cSharedState);
	CAutoLock	lObject(m_pLock);
	HRESULT		hr;
	BYTE*			pData;

	// Get a media sample to push downstream
	IMediaSample* pms;
	CHECK( GetDeliveryBuffer( &pms, NULL, NULL, 0) );

	pms->SetTime( &rt, NULL );

	// Set the discontinuty flag on the first buffer
	pms->SetDiscontinuity( bIsDiscontinuity );

	// Get a pointer to the buffer to be filled
	pms->GetPointer( &pData );

	// Calculate the final data length in bytes
	long cBytes = lSamples * m_wfx.nBlockAlign;
	DeferZeroFill( pms ); // we assume stereo

	// Set the final actual data length
	hr = pms->SetActualDataLength( cBytes );
	ASSERT( SUCCEEDED( hr ) );
	CHECK( hr );

	// Push it downstream
	hr = Deliver( pms );

	// As required by ActiveMovie, we must release the media sample obtained
	// by calling GetDeliveryBuffer()
	pms->Release();
	return hr;
}


//----------------------------------------------------------------------------
// We only support the media type being supplied by source of the audio data.

HRESULT CDxNullSourceStream::GetMediaType( int iPosition, CMediaType* pmt )
{
	CAutoLock l( static_cast<CDxNullSource*>(m_pFilter)->GetStateLock() );

	// Validate argument
	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	// Populate the media type structure
	pmt->SetType( &MEDIATYPE_Audio );
	pmt->SetSubtype( &MEDIASUBTYPE_PCM );
	pmt->SetFormatType( &FORMAT_WaveFormatEx );
	pmt->SetTemporalCompression( FALSE );

	// Fill a wave format structure
	LPWAVEFORMATEX	pwfxDst = (LPWAVEFORMATEX) pmt->AllocFormatBuffer( sizeof(m_wfx) );
	*pwfxDst = m_wfx;

	return S_OK;
}


//----------------------------------------------------------------------------
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.

HRESULT CDxNullSourceStream::DecideBufferSize( IMemAllocator* pAlloc,
														     ALLOCATOR_PROPERTIES* pProperties )
{
	CAutoLock l( static_cast<CDxNullSource*>(m_pFilter)->GetStateLock() );

	const UINT cBuffers = 2;

	ASSERT( pAlloc );
	ASSERT( pProperties );
	HRESULT hr = S_OK;

	// Make a few buffers
	pProperties->cbBuffer = m_cbSrcBuf;
	pProperties->cBuffers = cBuffers;

	// Ask the allocator to reserve us the memory
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties( pProperties, &Actual );
	if (FAILED(hr))
		return hr;

	// Is this allocator unsuitable?
	if (Actual.cbBuffer < pProperties->cbBuffer)
		return E_FAIL;

	ASSERT( Actual.cBuffers >= cBuffers );
	return S_OK;
}


//--------------------------------------------------------------------------------
// This code was derived from the CBaseOutputPin::DecideAllocator method
// We override this member function to use our custom allocator
// If the input pin fails the GetAllocator call then this will construct a CDxAllocator
// and call DecideBufferSize on that, and if that fails then we are completely hosed.
//	NOTE this is called during Connect() which therefore looks after grabbing and locking 
// the object's critical section


HRESULT CDxNullSourceStream::DecideAllocator( IMemInputPin* pPin, IMemAllocator** ppAlloc )
{
	HRESULT hr = S_OK;
	*ppAlloc = NULL;

	ALLOCATOR_PROPERTIES prop;
	ZeroMemory(&prop, sizeof(prop));

	// Get downstream prop request.
	// The derived class may modify this in DecideBufferSize, but
	// we assume that he will consistently modify it the same way,
	// so we only get it once

	// whatever he returns, we assume prop is either all zeros
	// or he has filled it out.
	pPin->GetAllocatorRequirements(&prop);

	// if he doesn't care about alignment, then set it to 1
	if (prop.cbAlign == 0) 
		prop.cbAlign = 1;

	// Try the allocator provided by the input pin
	hr = pPin->GetAllocator( ppAlloc );

	if (SUCCEEDED( hr )) 
	{
		CDxAllocator* pCustomAlloc = NULL;

		// Is ppAlloc our custom CDxAllocator?
		if (IsDeferZeroFillAllocator( *ppAlloc, &pCustomAlloc ))
		{
			// Let our output pin decide on the buffer size
			hr = DecideBufferSize( pCustomAlloc, &prop );
			if (SUCCEEDED( hr ))
			{
				hr = pPin->NotifyAllocator( pCustomAlloc, FALSE );
				if (SUCCEEDED( hr )) 
				{
					return S_OK;
				}
			}
		}
	}

	// We were not successful in getting our custom allocator from the input pin's allocator,
	// so its best to create our own DeferZeroFillAllocator

	// Clean up if something failed
	SAFE_RELEASE( *ppAlloc );

	// Let our output pin build its own allocator
	hr = InitAllocator(ppAlloc);

	if ( SUCCEEDED(hr) ) 
	{
		// Let our output pin decide on the buffer size
		hr = DecideBufferSize(*ppAlloc, &prop);
		if (SUCCEEDED(hr)) 
		{
			hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
			if (SUCCEEDED(hr)) 
				return S_OK;
		}
	}

	// Clean up if something failed
	SAFE_RELEASE( *ppAlloc );

	return hr;
}


//------------------------------------------------------------------------------
// Override to use our custom allocator

HRESULT CDxNullSourceStream::InitAllocator( IMemAllocator** ppAlloc )
{
	return GetNullSource()->GetAllocator( ppAlloc );
}

