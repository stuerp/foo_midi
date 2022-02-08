// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__61E1BB55_BA9C_4BE3_90AB_042F331DD131__INCLUDED_)
#define AFX_STDAFX_H__61E1BB55_BA9C_4BE3_90AB_042F331DD131__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers

#include <afxcmn.h> // MFC support for Windows Common Controls
#include <afxext.h> // MFC extensions
#include <afxwin.h> // MFC core and standard components

#ifdef _DEBUG
#define DEBUG (1) // the DirectShow headers use this symbol
#endif

#include <mmsystem.h>
#include <strmif.h>

#include <MidiFilter.h>

#include <MidiDefs.h>
#include <Templates.h>

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT (3)
#endif

#pragma warning(disable : 4786) // identifier was trucated to '255' characters in the debug information

#include <algorithm>
#include <map>
#include <vector>

using namespace std;

#ifndef THIS_FILE
#define THIS_FILE __FILE__
#endif

////////////////////////////////////////////////////////////////////////////////
// Call a function, returning if the result failed

static const char BASED_CODE szCheckFmt[] = "%s(%ld) Runtime error %08lx\r\n";
#define CHECK
#undef CHECK
#define CHECK(_fn)                                        \
	{                                                     \
		HRESULT __hr = _fn;                               \
		if(!SUCCEEDED(__hr)) {                            \
			TRACE(szCheckFmt, THIS_FILE, __LINE__, __hr); \
			return __hr;                                  \
		}                                                 \
	}

////////////////////////////////////////////////////////////////////////////////
// Release a pointer and set it to null

#undef SAFE_RELEASE
#define SAFE_RELEASE(_p)        \
	{                           \
		if(_p) (_p)->Release(); \
		(_p) = NULL;            \
	}

////////////////////////////////////////////////////////////////////////////////
// Delete a pointer and set it to null

#undef SAFE_DELETE
#define SAFE_DELETE(_p)    \
	{                      \
		if(_p) delete(_p); \
		(_p) = NULL;       \
	}

#undef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(_p) \
	{                         \
		if(_p) delete[](_p);  \
		(_p) = NULL;          \
	}

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__61E1BB55_BA9C_4BE3_90AB_042F331DD131__INCLUDED_)
