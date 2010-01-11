// MfxTempoMap.h: interface for the CMfxTempoMap class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFXTEMPOMAP_H__ED0F406D_EBC6_4F6E_98AE_BE756207F8E9__INCLUDED_)
#define AFX_MFXTEMPOMAP_H__ED0F406D_EBC6_4F6E_98AE_BE756207F8E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSeq; // forward
class CMfxTempoMap; // forward

////////////////////////////////////////////////////////////////////////////////

class CTempo
{
public:
	CTempo( LONG t = 0, int nBPM100 = 12000 ) :
		m_t(t), m_nBPM100(nBPM100), m_qwCnt(0)
	{}

	// Note: BPM is a misnomer here.  The actual units are 1/100th of quarter
	// notes per minute.
	static int GetMinBPM() { return 800; }
	static int GetMaxBPM() { return 25000; }

	int GetBPM() const { return m_nBPM100; }
	void SetBPM( int n ) { m_nBPM100 = n; }

	LONG GetTime() const { return m_t; }
	void SetTime( LONG l ) { m_t = l; }

	QWORD GetCount() const { return m_qwCnt; }

private:

	friend class CMfxTempoMap;

	LONG		m_t;
	int		m_nBPM100;
	QWORD		m_qwCnt;
};

////////////////////////////////////////////////////////////////////////////////

class CMfxTempoMap : public vector<CTempo>
{
public:
	CMfxTempoMap();
	virtual ~CMfxTempoMap();

	int GetPPQ() const { return m_nPPQ; }
	void SetPPQ( int nPPQ )
	{
		m_nPPQ = nPPQ;
		Recalc();
	}

	int IndexForTime( LONG lTicks ) const;
	int IndexForSeconds( double dSeconds ) const;

	LONG Tick2Sample( LONG lTicks, LONG lSamplesPerSec ) const;
	LONG Sample2Tick( LONG lSample, LONG lSamplesPerSec ) const;
	LONGLONG Sample2UTick( LONGLONG llSample, LONG lSamplesPerSec ) const;
	LONGLONG UTick2Sample( LONGLONG llUTicks, LONG lSamplesPerSec ) const;

	void Recalc( int ix = 0 );

private:

	int m_nPPQ;
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_MFXTEMPOMAP_H__ED0F406D_EBC6_4F6E_98AE_BE756207F8E9__INCLUDED_)
