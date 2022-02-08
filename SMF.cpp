// SMF.cpp - Standard MIDI File reader.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MfxSeq.h"
#include "xRPNContext.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef LIMIT
#define LIMIT(x, a, b) (((x) < (a)) ? (a) : ((x) > (b)) ? (b) : (x))
#endif

/////////////////////////////////////////////////////////////////////////////
//
//   Standard MIDI 1.0 Files
//
/////////////////////////////////////////////////////////////////////////////

const BYTE META_EVENT = 0xFF;

const BYTE META_SEQNUMBER = 0x00;

const BYTE META_TEXT = 0x01;
const BYTE META_COPYRIGHT = 0x02;
const BYTE META_TRACKNAME = 0x03;
const BYTE META_INSTRNAME = 0x04;
const BYTE META_LYRIC = 0x05;
const BYTE META_MARKER = 0x06;
const BYTE META_CUEPOINT = 0x07;

const BYTE META_CHANPREFIX = 0x20;
const BYTE META_CABLE = 0x21;

const BYTE META_ENDTRACK = 0x2F;

const BYTE META_TEMPO = 0x51;
const BYTE META_SMPTEOFS = 0x54;
const BYTE META_METER = 0x58;
const BYTE META_KEYSIG = 0x59;
const BYTE META_SEQSPEC = 0x7F;

/////////////////////////////////////////////////////////////////////////////

// Take a (32 bit signed int) and make sure it's a legal (LONG) value:
#define LEGALTIME(t) ((LONG)(min(max(0, (t)), LONG_MAX)))

/////////////////////////////////////////////////////////////////////////////

static int s_nLyriCMfxTrack = 3; // Track to hold lyrics from format 0 SMFs

/////////////////////////////////////////////////////////////////////////////
//
// 									Intel <--> Motorola
//
/////////////////////////////////////////////////////////////////////////////

#ifndef MAKEWORD
#define MAKEWORD(l, h) ((((WORD)l) & 0x00FF) | ((((WORD)h) << 8) & 0xFF00))
#endif

//--------------------------------------------------------------------------

inline WORD revWord(WORD w) {
	// Convert an Intel WORD to a Motorola DWORD, or vice versa,
	// by reversing the order of the bytes.

	return MAKEWORD(HIBYTE(w), LOBYTE(w));
}

//--------------------------------------------------------------------------

inline DWORD revDword(DWORD dw) {
	// Convert an Intel DWORD to a Motorola DWORD, or vice versa.
	// Their HIWORD is our LOWORD.  Within each word, swap HIBYTE/LOBYTE.

	return MAKELONG(revWord(HIWORD(dw)), revWord(LOWORD(dw)));
}

/////////////////////////////////////////////////////////////////////////////

inline BYTE readSMFByte(CFile& file, HRESULT* phr) {
	BYTE x;
	if(file.Read(&x, sizeof(BYTE)) != sizeof(BYTE))
		*phr = S_FALSE;
	return x;
}

//--------------------------------------------------------------------------

inline BYTE readSMFDataByte(CFile& file, HRESULT* phr) {
	BYTE x;
	if(file.Read(&x, sizeof(BYTE)) != sizeof(BYTE))
		*phr = S_FALSE;
	else {
		if(x > 0x7F) // possibly force into range
		{
			TRACE("SMF Data Byte out of range: %d\n", x);
			x &= 0x7F;
		}
	}

	return x;
}

//--------------------------------------------------------------------------

inline WORD readSMFWord(CFile& file, HRESULT* phr) {
	WORD x;
	if(file.Read(&x, sizeof(WORD)) != sizeof(WORD))
		*phr = S_FALSE;
	return revWord(x);
}

//--------------------------------------------------------------------------

inline DWORD readSMFDword(CFile& file, HRESULT* phr) {
	DWORD x;
	if(file.Read(&x, sizeof(DWORD)) != sizeof(DWORD))
		*phr = S_FALSE;
	return revDword(x);
}

//--------------------------------------------------------------------------

inline DWORD readSMFVarLen(CFile& file, HRESULT* phr) {
	DWORD dwValue;
	BYTE by;

	if((dwValue = readSMFByte(file, phr)) & 0x80) {
		dwValue &= 0x7F;
		do {
			dwValue = (dwValue << 7) + ((by = readSMFByte(file, phr)) & 0x7F);
		} while((S_OK == *phr) && (by & 0x80));
	}
	return dwValue;
}

//--------------------------------------------------------------------------

// Result code from readSMFChunkHeader(), tells which type of chunk was found:
enum EChunkType {
	MTerr, // Error reading header or unrecognized chunk type
	MThd, // "MThd" MIDI Header chunk
	MTrk, // "MTrk" MIDI Track Data chunk
};

//--------------------------------------------------------------------------

static EChunkType readSMFChunkHeader(CFile& rFile, DWORD* pdwLen, HRESULT* phr) {
	char szChunkHeader[5]; //  FOURCC?

	if(rFile.Read(szChunkHeader, 4) == 4) // Chunk name
	{
		szChunkHeader[4] = '\0';
		*pdwLen = readSMFDword(rFile, phr); // Chunk length

		if(S_OK == *phr) {
			// Which did we get?  Should be either "MThd" or "MTrk":

			if(lstrcmpA(szChunkHeader, "MThd") == 0)
				return MThd;
			//////

			if(lstrcmpA(szChunkHeader, "MTrk") == 0)
				return MTrk;
			//////

			ASSERT(FALSE); // Undefined MIDI chunk type!
		}
	} else
		*phr = S_FALSE;

	return MTerr;
}

/////////////////////////////////////////////////////////////////////////////
//
//  This class reads strings from files in SMF text event format.
//  This format uses a variable length followed by the (non-terminated) string data.
//

class CSMFText {
	public:
	~CSMFText() {
		SAFE_ARRAY_DELETE(m_pszString);
	}

	CSMFText() {
		m_pszString = NULL;
		m_nLength = 0;
	}

	BOOL SetText(const char* szString) {
		// Allocates a buffer and copies the string:

		m_nLength = lstrlenA(szString);
		delete[] m_pszString;
		m_pszString = new char[m_nLength + 1];
		if(m_pszString) {
			lstrcpyA(m_pszString, szString);
			m_pszString[m_nLength] = '\0';

			return TRUE; // Success
		}
		m_pszString = NULL;
		m_nLength = 0;

		return FALSE; // Allocation failure
	}

	void Read(CFile& rFile, HRESULT* phr) {
		// Allocates a buffer and reads the string from a file:

		m_nLength = (int)readSMFVarLen(rFile, phr);
		delete[] m_pszString;
		m_pszString = new char[m_nLength + 1];
		if(m_pszString) {
			if(rFile.Read(m_pszString, m_nLength) == static_cast<UINT>(m_nLength))
				m_pszString[m_nLength] = '\0';
			else
				*phr = S_FALSE;
		} else {
			ASSERT(FALSE);
			*phr = E_OUTOFMEMORY;
		}
	}

	const char* GetStringPtr() {
		return m_pszString;
	}
	char* GetHotStringPtr() {
		return m_pszString;
	}
	int GetLength() {
		return m_nLength;
	}
	BOOL IsEmpty() {
		return (m_nLength > 0) ? FALSE : TRUE;
	}

	BOOL NLToCRLF() {
		// Converts all occurences of "\n" to "\r\n" in the string.
		const BYTE CR = 0x0D; // Carriage return
		const BYTE LF = 0x0A; // Line feed

		if(m_pszString) {
			int ix = 0;
			while(m_pszString[ix]) {
				if((m_pszString[ix] == CR) || (m_pszString[ix] == LF)) {
					// If not CR followed by LF or vice-versa, insert one byte

					if(((m_pszString[ix + 1] != CR) && (m_pszString[ix + 1] != LF)) ||
					   ((m_pszString[ix] != m_pszString[ix + 1]))) {
						char* pszNewString = new char[m_nLength + 1 + 1];
						if(!pszNewString)
							return FALSE;

						// Shift over from \n on to make room for extra \r before \n.
						// Amount to copy is current length of string, plus 1 for \0.

						lstrcpyA(pszNewString, m_pszString);
						lstrcpyA(pszNewString + ix + 1, m_pszString + ix);

						delete[] m_pszString;
						m_pszString = pszNewString;
						m_nLength++;
						m_pszString[m_nLength] = '\0';
					}

					// Regardless of original order or type, force CR then LF:

					m_pszString[ix++] = CR;
					m_pszString[ix++] = LF;
				} else
					++ix;
			}
		}
		return TRUE;
	}

	BOOL Append(const char* szAppend) {
		// Append another string to the string.

		char* pszNewString = new char[m_nLength + lstrlenA(szAppend) + 1];
		if(!pszNewString)
			return FALSE;

		if(m_pszString) {
			lstrcpyA(pszNewString, m_pszString);
			delete[] m_pszString;
			lstrcatA(pszNewString, szAppend);
		} else
			lstrcpyA(pszNewString, szAppend);
		m_pszString = pszNewString;
		m_nLength += lstrlenA(szAppend);
		m_pszString[m_nLength] = '\0';

		return TRUE;
	}

	BOOL Prepend(const char* szPrepend) {
		// Prepend another string to the string.

		char* pszNewString = new char[m_nLength + lstrlenA(szPrepend) + 1];
		if(!pszNewString)
			return FALSE;

		lstrcpyA(pszNewString, szPrepend);
		if(m_pszString) {
			lstrcatA(pszNewString, m_pszString);
			delete[] m_pszString;
		}
		m_pszString = pszNewString;
		m_nLength += lstrlenA(szPrepend);
		m_pszString[m_nLength] = '\0';

		return TRUE;
	}

	BOOL PrependCRLF() {
		// Prepend a "\r\n" to the string.
		return Prepend("\r\n");
	}

	private:
	char* m_pszString;
	int m_nLength;
};

////////////////////////////////////////////////////////////////////////////
//
//  This class matches Note Offs with their corresponding Note On.
//
//

class CReadQueue {
	public:
	CReadQueue() {
		m_nHangingNotes = 0;
		for(int iChan = 0; iChan < 16; iChan++) {
			for(int iKey = 0; iKey < 128; iKey++)
				m_ixNote[iChan][iKey] = USHRT_MAX;
		}
	}

	int HangingNotes() {
		return m_nHangingNotes; // Number of "hanging" notes
	}

	BOOL IsHanging(int iChan, int iKey) {
		return (m_ixNote[iChan][iKey] != USHRT_MAX) ? TRUE : FALSE;
	}

	void Insert(CMfxTrack& trk, LONG tNow, int iChan, int iKey, int ixNote) {
		// Inserts record of note-on event for later handling during MIDI file read.

		// Cap off "linked list" of stream indices stored in the duration

		CMfxEvent evt(trk[ixNote]);
		ASSERT(evt.GetType() == MfxEvent::Note);
		evt.SetDur(USHRT_MAX);
		trk[ixNote] = evt;

		if(!IsHanging(iChan, iKey)) {
			// No note overlap, just blast it into the array:

			ASSERT((ixNote != USHRT_MAX) && (ixNote < trk.size()));
			m_ixNote[iChan][iKey] = ixNote;
		} else {
			// We already have a hanging Note On for this channel and key.
			// We could stop the existing note now, or chain the new note
			// to be handled after this one.  This is also the point at which
			// we can detect and handle legato problems.

			// Add new note to the end of the list:

			int ixLink = m_ixNote[iChan][iKey];
			int ixNext;

			while((ixNext = trk[ixLink].GetDur()) != USHRT_MAX) {
				ASSERT(trk[ixLink].GetTime() <= trk[ixNext].GetTime());
				ASSERT(trk[ixLink].GetType() == MfxEvent::Note);
				ixLink = ixNext;
			}

			// NOTE: We use each note's Duration field to store a link to the
			// next simultaneous hanging note (if any) at this key.  If no such
			// note exists, we store USHRT_MAX instead.

			CMfxEvent evt(trk[ixLink]);
			ASSERT(evt.GetType() == MfxEvent::Note);
			evt.SetDur(ixNote);
			trk[ixLink] = evt;
		}

		++m_nHangingNotes;
	}

	void RemoveChannel(CMfxTrack& trk, LONG tNow, int iChan) {
		for(int iKey = 0; (m_nHangingNotes != 0) && (iKey < 128); iKey++) {
			while(IsHanging(iChan, iKey))
				Remove(trk, tNow, iChan, iKey);
		}
	}

	void Remove(CMfxTrack& trk, LONG tNow, int iChan, int iKey) {
		int ix = m_ixNote[iChan][iKey];
		if(ix != USHRT_MAX && ix < trk.size()) {
			ASSERT(m_nHangingNotes > 0);

			CMfxEvent evt(trk[ix]);

			if(evt.GetType() == MfxEvent::Note) {
				m_ixNote[iChan][iKey] = evt.GetDur(); // Unlink
				evt.SetDur(tNow - evt.GetTime());
				trk[ix] = evt;

				--m_nHangingNotes;
			}
		} else {
			// Didn't find corresponding Note On
		}
	}

	private:
	int m_nHangingNotes; // Number of "hanging" notes
	int m_ixNote[16][128]; // Stream index of hanging note by channel and key, or USHRT_MAX
};

//--------------------------------------------------------------------------

static BOOL s_bGotTitle; // Have we set the song title yet?
static WORD s_wDivision; // PPQN for the song

//==========================================================================
// 										Header Chunk
//==========================================================================

static HRESULT readMThd(CFile& rFile, WORD* pwFormat, int* piNumTrks, WORD* pwDivision) {
	HRESULT hr = S_OK;
	*pwFormat = readSMFWord(rFile, &hr);
	CHECK(hr)
	*piNumTrks = (int)readSMFWord(rFile, &hr);
	CHECK(hr)
	*pwDivision = readSMFWord(rFile, &hr);
	CHECK(hr);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

static BOOL lookForMTrk(CFile& rFile, HRESULT* phr) {
	// See if what's next in the file looks like a MIDI file section header.
	// This is used by readMTrk() to try to read poorly-written .MID files.

	// Use local phr flag to prevent setting *phr unless we really want to.
	// Since we're only peeking ahead, an error isn't necessarily an error.
	HRESULT hr = S_OK;

	BOOL bLooksGood = FALSE;

	// Remember where we were, and see if we can read a "MTrk" section type
	// and a DWORD section length.  For added robustness, we also verify
	// that (position+length <= file length).

	DWORD dwPrevPos = rFile.GetPosition();

	DWORD dwChunkLength = 0;

	// Don't set *phr if this fails, we could be at the EOF:
	if((readSMFChunkHeader(rFile, &dwChunkLength, &hr) == MTrk) &&
	   (S_OK == hr)) {
		DWORD dwFileLen = rFile.Seek(0L, CFile::end);

		// The "8" on the next line represents the size of the chunk header itself:
		if((dwPrevPos + dwChunkLength + 8) <= dwFileLen)
			bLooksGood = TRUE; //  Looks like a new MfxTrack.
	} else {
		// If we didn't find a track, maybe we're at the end of file?
		DWORD dwFileLen = rFile.Seek(0L, CFile::end);
		if(dwPrevPos == dwFileLen)
			bLooksGood = TRUE; //  Looks like this is the end...
	}

	rFile.Seek(dwPrevPos, CFile::begin);

	return bLooksGood;
}

/////////////////////////////////////////////////////////////////////////////

static CMfxTrack* getTrack(CMfxSeq& rMfxSeq, WORD wFmt, int nTrk, int nChan = -1, BOOL bCreate = TRUE) {
	// The purpose of this function is to consolidate the idea of which
	// stream (track) something is supposed to go in, taking into account
	// whether we're reading Format 0 or 1 and whether or not we know
	// something is for a particular MIDI channel.
	//
	// We return NULL if it's impossible to determine a particular MfxTrack.

	if(0 == wFmt) {
		if(nChan < 0 || nChan > 15)
			return NULL;
		//////

		ASSERT(-1 <= nTrk && nTrk <= 15);

		nTrk = nChan; // channels 0..15 go to tracks 0..15
	}

	// Else if Format 1, then nTrk specifies the MfxTrack.  It may be -1, in
	// which case we bump this to 0.
	nTrk = max(nTrk, 0);

	// Create a track if necessary
	CMfxTrack* pTrk = rMfxSeq.GetTrack(nTrk, TRUE);
	return pTrk;
}

//--------------------------------------------------------------------------

static BOOL isGmOrXgSysx(BYTE* pSysxData, DWORD dwLen) {
	// Determine whether the given sysx bank is a "GM System On" or "GS Reset",
	// or an "XG Reset" message.

	const BYTE s_byGMSystemOn[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
	const BYTE s_byGSReset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
	const BYTE s_bySysModeSet[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x00, 0x01, 0xF7 };
	const BYTE s_byXGReset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

	if(dwLen == sizeof(s_byGMSystemOn) && 0 == memcmp(pSysxData, s_byGMSystemOn, dwLen))
		return TRUE;
	if(dwLen == sizeof(s_byGSReset) && 0 == memcmp(pSysxData, s_byGSReset, dwLen))
		return TRUE;
	if(dwLen == sizeof(s_bySysModeSet) && 0 == memcmp(pSysxData, s_bySysModeSet, dwLen))
		return TRUE;
	if(dwLen == sizeof(s_byXGReset) && 0 == memcmp(pSysxData, s_byXGReset, dwLen))
		return TRUE;

	return FALSE;
}

//--------------------------------------------------------------------------

static void AddSysxData(
BYTE* pSysxData,
DWORD dwLen,
LONG tSysx,
CMfxSeq& rMfxSeq,
int nTrk,
WORD wFmt,
BYTE byRunStat,
LONG* tFirstNote,
BOOL* pbInSetup,
HRESULT* phr) {
	CMfxTrack* pTrk = getTrack(rMfxSeq, wFmt, nTrk, CHAN(byRunStat));
	if(pTrk) {
		CMfxEvent event(MfxEvent::Sysx);
		event.SetTime(tSysx);
		event.SetSysxData(pSysxData, dwLen);

		if(!isGmOrXgSysx(pSysxData, dwLen))
			*pbInSetup = FALSE;

		if(*tFirstNote > event.GetTime())
			*tFirstNote = event.GetTime();

		pTrk->push_back(event);
	}
}

/////////////////////////////////////////////////////////////////////////////
// When consolidating mutliple events, we want to allow them to have
// slightly different times.  This allows us to succeed more often when
// we ought to.  Example: CC#0 CC#32 and PRG are each several ticks apart.
// So long as there are no other intervening events, they should be treated
// as a group.
//
// On the other hand, we probably don't want to ignore the time.  If the
// events are more than, say, a beat apart, they probably are not meant as
// a Bank Select set even if there aren't any intervening events.
//
// The case of PRG PRG Bank Select requires a tighter time limit.  A file
// may contain what are intended to be two totally separate PRG changes.
// Although we do require the first one to be patch >= 100, still, by
// coincidence the first of a pair might have this value even if they are
// not meant to be a Bank Select pair.

static LONG const tSlopCtrl = 480;
static LONG const tSlopPatch100 = 15;

inline BOOL isBankForPatch(const CMfxEvent& evtCtrl, int nCtrl, const CMfxEvent& evtPatch) {
	ASSERT(evtPatch.GetType() == CMfxEvent::Patch);
	ASSERT(evtCtrl.GetTime() <= evtPatch.GetTime());
	return (
	evtCtrl.GetType() == CMfxEvent::Control && // Is controller?
	evtCtrl.GetCtrlNum() == nCtrl && // Is correct controller?
	evtCtrl.GetChannel() == evtPatch.GetChannel() // Is same channel?
	);
}

/////////////////////////////////////////////////////////////////////////////

static BOOL findBankForPatch(const CMfxEvent& rEvtPatch, int nCtrl, CMfxTrack& trk, int* pix, BOOL* pbCanDelete) {
	// Look back for a recent Controller form of bank select event.

	int ix = *pix;
	while(0 < ix--) {
		const CMfxEvent evt = trk[ix];
		if(evt.GetTime() + tSlopCtrl < rEvtPatch.GetTime())
			break; // too far back in time; stop searching

		if(isBankForPatch(evt, nCtrl, rEvtPatch)) {
			*pix = ix;
			return TRUE; // *pix is index of event which was found
		} else if(evt.GetType() == CMfxEvent::Note) {
			*pbCanDelete = FALSE;
		}
	}
	return FALSE; // Event not found, *pix not modified!
}

/////////////////////////////////////////////////////////////////////////////

inline BOOL isPatch100BankForPatch(const CMfxEvent& evtPatch1, const CMfxEvent& evtPatch2) {
	ASSERT(evtPatch2.GetType() == CMfxEvent::Patch);
	ASSERT(evtPatch1.GetTime() <= evtPatch2.GetTime());

	return evtPatch1.GetType() == CMfxEvent::Patch && // Is patch?
	       evtPatch1.GetPatch() >= 100 && // Is in range 100.127?
	       evtPatch1.GetChannel() == evtPatch2.GetChannel() // Is same channel?
	;
}

/////////////////////////////////////////////////////////////////////////////

static BOOL findPatch100BankForPatch(const CMfxEvent& rEvtPatch, CMfxTrack& trk, int* pix, BOOL* pbCanDelete) {
	// Look back for a recent Patch 100 form of bank select event.

	int ix = *pix;
	while(0 < ix--) {
		const CMfxEvent& evt = trk[ix];
		LONG tEvt = evt.GetTime();
		if(tEvt + tSlopPatch100 < rEvtPatch.GetTime())
			break; // too far back in time; stop searching

		if(isPatch100BankForPatch(evt, rEvtPatch)) {
			*pix = ix;
			return TRUE; // *pix is index of event which was found
		} else if(evt.GetType() == CMfxEvent::Note) {
			*pbCanDelete = FALSE;
		}
	}
	return FALSE; // Event not found, *pix not modified!
}

/////////////////////////////////////////////////////////////////////////////

static BOOL combineBankAndPatch(const CMfxEvent& rEvtPatch, CMfxTrack& trk, int ixInsert, BOOL bCanDelete) {
	// Given up a series of CEvents, possibly converts them to a single
	// Bank/Patch CMfxEvent.
	//
	// rEvtPatch is a Patch event, which has NOT YET been added to trk.
	// ix is the index in the stream where the next event should be added.
	//
	// No matter what, we add the Patch event to the stream.  The question is,
	// at what index.  If we find prior controller events which constitute a
	// bank select, we remove them and insert the Patch event in their place,
	// combining their bank data into the Patch's SetBank() field.

	// We have a challenge with CC#0, CC#32, and Prog.
	// We want to collect these into a single "super" patch event.
	// To make life more interesting, we want to handle any of the
	// following:
	//
	//		CC#0, CC#32, Program		(CC#0 and CC#32 are MSB and LSB)
	//		CC#0, Program				(CC#0 is simply the bank number)
	//		CC#32, Program				(CC#32 is simply the bank number)
	//    Program+100, Program		(Two-in-a-row: 1st is (bank + 100), 2nd is (patch)
	//
	// Note that one combination which is not legal and which we
	// don't handle is CC#32 followed by CC#0.
	//
	// Don't be confused that we look for CC#32 first in the code
	// below -- this code is working BACKWARDS in the buffer.
	// We let the regular 'case CONTROL:' code store the bank
	// select controllers just like any others.  Only when we get
	// a patch change do we look back for these and eat them up.

	ASSERT(rEvtPatch.GetType() == CMfxEvent::Patch);
	ASSERT(ixInsert <= trk.size());

	int nBank = BANK_NONE;
	BankSelMethod bsm = Normal;
	int ix = ixInsert;

	// If there is a prior event, and it's a relevant Patch...
	BOOL bDelete = bCanDelete;
	if((0 < ix) && findPatch100BankForPatch(rEvtPatch, trk, &ix, &bDelete)) {
		// Transform that prior event rather than add a new one.

		// This is the two-patch-changes-in-a-row scheme.
		// The first patch change's number is the bank + 100.
		// E.g., bank 0 is patch number 100.
		// So subtract 100 to get the bank number.

		bsm = Patch100;
		nBank = trk[ix].GetPatch() - 100;
		if(bDelete) {
			trk.erase(trk.begin() + ix);
			--ixInsert; // Event was removed, move insertion point back also.
		}
	} else {
		// If there is a prior event, and it's a relevant CC#32...
		bDelete = bCanDelete;
		if((0 < ix) && findBankForPatch(rEvtPatch, 32, trk, &ix, &bDelete)) {
			// Transform that prior event rather than add a new one.

			// The bank number is what we stored for the controller
			// value at the prior event.

			if(trk[ix].GetType() == CMfxEvent::Control) {
				bsm = Ctrl32;
				nBank = trk[ix].GetCtrlVal();
				if(bDelete) {
					trk.erase(trk.begin() + ix);
					--ixInsert; // Event was removed, move insertion point back also.
				}
			} else if(trk[ix].GetType() == CMfxEvent::Patch) {
				// We found a previous Patch event that doesn't qualify as
				// a "Patch Patch"
				ix = 0; // Found previous Patch, cancel search for CC#0 below.
			}
		}

		// If there is a prior event, and it's a relevant CC#0...
		if((0 < ix) && findBankForPatch(rEvtPatch, 0, trk, &ix, &bDelete)) {
			// Transform that prior event rather than add a new one.

			// The bank number is what we stored for the controller
			// value at the prior event.  But also check to see if
			// we already found a bank number from a CC#32.  In that
			// case, this CC#0 is the high 7 bits to add to the low 7
			// bits we already have from the CC#32.

			if(BANK_NONE == nBank) {
				bsm = Ctrl0;
				nBank = (trk[ix].GetCtrlVal());
			} else {
				bsm = Normal;
				nBank = nBank + (trk[ix].GetCtrlVal() << 7);
			}

			if(bDelete) {
				trk.erase(trk.begin() + ix);
				--ixInsert; // Event was removed, move insertion point back also.
			}
		}
	}

	// rEvtPatch is 'const', so make a copy
	CMfxEvent evtNew = rEvtPatch;
	evtNew.SetBankSelectMethod(bsm); // set the bank select method we determined
	evtNew.SetBank(nBank); // set the bank number we determined

	trk.insert(trk.begin() + ixInsert, evtNew);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

static DWORD usecsToFromTempo100(DWORD dwIn) {
	// This can convert either from 100*BPM to uSec/QN or vice-versa, since the
	// conversion is the same either way.  Calculates (60,000,000) / (x / 100)
	// without losing any precision.  The dwTempo100 argument has been expanded
	// to a full DWORD so that a microseconds per quarter note count may be
	// passed instead.  This is more flexible but slower than the TempoToUsecs()
	// function above.

	const DWORD dwQuo = 3000000000L / dwIn; // Scale numerator down by factor of 2
	const DWORD dwRem = 3000000000L % dwIn; // (note remainder is a DWORD, like divisor)
	DWORD dwOut = dwQuo * 2; // Scale quotient back up by factor of 2
	long lRemFactor = (dwRem * 4) - dwIn; // and remainder
	if(lRemFactor >= 0) {
		dwOut += 1; // Bump up 1 for >=.5 round-up
		lRemFactor -= dwIn * 2;
		if(lRemFactor >= 0)
			dwOut += 1; // Bump up 1 for numerator scale factor's effect
	}

	return dwOut;
}

/////////////////////////////////////////////////////////////////////////////

static BOOL readMTrk(CFile& rFile, CMfxSeq& rMfxSeq, int nTrk,
                     WORD wFmt, DWORD dwLength,
                     CReadQueue& readQueue, HRESULT* phr) {
	// This function should be called with increasing values for 'nTrk',
	// starting with -1 for the very first track chunk.  If done this way,
	// this function can handle both format 0 and format 1 files with no
	// problem:
	//
	// If nTrk == -1, we ignore track-specific events like text events,
	// treating this as the "conductor" track of a format 1 file or the
	// only track of a format 0 file.
	//
	// Any MIDI voice events that occur when nTrk==-1 are put on track zero,
	// on the assumption that this is therefore a format 0 file.
	//
	// "Conductor" events like meter, tempo, and keysig are ignored unless
	// nTrk == -1.
	//
	// For a Format 1 file we should get only conductor events for
	// nTrk == -1.  Subsequent values of nTrk will map to increasing
	// track numbers.  If we get real track events on the first track,
	// it's up to caller to notice this and give nTrk an extra increment
	// before passing it on the next call to us.
	//
	// Also, we combine any consecutive Sysx messages that happen in the same
	// track and without any other intervening events into a single bank.

	enum { ECHAN_0 = 0,
		   ECHAN_15 = 15,
		   ECHAN_NONE,
		   ECHAN_MULTI };
	int ixBankPatch = -1;
	int eChan = ECHAN_NONE;
	LONG tAbs = 0;
	LONG tFirstNote = LONG_MAX;
	BOOL bInSetup = TRUE; // In Setup Mesasures? (Seen any notes yet?)
	BYTE byRunStat = 0;
	BYTE byCable = 0xFF;
	LONG tSysx = LONG_MAX;
	BYTE* pSysxData = NULL; // Used for SysxData
	DWORD dwSysxLen = 0;
	int ixPrevLyric = 0;
	BOOL bWasPrevLyric = FALSE;
	int nLFs = 0;
	int nCRs = 0;

	CxRPNContext xRPNContext; // for compiling Controllers into xRPN CEvents

	ASSERT(readQueue.HangingNotes() == 0);

	// Detect mutant MIDI files that don't contain End Track events:
	// The Roland JW-50 w/ disk drive writes such files with a correct
	// track length, but no META_ENDTRACK event.

	DWORD dwStart = rFile.GetPosition();
	while(*phr == S_OK) {
		//  If we reach the EXACT end of a track without getting an ENDTRACK
		//  event, see if the next thing in the file looks like another MfxTrack.
		//  This is so that the JW-50 fix doesn't break anything else.  If
		//  someone wrote a bad length, but an ENDTRACK at the end, we would
		//  have handled that OK before.

		if(((rFile.GetPosition() - dwStart) == dwLength) && lookForMTrk(rFile, phr))
			goto ENDTRACK;

		tAbs += readSMFVarLen(rFile, phr);

		BYTE by = readSMFByte(rFile, phr);

		if(by == 0xF0) // Sysx event
		{
			// F0 <length> <bytes not including F0 but including F7>
			DWORD dwLen = readSMFVarLen(rFile, phr); // doesn't include F0

			// We now always attempt to do this when there are no intervening
			// events bewtween sysx banks.
			LONG const tDiff = tAbs - tSysx;
			BOOL const bSameTime = (tDiff == 0);
			if((tSysx != LONG_MAX) && bSameTime) {
				// Don't create new message if Sysx at same time as previous message.
				ASSERT(pSysxData); // pSysxData points to previous Sysx data.
				while((S_OK == *phr) && (by != 0xF7)) {
					BYTE* pSysxAdd = new BYTE[dwSysxLen + dwLen + 1];
					if(pSysxAdd) {
						// Copy old array to new
						CopyMemory(pSysxAdd, pSysxData, dwSysxLen);
						delete[] pSysxData;
						pSysxData = pSysxAdd;
						pSysxAdd = pSysxData + dwSysxLen; // point to start of new message
						*pSysxAdd = SYSX;
						if((DWORD)rFile.Read(&pSysxAdd[1], dwLen) != dwLen) {
							*phr = S_FALSE;
							delete[] pSysxData;
							pSysxData = NULL;
						}
						dwSysxLen += dwLen + 1;
						pSysxAdd = NULL;
					} else {
						ASSERT(FALSE);
						*phr = E_OUTOFMEMORY;
					}

					// Check for missing F7, which means we need to read
					// continuation events.  We'll ignore the leading timing.
					// There can be one or more continuation events.  All
					// will start with F7.  None except the last will end
					// with F7.
					//
					// MIDI File spec is not as clear as it could be, but
					// it implies the next event must be a sysx F7 event.
					// So, we'll require that or raise error.

					by = pSysxData[dwSysxLen - 1];

					if(by != 0xF7)
						(void)readSMFVarLen(rFile, phr); // ignore the time
				}
			} else {
				// Put Sysx message into byte array.
				if(pSysxData) // previous message must've been Sysx at a different time
				{
					AddSysxData(
					pSysxData, dwSysxLen, tSysx,
					rMfxSeq, nTrk, wFmt, byRunStat,
					&tFirstNote, &bInSetup, phr);
					SAFE_ARRAY_DELETE(pSysxData);
				}
				dwSysxLen = 0;
				tSysx = tAbs; // Remember time of last sysx event
				pSysxData = new BYTE[dwLen + 1];
				if(pSysxData) {
					*pSysxData = SYSX; // stick in original Sysx ID
					if((DWORD)rFile.Read(&pSysxData[1], dwLen) != dwLen) {
						*phr = S_FALSE;
						delete[] pSysxData;
						pSysxData = NULL;
					}
					dwSysxLen = dwLen + 1;
				}
			}
		} else if(by == 0xF7) // Sysx continuation event
		{
			// F7 <length> <bytes not including F0 but including F7>
			// Technically incorrect, so skip it.
			if(pSysxData) {
				AddSysxData(
				pSysxData, dwSysxLen, tSysx,
				rMfxSeq, nTrk, wFmt, byRunStat,
				&tFirstNote, &bInSetup, phr);
				SAFE_ARRAY_DELETE(pSysxData);
			}
			dwSysxLen = 0;
			tSysx = LONG_MAX; // Reset "merge sysx banks" logic

			if(rFile.Seek(readSMFVarLen(rFile, phr), CFile::current) < 0L)
				*phr = STG_E_INVALIDFUNCTION;
		} else if(by == META_EVENT) // Meta-Event
		{
			if(pSysxData) {
				AddSysxData(
				pSysxData, dwSysxLen, tSysx,
				rMfxSeq, nTrk, wFmt, byRunStat,
				&tFirstNote, &bInSetup, phr);
				SAFE_ARRAY_DELETE(pSysxData);
			}
			dwSysxLen = 0;
			tSysx = LONG_MAX; // Reset "merge sysx banks" logic

			BYTE byMeta = readSMFByte(rFile, phr);
			switch(byMeta) {
				case META_CHANPREFIX:
					readSMFByte(rFile, phr); // skip the length byte
					byCable = readSMFByte(rFile, phr);
					break;

				case META_CABLE:
					readSMFByte(rFile, phr); // skip the length byte
					byCable = readSMFByte(rFile, phr);

					if(tAbs == 0) {
						// This will return NULL if reading conductor track:
						CMfxTrack* pTrk = getTrack(rMfxSeq, wFmt, nTrk);
						if(pTrk)
							pTrk->SetPort((int)byCable);
					}
					// else if not beginning of track, ignore. What else can we do?
					break;

				case META_COPYRIGHT:
					goto SKIP_IT;
					break;

				case META_TEXT:
					if(0 == tAbs) {
						// Text at time 0 are comments, skip them
						goto SKIP_IT;
						break;
					}
					// Text after time 0 gets loaded below as a normal text event...
					goto FALL_THRU;

				case META_TRACKNAME:
					if(nTrk == -1) {
						// Track name in conductor is really "sequence name" or Title:
						goto SKIP_IT;
						break;
					}
					// else we already have a Song Title, so fall through...
				FALL_THRU:
				case META_INSTRNAME:
				case META_MARKER:
				case META_CUEPOINT:
					if(nTrk != -1) {
						// Allow for MIDI Files where track name is stored
						// as General Text.  If time is 0 and no track name
						// already treat a General Text as track name.

						CMfxTrack* pTrk = getTrack(rMfxSeq, wFmt, nTrk);
						if(NULL != pTrk) {
							if(
							byMeta == META_TRACKNAME ||
							((byMeta == META_TEXT) && (tAbs == 0) &&
							 ((pTrk->GetName() == NULL) || (0 == strlen(pTrk->GetName()))))) {
								// Use as track's primary name

								CSMFText spTrkName;

								spTrkName.Read(rFile, phr);
								if(S_OK == *phr) {
									pTrk->SetName(spTrkName.GetStringPtr());
								}
							} else {
								// Insert as text event

								CSMFText spText;

								spText.Read(rFile, phr);
								if(S_OK == *phr) {
									CMfxEvent event(MfxEvent::Text);
									event.SetText(spText.GetStringPtr());
									event.SetTime(tAbs);
									pTrk->push_back(event);
								}
							}
						} else {
							ASSERT(FALSE);
							*phr = E_OUTOFMEMORY;
						}
					} else {
						// Track name in conductor is really "sequence name" or Title:
						if(
						byMeta == META_TRACKNAME ||
						((byMeta == META_TEXT) && (tAbs == 0))) {
							CSMFText spTitle;
							spTitle.Read(rFile, phr);
						} else {
							// Conductor track: treat as Markers.

							CSMFText spMarker;
							spMarker.Read(rFile, phr);
							if(S_OK == *phr) {
								CMarker marker;
								marker.SetTime(tAbs);
								marker.SetName(spMarker.GetStringPtr());
								rMfxSeq.m_markers.push_back(marker);
							}
						}
					}
					break;

				case META_LYRIC:
					// Lyrics after time 0 get loaded as normal lyric events...
					{
						CSMFText spLyric;

						// In type 0 MIDI files, we won't get a stream handle
						// unless we specify a channel in the call to getStream().
						// According to MMA guidelines, Lyrics should default to track #4

						CMfxTrack* pTrk;
						if(-1 == nTrk)
							pTrk = getTrack(rMfxSeq, wFmt, nTrk, s_nLyriCMfxTrack); // Track #4
						else
							pTrk = getTrack(rMfxSeq, wFmt, nTrk, 0); // Track #nTrk

						if(pTrk) {
							spLyric.Read(rFile, phr);
							if(S_OK == *phr) {
								// Possibly combine newlines with the previous lyric event:
								int iLen = lstrlenA(spLyric.GetStringPtr());
								char* pLastChar = spLyric.GetHotStringPtr() + (iLen - 1);
								if((iLen == 1) && ((*pLastChar == '\r') || (*pLastChar == '\n'))) {
									if(bWasPrevLyric) {
										CMfxEvent event = (*pTrk)[ixPrevLyric];
										spLyric.Prepend(event.GetText());
										event.SetText(spLyric.GetStringPtr());
										(*pTrk)[ixPrevLyric] = event;
									}
									// Else discard leading newlines.
								} else {
									CMfxEvent event(MfxEvent::Lyric);
									event.SetText(spLyric.GetStringPtr());
									event.SetTime(tAbs);
									ixPrevLyric = pTrk->size();
									pTrk->push_back(event);

									bWasPrevLyric = TRUE;
									nCRs = 0;
									nLFs = 0;
								}
							}
						} else {
							ASSERT(FALSE);
							*phr = E_OUTOFMEMORY;
						}
					}
					break;

				case META_ENDTRACK:
					readSMFByte(rFile, phr); // skip length byte
					goto ENDTRACK;

				case META_TEMPO:
					if(nTrk == -1) // Conductor track only.
					{
						readSMFByte(rFile, phr); // skip length byte
						DWORD ms = (DWORD)readSMFByte(rFile, phr) << 16;
						ms |= (DWORD)readSMFByte(rFile, phr) << 8;
						ms |= readSMFByte(rFile, phr);

						if(ms > 0) // don't cause div by 0 on bad tempos =>ignore them
						{
							int bpm = usecsToFromTempo100(ms);

							// Enforce maximum tempo of 250 BPM:

							if(bpm < CTempo::GetMinBPM())
								bpm = CTempo::GetMinBPM();
							else if(bpm > CTempo::GetMaxBPM())
								bpm = CTempo::GetMaxBPM();

							CTempo tempo(tAbs, bpm);
							rMfxSeq.m_tempoMap.push_back(tempo);
							rMfxSeq.m_tempoMap.Recalc();
						}
					} else
						goto SKIP_IT;
					break;

				case META_SMPTEOFS:
					goto SKIP_IT;
					break;

				case META_METER:
					if(nTrk == -1) {
						readSMFByte(rFile, phr); // skip length byte
						BYTE byNum = readSMFByte(rFile, phr);
						BYTE byDen = readSMFByte(rFile, phr);
						readSMFByte(rFile, phr); // clocks per metro tick
						readSMFByte(rFile, phr); // 32nd notes per MIDI 1/4 note

						// Since we store meters and keys together, be careful
						// to get the current meter/key, then "or" in our new
						// meter value.

						CMfxMeterKeySigMap& rMfxMeterKeySigMap = rMfxSeq.m_meterKeySigMap;
						CMfxMeterKeySigMap::iterator it = find_if(rMfxMeterKeySigMap.begin(), rMfxMeterKeySigMap.end(), TimeCompareEQ<CMeterKey>(tAbs));
						if(it != rMfxMeterKeySigMap.end()) {
							CMeterKey& mk = *it;
							mk.SetTop(byNum);
							mk.SetBottom(byDen);
						} else {
							CMeterKey mk;
							mk.SetMeasure(rMfxMeterKeySigMap.Tick2Meas(tAbs));
							mk.SetTop(byNum);
							mk.SetBottom(byDen);
							rMfxMeterKeySigMap.push_back(mk);
						}
						rMfxMeterKeySigMap.Recalc();
					} else
						goto SKIP_IT;
					break;

				case META_KEYSIG:
					if(nTrk == -1) {
						readSMFByte(rFile, phr); // skip length
						char cKey = (char)readSMFByte(rFile, phr);
						readSMFByte(rFile, phr); // major/minor - ignore

						// See notes for meter.  Same idea here, but we're
						// adding a key signature to prevailing meter.

						CMfxMeterKeySigMap& rMfxMeterKeySigMap = rMfxSeq.m_meterKeySigMap;
						CMfxMeterKeySigMap::iterator it = find_if(rMfxMeterKeySigMap.begin(), rMfxMeterKeySigMap.end(), TimeCompareEQ<CMeterKey>(tAbs));
						if(it != rMfxMeterKeySigMap.end()) {
							CMeterKey& mk = *it;
							mk.SetKeySig(cKey);
						} else {
							CMeterKey mk;
							mk.SetMeasure(rMfxMeterKeySigMap.Tick2Meas(tAbs));
							mk.SetKeySig(cKey);
							rMfxMeterKeySigMap.push_back(mk);
						}
						rMfxMeterKeySigMap.Recalc();
					} else
						goto SKIP_IT;
					break;

				case META_SEQSPEC:
					goto SKIP_IT;
					break;

				default: // no one read it; need to skip it
				SKIP_IT : {
					DWORD dwLen = readSMFVarLen(rFile, phr);
					if(rFile.Seek(dwLen, CFile::current) < 0L)
						*phr = STG_E_INVALIDFUNCTION;
					break;
				}
			}
		} // if (by == META_EVENT)
		else {
			if(pSysxData) {
				AddSysxData(
				pSysxData, dwSysxLen, tSysx,
				rMfxSeq, nTrk, wFmt, byRunStat,
				&tFirstNote, &bInSetup, phr);
				SAFE_ARRAY_DELETE(pSysxData);
			}
			dwSysxLen = 0;
			tSysx = LONG_MAX; // Reset "merge sysx banks" logic

			if(0x80 <= by && by <= 0xEF) // status byte
			{
				byRunStat = by;

				ASSERT(0x00 != byRunStat); // running status cancelled!
				ASSERT(0x80 <= byRunStat && byRunStat <= 0xEF); // valid status byte

				// This tracks the channels of events to see if all the same and
				// thus if we may apply a forced channel for the stream when done
				// at the end-of-track meta-event below.

				if(eChan == ECHAN_NONE)
					eChan = CHAN(byRunStat);
				else if(eChan != ECHAN_MULTI) {
					if(eChan != (int)CHAN(byRunStat))
						eChan = ECHAN_MULTI;
				}

				by = readSMFByte(rFile, phr);
			}

			if(by < 0x80) {
				ASSERT(0x00 != byRunStat); // running status cancelled!
				ASSERT(0x80 <= byRunStat && byRunStat <= 0xEF); // valid status byte

				CMfxTrack* pTrk = getTrack(rMfxSeq, wFmt, nTrk, CHAN(byRunStat));
				if(NULL == pTrk) {
					ASSERT(FALSE);
					*phr = E_OUTOFMEMORY;
					return FALSE; // must be E_OUTOFMEMORY, since we supplied channel
				}

				switch(byRunStat & 0xF0) {
					case CHANAFT: // CHANAFT <pressure>
					{
						CMfxEvent event(MfxEvent::ChanAft);
						event.SetTime(tAbs);
						event.SetChannel(CHAN(byRunStat));
						event.SetPressure(by);

						pTrk->push_back(event);
						break;
					}

					case KEYAFT: // KEYAFT <key> <pressure>
					{
						CMfxEvent event(MfxEvent::KeyAft);
						event.SetTime(tAbs);
						event.SetChannel(CHAN(byRunStat));
						event.SetKey(by);
						event.SetPressure(readSMFDataByte(rFile, phr));

						pTrk->push_back(event);
						break;
					}

					case WHEEL: // WHEEL <wheellsb> <wheelmsb>
					{
						CMfxEvent event(MfxEvent::Wheel);
						event.SetTime(tAbs);
						event.SetChannel(CHAN(byRunStat));
						BYTE byLSB = by;
						BYTE byMSB = readSMFDataByte(rFile, phr);
						short nWheel = static_cast<short>(static_cast<WORD>(byMSB) << 7) | byLSB;
						nWheel -= 8192;
						event.SetWheel(nWheel);

						pTrk->push_back(event);
						break;
					}

					case CONTROL: // CONTROL <ctrlnum> <value>
					{
						CMfxEvent event(MfxEvent::Control);
						event.SetTime(tAbs);
						event.SetChannel(CHAN(byRunStat));
						event.SetCtrlNum(by);
						event.SetCtrlVal(readSMFDataByte(rFile, phr));

						if(!xRPNContext.OnController(event.GetTime(), event.GetChannel(), event.GetCtrlNum(), event.GetCtrlVal(), pTrk, pTrk->size()))
							*phr = E_OUTOFMEMORY;
						break;
					}

					case NOTE_OFF: // NOTE_OFF <key> <velocity>	(yes, OFFs have velocities too!)
					case NOTE_ON: // NOTE_ON <key> <velocity>
					{
						CMfxEvent event(MfxEvent::Note);
						event.SetTime(tAbs);
						event.SetChannel(CHAN(byRunStat));
						event.SetKey(by);
						event.SetVel(readSMFDataByte(rFile, phr));
						event.SetDur(1);

						bInSetup = FALSE; // We've seen a note in this track!
						if(tFirstNote > event.GetTime())
							tFirstNote = event.GetTime();

						if(
						((byRunStat & 0xF0) == NOTE_OFF) ||
						((byRunStat & 0xF0) == NOTE_ON && event.GetVel() == 0)) {
							// It's a note off.  Queue record of it for later handling.
							// Then skip back to top of loop.

							if(readQueue.IsHanging(event.GetChannel(), event.GetKey())) {
								readQueue.Remove(*pTrk, tAbs, event.GetChannel(), event.GetKey());
							} else {
								// There was no Note On for this Note Off.
								// Insert a zero-duration Note:

								event.SetDur(0);

								pTrk->push_back(event);

								// Should we also try to detect a later Note On
								// at the same MIDI time?
							}

							// don't insert as event in track
						} else {
							// Note-on. Duration gets filled in with LONG_MAX to flag that
							// it needs a duration when we get the note-off and search
							// back for it (above).
							event.SetDur(USHRT_MAX);

							// Handle due queued note-off's

							int ixNote = pTrk->size();
							pTrk->push_back(event);
							readQueue.Insert(*pTrk, tAbs, event.GetChannel(), event.GetKey(), ixNote);
						}
						break;
					}

					case PROGRAM: // PROGRAM <patch>
					{
						CMfxEvent event(MfxEvent::Patch);
						event.SetTime(tAbs);
						event.SetChannel(CHAN(byRunStat));
						event.SetPatch(by);

						// If time is 1:1:0 and track has no starting patch or bank
						// parameter yet, then don't insert event in stream but
						// rather set the stream parameter(s).

						// Can only delete bank controller events if no hanging notes!
						if(!combineBankAndPatch(event, *pTrk, pTrk->size(), 0 == readQueue.HangingNotes())) {
							*phr = E_OUTOFMEMORY;
						}

						if(bInSetup && (PATCH_NONE == pTrk->GetPatch())) {
							// Get the bank/patch event from the track
							ixBankPatch = pTrk->size() - 1;
							event = (*pTrk)[ixBankPatch];
							ASSERT(event.GetType() == MfxEvent::Patch);
#if 0
							if (readQueue.HangingNotes() == 0)
							{
								// maybe remove later for Type 1 file (only if forced channel)
								if (0 == wFmt)								
									pTrk->erase( pTrk->begin() + ixBankPatch ); // and delete it!
							}
#endif
							// Set track parameters (for now => we may remove if no forced channel):
							pTrk->SetBankSelectMethod(event.GetBankSelectMethod());
							pTrk->SetBank(event.GetBank());
							pTrk->SetPatch(event.GetPatch());
						}
						break;
					}
				}
			} // if (by < 0x80)
		} // else not meta-event
	} // while no error

ENDTRACK:
	if(*phr == S_OK) {
		if(1 == wFmt) {
			CMfxTrack* pTrk = getTrack(rMfxSeq, wFmt, nTrk, -1, FALSE); // bCreate=FALSE
			if(pTrk && !pTrk->empty()) {
				// last call for note-off's
				for(int nChan = 0; nChan < 16; ++nChan)
					readQueue.RemoveChannel(*pTrk, tAbs, nChan);
				ASSERT(readQueue.HangingNotes() == 0);

				MfxMuteMask mute;
				mute.Manual = 1;
				if(pTrk->empty())
					pTrk->SetMuteState(mute, 0);
				else
					pTrk->SetMuteState(0, mute);

				if(ECHAN_0 <= eChan && eChan <= ECHAN_15) {
					pTrk->SetChannel(eChan);
#if 0
					if (-1 != ixBankPatch)
					{
						// this was retained earlier, but now that we have a forced channel, yank it...
						pTrk->erase( pTrk->begin() + ixBankPatch );	
					}
#endif
				} else {
#if 1
					// No forced channel: remove any BankPatch:
					pTrk->SetBankSelectMethod(Normal);
					pTrk->SetBank(BANK_NONE);
					pTrk->SetPatch(PATCH_NONE);
#endif
				}
			}
		} else if(0 == wFmt) {
			for(int nChan = 0; nChan < 16; ++nChan) {
				CMfxTrack* pTrk = getTrack(rMfxSeq, wFmt, nTrk, nChan, FALSE); // bCreate=FALSE
				if(pTrk && !pTrk->empty()) {
					// last call for note-off's
					readQueue.RemoveChannel(*pTrk, tAbs, nChan);

					MfxMuteMask mute;
					mute.Manual = 1;
					if(pTrk->empty())
						pTrk->SetMuteState(mute, 0);
					else
						pTrk->SetMuteState(0, mute);

					pTrk->SetChannel(nChan);

					if(0 != (*pTrk)[0].GetTime())
						continue;

					// check for the prescence of a program/bank change at time zero:
					// if after the loop there is a known program change (other than -1)
					// we will set it in the track and remove the event at the appropriate ix.
					// the same goes for the bank change (cc's)
					int nProgCh = -1; // program change number, defaults to unknown (-1)
					int ixProgCh = -1; // index of program change event: defaults to unknown. (-1)

					int nProgChGE100 = -1;
					int ixProgChGE100 = -1;

					int nCC0 = -1;
					int ixCC0 = -1;

					int nCC32 = -1;
					int ixCC32 = -1;

					for(int ixEvt = 0; ixEvt < pTrk->size(); ixEvt++) {
						const CMfxEvent& evt = (*pTrk)[ixEvt];
						BOOL bRelevant = FALSE;
						switch(evt.GetType()) {
							case MfxEvent::Patch:
								if(evt.GetPatch() >= 100) {
									nProgChGE100 = evt.GetPatch();
									ixProgChGE100 = ixEvt;
									bRelevant = TRUE;
								} else {
									nProgCh = evt.GetPatch();
									ixProgCh = ixEvt;
									bRelevant = TRUE;
								}
								break;

							case MfxEvent::Control:
								switch(evt.GetCtrlNum()) {
									case 32:
										ixCC32 = ixEvt;
										nCC32 = evt.GetCtrlVal();
										bRelevant = TRUE;
										break;
									case 0:
										ixCC0 = ixEvt;
										nCC0 = evt.GetCtrlVal();
										bRelevant = TRUE;
										break;
									default:; // nothing...
								}
								break;
						} // end switch

						if(bRelevant == FALSE)
							break;
					} // ends for

					// if the patch change is determined, turn it into a track parameter.

					BankSelMethod bsm = Normal;

					if(nProgChGE100 >= 0) {
						if(nProgCh >= 0) {
							bsm = Patch100;
							pTrk->SetBankSelectMethod(bsm);
							pTrk->SetBank(nProgChGE100 - 100);
#if 0
							pTrk->erase( pTrk->begin() + ixProgChGE100 );
#endif
							if(ixProgChGE100 < ixProgCh)
								ixProgCh--;

							ixCC0 = -1;
							ixCC32 = -1;
						} else {
							nProgCh = nProgChGE100;
							ixProgCh = ixProgChGE100;
							nProgChGE100 = -1;
							ixProgChGE100 = -1;
						}
					}

					if(nProgCh >= 0) {
						pTrk->SetPatch(nProgCh);
#if 0
						pTrk->erase( pTrk->begin() + ixProgCh );
#endif
						if(ixProgCh < ixCC0)
							ixCC0--;

						if(ixProgCh < ixCC32)
							ixCC32--;
					}
					if(nCC0 >= 0 && nCC32 >= 0) {
						bsm = Normal;
						pTrk->SetBankSelectMethod(bsm);

						pTrk->SetBank(nCC32 + (nCC0 << 7));
#if 0						
						pTrk->erase( pTrk->begin() + ixCC0 );
#endif
						if(ixCC0 < ixCC32)
							ixCC32--;
#if 0						
						pTrk->erase( pTrk->begin() + ixCC32 );
#endif
						// nothing else to remove or "shift its index"
					} else if(nCC0 >= 0) {
						bsm = Ctrl0;
						pTrk->SetBankSelectMethod(bsm);

						pTrk->SetBank(nCC0);
#if 0
						pTrk->erase( pTrk->begin() + ixCC0 );
#endif
					}
				} // ends if track "valid" (see above)
			} // ends for "every channel"

			ASSERT(readQueue.HangingNotes() == 0);
		}

		///////////////////////////
		//
		// SUCCESSFUL RETURN
		//
		///////////////////////////

		return TRUE;
		///////////
	} else {
		// Don't bother destroying the MfxTrack.  Whole SEQ will get destroyed by
		// caller.

		return FALSE;
	}
}

/////////////////////////////////////////////////////////////////////////////

HRESULT LoadMid(CMfxSeq& rSeq, CFile& rFile) {
	// Call this function immediately after creating a new CMfxSeq with
	// CMfxSeq::Create().  We do not try to "empty" any existing data.

	// This function reads a MIDI File from disk or memory.

	WORD wFormat;
	int n;
	int nTrks;
	CReadQueue readQueue;
	HRESULT hr = S_OK;

	s_bGotTitle = FALSE; // Have we set the song title yet?

	// Valid MThd?

	// Offset in file to start of .MID file
	DWORD dwMidOffset = rFile.GetPosition();
	DWORD dwChunkLength = 0;

	if((readSMFChunkHeader(rFile, &dwChunkLength, &hr) != MThd) ||
	   (dwChunkLength < 6L)) {
		// This file may have come from a Macintosh.  In that case, it will
		// have a 128-byte header slapped on the beginning of the file.  Let's
		// see if we can find a "MThd" at offset 128:
		rFile.Seek(dwMidOffset + 128, CFile::begin);
		if((readSMFChunkHeader(rFile, &dwChunkLength, &hr) != MThd) ||
		   (dwChunkLength < 6L)) {
			CHECK(STG_E_DOCFILECORRUPT);
		}
	}

	// The MIDI 1.0 file specification says that lengths longer than 6
	// (current value) are reserved for future expansion, and that
	// excess bytes should be ignored.

	CHECK(readMThd(rFile, &wFormat, &nTrks, &s_wDivision));

	if(dwChunkLength > 6L) {
		ASSERT(FALSE);

		// Skip past extra bytes presumably added to header specification:
		if(rFile.Seek(dwChunkLength - 6L, CFile::current) < 0L) {
			CHECK(STG_E_INVALIDFUNCTION);
		}
	}

	// Set the file type.  Only format 1 MIDI files are supported.
	else if(0 != wFormat && 1 != wFormat) {
		CHECK(STG_E_DOCFILECORRUPT);
	}

	TRACE("Format %d MIDI file\r\n", wFormat);

	rSeq.SetPPQ(s_wDivision);

	// Now read the track chunks.  Note that we start with a track number of
	// -1, not zero, as a flag to readMTrk that the first track is to be
	// interpreted as a "conductor" MfxTrack.

	for(n = -1; n < nTrks - 1;) {
		if(readSMFChunkHeader(rFile, &dwChunkLength, &hr) == MTrk) {
			if(readMTrk(rFile, rSeq, n, wFormat, dwChunkLength, readQueue, &hr)) {
				// If this is first MIDI File track, check to see whether
				// it (incorrectly) contains non-conductor events.  If so,
				// give our track number counter an extra boost, so that
				// the first two tracks aren't combined.  readMTrk() puts
				// non-conductor events found in the MF conductor track in
				// a stream with SEQ_TRKNUM == 0.  So we want to bump our
				// counter from -1 to 1, not to 0.

				if(n == -1) {
					// Check for actual events in an actual stream:
					CMfxTrack* pTrk = rSeq.GetTrack(0);
					if(pTrk && !pTrk->empty()) {
						++n; // extra increment
						++nTrks; // inc total tracks too!
					}
				}
				++n;
			} else {
				hr = S_FALSE;
				break;
				/////
			}
		} else if(S_OK == hr) {
			// Unknown chunk type: skip it
			if(rFile.Seek(dwChunkLength, CFile::current) < 0L)
				hr = STG_E_INVALIDFUNCTION;
		} else // error: can't continue!
			break;
		/////
	}

	// Sort all tracks when we're done
	TimeCompare<CMfxEvent> pred;
	map<int, CMfxTrack>::iterator it = rSeq.GetBeginTrack();
	while(it != rSeq.GetEndTrack()) {
		CMfxTrack& trk = it->second;
		sort(trk.begin(), trk.end(), pred);
		it++;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
