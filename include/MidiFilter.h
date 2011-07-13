// Copyright (c) 1998-99 by Twelve Tone Systems, Inc. All rights reserved.
#pragma once
/////////////////////////////////////////////////////////////////////////////
//
// MFX
//
/////////////////////////////////////////////////////////////////////////////

// Make sure to we've defined an MFX version number
#if !defined(MFX_VERSION)
	#define MFX_VERSION (11)
#endif

// This file must be compiled in C++
#if !defined(__cplusplus)
	#pragma error "MidiFilter.h requires C++ compilation"
#endif

/////////////////////////////////////////////////////////////////////////////

// {8D7FDE01-1B1B-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxEventFilter =
	{ 0x8d7fde01, 0x1b1b, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {FF76F7C2-F166-11d1-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxEventQueue =
	{ 0xff76f7c2, 0xf166, 0x11d1, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {7A37A621-1B1B-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxDataQueue =
	{ 0x7a37a621, 0x1b1b, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };
	
// {FF76F7C3-F166-11d1-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxTempoMap =
	{ 0xff76f7c3, 0xf166, 0x11d1, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf} };

// {FF76F7C4-F166-11d1-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxMeterMap =
	{ 0xff76f7c4, 0xf166, 0x11d1, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf} };

// {A489E601-0247-11d2-A8E0-0000A0090B97}
extern "C" const GUID __declspec(selectany) IID_IMfxKeySigMap =
	{ 0xa489e601, 0x247, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xb, 0x97 } };

// {6BFCA001-043D-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxInstruments =
	{ 0x6bfca001, 0x43d, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {24385AC1-02A8-11d2-A8E0-0000A0090B97}
extern "C" const GUID __declspec(selectany) IID_IMfxNameListSet =
	{ 0x24385ac1, 0x2a8, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xb, 0x97 } };

// {857F3281-02A9-11d2-A8E0-0000A0090B97}
extern "C" const GUID __declspec(selectany) IID_IMfxNameList =
	{ 0x857f3281, 0x2a9, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xb, 0x97 } };

// {838C6961-0C50-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxSelection =
	{ 0x838c6961, 0xc50, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {F5C46121-3C4A-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxInputPulse =
	{ 0xf5c46121, 0x3c4a, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

////////////////////////////////////////////////////////////////////////////////
// The following are recognized by Cakewalk Pro Audio v9.x or better:

#if (MFX_VERSION > 8)

// {F228EE01-DE34-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxDocumentFactory =
	{ 0xf228ee01, 0xde34, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {089058C1-EEB3-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxDocument =
	{ 0x89058c1, 0xeeb3, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {1DA99B21-E091-11d2-A8E0-0000A0090B97}
extern "C" const GUID __declspec(selectany) IID_IMfxMarkerMap =
	{ 0x1da99b21, 0xe091, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xb, 0x97 } };

// {C779B641-EDE2-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxBufferFactory =
	{ 0xc779b641, 0xede2, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

// {C51273A1-F0EA-11d2-A8E0-0000A0090DAF}
extern "C" const GUID __declspec(selectany) IID_IMfxEventQueue2 =
	{ 0xc51273a1, 0xf0ea, 0x11d2, { 0xa8, 0xe0, 0x0, 0x0, 0xa0, 0x9, 0xd, 0xaf } };

#endif // (MFX_VERSION > 8)

////////////////////////////////////////////////////////////////////////////////
// The following are recognized by Cakewalk Sonar 1.0 or better:

#if (MFX_VERSION > 9)

// {5BBDF239-3046-4daa-8ADD-3B9373545B74}
extern "C" const GUID __declspec(selectany) IID_IMfxSoftSynth =
	{ 0x5bbdf239, 0x3046, 0x4daa, { 0x8a, 0xdd, 0x3b, 0x93, 0x73, 0x54, 0x5b, 0x74 } };

// {BADA6CF9-77A5-4f48-B6FA-66154E4B163B}
extern "C" const GUID __declspec(selectany) IID_IMfxNameList2 =
	{ 0xbada6cf9, 0x77a5, 0x4f48, { 0xb6, 0xfa, 0x66, 0x15, 0x4e, 0x4b, 0x16, 0x3b } };

// {753B2366-6CD2-4e61-999B-9128FCDB7BCD}
extern "C" const GUID __declspec(selectany) IID_IMfxNotify =
	{ 0x753b2366, 0x6cd2, 0x4e61, { 0x99, 0x9b, 0x91, 0x28, 0xfc, 0xdb, 0x7b, 0xcd } };

#endif // (MFX_VERSION > 9)

////////////////////////////////////////////////////////////////////////////////
// The following are recognized by Cakewalk Sonar 2.0 or better:

#if (MFX_VERSION > 10)

// {FF94ABA0-4DF9-4120-947D-5E303E49B81F}
extern "C" const GUID __declspec(selectany) IID_IMfxTimeConverter =
	{ 0xff94aba0, 0x4df9, 0x4120, { 0x94, 0x7d, 0x5e, 0x30, 0x3e, 0x49, 0xb8, 0x1f } };

// {439E1EF6-2CBA-48f8-A857-51C5724A4F6C}
extern "C" const GUID __declspec(selectany) IID_IMfxNotifyHost =
	{ 0x439e1ef6, 0x2cba, 0x48f8, { 0xa8, 0x57, 0x51, 0xc5, 0x72, 0x4a, 0x4f, 0x6c } };

// {2AC171DF-81E3-496c-BA7A-92EF10B417D9}
extern "C" const GUID __declspec(selectany) IID_IMfxInstrument = 
	{ 0x2ac171df, 0x81e3, 0x496c, { 0xba, 0x7a, 0x92, 0xef, 0x10, 0xb4, 0x17, 0xd9 } };

// {6E88453F-4A85-4159-B658-88C43D288287}
extern "C" const GUID __declspec(selectany) IID_IMfxSoftSynth2 =
	{ 0x6e88453f, 0x4a85, 0x4159, { 0xb6, 0x58, 0x88, 0xc4, 0x3d, 0x28, 0x82, 0x87 } };

// {FA1B1A27-8B1E-4c4c-9EAE-F8C60E05381D}
extern "C" const GUID __declspec(selectany) IID_IMfxInputCallback = 
	{ 0xfa1b1a27, 0x8b1e, 0x4c4c, { 0x9e, 0xae, 0xf8, 0xc6, 0xe, 0x5, 0x38, 0x1d } };

// {49C2EE08-7DCA-4df3-9A41-35CFDB44F12E}
extern "C" const GUID __declspec(selectany) IID_IMfxInputPort =
	{ 0x49c2ee08, 0x7dca, 0x4df3, { 0x9a, 0x41, 0x35, 0xcf, 0xdb, 0x44, 0xf1, 0x2e } };

// {1516243C-0D2C-4f9d-964D-F3D73D53F5D1}
extern "C" const GUID __declspec(selectany) IID_IMfxMarkerMap2 =
	{ 0x1516243c, 0x0d2c, 0x4f9d, { 0x96, 0x4d, 0xf3, 0xd7, 0x3d, 0x53, 0xf5, 0xd1 } };

#endif // (MFX_VERSION > 10)

////////////////////////////////////////////////////////////////////////////////

#ifndef __MFX_H__
#define __MFX_H__

#pragma pack( push, ENTER_MFX_H )	// save struct packing

#pragma warning( disable: 4201 )		// nonstandard extension used : nameless struct/union

/////////////////////////////////////////////////////////////////////////////

// Name of key under HKEY_LOCAL_MACHINE; create a subkey with the string
// form of your CLSID
#define SZ_MIDI_FILTER_REGKEY "Software\\Cakewalk Music Software\\MIDI Filters"
#define SZ_SOFT_SYNTH_REGKEY	"Software\\Classes\\MfxSoftSynths"

/////////////////////////////////////////////////////////////////////////////

interface IMfxEventFilter;
interface IMfxEventQueue;
interface IMfxDataQueue;
interface IMfxTempoMap;
interface IMfxMeterMap;
interface IMfxKeySigMap;
interface IMfxInstruments;
interface IMfxNameListSet;
interface IMfxNameList;
interface IMfxSelection;
interface IMfxInputPulse;

struct MfxEvent;
struct MfxData;

#if (MFX_VERSION > 8)
interface IMfxDocumentFactory;	// added after Pro Audio 8
interface IMfxDocument;				// added after Pro Audio 8
interface IMfxMarkerMap;			// added after Pro Audio 8
interface IMfxBufferFactory;		// added after Pro Audio 8
#endif // (MFX_VERSION > 8)

#if (MFX_VERSION > 9)
interface IMfxSoftSynth;
interface IMfxNameList2;
interface IMfxNotify;
struct MfxNotifyMsg;
#endif // (MFX_VERSION > 9)

#if (MFX_VERSION > 10)
struct MfxDataEx;
interface IMfxTimeConverter;
interface IMfxNotifyHost;
interface IMfxInstrument;
interface IMfxSoftSynth2;
interface IMfxInputCallback;
interface IMfxInputPort;
interface IMfxMarkerMap2;
#endif // (MFX_VERSION > 10)

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxEventFilter, IUnknown )
{
	// This is the interface that a MIDI plugin filter must implement.
	// It might also implement some other standard COM interfaces:
	//		ISpecifyPropertyPages - To provide a user interface
	//		IPersistStream - To provide support for presets

	// This is called before any other members are called.
	// You can use the IUnknown* pContext to QueryInterface() for host
	// interfaces like IMfxTempoMap or IMfxMeterMap.
	//
	// Unless you retain a copy of the IUnknown* pContext, be sure to call
	// pContext->Release().
	//
	// If you QueryInterface() for something and it's not available, you
	// can return E_FAIL to fail; the application will not use your filter.
	// Otherwise you should return NOERROR or S_OK if you got what you needed.
	STDMETHOD( Connect )(THIS_ IUnknown* pContext ) PURE;

	// This is called when the filter will not be used any longer.
	// You must call Release() on any interface pointers you obtained
	// from the IUnknown* passed to you in Connect().
	STDMETHOD( Disconnect )(THIS) PURE;

	// Called when playback (or other streaming session) starts, with the
	// start time.
	STDMETHOD( OnStart )(THIS_ LONG lTime, IMfxEventQueue* pqOut ) PURE;

	// Called when playback (or other streaming session) loops back to an
	// earlier time without stopping and restarting.
	STDMETHOD( OnLoop )(THIS_ LONG lTimeRestart, LONG lTimeStop, IMfxEventQueue* pqOut  ) PURE;

	// Called when playback (or other streaming session) stops, with the
	// stop time.
	STDMETHOD( OnStop )(THIS_ LONG lTime, IMfxEventQueue* pqOut  ) PURE;

	// This is called when a window of time is being processed, represented by
	// lTimeFrom..lTimeThru.
	//
	// If there are any events to process, they are in pqIn.
	//
	// For each MfxEvent item in pqIn:
	// - To pass it on unchanged, pass it to IMfxEventQueue::Add() [pqOut->Add()].
	// - To modify it, pass the modified MfxEvent to IMfxEventQueue::Add().
	// - To delete it, do not call IMfxEventQueue::Add().
	// To create one or more additional events, call IMfxEventQueue::Add().
	STDMETHOD( OnEvents )(THIS_ LONG lTimeFrom, LONG lTimeThru, IMfxEventQueue* pqIn, IMfxEventQueue* pqOut ) PURE;

	// This is called to give you an opportunity to process MIDI input.
	//
	// For each MfxData item in pqIn:
	// - To pass it on unchanged, pass it to IMfxDataQueue::Add() [pqOut->Add()].
	// - To modify it, pass the modified MfxData to IMfxDataQueue::Add().
	// - To delete it, do not call IMfxDataQueue::Add().
	// To create one or more additional MfxData items, call IMfxDataQueue::Add().
	STDMETHOD( OnInput )(THIS_ IMfxDataQueue* pqIn, IMfxDataQueue* pqOut ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

#pragma pack( push, 1 )	// save space

struct MfxData
{
	// Musical ticks. For MfxData items passed to IMfxEventFilter::OnInput(),
	// this will correspond to the current time. If the application isn't playing,
	// the current time will not change.
	LONG	m_lTime;

	union
	{
		DWORD	m_dwData;	// this is in midiOutShortMsg() format...
		struct
		{
			BYTE	m_byStatus;
			BYTE	m_byData1;
			BYTE	m_byData2;
		};
	};
};

#pragma pack( pop )

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxDataQueue, IUnknown )
{
	// IMfxDataQueue represents a time-ordered collection of MfxData structs.

	// Add() returns:
	//	S_OK:				Success.
	// E_OUTOFMEMORY: Data could not be added.
	// E_INVALIDARG:	One or more MfxData fields had bad values; data not added.
	STDMETHOD( Add )(THIS_ const MfxData& data ) PURE;

	// GetCount() provides the number of events in the queue.
	STDMETHOD( GetCount )(THIS_ int* pnCount ) PURE;

	// GetAt() provides the MfxEvent at index 'ix'.
	STDMETHOD( GetAt )(THIS_ int ix, MfxData* pData ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
DECLARE_HANDLE( MFX_HBUFFER );	// Use DECLARE_HANDLE() for STRICT benefits
#else
typedef DWORD MFX_HBUFFER;
#endif

#if (MFX_VERSION > 9)
typedef long MFX_CHANNEL;
enum { MFX_CHANNEL_NONE = -1 };
#endif // (MFX_VERSION > 9)

//---------------------------------------------------------------------------

#pragma pack( push, 1 )	// save space

//---------------------------------------------------------------------------

#if (MFX_VERSION > 9)

struct MfxMuteMask
{
	MfxMuteMask() { byte = 0; }
	MfxMuteMask(int b) { byte = BYTE(b); }
	BOOL AnySet() const { return byte; }
	operator BYTE() const { return byte; }
	BYTE operator=(int n) { byte = BYTE(n); return byte; }
	union
	{
		 BYTE    byte;
		 struct
		 {
			  BYTE    Manual:1;
			  BYTE    Auto:1;
			  BYTE    Solo:1;
			  BYTE    Record:1;
			  BYTE    Scrub:1;
		 };
	};
};

#endif // (MFX_VERSION > 9)

//---------------------------------------------------------------------------

struct MfxEvent
{
	// This describes a MIDI event.
	// This is a bit higher-level than raw MIDI voice messages. For example,
	// Note, Patch, RPN and NRPN MfxEvents consolidate multiple MIDI voice
	// messages into a single logical MfxEvent, for easier processing.

	enum Type
	{
		Note,
		KeyAft,
		Control,
		Patch,
		ChanAft,
		Wheel,
		RPN,
		NRPN,

#if (MFX_VERSION > 8)
		Sysx,
		Text,
		Lyric,
#endif // (MFX_VERSION > 8)

#if (MFX_VERSION > 9)
		MuteMask,
		VelOfs,
		VelTrim,
		KeyOfs,
		KeyTrim,
#endif // (MFX_VERSION > 9)

#if (MFX_VERSION > 10)
		ShortMsg,
#endif // (MFX_VERSION > 10)
	};
	
	LONG	m_lTime;		// in musical ticks

	union
	{
		struct
		{
			BYTE	m_byPort;		// MIDI port for this event
			BYTE	m_byChan;		// MIDI channel for this event
		};
		struct
		{
			MfxMuteMask	m_maskSet;		// for MuteMask, bits to set in the mask
			MfxMuteMask	m_maskClear;	// for MuteMask, bits to clear from the mask
		};
		char	m_nOfs;			// for VelOfs and KeyOfs
		char	m_nTrim;			// for VelTrim and KeyTrim
	};

	Type m_eType;

	union
	{
		struct // m_eType == Note
		{
			BYTE	m_byKey;	// 0..127; key number
			BYTE	m_byVel;	// 1..127; attack velocity
			BYTE	m_byVelOff;	// 1..127 release velocity (64 = default)
			DWORD	m_dwDuration;	// 0..ULONG_MAX; in musical ticks
		};
		struct // m_eType == KeyAft
		{
			BYTE	m_byKey;	// 0..127
			BYTE	m_byAmt;	// 0..127
		};
		struct // m_eType == Control
		{
			BYTE	m_byNum;	// 0..127; controller number
			BYTE	m_byVal;	// 0..127; controller value
		};
		struct // m_eType == Patch
		{
			BYTE	m_byPatch;	// 0..127
			BYTE	m_byBankSelMethod; // 0==Normal, 1==Ctrl0, 2==Ctrl32, 3==Patch100
			short	m_nBank;	// -1..16383 (-1 == "none")
		};
		struct // m_eType == ChanAft
		{
			BYTE	m_byAmt;	// 0..127
		};
		struct // m_eType == Wheel
		{
			short	m_nVal;	// -8191..+8191 (0 == center)
		};
		struct // m_eType == RPN or NRPN
		{
			WORD	m_wNum;	// 0..16383
			WORD	m_wVal;	// 0..16383
		};

#if (MFX_VERSION > 8)
		struct // m_eType == Sysx, Text, or Lyric
		{
			// Pass the MFX_HBUFFER to the IMfxFactory::GetPointer() method
			// to cash it in for a pointer and length.
			// Note: Text and Lyric events are wchar_t pointers -- i.e. Unicode
			// strings -- following COM convention.
			MFX_HBUFFER	m_hBuffer;
		};
#endif // (MFX_VERSION > 8)

#if (MFX_VERSION > 9)
		struct // m_eType == Mute, VelOfs, VelTrim, KeyOfs, KeyTrim
		{
			MFX_CHANNEL m_mfxChannel;
		};
#endif // (MFX_VERSION > 9)

#if (MFX_VERSION > 10)
		struct // m_eType == ShortMsg
		{
			DWORD m_dwShortMsg;
		};
#endif // (MFX_VERSION > 9)
	};
};

#pragma pack( pop )

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 8)

DECLARE_INTERFACE_( IMfxBufferFactory, IUnknown )
{
	// MFX buffers are used to support variable-length data associated with
	// certain MfxEvent types like System Exclusive and text.
	//
	// MFX buffers are valid only during the call to your OnEvents() member.
	// This way the host application can do automatic garbage collection to
	// prevent memory leaks.
	//
	// MFX buffers may not be resized. If you need to resize a buffer, you must
	// call CreateCopy() to create a new buffer of the desired length and copy
	// the contents of the old buffer to the new one.

	// This creates a new MFX buffer with the capacity to store dwLength bytes.
	// If hbufOld is not NULL, the contents of hbufOld will be copied to hbufNew
	// (up to but not exceeding the lenght of the new buffer).
	// Returns: S_OK or E_OUTOFMEMORY.
	STDMETHOD( Create )(THIS_ DWORD dwLength, MFX_HBUFFER* phbufNew ) PURE;

	// This creates a new MFX buffer with the capacity to store dwLength bytes.
	// In addition, the contents of hbufOld will be copied to hbufNew
	// (up to but not exceeding the lenght of the new buffer).
	// Returns: S_OK, E_OUTOFMEMORY, or E_INVALIDARG if hbufOld is invalid.
	STDMETHOD( CreateCopy )(THIS_ MFX_HBUFFER hbufOld, DWORD dwLength, MFX_HBUFFER* phbufNew ) PURE;

	// This returns a read/write pointer to the data in ppvData.
	// If pdwLength is not NULL, this return the number of bytes that the buffer contains.
	// Returns: S_OK, or E_INVALIDARG if the MFX_HBUFFER is invalid.
	STDMETHOD( GetPointer )(THIS_ MFX_HBUFFER hbuf, void** ppvData, DWORD* pdwLength ) PURE;
};

#endif // (MFX_VERSION > 8)

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxEventQueue, IUnknown )
{
	// IMfxEventQueue represents a time-ordered collection of MfxEvents.

	// Add() returns:
	//	S_OK				Success.
	// E_OUTOFMEMORY	Event could not be added.
	// E_INVALIDARG	One or more MfxEvent fields had bad values; event not added.
	STDMETHOD( Add )(THIS_ const MfxEvent& event ) PURE;

	// GetCount() provides the number of events in the queue.
	STDMETHOD( GetCount )(THIS_ int* pnCount ) PURE;

	// GetAt() provides the MfxEvent at index 'ix'.
	STDMETHOD( GetAt )(THIS_ int ix, MfxEvent* pEvent ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 8)

DECLARE_INTERFACE_( IMfxEventQueue2, IMfxEventQueue )
{
	// IMfxEventQueue2 represents a time-ordered collection of MfxEvents.
	//
	// It contains all of the same methods as IMfxEventQueue, plus the following
	// methods:

	// Use this to obtain the IMfxBufferFactory associated with this IMfxEventQueue.
	// You must call Release() on the IMfxBufferFactory* it returns, when you are done
	// with it.
	// Returns:
	// S_OK			Success
	// E_POINTER	The pointer argument is bad
	STDMETHOD( GetBufferFactory )(THIS_ IMfxBufferFactory** ppIMfxBufferFactory ) PURE;
};

#endif // (MFX_VERSION > 8)

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxTempoMap, IUnknown )
{
	// IMfxTempoMap represents the tempo map.

	// Convert between musical ticks and absolute milliseconds
	STDMETHOD_(LONG, TicksToMsecs)(THIS_ LONG lTicks ) PURE;
	STDMETHOD_(LONG, MsecsToTicks)(THIS_ LONG lMsecs ) PURE;

	// Get the number of ticks per quarter-note (a.k.a. "PPQ or "timebase")
	STDMETHOD_(int, GetTicksPerQuarterNote)( THIS ) PURE;

	// Get the index of the tempo change in effect for time 'dwTicks'
	STDMETHOD_(int, GetTempoIndexForTime)(THIS_ LONG lTicks ) PURE;

	// Get the number of tempo changes in the map
	STDMETHOD_(int, GetTempoCount)(THIS) PURE;

	// Get a tempo change. Returns:
	// pdwTicks: the time of the tempo change
	// pnBPM100: the value of the tempo in 100ths of beats per minute (for
	// example, 100.05 BPM is returned as 10005).
	STDMETHOD( GetTempoAt )(THIS_ int ix, LONG* plTicks, int* pnBPM100 ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 10)

#include <MfxTime.h>

DECLARE_INTERFACE_( IMfxTimeConverter, IUnknown )
{
	// Convert from one time format to another
	STDMETHOD_(HRESULT, ConvertMfxTime)(THIS_ MFX_TIME* pTime, MFX_TIME_FORMAT newFormat ) PURE;
};

#endif // (MFX_VERSION > 10)

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxMeterMap, IUnknown )
{
	// IMfxMeterMap represents the meter (time signature) map.

	// Convert between musical ticks and measure:beat:ticks
	STDMETHOD( TicksToMBT )(THIS_ LONG lTicks, int* pnMeasure, int* pnBeat, int* pnTicks ) PURE;
	STDMETHOD_(LONG, MBTToTicks)(THIS_ int nMeasure, int nBeat, int nTicks ) PURE;

	// Get the index of the meter change in effect for time 'dwTicks'
	STDMETHOD_(int, GetMeterIndexForTime)(THIS_ LONG lTicks ) PURE;

	// Get the number of meter changes in the map
	STDMETHOD_(int, GetMeterCount)(THIS) PURE;

	// Get a meter change. Returns:
	// pnMeasure: the measure number of the meter change
	// pnTop: the number of beats per measure
	// pnBottom: the beat value (shift 1 left by this amount to get the
	// beat value; for example, 2 means 1 << 2 or 4.
	enum EBeatValue { Beat1 = 0, Beat2 = 1, Beat4 = 2, Beat8 = 3, Beat16 = 4, Beat32 = 5 };
	STDMETHOD( GetMeterAt )(THIS_ int ix, int* pnMeasure, int* pnTop, EBeatValue* eBottom ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 8)

DECLARE_INTERFACE_( IMfxMarkerMap, IUnknown )
{
	// IMfxMarkerMap represents the marker map.

	// Get the index of the marker in effect for time 'dwTicks'
	STDMETHOD_(int, GetMarkerIndexForTime)(THIS_ LONG lTicks ) PURE;

	// Get the number of markers in the map
	STDMETHOD_(int, GetMarkerCount)(THIS) PURE;

	// Get a marker. Returns:
	// plTime: the time of the marker, in the units returned by peUnits
	// peUnits: the units of the time
	// pwszName: the name fo the marker -- you must free this using CoTaskMemFree() !!!
	enum EUnits { Ticks, Frames, Samples };
	STDMETHOD( GetMarkerAt )(THIS_ int ix, LONG* plTime, EUnits* peUnits, wchar_t** ppwszName ) PURE;
};

#endif // (MFX_VERSION > 8)

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 10)

DECLARE_INTERFACE_( IMfxMarkerMap2, IMfxMarkerMap )
{
	// IMfxMarkerMap2 represents the marker map, including pitch markers.

	STDMETHOD( GetPitchAtTime )(THIS_ MFX_TIME* pTime, int* pnPitch ) PURE;
};

#endif // (MFX_VERSION > 10)

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxKeySigMap, IUnknown )
{
	// IMfxKeySigMap represents the key signature map.

	// Get the index of the key signature change in effect for time 'dwTicks'
	STDMETHOD_(int, GetKeySigIndexForTime)(THIS_ LONG lTicks ) PURE;

	// Get the number of key signature changes in the map
	STDMETHOD_(int, GetKeySigCount)(THIS) PURE;

	// Get a meter change. Returns:
	// pnMeasure: the measure number of the key signature change
	// pnKeySig: the key signature (-7 = 7 flats, 0 = C Major, +7 = 7 sharps)
	STDMETHOD( GetKeySigAt )(THIS_ int ix, int* pnMeasure, int* pnKeySig ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxInstruments, IUnknown )
{
	// Get the IMfxNameListSet containing all the IMfxNameLists for patch names.
	// Important: You must call Release() on the IMfxNameListSet* when you're done.
	STDMETHOD( GetThePatchNameLists )(THIS_ IMfxNameListSet** ppIMfxNameListSet ) PURE;

	// Get the IMfxNameListSet containing all the IMfxNameLists for note names.
	// Important: You must call Release() on the IMfxNameListSet* when you're done.
	STDMETHOD( GetTheNoteNameLists )(THIS_ IMfxNameListSet** ppIMfxNameListSet ) PURE;

	// Get the IMfxNameListSet containing all the IMfxNameLists for controller names.
	// Important: You must call Release() on the IMfxNameListSet* when you're done.
	STDMETHOD( GetTheControllerNameLists )(THIS_ IMfxNameListSet** ppIMfxNameListSet ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxNameListSet, IUnknown )
{
	// Get the number of IMfxNameLists available.
	STDMETHOD( GetCount )(THIS_ int* pnCount ) PURE;

	// Get the indicated IMfxNameList.
	// Important: You must call Release() on the IMfxNameList* when you're done.
	STDMETHOD( GetAt )(THIS_ int ixList, IMfxNameList** ppNameList ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxNameList, IUnknown )
{
	// Get the title of this name list (e.g. "General MIDI").
	STDMETHOD( GetTitle )(THIS_ char* pszTitle, int cbTitle ) PURE;

	// Get the maximum number of names that this list can contain.
	// E.g. 128 for note and controller names, 16384 for patch names.
	// This number is one greater than the maximum valid value for the 'ix'
	// argument to GetAt().
	STDMETHOD( GetMaxNames )(THIS_ int* pnCount ) PURE;

	// Get the specified name.
	STDMETHOD( GetAt )(THIS_ int ix, char* pszName, int cbName ) PURE;
};

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxSelection, IUnknown )
{
	// IMfxSelection represents information about the selection when a filter
	// is being used as an offline edit command. If you QueryInterface() the
	// 'pContext' pointer passed to IMfxEventFilter::Connect(), and the QI()
	// fails, then you know that you're NOT being used as an offline edit.

	STDMETHOD_(LONG, GetFrom)(THIS) PURE;
	STDMETHOD_(LONG, GetThru)(THIS) PURE;
	STDMETHOD_(LONG, GetFirstEventStartTime)(THIS) PURE;
	STDMETHOD_(LONG, GetLastEventStartTime)(THIS) PURE;
	STDMETHOD_(LONG, GetLastEventEndTime)(THIS) PURE;
};

/////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_( IMfxInputPulse, IUnknown )
{
	// Call BeginPulse() to cause your IMfxEventFilter::OnInput() method to
	// be called regularly, even if there has been no actual MIDI input.
	// GetPulseInterval() returns the millisecond interval at which you will be called.
	// The interval might be (for example) on the order of 200 msec.
	//
	// NOTE: The first call to your OnInput() member may occur prior to the
	// call to BeginPulse() returning. Make sure OnInput() is ready to be
	// called, before you call BeginPulse()!
	//
	// Use this if you have an effect where you want to generate output for
	// the future. This interval represents the furthest into the future you
	// should generate the material.
	//
	// Note: Given a chain of multiple effects, if any effect asks for the pulse,
	// all will receive it. In addition, although the first effect will see
	// an input queue that is empty, anything it outputs will be input for effects
	// further along the chain.
	STDMETHOD( GetPulseInterval )(THIS_ LONG* plIntervalMsec ) PURE;
	STDMETHOD( BeginPulse )(THIS) PURE;
	STDMETHOD( EndPulse )(THIS) PURE;
};

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 8)

DECLARE_INTERFACE_( IMfxDocumentFactory, IUnknown )
{
	// IClassFactory
   STDMETHOD( CreateInstance )(THIS_ IUnknown *pUnkOuter, REFIID riid, void** ppvObject ) PURE;
	STDMETHOD( LockServer )(THIS_ BOOL fLock ) PURE;
};

#endif // (MFX_VERSION > 8)

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 8)

DECLARE_INTERFACE_( IMfxDocument, IUnknown )
{
	// IMfxDocument provides a way for the host application to expose its
	// ability to open Standard MIDI Files. This way the MFX need not supply
	// all the file reading/parsing code itself.
	//
	// This may also support the native formats of the host application.
	// Example: Pro Audio project (.WRK) files.

	// Call this to try to open a file.
	// Until you call this method and it must succeeds, the other methods
	// will simply return E_FAIL.
	STDMETHOD( Open )(THIS_ IStream* pIStream ) PURE;

	// Get the events from track nTrackNumber in an IMfxEventQueue2.
	// You MUST call Release() on the IMfxEventQueue pointer when you are done.
	STDMETHOD( GetTrack )(THIS_  int nTrackNumber, IMfxEventQueue2** ppIMfxEventQueue2 ) PURE;

	// Get the tempo map.
	// You MUST call Release() on the IMfxTempoMap pointer when you are done.
	STDMETHOD( GetTempoMap )(THIS_ IMfxTempoMap** ppIMfxTempoMap ) PURE;

	// Get the meter map.
	// You MUST call Release() on the IMfxMeterMap pointer when you are done.
	STDMETHOD( GetMeterMap )(THIS_ IMfxMeterMap** ppIMfxMeterMap ) PURE;

	// Get the marker map.
	// You MUST call Release() on the IMfxMarkerMap pointer when you are done.
	STDMETHOD( GetMarkerMap )(THIS_ IMfxMarkerMap** ppIMfxMarkerMap ) PURE;
};

#endif // (MFX_VERSION > 8)


////////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 9)

typedef struct {} * MFX_POSITION;

DECLARE_INTERFACE_( IMfxNameList2, IMfxNameList )
{
	// IMfxNameList2 provides a way for a soft synth to provide iteration over
	// lists with sparse mappings from integers to strings, i.e., to supply
	// names of notes, controllers, etc.

	STDMETHOD( GetCount )( THIS_ int* pnCount ) PURE;
	STDMETHOD( GetStartPosition )( THIS_ MFX_POSITION* pPos ) PURE;
	STDMETHOD( GetNextAssoc )( THIS_ MFX_POSITION* pPos, int* pnKey, char* pszString, int cbString ) PURE;
	STDMETHOD( Lookup )( THIS_ int nKey, char* pszString, int cbString ) PURE;
};

#endif // (MFX_VERSION > 9)

/////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 9)

DECLARE_INTERFACE_( IMfxSoftSynth, IUnknown )
{
	// This is the interface that a soft-synth filter must implement.
	// It might also implement some other standard COM interfaces:
	//		ISpecifyPropertyPages - To provide a user interface
	//		IPersistStream - To provide support for presets

	// This is called before any other members are called.
	// You can use the IUnknown* pContext to QueryInterface() for host
	// interfaces like IMfxTempoMap or IMfxMeterMap.
	//
	// Unless you retain a copy of the IUnknown* pContext, be sure to call
	// pContext->Release().
	//
	// If you QueryInterface() for something and it's not available, you
	// can return E_FAIL to fail; the application will not use your filter.
	// Otherwise you should return NOERROR or S_OK if you got what you needed.
	STDMETHOD( Connect )(THIS_ IUnknown* pContext ) PURE;

	// This is called when the filter will not be used any longer.
	// You must call Release() on any interface pointers you obtained
	// from the IUnknown* passed to you in Connect().
	STDMETHOD( Disconnect )(THIS) PURE;

	// Called when playback (or other streaming session) starts, with the
	// start time.
	STDMETHOD( OnStart )(THIS_ LONG lTime ) PURE;

	// Called when playback (or other streaming session) loops back to an
	// earlier time without stopping and restarting.
	STDMETHOD( OnLoop )(THIS_ LONG lTimeRestart, LONG lTimeStop ) PURE;

	// Called when playback (or other streaming session) stops, with the
	// stop time.
	STDMETHOD( OnStop )(THIS_ LONG lTime ) PURE;

	// This is called when a window of time is being processed, represented by
	// lTimeFrom..lTimeThru.
	//
	// If there are any events to process, they are in pqIn.  The synth is free
	// to remove elements from pqIn if desired, but this is not required, since
	// the synth is always the last processing element in the chain.
	STDMETHOD( OnEvents )(THIS_ LONG lTimeFrom, LONG lTimeThru, MFX_CHANNEL mfxChannel, IMfxEventQueue* pqIn ) PURE;

	// This is called to give you an opportunity to process MIDI input.
	//
	// The synth is free to remove elements from pqIn if desired, but this is
	// not required, since the synth is always the last processing element in the chain.
	STDMETHOD( OnInput )(THIS_ MFX_CHANNEL mfxChannel, IMfxDataQueue* pqIn ) PURE;

	// Allows the soft-synth to expose various names.  Valid ranges for different
	// types of names are as follows:
	//		Patch			0..16383
	//		Note			0..127
	//		Controller	0..127
	//		RPN			0..16383
	//		NRPN			0..16383
	STDMETHOD( GetBanksForPatchNames )( THIS_ int** panBank, int* cBank ) PURE;
	STDMETHOD( GetIsDrumPatch )( THIS_ int nBank, int nPatch ) PURE;
	STDMETHOD( GetIsDiatonicNoteNames )( THIS_ int nBank, int nPatch ) PURE;
	STDMETHOD( GetPatchNames )(THIS_ int nBank, IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetNoteNames )(THIS_ int nBank, int nPatch, IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetControllerNames )(THIS_ IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetRpnNames )(THIS_ IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetNrpnNames )(THIS_ IMfxNameList2** ppMap ) PURE;
};

#endif // (MFX_VERSION > 9)

////////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 9)

//------------------------------------------------------------------------------

struct MfxNotifyMsg
{
	enum Type
	{
		ChannelMidiChannel,
		ChannelMuteMask,
		ChannelVelOfs,
		ChannelVelTrim,
		ChannelKeyOfs,
		ChannelKeyTrim,
#if (MFX_VERSION > 10)
		PortListChanged,
#endif
	};

	Type				m_type;			// what's happened?
	union
	{
		struct // ChannelMidiChannel
		{
			MFX_CHANNEL	m_mfxChannel;
			char			m_nMidiChannel;
		};
		struct // ChannelMuteMask
		{
			MFX_CHANNEL	m_mfxChannel;
			MfxMuteMask	m_maskSet;
			MfxMuteMask	m_maskClear;
		};
		struct // ChannelVelOfs, ChannelKeyOfs
		{
			MFX_CHANNEL	m_mfxChannel;
			char			m_nOfs;
		};
		struct // ChannelVelTrim, ChannelKeyTrim
		{
			MFX_CHANNEL	m_mfxChannel;
			char			m_nTrim;				// m_type == VelTrim, KeyTrim
		};
	};
};

//------------------------------------------------------------------------------

DECLARE_INTERFACE_( IMfxNotify, IUnknown )
{
	// This interface allows the host application to tell an MFX object (plug-in
	// or soft-synth) about interesting changes that may have occurred
	STDMETHOD( OnMfxNotify )( THIS_ MfxNotifyMsg* pMsg ) PURE;
};

#endif // (MFX_VERSION > 9)

////////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 10)

DECLARE_INTERFACE_( IMfxInputCallback, IUnknown )
{
	// Allow a synth to provide different instruments per-channel
	STDMETHOD( OnEvent )( THIS_ IMfxInputPort* pPortFrom, const MfxEvent& mfxEvent ) PURE;
};

DECLARE_INTERFACE_( IMfxInputPort, IUnknown )
{
	// Implemented by MFX filters and DXi's who need to send MIDI data "just in
	// time" to the host application, e.g., for automation.
	STDMETHOD( SetInputCallback )( THIS_ IMfxInputCallback* pCallback ) PURE;
	STDMETHOD( GetInputCallback )( THIS_ IMfxInputCallback** ppCallback ) PURE;
};

#endif // (MFX_VERSION > 10)

////////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 10)

DECLARE_INTERFACE_( IMfxInstrument, IUnknown )
{
	// Get the name of this instrument (e.g., "TONAR tonez")
	STDMETHOD( GetInstrumentName )(THIS_ char* pszName, int cbName ) PURE;

	// Allows the soft-synth to expose various names.  Valid ranges for different
	// types of names are as follows:
	//		Patch			0..16383
	//		Note			0..127
	//		Controller	0..127
	//		RPN			0..16383
	//		NRPN			0..16383
	STDMETHOD( GetBanksForPatchNames )( THIS_ int** panBank, int* cBank ) PURE;
	STDMETHOD( GetIsDrumPatch )( THIS_ int nBank, int nPatch ) PURE;
	STDMETHOD( GetIsDiatonicNoteNames )( THIS_ int nBank, int nPatch ) PURE;
	STDMETHOD( GetPatchNames )(THIS_ int nBank, IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetNoteNames )(THIS_ int nBank, int nPatch, IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetControllerNames )(THIS_ IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetRpnNames )(THIS_ IMfxNameList2** ppMap ) PURE;
	STDMETHOD( GetNrpnNames )(THIS_ IMfxNameList2** ppMap ) PURE;
};

#endif // (MFX_VERSION > 10)

////////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 10)

DECLARE_INTERFACE_( IMfxSoftSynth2, IMfxSoftSynth )
{
	// Allow a synth to provide different instruments per-channel
	STDMETHOD( GetInstrument )( THIS_ int nChannel, IMfxInstrument** ppInstrument ) PURE;
};

#endif // (MFX_VERSION > 10)

////////////////////////////////////////////////////////////////////////////////

#if (MFX_VERSION > 10)

DECLARE_INTERFACE_( IMfxNotifyHost, IUnknown )
{
	// This interface allows the MFX object (plug-in or soft-synth) to tell the
	// host about interesting changes that may have occurred
	STDMETHOD( OnMfxNotifyHost )( THIS_ UINT uMessage, IUnknown* pUnkFrom, LPARAM lParam ) PURE;
};

// The caller is a multi-output DXi, and is notifying the host about a change
// in its audio output count.  If the host returns an error code from this
// notification, the DXi must leave its output count in the state it was in
// before making this request.
#define MH_SYNTH_AUDIO_PORTS_CHANGE_BEGIN		(1)		// lParam = new number of synth audio outputs
#define MH_SYNTH_AUDIO_PORTS_CHANGE_END		(2)		// lParam = new number of synth audio outputs

// Notifications greater than MH_RESERVED_BASE are reserved for private use
// within Cakewalk products.
#define MH_CAKEWALK_RESERVED_BASE		(0x8000000)

#endif // (MFX_VERSION > 10)

/////////////////////////////////////////////////////////////////////////////

#pragma pack( pop, ENTER_MFX_H )	// restore struct packing

#endif // __MFX_H__
