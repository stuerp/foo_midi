// MfxEventQueue.cpp: implementation of the CMfxEventQueue class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxEventQueue.h"
#include "MfxBufferFactory.h"
#include "MfxTrack.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// IUnknown

HRESULT CMfxEventQueue::QueryInterface( REFIID riid, LPVOID* ppv )
{
	if (NULL == ppv || IsBadWritePtr( ppv, sizeof(LPVOID) ))
		return E_POINTER;

	if (IID_IUnknown == riid)
		*ppv = static_cast<IUnknown*>(this);
	else if (IID_IMfxEventQueue == riid)
		*ppv = static_cast<IMfxEventQueue*>(this);
	else if (IID_IMfxEventQueue2 == riid)
		*ppv = static_cast<IMfxEventQueue2*>(this);
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

ULONG CMfxEventQueue::AddRef()
{
	return InterlockedIncrement( &m_cRef );
}

ULONG CMfxEventQueue::Release()
{
	ASSERT( 0 < m_cRef );
	return InterlockedDecrement( &m_cRef );
}


////////////////////////////////////////////////////////////////////////////////
// IMfxEventQueue

HRESULT CMfxEventQueue::Add( const MfxEvent& event )
{
	// Event queues given to DXi's are read-only
	return E_FAIL;
}

HRESULT CMfxEventQueue::GetCount( int* pnCount )
{
	if (NULL == pnCount)
		return E_POINTER;

	*pnCount = (m_ixUpTo - m_ixFrom);
	return S_OK;
}

HRESULT CMfxEventQueue::GetAt( int ix, MfxEvent* pEvent )
{
	if (NULL == pEvent)
		return E_POINTER;
	if (ix < 0 || ix >= m_ixUpTo - m_ixFrom)
		return E_INVALIDARG;

	*pEvent = m_trk[ m_ixFrom + ix ];

	if (-1 != m_trk.GetChannel())
		pEvent->m_byChan = m_trk.GetChannel();

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IMfxEventQueue2

HRESULT CMfxEventQueue::GetBufferFactory( IMfxBufferFactory** ppIMfxBufferFactory )
{
	if (NULL == ppIMfxBufferFactory)
		return E_POINTER;
	*ppIMfxBufferFactory = &theBufferFactory;
	theBufferFactory.AddRef();
	return S_OK;
}
