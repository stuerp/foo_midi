
/** $VER: SoundFonts.h (2025.03.08) - Support functions for working with sound font files **/

#pragma once

#define NOMINMAX

#include <sdk/foobar2000-lite.h>

bool GetSoundFontFilePath(const pfc::string & filePath, pfc::string & soundFontPath, abort_callback & abortHandler) noexcept;
bool WriteSoundFontFile(const std::vector<uint8_t> data, bool isDLS, std::string & filePath) noexcept;
