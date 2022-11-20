// DxFilterGraph.cpp - Implementation of CDxFilterGraph class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "DxFilterGraph.h"
#include "DxNullSource.h"
#include "DxBufRenderer.h"
#include "DxAllocator.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//------------------------------------------------------------------------------
// Ctor

CDxFilterGraph::CDxFilterGraph( HRESULT* phr ) :
	m_bIsConnected(FALSE),
	m_pGraphBuilder(NULL),
	m_pMediaControl(NULL),
	m_nSamplesPerBuf(441),
	m_lSampleRate(44100),
	m_pSource(NULL),
	m_pRenderer(NULL),
	m_pFilter(NULL),
	m_pMemAllocator(NULL),
	m_llSamplePos(0),
	m_bDiscontinuity(FALSE)
{
	// Get a graph builder for the filter graph
	*phr = CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
								    IID_IGraphBuilder, (void**)&m_pGraphBuilder );

	// Allocate our critical section
	if (SUCCEEDED( *phr ))
	{
		m_pcs = new CCritSec;
		if (NULL == m_pcs)
			*phr = E_OUTOFMEMORY;
	}

	// Create an allocator for the filter graph
	if (SUCCEEDED( *phr ))
	{
		*phr = CDxAllocator::Create( &m_pMemAllocator );
	}

	// Get the IMediaControl for the graph
	if (SUCCEEDED( *phr ))
	{
		*phr = m_pGraphBuilder->QueryInterface( IID_IMediaControl, (void**)&m_pMediaControl );
	}
}


//------------------------------------------------------------------------------
// Dtor

CDxFilterGraph::~CDxFilterGraph()
{
	Stop();
	DestroyGraph();

	SAFE_RELEASE( m_pMediaControl );
	SAFE_RELEASE( m_pGraphBuilder );
	SAFE_RELEASE( m_pFilter );
	SAFE_RELEASE( m_pMemAllocator );

	SAFE_DELETE( m_pcs );
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::SetFilter( IBaseFilter* pFilter )
{
	if (pFilter == m_pFilter)
		return S_OK; // nothing to do

	DestroyGraph();

	if (m_pFilter)
		m_pFilter->Release();
	m_pFilter = pFilter;
	if (m_pFilter)
		m_pFilter->AddRef();

	if (m_pFilter)
		CHECK( CreateGraph() );

	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::GetOutPinCount( int* pnRetVal )
{
	CAutoLock lock(m_pcs);

	long count = 0;

	// For each pin...
	IEnumPins* pIEnumPins;
	CHECK( m_pFilter->EnumPins( &pIEnumPins ) );
	IPin* pIPin;
	ULONG	cPin = 0;
	while (S_OK == pIEnumPins->Next( 1, &pIPin, &cPin ))
	{
		PIN_INFO PinInfo;
		PinInfo.pFilter = NULL;

		HRESULT hr = pIPin->QueryPinInfo(&PinInfo);
		pIPin->Release();
		CHECK( hr );
		if (PinInfo.pFilter)
			PinInfo.pFilter->Release();

		if ( PINDIR_OUTPUT == PinInfo.dir )
			count++;
	}
	pIEnumPins->Release();
	*pnRetVal = count;
	return S_OK;
}

//------------------------------------------------------------------------------

IPin* CDxFilterGraph::getOutPin( int nOutPin )
{
	if (NULL == m_pFilter)
		return NULL; // no filter?  no pin.

	// Walk through all of the filter's pin until we find the right output pin
	long count = 0;

	// For each pin...
	IEnumPins* pIEnumPins;
	if (SUCCEEDED( m_pFilter->EnumPins( &pIEnumPins ) ))
	{
		IPin*	pIPin = NULL;
		ULONG	cPin = 0;
		while (S_OK == pIEnumPins->Next( 1, &pIPin, &cPin ))
		{
			// Get the next pin's info
			PIN_INFO PinInfo;
			PinInfo.pFilter = NULL;
			HRESULT hr = pIPin->QueryPinInfo( &PinInfo );
			if (SUCCEEDED( hr ))
			{
				if (PinInfo.pFilter)
					PinInfo.pFilter->Release();
				if (PINDIR_OUTPUT == PinInfo.dir)
				{
					// We found the output pin they want
					if (count == nOutPin)
					{
						pIEnumPins->Release();
						pIPin->Release();
						return pIPin;
					}

					// Tally up another output pin, and keep looking
					count++;
				}
			}
			pIPin->Release();
			if (FAILED( hr ))
				return NULL;
		}
		pIEnumPins->Release();
	}
	return NULL;
}

//------------------------------------------------------------------------------

IPin* CDxFilterGraph::getInPin()
{
	if (NULL == m_pFilter)
		return NULL; // no filter?  no pin.

	// Walk through all of the filter's pin until we find an input pin
	IEnumPins* pIEnumPins;
	if (SUCCEEDED( m_pFilter->EnumPins( &pIEnumPins ) ))
	{
		IPin*	pIPin = NULL;
		ULONG	cPin = 0;
		while (S_OK == pIEnumPins->Next( 1, &pIPin, &cPin ))
		{
			// Get info about this pin
			PIN_INFO PinInfo;
			PinInfo.pFilter = NULL;
			HRESULT hr = pIPin->QueryPinInfo( &PinInfo );
			if (SUCCEEDED( hr ))
			{
				if (PinInfo.pFilter)
					PinInfo.pFilter->Release();

				// If we found the input pin, return a non-AddRef pointer to it.
				if (PINDIR_INPUT == PinInfo.dir)
				{
					pIEnumPins->Release();
					pIPin->Release();
					return pIPin;
				}
			}
			pIPin->Release();
			if (FAILED( hr ))
				return NULL;
		}
		pIEnumPins->Release();
	}

	return NULL;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::SetBufferSize( int nSamplesPerBuf )
{
	if (nSamplesPerBuf <= 0)
		return E_INVALIDARG;
	if (nSamplesPerBuf == m_nSamplesPerBuf)
		return S_OK; // nothing to do

	CHECK( DestroyGraph() );
	m_nSamplesPerBuf = nSamplesPerBuf;
	CHECK( CreateGraph() );

	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::SetSampleRate( long lSampleRate )
{
	if (lSampleRate <= 0)
		return E_INVALIDARG;
	if (lSampleRate == m_lSampleRate)
		return S_OK; // nothing to do

	CHECK( DestroyGraph() );
	m_lSampleRate = lSampleRate;
	CHECK( CreateGraph() );

	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::CreateGraph()
{
	HRESULT hr = doCreateGraph();
	if (FAILED( hr ))
		DestroyGraph();
	return hr;
}

HRESULT CDxFilterGraph::doCreateGraph()
{
	CAutoLock lock(m_pcs);
	HRESULT hr = S_OK;

	if (m_bIsConnected)
		return S_OK;
	if (NULL == m_pGraphBuilder)
		return E_FAIL;

	ASSERT( NULL == m_pSource );
	ASSERT( NULL == m_pRenderer );
	ASSERT( NULL != m_pFilter );

	IPin* pOPin = NULL;
	IPin* pIPin = NULL;

	CMediaType mtSrc;
	CHECK( fillFloatMediaType( &mtSrc, 2 ) );
	WAVEFORMATEX* pwfxSrc = reinterpret_cast<WAVEFORMATEX*>( mtSrc.Format() );

	CMediaType mtDstStereo;
	//CMediaType mtDstMono;
	CHECK( fillFloatMediaType( &mtDstStereo, 2 ) );
	//CHECK( fillFloatMediaType( &mtDstMono, 1 ) );

	// Create the render filter
	m_pRenderer = new CDxBufRenderer( NULL, &hr, m_nSamplesPerBuf * 2 * sizeof(float) );
	CHECK( hr );
	m_pRenderer->AddRef();

	// Create the source filter
	m_pSource = new CDxNullSource( NULL, &hr, pwfxSrc, m_nSamplesPerBuf * 2 * sizeof(float) );
	CHECK( hr );
	m_pSource->AddRef();

	// Add the filters to the graph
	CHECK( m_pSource->SetAllocator( m_pMemAllocator ) );
	CHECK( m_pRenderer->SetAllocator( m_pMemAllocator ) );
	CHECK( m_pGraphBuilder->AddFilter( m_pSource, L"FilterGraph Src" ) );
	CHECK( m_pGraphBuilder->AddFilter( m_pFilter, L"FilterGraph Xform" ) );
	CHECK( m_pGraphBuilder->AddFilter( m_pRenderer, L"FilterGraph Dst" ) );

	// Connect the source to the filter
	pOPin = m_pSource->GetPin(0);
	pIPin = getInPin();
	CHECK( m_pGraphBuilder->ConnectDirect( pOPin, pIPin, &mtSrc ) );
		
	// Connect the filter output to the renderer
	pOPin = getOutPin( 0 );
	pIPin = m_pRenderer->GetPin( 0 );
	CHECK( m_pGraphBuilder->ConnectDirect( pOPin, pIPin, &mtDstStereo ) );
	/*hr = m_pGraphBuilder->ConnectDirect( pOPin, pIPin, &mtDstStereo );
	if (FAILED( hr ))
	{
		// Try mono if stereo failed
		CHECK( m_pGraphBuilder->ConnectDirect( pOPin, pIPin, &mtDstMono ) );
	}*/

	// Allocate memory
	CHECK( m_pMemAllocator->AllocateAll() );

	m_bIsConnected = TRUE;
	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::DestroyGraph()
{
	CAutoLock lock(m_pcs);

	HRESULT	hr = S_OK;
	IPin*		pPin = NULL;
	
	ReleaseBuffers();

	// Disconnect the source's output pin
	pPin = m_pSource ? m_pSource->GetPin( 0 ) : NULL;
	if (pPin)
		m_pGraphBuilder->Disconnect( pPin );

	// Disconnect the filter's input pin
	pPin = getInPin();
	if (pPin)
		m_pGraphBuilder->Disconnect( pPin );

	// Disconnect the filter's output pin
	pPin = getOutPin( 0 );
	if (pPin)
		m_pGraphBuilder->Disconnect( pPin );

	// Disconnect the renderer's input poin
	pPin = m_pRenderer ? m_pRenderer->GetPin( 0 ) : NULL;
	if (pPin)
		m_pGraphBuilder->Disconnect( pPin );

	// Remove filters from the graph
	if (m_pSource)
		m_pGraphBuilder->RemoveFilter( m_pSource );
	if (m_pFilter)
		m_pGraphBuilder->RemoveFilter( m_pFilter );
	if (m_pRenderer)
		m_pGraphBuilder->RemoveFilter( m_pRenderer );

	SAFE_RELEASE( m_pSource );
	SAFE_RELEASE( m_pRenderer );

	m_bIsConnected = FALSE;

	CHECK( m_pMemAllocator->FreeAll() );
	CHECK( m_pMemAllocator->ResetProperties( 0 ) );

	return hr;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::Pause()
{
	CAutoLock lock(m_pcs);

	if (NULL == m_pMediaControl)
		return E_FAIL;

	CHECK( m_pMediaControl->Pause() );
	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::Play()
{
	CAutoLock lock(m_pcs);

	if (NULL == m_pMediaControl)
		return E_FAIL;

	m_bDiscontinuity = TRUE;

	CHECK( m_pMediaControl->Run() );

	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::Stop()
{
	CAutoLock lock(m_pcs);

	if (NULL == m_pMediaControl)
		return E_FAIL;

	CHECK( m_pMediaControl->Stop() );
	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::SetPosition( LONGLONG llSamplePos )
{
	CAutoLock lock(m_pcs);

	m_llSamplePos = llSamplePos;
	m_bDiscontinuity = TRUE;

	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::Process( float** ppfBuf, int cSamples )
{
	CAutoLock lock(m_pcs);

	if (!m_bIsConnected || NULL == m_pRenderer)
		CHECK(E_FAIL);
	if (cSamples > m_nSamplesPerBuf)
		CHECK(E_INVALIDARG);
	if (cSamples <= 0)
		CHECK(E_INVALIDARG);

	// Convert current position to a reference time
	REFERENCE_TIME rt = (m_llSamplePos * 10000000) / GetSampleRate();

	// Push silence out of the source filter
	CHECK( m_pSource->PushBuffer( rt, cSamples, m_bDiscontinuity ) );

	// Update our position
	m_llSamplePos += cSamples;
	m_bDiscontinuity = FALSE;

	// Fetch the results from streaming
	int nActual = 0;
	CHECK( m_pRenderer->GetOutputData( ppfBuf, &nActual ) );
	ASSERT( nActual == cSamples );

	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::ReleaseBuffers()
{
	if (m_pRenderer)
		m_pRenderer->ReleaseMediaSample();
	return S_OK;
}

//------------------------------------------------------------------------------

HRESULT CDxFilterGraph::fillFloatMediaType( CMediaType* pmt, int nChannels )
{
	// Allocate wave format structure
	WAVEFORMATEX* pwfx = (WAVEFORMATEX*) pmt->AllocFormatBuffer( sizeof(*pwfx) );
	if (!pwfx)
		return E_OUTOFMEMORY;

	// Populate the structure
	pwfx->wFormatTag			= WAVE_FORMAT_IEEE_FLOAT;
	pwfx->wBitsPerSample		= 32;
	pwfx->nChannels			= static_cast<WORD>( nChannels );
	pwfx->nSamplesPerSec		= m_lSampleRate;
	pwfx->nBlockAlign			= sizeof(float) * pwfx->nChannels;
	pwfx->nAvgBytesPerSec	= pwfx->nSamplesPerSec * pwfx->nBlockAlign;
	pwfx->cbSize				= 0;

	// Set other CMediaType properties
	pmt->SetType( &MEDIATYPE_Audio );
	pmt->SetSubtype( &MEDIASUBTYPE_PCM );
	pmt->SetFormatType( &FORMAT_WaveFormatEx );
	pmt->SetTemporalCompression( FALSE );
	pmt->SetSampleSize( pwfx->nBlockAlign );

	return S_OK;
}
