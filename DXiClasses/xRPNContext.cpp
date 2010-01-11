// xRPNContext.cpp: implementation of the CxRPNContext class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxEvent.h"
#include "MfxTrack.h"
#include "xRPNContext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CxRPNContext
//
/////////////////////////////////////////////////////////////////////////////

CxRPNContext::CxRPNContext()
{}

/////////////////////////////////////////////////////////////////////////////

CxRPNContext::~CxRPNContext()
{}

/////////////////////////////////////////////////////////////////////////////

void CxRPNContext::Reset()
{
	// Reset each of our 16 component CxRPNChannelContext objects
	for (int ix = 0; ix < 16; ++ix)
		m_aChan[ ix ].Reset();
}

/////////////////////////////////////////////////////////////////////////////

BOOL CxRPNContext::OnController( LONG t, int nChan, int nNum, int nVal, CMfxTrack* pTrk, int ixEvt )
{
	ASSERT( 0 <= nChan && nChan <= 15 );
	return m_aChan[ nChan ].OnController( t, nChan, nNum, nVal, pTrk, ixEvt );
}

/////////////////////////////////////////////////////////////////////////////

CxRPNContext::Action CxRPNContext::OnController( LONG t, int nChan, int nNum, int nVal, CMfxEvent* pevtPrev, CMfxEvent* pevtNew )
{
	ASSERT( 0 <= nChan && nChan <= 15 );
	return m_aChan[ nChan ].OnController( t, nChan, nNum, nVal, pevtPrev, pevtNew );
}

/////////////////////////////////////////////////////////////////////////////
//
// CxRPNContext::CxRPNChannelContext
//
/////////////////////////////////////////////////////////////////////////////

void CxRPNContext::CxRPNChannelContext::Reset()
{
	m_eType = Unknown;
	m_PrmMSB = 0;
	m_PrmLSB = 0;
	m_ValMSB = 0;
	m_ValLSB = 0;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CxRPNContext::CxRPNChannelContext::OnController(
	LONG t, int nChan, int nNum, int nVal,	CMfxTrack* pTrk, int ix
)
{
	// This is a wrapper that is parasitic on the more fundamental other
	// version of OnController(), which takes a pair of CEvent*s.
	// [Originally this was the sole version of OnController(), until
	// we needed a version that could work without a CStream. For code
	// which prefers to work via a CStream, we continue to provide this,
	// built now on the more primitive new version.]

	ASSERT( pTrk );
	ASSERT( 0 <= ix && ix <= pTrk->size() );

	// If there's a previous event, get it in evtPrev.
	// If not, it's OK to pass a pointer to the CEvent to OnController(),
	// because the type of the CEvent is "Neuter".
	CMfxEvent evtPrev;
	if (0 < ix)
		evtPrev = (*pTrk)[ ix - 1 ];

	CMfxEvent evtNew;

	switch( OnController( t, nChan, nNum, nVal, &evtPrev, &evtNew ) )
	{
		case None:
			return TRUE;
		case Replace:
			(*pTrk)[ ix - 1 ] = evtPrev;
			return TRUE;
		case Add:
			pTrk->insert( pTrk->begin() + ix, evtNew );
			return TRUE;
		default:
			ASSERT( FALSE );
			return FALSE;
	}
}

/////////////////////////////////////////////////////////////////////////////

CxRPNContext::Action CxRPNContext::CxRPNChannelContext::OnController(
	LONG t, int nChan, int nNum, int nVal,	CMfxEvent* pevtPrev, CMfxEvent* pevtNew
)
{
	ASSERT( 0 <= nChan && nChan <= 15 );
	ASSERT( 0x00 <= nNum && nNum <= 0x7F );
	ASSERT( 0x00 <= nVal && nVal <= 0x7F );
	ASSERT( pevtNew );
	// pevtPrev may be NULL

	BOOL bMaybeInsert = FALSE;

	switch( nNum )
	{
		case 101:	// Registered Prm MSB
			m_eType	= Registered;
			m_PrmMSB	= nVal;
			m_PrmLSB	= 0;	// reset to zero, as recommended by MIDI spec
			m_ValMSB	= 0;
			m_ValLSB	= 0;
			break;
		case 100:	// Registered Prm LSB
			if (m_eType != Registered)
			{
				m_eType	= Registered;
				m_PrmMSB = 0;
			}
			// else m_PrmMSB remains whatever it is
			m_PrmLSB	= nVal;
			m_ValMSB	= 0;
			m_ValLSB	= 0;
			break;

		case 99:		// NonRegistered Prm MSB
			m_eType	= NonRegistered;
			m_PrmMSB	= nVal;
			m_PrmLSB	= 0;	// reset to zero, as recommended by MIDI spec
			m_ValMSB	= 0;
			m_ValLSB	= 0;
			break;
		case 98:		// NonRegistered Prm LSB
			if (m_eType != NonRegistered)
			{
				m_eType	= NonRegistered;
				m_PrmMSB = 0;
			}
			// else m_PrmMSB remains whatever it is
			m_PrmLSB	= nVal;
			m_ValMSB	= 0;
			m_ValLSB	= 0;
			break;

		case 6:		// Val MSB
			if (m_eType != Unknown)
			{
				m_ValMSB = nVal;
				m_ValLSB = 0;		// reset to zero, as recommended by MIDI spec
				bMaybeInsert = TRUE;
			}
			else
				goto INSERT_ORDINARY;
			break;

		case 38:		// Val LSB
			if (m_eType != Unknown)
			{
				m_ValLSB = nVal;
				bMaybeInsert = TRUE;
			}
			else
				goto INSERT_ORDINARY;
			break;

		case 96:		// Value Increment
			if (m_eType != Unknown)
			{
				int n = (m_ValMSB << 7) | m_ValLSB;
				if (++n < 16383)
				{
					m_ValMSB = (n >> 7) & 0x7F;
					m_ValLSB = n & 0x7F;
					bMaybeInsert = TRUE;
				}
				// else ignore
			}
			else
				goto INSERT_ORDINARY;
			break;

		case 97:		// Value Decrement
			if (m_eType != Unknown)
			{
				int n = (m_ValMSB << 7) | m_ValLSB;
				if (0 < n--)
				{
					m_ValMSB = (n >> 7) & 0x7F;
					m_ValLSB = n & 0x7F;
					bMaybeInsert = TRUE;
				}
				// else ignore
			}
			else
				goto INSERT_ORDINARY;
			break;

		INSERT_ORDINARY:
		default:
		{
			// Ordinary controller event:
			pevtNew->SetTime( t );
			pevtNew->SetType( CMfxEvent::Control );
			pevtNew->SetChannel( nChan );
			pevtNew->SetCtrlNum( nNum );
			pevtNew->SetCtrlVal( nVal );

			return Add;
			//////
		}
	}

	// Having got a value, it's LONG to insert a CMfxEvent::RPN or CMfxEvent::NRPN
	// event, or, update the value of an existing prior one.
	if (bMaybeInsert && m_eType != Unknown)
	{
		CMfxEvent::Type const type = (m_eType == Registered) ? CMfxEvent::RPN : CMfxEvent::NRPN;

		// If this is CC# 38, then check in pStm for an xRPN which we
		// might have already inserted upon getting a CC# 6. We require
		// this to be immediately preceding and within a max LONG window.
		if (NULL != pevtPrev && nNum == 38)
		{
			if (
					// Same type (RPN or NRPN)?
					pevtPrev->GetType() == type &&
					// Same controller number (14-bit parameter number)?
					pevtPrev->GetCtrlNum() == ((m_PrmMSB << 7) | m_PrmLSB) &&
					// Same controller value (14-bit parameter value) as the MSB we have?
					// Note that, when there's a series of solo 38's in a row, this test
					// will fail unless the value of this one is 0, because we've already
					// OR'd in the 38 for the prior one.
					pevtPrev->GetCtrlVal() == (m_ValMSB << 7) &&
					// Same time, or recent enough?
					pevtPrev->GetTime() >= t - 120 &&
					// Same MIDI channel?
					pevtPrev->GetChannel() == nChan
				)
			{
				// Do not insert a redundant RPN or NRPN.
				// Simply OR the GetCtrlVal() of the existing prior event.
				pevtPrev->SetCtrlVal( pevtPrev->GetCtrlVal() | m_ValLSB );
				return Replace;
				/////
			}
		}
		// Else need to insert a new RPN or NRPN event:
		pevtNew->SetTime( t );
		pevtNew->SetType( type );
		pevtNew->SetChannel( nChan );
		pevtNew->SetCtrlNum( (m_PrmMSB << 7) | m_PrmLSB );
		pevtNew->SetCtrlVal( (m_ValMSB << 7) | m_ValLSB );

		return Add;
		//////
	}
	else
	{
		// Do nothing: Do not insert any CMfxEvent::Control event.
		return None;
	}
}

/////////////////////////////////////////////////////////////////////////////
