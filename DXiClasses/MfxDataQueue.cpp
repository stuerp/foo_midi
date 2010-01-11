// MfxDataQueue.cpp: implementation of the CMfxDataQueue class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxDataQueue.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

CMfxDataQueue* CMfxDataQueue::Create()
{
	CMfxDataQueue* pObj = new CMfxDataQueue;
	if (pObj)
		pObj->AddRef();
	return pObj;
}


/////////////////////////////////////////////////////////////////////////////
// IUnknown

HRESULT CMfxDataQueue::QueryInterface( REFIID riid, LPVOID* ppv )
{
	if (NULL == ppv || IsBadWritePtr( ppv, sizeof(LPVOID) ))
		return E_POINTER;

	if (IID_IUnknown == riid)
		*ppv = static_cast<IUnknown*>(this);
	else if (IID_IMfxDataQueue == riid)
		*ppv = static_cast<IMfxDataQueue*>(this);
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

ULONG CMfxDataQueue::AddRef()
{
	return InterlockedIncrement( &m_cRef );
}

ULONG CMfxDataQueue::Release()
{
	ASSERT( 0 < m_cRef );
	if (0 == InterlockedDecrement( &m_cRef ))
	{
		delete this;
		return 0;
	}
	return m_cRef;
}


////////////////////////////////////////////////////////////////////////////
// IMfxDataQueue

HRESULT CMfxDataQueue::Add( const MfxData& data )
{
	if (m_cData == MAX_ELEMENTS)
		return E_FAIL;

	m_aData[ m_cData ] = data;
	m_cData++;
	return S_OK;
}

HRESULT CMfxDataQueue::GetCount( int* pnCount )
{
	if (NULL == pnCount)
		return E_POINTER;

	*pnCount = m_cData;
	return S_OK;
}

HRESULT CMfxDataQueue::GetAt( int ix, MfxData* pData )
{
	if (NULL == pData)
		return E_POINTER;
	if (ix < 0 || ix >= m_cData)
		return E_INVALIDARG;

	*pData = m_aData[ ix ];

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////

HRESULT CMfxDataQueue::Reset()
{
	m_cData = 0;
	return S_OK;
}
