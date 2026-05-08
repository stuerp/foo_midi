
/** $VER: libmsc.h (2026.04.10) P. Stuer - My Support Classes, The "Most Original Name" Winner **/

#pragma once

// UTF-8 Everywhere recommendation
#ifndef _UNICODE
#error Unicode character set compilation not enabled.
#endif

#include <SDKDDKVer.h>

#define NOMINMAX

#include <windows.h>

#include <filesystem>

namespace fs = std::filesystem;

#include "CriticalSection.h"
#include "Encoding.h"
#include "Enum.h"
#include "Exception.h"
#include "RAII.h"
#include "Stream.h"
#include "Support.h"
