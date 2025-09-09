
/** $VER: libmsc.h (2025.09.06) P. Stuer - My Support Classes, The "Most Original Name" Winner **/

#pragma once

// Avoid conflict or confusion with std::filesystem
#ifdef _FILESYSTEM_
#error std::filesystem is already included.
#endif

// UTF-8 Everywhere recommendation
#ifndef _UNICODE
#error Unicode character set compilation not enabled.
#endif

#include <SDKDDKVer.h>
#include <windows.h>

#include "ghc\filesystem.hpp"

namespace fs = ghc::filesystem;

#include "Encoding.h"
#include "Exception.h"
#include "RAII.h"
#include "Stream.h"
#include "Support.h"
