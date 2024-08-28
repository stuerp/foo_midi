
/** $VER: Exception.h (2024.08.28) P. Stuer **/

#pragma once

#include <pfc/pfc-lite.h>

namespace midi
{

//! Generic I/O error. Root class for I/O failure exception. See relevant default message for description of each derived exception class.
PFC_DECLARE_EXCEPTION(exception_t, pfc::exception, "I/O error.");

std::string GetErrorMessage(const std::string & errorMessage, DWORD errorCode, ...);

}