// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <objbase.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#include <vector>

#include "SCCore.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
