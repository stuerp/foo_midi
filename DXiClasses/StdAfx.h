// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4B8662B4_46A5_4AD4_B977_215D4BC0A767__INCLUDED_)
#define AFX_STDAFX_H__4B8662B4_46A5_4AD4_B977_215D4BC0A767__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#ifdef _DEBUG
#define DEBUG (1)	// the DirectShow headers use this symbol
#endif

#include <objbase.h>
#include <streams.h>

#include <MidiFilter.h>

#include "MidiDefs.h"
#include "Templates.h"

#ifndef WAVE_FORMAT_IEEE_FLOAT
	#define WAVE_FORMAT_IEEE_FLOAT (3)
#endif

#pragma warning( disable: 4786 ) // identifier was trucated to '255' characters in the debug information

#include <vector>
#include <algorithm>
#include <map>

using namespace std;

#ifndef THIS_FILE
#define THIS_FILE __FILE__
#endif

////////////////////////////////////////////////////////////////////////////////
// Call a function, returning if the result failed

#undef CHECK

#ifdef _DEBUG

static const char szCheckFmt[] = "%s(%ld) Runtime error %08lx\r\n";

#define CHECK(_fn) \
	{	HRESULT __hr = _fn; \
		if (FAILED(__hr)) { \
			char sz[ 256 ]; \
			wsprintf( sz, szCheckFmt, THIS_FILE, __LINE__, __hr ); \
			OutputDebugString( sz ); \
			return __hr; } }

#else

#define CHECK(_fn) \
	{	HRESULT __hr = _fn; \
		if (FAILED(__hr)) { \
			return __hr; } }

#endif


////////////////////////////////////////////////////////////////////////////////
// Release a pointer and set it to null

#undef SAFE_RELEASE
#define SAFE_RELEASE(_p) { if (_p) (_p)->Release(); (_p) = NULL; }


////////////////////////////////////////////////////////////////////////////////
// Delete a pointer and set it to null

#undef SAFE_DELETE
#define SAFE_DELETE(_p) { if (_p) delete (_p); (_p) = NULL; }

#undef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(_p) { if (_p) delete [] (_p); (_p) = NULL; }


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4B8662B4_46A5_4AD4_B977_215D4BC0A767__INCLUDED_)
