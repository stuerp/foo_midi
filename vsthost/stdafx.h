// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here

#include <Windows.h>
#include <CommCtrl.h>

#include <stdint.h>

#include <io.h>
#include <fcntl.h>

#include <vector>

// #define VST_SDK_2_3
#ifdef VST_SDK_2_3
#include "c:\\programming\\vstsdk2.3\\source\\common\\aeffect.h"
#include "c:\\programming\\vstsdk2.3\\source\\common\\aeffectx.h"
#else
#include "c:\\programming\\vstsdk2.4\\pluginterfaces\\vst2.x\\aeffect.h"
#include "c:\\programming\\vstsdk2.4\\pluginterfaces\\vst2.x\\aeffectx.h"
#endif

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

typedef AEffect * (*main_func)(audioMasterCallback audioMaster);

#ifdef VST_SDK_2_3
struct VstMidiSysexEvent
{
	long type;        // kVstSysexType
	long byteSize;    // 24
	long deltaFrames;
	long flags;       // none defined yet
	long dumpBytes;   // byte size of sysexDump
	long resvd1;      // zero
	char *sysexDump;
	long resvd2;      // zero
};
#endif

template<typename T>
static void append_be( std::vector<uint8_t> & out, const T & value )
{
	union
	{
		T original;
		uint8_t raw[sizeof(T)];
	} carriage;
	carriage.original = value;
	for ( unsigned i = 0; i < sizeof(T); ++i )
	{
		out.push_back( carriage.raw[ sizeof(T) - 1 - i ] );
	}
}

template<typename T>
static void retrieve_be( T & out, const uint8_t * & in, unsigned & size )
{
	if ( size < sizeof(T) ) return;

	size -= sizeof(T);

	union
	{
		T original;
		uint8_t raw[sizeof(T)];
	} carriage;
	for ( unsigned i = 0; i < sizeof(T); ++i )
	{
		carriage.raw[ sizeof(T) - 1 - i ] = *in++;
	}

	out = carriage.original;
}
