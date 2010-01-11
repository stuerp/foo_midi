// DxBufRenderer.h: interface for the CDxBufRendererXxx classes.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DXBUFRENDER_H_
#define _DXBUFRENDER_H_

class CDxBufRenderer;
class CDxBufRendererInputPin;

//----------------------------------------------------------------------------

extern "C" const GUID CLSID_PluginBufRender;

//----------------------------------------------------------------------------

class CDxBufRendererInputPin : public CBaseInputPin
{
public:
	CDxBufRendererInputPin( CDxBufRenderer* pRenderer, HRESULT *phr, LPCWSTR wszName );
	virtual ~CDxBufRendererInputPin();

	// Renders the media sample we are given
	STDMETHODIMP Receive( IMediaSample* pMediaSample );

	// Delegate to filter that owns us
	HRESULT CheckMediaType( const CMediaType* pmt );
	HRESULT SetMediaType( const CMediaType* pmt );

	// Override to use our custom allocator
	STDMETHODIMP GetAllocator( IMemAllocator** ppAllocator );

	// Helper to set our custom allocator
	HRESULT SetAllocator( IMemAllocator* pAllocator );

	// Let pin access the filter graph, to get its allocator
	CDxBufRenderer* GetRenderer() { return m_pRenderer; }

private:
	CDxBufRenderer*	m_pRenderer;
	IMemAllocator*		m_pAllocator;
};

//----------------------------------------------------------------------------

class CDxBufRenderer : public CBaseFilter
{
	friend class CDxBufRendererInputPin;

public:

	CDxBufRenderer( LPUNKNOWN pUnk, HRESULT* phr, long cbDstBuf );
	virtual ~CDxBufRenderer();

	DECLARE_IUNKNOWN;

public:

	// Override to ensure we use our custom input pin
	virtual CBasePin* GetPin(int n);
	virtual int GetPinCount() { return 1; }

public:

	int GetRenderedCount() const // the number we got from DShow's last DoRenderSample Call
	{
		if (m_pms)
			return m_pms->GetActualDataLength() / m_wfx.nBlockAlign;
		else
			return 0;
	}

	HRESULT GetOutputData( float** pfBufDest, int* pnSamples );

	HRESULT ReleaseMediaSample();

	HRESULT SetAllocator( IMemAllocator* pma ) { return m_pPin->SetAllocator( pma ); }

private:

	void setMediaSample(IMediaSample* pms);
	void setWaveFormat( LPCWAVEFORMATEX pwfx ) { m_wfx = *pwfx; }

	WAVEFORMATEX				m_wfx;
	IMediaSample*				m_pms;
	CCritSec						m_cs;
	CDxBufRendererInputPin*	m_pPin;
};

//----------------------------------------------------------------------------

#endif // _DXBUFRENDER_H_
