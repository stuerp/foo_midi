// MfxEvent.h: interface for the CMfxEvent class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXEVENT_H__972DAE26_C078_495C_AB58_04CCE46F3A2C__INCLUDED_)
#define AFX_MFXEVENT_H__972DAE26_C078_495C_AB58_04CCE46F3A2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMfxEvent : public MfxEvent  
{
public:

	CMfxEvent();
	CMfxEvent( Type eType );
	CMfxEvent( const CMfxEvent& rhs );
	CMfxEvent& operator=( const CMfxEvent& rhs );
	~CMfxEvent();

	Type GetType() const
	{
		return m_eType;
	}

	void SetType( Type t )
	{
		m_eType = t;
	}

	LONG GetTime() const
	{
		return m_lTime;
	}

	void SetTime( LONG l )
	{
		m_lTime = l;
	}

	DWORD GetDur() const
	{
		ASSERT( GetType() == Note );
		return m_dwDuration;
	}

	void SetDur( DWORD dw )
	{
		ASSERT( GetType() == Note );
		m_dwDuration = dw;
	}

	BYTE GetPort() const
	{
		return m_byPort;
	}

	void SetPort( BYTE by )
	{
		m_byPort = by;
	}

	BYTE GetChannel() const
	{
		return m_byChan;
	}

	void SetChannel( BYTE by )
	{
		ASSERT( by < 16 );
		m_byChan = by;
	}

	BYTE GetKey() const
	{ 
		ASSERT( GetType() == Note || GetType() == KeyAft );
		return m_byKey;
	}
	
	void SetKey( BYTE by )
	{
		ASSERT( by < 128 );
		ASSERT( GetType() == Note || GetType() == KeyAft );
		m_byKey = by; 
	}

	BYTE GetVel() const
	{
		ASSERT( GetType() == Note );
		return m_byVel; 
	}
	
	void SetVel( BYTE by )
	{
		ASSERT( by < 128 );
		ASSERT( GetType() == Note );
		m_byVel = by; 
	}
	
	BYTE GetPressure() const
	{
		ASSERT( GetType() == KeyAft || GetType() == ChanAft );
		return m_byAmt; 
	}
	
	void SetPressure( BYTE by )
	{
		ASSERT( by < 128 );
		ASSERT( GetType() == KeyAft || GetType() == ChanAft );
		m_byAmt = by; 
	}
	
	short GetBank() const
	{
		ASSERT( GetType() == Patch );
		return m_nBank; 
	}
	
	void SetBank( short n )
	{
		ASSERT( MIN_BANK <= n && n <= MAX_BANK );
		ASSERT( GetType() == Patch );
		m_nBank = n; 
	}
	
	BYTE GetPatch() const
	{
		ASSERT( GetType() == Patch );
		return m_byPatch; 
	}
	
	void SetPatch( BYTE by )
	{
		ASSERT( by < 128 );
		ASSERT( GetType() == Patch );
		m_byPatch = by; 
	}
	
	BYTE GetBankSelectMethod() const
	{
		ASSERT( GetType() == Patch );
		return m_byBankSelMethod; 
	}
	
	void SetBankSelectMethod( BYTE by )
	{
		ASSERT( by < 128 );
		ASSERT( GetType() == Patch );
		m_byBankSelMethod = by; 
	}

	short GetWheel() const
	{
		ASSERT( GetType() == Wheel );
		return m_nVal; 
	}
	
	void SetWheel( short n )
	{
//		ASSERT( WHEEL_MIN <= n && n <= WHEEL_MAX );
		ASSERT( GetType() == Wheel );
		m_nVal = n; 
	}
	
	int GetCtrlNum() const;
	void SetCtrlNum( int n );
	int GetCtrlVal() const;
	void SetCtrlVal( int n );
	
	const char* GetText() const;
	HRESULT SetText( const char* psz );

	HRESULT SetSysxData( BYTE* pbData, DWORD dwLen );

private:

	void freeMemory();
	void setToDefaults();
	void copyData( const CMfxEvent& rhs );
	BOOL hasAllocatedMemory() const;
};

#endif // !defined(AFX_MFXEVENT_H__972DAE26_C078_495C_AB58_04CCE46F3A2C__INCLUDED_)
