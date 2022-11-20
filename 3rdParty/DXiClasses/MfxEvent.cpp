// MfxEvent.cpp: implementation of the CMfxEvent class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxEvent.h"
#include "MfxBufferFactory.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMfxEvent::CMfxEvent()
{
	SetType( Note );
	setToDefaults();
}

CMfxEvent::CMfxEvent( Type eType )
{
	SetType( eType );
	setToDefaults();
}

void CMfxEvent::setToDefaults()
{
	SetTime( 0 );
	SetPort( 0 );
	SetChannel( 0 );

	switch (GetType())
	{
		case Note:
			SetKey( 60 );
			SetVel( 64 );
			SetDur( 120 );
			break;

		case KeyAft:
			SetKey( 60 );
			SetPressure( 0 );
			break;

		case Control:
			SetCtrlNum( CTL_MODULATION );
			SetCtrlVal( 0 );
			break;

		case Patch:
			SetBankSelectMethod( Normal );
			SetBank( BANK_NONE );
			SetPatch( 0 );
			break;

		case ChanAft:
			SetPressure( 0 );
			break;

		case Wheel:
			SetWheel( 0 );
			break;

		case RPN:
		case NRPN:
			SetCtrlNum( 0 );
			SetCtrlVal( 0 );
			break;

		case Sysx:
		case Text:
		case Lyric:
			m_hBuffer = NULL;
			break;

		case MuteMask:
			m_mfxChannel = 0;
			m_maskSet = 0;
			m_maskClear = 0;
			break;

		case VelOfs:
		case KeyOfs:
			m_mfxChannel = 0;
			m_nOfs = 0;
			break;

		case VelTrim:
		case KeyTrim:
			m_mfxChannel = 0;
			m_nTrim = 0;
			break;

		case ShortMsg:
			m_dwShortMsg = 0;
			break;
			
		default:
			ASSERT(FALSE);
			break;
	}
}

CMfxEvent::CMfxEvent( const CMfxEvent& rhs )
{
	copyData( rhs );
}

CMfxEvent& CMfxEvent::operator=( const CMfxEvent& rhs )
{
	if (&rhs != this)
		copyData( rhs );
	return *this;
}

void CMfxEvent::copyData( const CMfxEvent& rhs )
{
	freeMemory();

	ASSERT( sizeof(CMfxEvent) == sizeof(MfxEvent) );
	memcpy( this, &rhs, sizeof(CMfxEvent) );

	if (rhs.hasAllocatedMemory())
	{
		void*	pv;
		DWORD	cb;
		theBufferFactory.GetPointer( rhs.m_hBuffer, &pv, &cb );
		theBufferFactory.CreateCopy( rhs.m_hBuffer, cb, &m_hBuffer );
	}
}

CMfxEvent::~CMfxEvent()
{
	freeMemory();
}

////////////////////////////////////////////////////////////////////////////////

BOOL CMfxEvent::hasAllocatedMemory() const
{
	return GetType() == Text || GetType() == Lyric || GetType() == Sysx;
}

////////////////////////////////////////////////////////////////////////////////

void CMfxEvent::freeMemory()
{
	if (hasAllocatedMemory() && NULL != m_hBuffer)
	{
		theBufferFactory.Destroy( m_hBuffer );
		m_hBuffer = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

int CMfxEvent::GetCtrlNum() const		// Control, RPN, and NRPN
{
	if (GetType() == Control)
		return m_byNum;
	else if (GetType() == RPN || GetType() == NRPN)
		return m_wNum;

	ASSERT( FALSE );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void CMfxEvent::SetCtrlNum( int n )
{
	if (GetType() == Control)
	{
		ASSERT( 0 <= n && n <= 127 );
		m_byNum = static_cast<BYTE>(n);
	}
	else if (GetType() == RPN || GetType() == NRPN)
	{
		ASSERT( 0 <= n && n <= 16383 );
		m_wNum = static_cast<WORD>( n );
	}
	else
		ASSERT( FALSE );
}

////////////////////////////////////////////////////////////////////////////////

int CMfxEvent::GetCtrlVal() const
{
	if (GetType() == Control)
		return m_byVal;
	else if (GetType() == RPN || GetType() == NRPN)
		return m_wVal;

	ASSERT( FALSE );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void CMfxEvent::SetCtrlVal( int n )
{
	if (GetType() == Control)
	{
		ASSERT( 0 <= n && n <= 127 );
		m_byVal = static_cast<BYTE>(n);
	}
	else if (GetType() == RPN || GetType() == NRPN)
	{
		ASSERT( 0 <= n && n <= 16383 );
		m_wVal = static_cast<WORD>( n );
	}
	else
		ASSERT( FALSE );
}

////////////////////////////////////////////////////////////////////////////////

const char* CMfxEvent::GetText() const
{
	if (GetType() != Text && GetType() != Lyric)
	{
		ASSERT(FALSE);
		return NULL;
	}

	return (const char*) m_hBuffer + sizeof(DWORD);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CMfxEvent::SetText( const char* psz )
{
	if (GetType() != Text && GetType() != Lyric)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	if (!psz)
		return E_POINTER;

	// Free the old buffer
	freeMemory();

	// Allocate the new buffer
	CHECK( theBufferFactory.Create( strlen(psz) + 1, &m_hBuffer ) );

	// Copy in the text
	void*	pv;
	DWORD	cb;
	CHECK( theBufferFactory.GetPointer( m_hBuffer, &pv, &cb ) );
	ASSERT( cb == strlen(psz)+1 );
	strcpy( (char*)pv, psz );
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CMfxEvent::SetSysxData( BYTE* pbData, DWORD dwLen )
{
	ASSERT( GetType() == Sysx );

	// Free the old buffer
	freeMemory();

	// Allocate the new buffer
	CHECK( theBufferFactory.Create( dwLen, &m_hBuffer ) );

	// Copy in the data
	void*	pv;
	DWORD	cb;
	CHECK( theBufferFactory.GetPointer( m_hBuffer, &pv, &cb ) );
	ASSERT( cb == dwLen );
	memcpy( pv, pbData, dwLen );
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
