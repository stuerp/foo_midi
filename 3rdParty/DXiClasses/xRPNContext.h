// xRPNContext.h: interface for the CxRPNContext class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XRPNCONTEXT_H__B0DB7AF8_C212_4EC3_9441_6CEB9882E85D__INCLUDED_)
#define AFX_XRPNCONTEXT_H__B0DB7AF8_C212_4EC3_9441_6CEB9882E85D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxEvent; // forward
class CMfxTrack; // forward

class CxRPNContext
{
public:
// Ctors
	CxRPNContext();
	~CxRPNContext();

// Operations
	void Reset();

	enum Action
	{
		None,		// Do nothing
		Add,		// pevtNew is a new CMfxEvent to add
		Replace	// pevtPrev was modified and should be replaced
	};

	Action OnController(	LONG t, int nChan, int nNum, int nVal,	CMfxEvent* pevtPrev, CMfxEvent* pevtNew );

	// Variation which works on a track
	BOOL OnController( LONG t, int nChan, int nNum, int nVal, CMfxTrack* pTrk, int ixEvt );

// Implementation
private:
	class CxRPNChannelContext
	{
		// NOTE: This class covers only a *single* MIDI channel.
	public:
	// Ctors
		CxRPNChannelContext() { Reset(); }

	// Operations
		void Reset();
		Action OnController(	LONG t, int nChan, int nNum, int nVal,	CMfxEvent* pevtPrev, CMfxEvent* pevtNew );
		BOOL OnController( LONG t, int nChan, int nNum, int nVal, CMfxTrack* pTrk, int ixEvt );

	// Implementation
	private:
		enum EType { Registered, NonRegistered, Unknown } m_eType;

		int	m_PrmMSB;
		int	m_PrmLSB;
		int	m_ValMSB;
		int	m_ValLSB;
	};

	CxRPNChannelContext	m_aChan[ 16 ];
};

#endif // !defined(AFX_XRPNCONTEXT_H__B0DB7AF8_C212_4EC3_9441_6CEB9882E85D__INCLUDED_)
