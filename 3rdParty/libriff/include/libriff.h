
/** $VER: libriff.h (2025.07.27) P. Stuer **/

#pragma once

#ifdef __TRACE
extern uint32_t __TRACE_LEVEL;

#define TRACE_RESET()           { __TRACE_LEVEL = 0; }
#define TRACE_INDENT()          { ::printf("%*s{\n", __TRACE_LEVEL * 4, ""); __TRACE_LEVEL++; }
#define TRACE_FORM(type, size)  { char s[5]; *(uint32_t *) s = type; s[4] = 0; ::printf("%*sFORM \"%s\", %u bytes\n", __TRACE_LEVEL * 4, "", s, size); }
#define TRACE_LIST(type, size)  { char s[5]; *(uint32_t *) s = type; s[4] = 0; ::printf("%*sLIST \"%s\", %u bytes\n", __TRACE_LEVEL * 4, "", s, size); }
#define TRACE_CHUNK(id, size)   { char s[5]; *(uint32_t *) s = id; s[4] = 0; ::printf("%*s%s, %u bytes\n", __TRACE_LEVEL * 4, "", s, size); }
#define TRACE_UNINDENT()        { __TRACE_LEVEL--; ::printf("%*s}\n", __TRACE_LEVEL * 4, ""); }

#else

#define TRACE_RESET()           {  }
#define TRACE_INDENT()          {  }
#define TRACE_FORM(type, size)  {  }
#define TRACE_LIST(type, size)  {  }
#define TRACE_CHUNK(id, size)   {  }
#define TRACE_UNINDENT()        {  }

#endif

#include "RIFFReader.h"
#include "RIFFWriter.h"

#include "Exception.h"
