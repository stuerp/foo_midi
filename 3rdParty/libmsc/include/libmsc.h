
/** $VER: libmsc.h (2026.08.14) P. Stuer - My Support Classes, The "Most Original Name" Winner **/

#pragma once

// UTF-8 Everywhere recommendation
#ifndef _UNICODE
#error Unicode character set compilation not enabled.
#endif

#define NOMINMAX

#include <SDKDDKVer.h>
#include <windows.h>

#include <filesystem>

namespace fs = std::filesystem;

#include "CriticalSection.h"
#include "Encoding.h"
#include "Enum.h"
#include "Exception.h"
#include "NLS.h"
#include "RAII.h"
#include "Stream.h"
#include "Support.h"
#include "Win32.h"
