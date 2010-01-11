// MfxDataQueue.h: interface for the CMfxDataQueue class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXDATAQUEUE_H__9B2E4694_D7DC_433D_BDD1_5296610F7A67__INCLUDED_)
#define AFX_MFXDATAQUEUE_H__9B2E4694_D7DC_433D_BDD1_5296610F7A67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxDataQueue : public IMfxDataQueue
{
public:

	static CMfxDataQueue* Create();

	// IUnknown
	STDMETHODIMP_(HRESULT) QueryInterface( REFIID riid, LPVOID* ppvObj );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMfxDataQueue
	STDMETHODIMP_(HRESULT) Add( const MfxData& data );
	STDMETHODIMP_(HRESULT) GetCount( int* pnCount );
	STDMETHODIMP_(HRESULT) GetAt( int ix, MfxData* pData );

// Operations
public:

	HRESULT Reset();
	BOOL IsEmpty() const { return 0 == m_cData; }

	MfxData& operator[]( int ix )
	{
		ASSERT( ix >= 0 && ix < m_cData );
		return m_aData[ ix ];
	}

private:

	enum { MAX_ELEMENTS = 512 };

	MfxData				m_aData[ MAX_ELEMENTS ];
	int					m_cData;
	LONG					m_cRef;

private:
	CMfxDataQueue() : m_cRef(0), m_cData(0) {}
	virtual ~CMfxDataQueue() { ASSERT( 0 == m_cRef ); }
};

#endif // !defined(AFX_MFXDATAQUEUE_H__9B2E4694_D7DC_433D_BDD1_5296610F7A67__INCLUDED_)
