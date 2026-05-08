
/** $VER: libmidi.h (2025.09.05) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>

#define NOMINMAX

#include <winsock2.h>
#include <windows.h>
#include <wincodec.h>

#ifdef __TRACE
extern uint32_t __TRACE_LEVEL;

#define TRACE_RESET()           { __TRACE_LEVEL = 0; }
#define TRACE_INDENT()          { ::printf("%*s{\n", __TRACE_LEVEL * 4, ""); __TRACE_LEVEL++; }
#define TRACE_UNINDENT()        { __TRACE_LEVEL--; ::printf("%*s}\n", __TRACE_LEVEL * 4, ""); }

#else

#define TRACE_RESET()           {  }
#define TRACE_INDENT()          {  }
#define TRACE_FORM(type, size)  {  }
#define TRACE_LIST(type, size)  {  }
#define TRACE_CHUNK(id, size)   {  }
#define TRACE_UNINDENT()        {  }

#endif
