// MfxBufferFactory.h: interface for the CMfxBufferFactory class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXBUFFERFACTORY_H__D111899F_F94F_4E87_B3F7_112DB20093D0__INCLUDED_)
#define AFX_MFXBUFFERFACTORY_H__D111899F_F94F_4E87_B3F7_112DB20093D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxBufferFactory : public IMfxBufferFactory
{
public:
	CMfxBufferFactory() : m_cRef(0) {}
	virtual ~CMfxBufferFactory() { ASSERT( 0 == m_cRef ); }

	// IUnknown
	STDMETHODIMP_(HRESULT) QueryInterface( REFIID riid, LPVOID* ppvObj );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMfxBufferFactory
	STDMETHODIMP_(HRESULT) Create( DWORD dwLength, MFX_HBUFFER* phbufNew );
	STDMETHODIMP_(HRESULT) CreateCopy( MFX_HBUFFER hbufOld, DWORD dwLength, MFX_HBUFFER* phbufNew );
	STDMETHODIMP_(HRESULT) GetPointer( MFX_HBUFFER hbuf, void** ppvData, DWORD* pdwLength );

// Operations
public:
	HRESULT Destroy( MFX_HBUFFER hbuf );

private:
	static BOOL isValid( MFX_HBUFFER hbuf );

private:
	LONG m_cRef;
};

extern CMfxBufferFactory theBufferFactory;

#endif // !defined(AFX_MFXBUFFERFACTORY_H__D111899F_F94F_4E87_B3F7_112DB20093D0__INCLUDED_)
