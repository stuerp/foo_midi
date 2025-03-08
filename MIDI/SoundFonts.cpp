 
/** $VER: SoundFonts.cpp (2025.03.08) - Support functions for working with sound font files **/

#include "framework.h"

#include "SoundFonts.h"

#pragma hdrstop

/// <summary>
/// Gets the path name of a sound font file with the same base name as the specified MIDI file, if any.
/// </summary>
bool GetSoundFontFilePath(const pfc::string & filePath, pfc::string & soundFontPath, abort_callback & abortHandler) noexcept
{
    static const char * FileExtensions[] =
    {
        "json",
        "sflist",
        "sf2pack",
        "sfogg",
        "sf2",
        "sf3",
        "dls"
    };

    soundFontPath = filePath;

    const char * Period = ::strrchr(soundFontPath.c_str(), '.');

    if (Period == nullptr)
        return false;

    for (const char * & FileExtension : FileExtensions)
    {
        soundFontPath.truncate((Period - soundFontPath.c_str()) + (size_t) 1);
        soundFontPath += FileExtension;

        if (filesystem::g_exists(soundFontPath, abortHandler))
            return true;
    }

    return false;
}

/// <summary>
/// Writes a buffer containing sound font data to a temporary file.
/// </summary>
bool WriteSoundFontFile(const std::vector<uint8_t> data, bool isDLS, std::string & filePath) noexcept
{
    char TempPath[MAX_PATH] = {};

    if (::GetTempPathA(_countof(TempPath), TempPath) == 0)
        return false;

    char TempFilePath[MAX_PATH] = {};

    if (::GetTempFileNameA(TempPath, "SF-", 0, TempFilePath) == 0)
        return false;

    std::string FilePath = TempFilePath;

    // BASSMIDI requires SF2 and SF3 sound fonts with an .sf2 or .sf3 extension. FluidSynth also supports DLS.
    FilePath.append(isDLS ? ".dls" : ".sf2");

    {
        FILE * fp = nullptr;

        if (::fopen_s(&fp, FilePath.c_str(), "wb") == 0)
        {
            ::fwrite(data.data(), data.size(), 1, fp);

            ::fclose(fp);

            filePath = FilePath;
        }
    }

    ::DeleteFileA(TempFilePath);

    return true;
}
