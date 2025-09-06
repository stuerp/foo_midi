
/** $VER: framework.h (2025.08.17) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#define NOMINMAX

#include <WinSock2.h>
#include <Windows.h>
#include <wincodec.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strsafe.h>
#include <string.h>

#pragma warning(disable: 4242)
#include <algorithm>
#pragma warning(default: 4242)
#include <cmath>
#include <cassert>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <libmsc.h>

#ifndef Assert
#if defined(DEBUG) || defined(_DEBUG)
#define Assert(b) do {if (!(b)) { ::OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif
#endif

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

#ifndef THIS_HINSTANCE
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define THIS_HINSTANCE ((HINSTANCE) &__ImageBase)
#endif

#ifdef _DEBUG
#define _RCP_VERBOSE
#else
#undef _RCP_VERBOSE
#endif
