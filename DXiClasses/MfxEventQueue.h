// MfxEventQueue.h: interface for the CMfxEventQueue class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXEVENTQUEUE_H__48EE0D26_A7BB_494F_A946_BD7FB6DC6BE4__INCLUDED_)
#define AFX_MFXEVENTQUEUE_H__48EE0D26_A7BB_494F_A946_BD7FB6DC6BE4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxTrack; // forward

class CMfxEventQueue : public IMfxEventQueue2
{
public:

	CMfxEventQueue( const CMfxTrack& trk, int ixFrom, int ixUpTo ) :
		m_trk(trk), m_ixFrom(ixFrom), m_ixUpTo(ixUpTo), m_cRef(0)
	{}

	virtual ~CMfxEventQueue() { ASSERT( 0 == m_cRef ); }

	// IUnknown
	STDMETHODIMP_(HRESULT) QueryInterface( REFIID riid, LPVOID* ppvObj );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMfxEventQueue
	STDMETHODIMP_(HRESULT) Add( const MfxEvent& event );
	STDMETHODIMP_(HRESULT) GetCount( int* pnCount );
	STDMETHODIMP_(HRESULT) GetAt( int ix, MfxEvent* pEvent );

	// IMfxEventQueue2
	STDMETHODIMP_(HRESULT) GetBufferFactory( IMfxBufferFactory** ppIMfxBufferFactory );

private:

	const CMfxTrack&	m_trk;
	int				m_ixFrom;
	int				m_ixUpTo;
	LONG				m_cRef;
};

#endif // !defined(AFX_MFXEVENTQUEUE_H__48EE0D26_A7BB_494F_A946_BD7FB6DC6BE4__INCLUDED_)
