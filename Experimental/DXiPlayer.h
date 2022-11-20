// DXiPlayer.h: interface for the CDXiPlayer class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DXIPLAYER_H__49E279E0_1EFC_466B_BC0B_F34D0396D537__INCLUDED_)
#define AFX_DXIPLAYER_H__49E279E0_1EFC_466B_BC0B_F34D0396D537__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDirectSound;
class CDxFilterGraph;
class CMfxSeq;
class CMfxHostSite;
class IMfxSoftSynth;
struct IBaseFilter;

////////////////////////////////////////////////////////////////////////////////

static const UINT MIN_BUFFER_MSEC = 1;
static const UINT MAX_BUFFER_MSEC = 2000;

class CDXiPlayer {
	public:
	CDXiPlayer();
	virtual ~CDXiPlayer();

	HRESULT Initialize();
	HRESULT Terminate();

	CDxFilterGraph* GetFilterGraph() const {
		return m_pFilterGraph;
	}
	IMfxSoftSynth* GetSoftSynth() const {
		return m_pMfxSynth;
	}
	CMfxHostSite* GetHostSite() const {
		return m_pMfxHostSite;
	}

	CMfxSeq* GetSeq() const {
		return m_pSeq;
	}
	HRESULT SetSeq(CMfxSeq* pSeq);

	int GetCurrentTrack() const {
		return m_nCurrentTrack;
	}
	void SetCurrentTrack(int n) {
		m_nCurrentTrack = n;
	}

	HRESULT SetFilter(IBaseFilter* pFilter);

	HRESULT Play(BOOL bPlaySeq);
	HRESULT Stop();
	HRESULT Rewind();

	LONG GetPosition();
	HRESULT SetPosition(LONG lTicks);

	HRESULT FillBuffer(void* pv, DWORD cs);

	BOOL IsPlaying() const {
		return m_bPlaying;
	}

	BOOL IsLooping() const {
		return m_bLooping;
	}
	void SetLooping(BOOL b);

	LONG GetLoopStart() const {
		return m_tLoopStart;
	}
	HRESULT SetLoopStart(LONG t);

	LONG GetLoopEnd() const {
		return m_tLoopEnd;
	}
	HRESULT SetLoopEnd(LONG t);

	unsigned GetSampleRate() const {
		return m_uSampleRate;
	}
	HRESULT SetSampleRate(unsigned rate) {
		m_uSampleRate = rate;
		return S_OK;
	}

	private:
	HRESULT updateLoopSampleTimes();

	private:
	CDxFilterGraph* m_pFilterGraph;
	IBaseFilter* m_pFilter;
	IMfxSoftSynth* m_pMfxSynth;
	CMfxHostSite* m_pMfxHostSite;

	CMfxSeq* m_pSeq;

	BOOL m_bRunning;
	BOOL m_bPlaying;
	BOOL m_bLooping;
	LONG m_tLoopStart;
	LONG m_tLoopEnd;
	LONGLONG m_llSampLoopStart;
	LONGLONG m_llSampLoopEnd;
	LONG m_tNow;
	int m_nCurrentTrack;

	unsigned m_uSampleRate;
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_DXIPLAYER_H__49E279E0_1EFC_466B_BC0B_F34D0396D537__INCLUDED_)
