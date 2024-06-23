
/** $VER: Exceptions.h (2023.06.03) P. Stuer **/

#pragma once

#include <pfc/pfc-lite.h>

//! Generic I/O error. Root class for I/O failure exception. See relevant default message for description of each derived exception class.
PFC_DECLARE_EXCEPTION(exception_midi, pfc::exception, "I/O error.");
