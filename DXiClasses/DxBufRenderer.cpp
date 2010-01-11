// DxBufRenderer.cpp - Implementation of CDxBufRenderer filter and pin.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "DxAllocator.h"
#include "DxBufRenderer.h"

//DEFINE_CUSTOM_NEW( CDxBufRenderer );

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//----------------------------------------------------------------------------
// CLSIDs implemented by this module
// 	CLSID_BufRender

extern "C" const GUID CLSID_BufRenderer =
	{ 0xd8b2a4c3, 0xd817, 0x489e, 0xa6, 0xb0, 0xe4, 0xae, 0x8b, 0xdb, 0xf2, 0xb2 };


//****************************************************************************
// CDxBufRenderer - An ActiveMovie renderer which writes audio samples to a
//	buffer that can be read asynchronously.
//****************************************************************************

CDxBufRenderer::CDxBufRenderer( LPUNKNOWN pUnk, HRESULT* phr, long cbDstBuf ) :
	CBaseFilter( NAME("BufRenderer"), NULL, &m_cs, CLSID_BufRenderer, phr ),
	m_pms( NULL )
{
	m_pPin = new CDxBufRendererInputPin( this, phr, L"In" );
	if (NULL == m_pPin)
		*phr = E_OUTOFMEMORY;
}

CDxBufRenderer::~CDxBufRenderer()
{
	ReleaseMediaSample();
	SAFE_DELETE( m_pPin );
}


//----------------------------------------------------------------------------
// Remember the most recent media sample we were given

void CDxBufRenderer::setMediaSample(IMediaSample* pms)
{
	if (m_pms != pms)
	{
		if (m_pms != NULL)
			m_pms->Release();
		m_pms = pms;
		if (m_pms != NULL)
			m_pms->AddRef();
	}
}


//------------------------------------------------------------------------------
// Gets the most recently rendered output buffer

HRESULT CDxBufRenderer::GetOutputData( float** ppfBuf, int* pnSamples )
{
	if (NULL == m_pms)
		return E_FAIL;

	// Get a pointer to the buffer
	m_pms->GetPointer( (BYTE**)ppfBuf );

	// Get the number of samples filled
	*pnSamples = m_pms->GetActualDataLength() / m_wfx.nBlockAlign;

	return S_OK;
}


//------------------------------------------------------------------------------
// Free any allocated memory

HRESULT CDxBufRenderer::ReleaseMediaSample()
{
	HRESULT hr = (m_pms == NULL) ? S_FALSE : S_OK;
	setMediaSample(NULL);
	return hr;
}


//------------------------------------------------------------------------------

CBasePin* CDxBufRenderer::GetPin(int n)
{
	CAutoLock cRendererLock(&m_cs);

	// Should only ever be called with zero
	if (0 != n)
		return NULL;
	else
		return m_pPin;
}


//****************************************************************************
// CDxBufRendererInputPin - used by CDxBufRenderer, uses our custom allocator.
//	buffer that can be read asynchronously.
//****************************************************************************

CDxBufRendererInputPin::CDxBufRendererInputPin( CDxBufRenderer* pRenderer, HRESULT* phr, LPCWSTR wszName ) :
	CBaseInputPin( NAME("Audio renderer"), pRenderer, &pRenderer->m_cs, phr, wszName ),
	m_pRenderer( pRenderer ),
	m_pAllocator( NULL )
{
}

CDxBufRendererInputPin::~CDxBufRendererInputPin()
{
	SAFE_RELEASE( m_pAllocator );
}


//------------------------------------------------------------------------------
// Check if we allow a given media type

HRESULT CDxBufRendererInputPin::CheckMediaType( const CMediaType* pmt )
{
	LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX) pmt->Format();

	if (pwfx == NULL)
		return E_INVALIDARG;

	if (*(pmt->Type()) != MEDIATYPE_Audio)	
		return E_INVALIDARG;	// must be audio

	if (*(pmt->FormatType()) != FORMAT_WaveFormatEx)
		return E_INVALIDARG; // must be WAVEFORMATEX

	if (pmt->IsTemporalCompressed())
		return E_INVALIDARG; // must not be compressed

	if (!pmt->IsFixedSize())
		return E_INVALIDARG; // must be fixed size

	if (WAVE_FORMAT_IEEE_FLOAT != pwfx->wFormatTag)
		return E_INVALIDARG; // must be floating point

	if (32 != pwfx->wBitsPerSample)
		return E_INVALIDARG; // bit depth must be 32

	if (2 != pwfx->nChannels && 1 != pwfx->nChannels)
		return E_INVALIDARG;	// must be stereo or mono

	return S_OK;
}


//------------------------------------------------------------------------------
// Set our media type

HRESULT CDxBufRendererInputPin::SetMediaType( const CMediaType* pmt )
{
	// Remember the input wave format
	m_pRenderer->setWaveFormat( LPCWAVEFORMATEX(pmt->Format()) );
	return S_OK;
}


//------------------------------------------------------------------------------
// Get our custom allocator, it is available

HRESULT CDxBufRendererInputPin::GetAllocator( IMemAllocator** ppAllocator )
{
	CAutoLock cObjectLock(m_pLock);

	if (NULL == ppAllocator)
		return E_POINTER;

	if (m_pAllocator)
	{
		*ppAllocator = m_pAllocator;
		(*ppAllocator)->AddRef();
		return S_OK;
	}

	// No custom allocator was given to us.  Use the base class' allocator.
	return CBaseInputPin::GetAllocator( ppAllocator );
}


//------------------------------------------------------------------------------
// Set our custom allocator

HRESULT CDxBufRendererInputPin::SetAllocator( IMemAllocator* pma )
{
	if (pma != m_pAllocator)
	{
		if (pma)
			pma->AddRef();
		if (m_pAllocator)
			m_pAllocator->Release();
		m_pAllocator = pma;
	}
	return S_OK;
}


//------------------------------------------------------------------------------
// Receive a media sample.  All we do is pass it on to our parent filter.

HRESULT CDxBufRendererInputPin::Receive( IMediaSample* pMediaSample )
{
	CAutoLock cObjectLock(m_pLock);

	if (NULL == pMediaSample)
		return S_FALSE;

	m_pRenderer->setMediaSample( pMediaSample );
	return S_OK;
}

