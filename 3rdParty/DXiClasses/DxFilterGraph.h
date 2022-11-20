// DxFilterGraph.h: interface for the CDxFilterGraph class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DXFILTERGRAPH_H_
#define _DXFILTERGRAPH_H_

class CCritSec;
class CMediaType;

class CDxNullSource;
class CDxBufRenderer;
class CDxAllocator;

interface IMediaControl;

//------------------------------------------------------------------------------

class CDxFilterGraph
{
public:

	CDxFilterGraph( HRESULT* phr );
	virtual ~CDxFilterGraph();

public:

	// Create all the elements we need for the graph, and connect them
	HRESULT CreateGraph();

	// Disconnect all elements from the graph, and delete them
	HRESULT DestroyGraph();

	// Set/get the streaming buffer size
	HRESULT SetBufferSize( int nSamplesPerBuf );
	int GetBufferSize() const { return m_nSamplesPerBuf; }

	// Set/get the sample rate
	HRESULT SetSampleRate( long lSampleRate );
	long GetSampleRate() const { return m_lSampleRate; }

	// Set/get the DirectX filter we will stream through
	HRESULT SetFilter( IBaseFilter* pFilter );
	HRESULT GetFilter( IBaseFilter** ppFilter );

	// Returns the number of output pins available on the connected filter (DXi)
	HRESULT GetOutPinCount( int* pnRetVal );

	// Transport control
	HRESULT Pause();
	HRESULT Play();
	HRESULT Stop();
	HRESULT SetPosition( LONGLONG llSamplePos );
	LONGLONG GetPosition() const { return m_llSamplePos; }

	// Process another buffer through the filter in the graph.  On return,
	// *ppfBuf will point to the processed output.
	HRESULT Process( float** ppfBuf, int cSamples );

	HRESULT ReleaseBuffers();

private:

	HRESULT				doCreateGraph();
	CDxBufRenderer*	getRenderer() { return m_pRenderer; }
	IPin*					getOutPin(int nOutPin);
	IPin*					getInPin();
	HRESULT				fillFloatMediaType( CMediaType* pmt, int nChannels );

	CCritSec*			m_pcs;
	BOOL					m_bIsConnected;
	IBaseFilter*		m_pFilter;
	IGraphBuilder*		m_pGraphBuilder;
	IMediaControl*		m_pMediaControl;
	int					m_nSamplesPerBuf;
	long					m_lSampleRate;
	CDxNullSource*		m_pSource;
	CDxBufRenderer*	m_pRenderer;
	WAVEFORMATEX		m_wfx;
	CDxAllocator*		m_pMemAllocator;
	LONGLONG				m_llSamplePos;
	BOOL					m_bDiscontinuity;
};

//------------------------------------------------------------------------------

#endif // _DXFILTERGRAPH_H_
