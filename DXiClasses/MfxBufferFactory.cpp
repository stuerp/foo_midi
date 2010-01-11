// MfxBufferFactory.cpp: implementation of the MfxBufferFactory class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxBufferFactory.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

CMfxBufferFactory theBufferFactory;

/////////////////////////////////////////////////////////////////////////////
// IUnknown

HRESULT CMfxBufferFactory::QueryInterface( REFIID riid, LPVOID* ppv )
{
	if (NULL == ppv || IsBadWritePtr( ppv, sizeof(LPVOID) ))
		return E_POINTER;

	if (IID_IUnknown == riid)
		*ppv = static_cast<IUnknown*>(this);
	else if (IID_IMfxBufferFactory == riid)
		*ppv = static_cast<IMfxBufferFactory*>(this);
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

ULONG CMfxBufferFactory::AddRef()
{
	return InterlockedIncrement( &m_cRef );
}

ULONG CMfxBufferFactory::Release()
{
	ASSERT( 0 < m_cRef );
	InterlockedDecrement( &m_cRef );
	return m_cRef;
}


////////////////////////////////////////////////////////////////////////////////
// IMfxBufferFactory

static const char MFX_TAG[4] = { 'M', 'f', 'x', 'B' };

struct MfxMemBufHdr
{
	char	szTag[ sizeof(MFX_TAG) ];
	DWORD	cb;
	BYTE	byData[ 1 ];
};

////////////////////////////////////////////////////////////////////////////////

HRESULT CMfxBufferFactory::Create( DWORD dwLength, MFX_HBUFFER* phbufNew )
{
	if (NULL == phbufNew)
		return E_POINTER;

	// Allocate the buffer
	*phbufNew = NULL;
	MfxMemBufHdr* pBuf = (MfxMemBufHdr*) malloc( sizeof(MfxMemBufHdr) + dwLength );
	if (NULL == pBuf)
		return E_OUTOFMEMORY;

	// Populate the header
	memcpy( pBuf->szTag, MFX_TAG, sizeof(MFX_TAG) );
	pBuf->cb = dwLength;

	// Return the pointer
	*phbufNew = (MFX_HBUFFER)pBuf;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CMfxBufferFactory::CreateCopy( MFX_HBUFFER hbufOld, DWORD dwLength, MFX_HBUFFER* phbufNew )
{
	void*	pvSrc;
	void* pvDst;
	DWORD	cbSrc, cbDst;

	CHECK( GetPointer( hbufOld, &pvSrc, &cbSrc ) );
	CHECK( Create( dwLength, phbufNew ) );
	CHECK( GetPointer( *phbufNew, &pvDst, &cbDst ) );

	memcpy( pvDst, pvSrc, min( cbSrc, cbDst ) );

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CMfxBufferFactory::GetPointer( MFX_HBUFFER hbuf, void** ppvData, DWORD* pdwLength )
{
	if (NULL == ppvData || NULL == pdwLength)
		return E_POINTER;
	if (!isValid( hbuf ))
		return E_INVALIDARG;

	MfxMemBufHdr* pBuf = (MfxMemBufHdr*)hbuf;
	*ppvData = (void*) pBuf->byData;
	*pdwLength = pBuf->cb;
	return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// Destroy (free) an MFX_HBUFFER

HRESULT CMfxBufferFactory::Destroy( MFX_HBUFFER hbuf )
{
	if (!isValid( hbuf ))
		return E_INVALIDARG;
	memset( ((MfxMemBufHdr*)hbuf)->szTag, 0xDD, sizeof(MFX_TAG) );
	free( hbuf );
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

BOOL CMfxBufferFactory::isValid( MFX_HBUFFER hbuf ) // static
{
	if (NULL == hbuf || IsBadReadPtr( (void*)hbuf, sizeof(MfxMemBufHdr) ))
		return FALSE;
	if (0 != memcmp( MFX_TAG, ((MfxMemBufHdr*)hbuf)->szTag, sizeof(MFX_TAG) ))
		return FALSE;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
