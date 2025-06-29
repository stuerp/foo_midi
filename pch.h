
/** $VER: pch.h (2025.06.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#define NOMINMAX
#include <helpers/foobar2000+atl.h>
#include <helpers/helpers.h>
#undef NOMINMAX

#include <wincodec.h>

#include <stdlib.h>
#include <strsafe.h>

#include <algorithm>
#include <cmath>
#include <cassert>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

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
