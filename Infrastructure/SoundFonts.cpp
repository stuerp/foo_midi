 
/** $VER: SoundFonts.cpp (2025.07.125) - Support functions for working with sound font files **/

#include "pch.h"

#include "InputDecoder.h"
#include "Log.h"

#pragma hdrstop

static bool GetSoundFontFilePath(const pfc::string & filePath, pfc::string & soundFontPath, abort_callback & abortHandler) noexcept;
static bool WriteSoundFontFile(const std::vector<uint8_t> data, bool isDLS, std::string & filePath) noexcept;

/// <summary>
/// Gets the sound fonts and adjusts the player type, if necessary.
/// </summary>
void  InputDecoder::GetSoundFonts(const pfc::string & defaultSoundFontFilePath, abort_callback & abortHandler)
{
    // Delete all embedded sound font files from a previous run.
    {
        for (const auto & sf : _SoundFonts)
        {
            if (sf.IsEmbedded())
                ::DeleteFileA(sf.FilePath().c_str());
        }

        _SoundFonts.clear();
    }

    /** IMPORTANT: The following sequence of adding sound fonts is optimal for BASSMIDI. For FluidSynth, we'll reverse it. **/
    bool HasNonDefaultSoundFonts = false; // True if an embedded or named sound font is found.
    bool HasDLS = false;

    // First, add the embedded sound font, if present.
    {
        const auto & Data = _Container.GetSoundFontData();

        if (Data.size() > 12)
        {
            bool IsDLS = (::memcmp(Data.data() + 8, "DLS ", 4) == 0);

            std::string FilePath;

            if (WriteSoundFontFile(Data, IsDLS, FilePath))
            {
                _SoundFonts.push_back({ FilePath, 1.f, _Container.GetBankOffset(), true, IsDLS });

                HasNonDefaultSoundFonts = true;
            }
        }
    }
        
    // Then, add the sound font named like the MIDI file, if present.
    {
        pfc::string FilePath = _FilePath;
        pfc::string TempSoundFontFilePath;

        bool FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);

        if (!FoundSoundFile)
        {
            size_t FileExtensionIndex = FilePath.find_last('.');

            if (FileExtensionIndex > FilePath.scan_filename())
            {
                FilePath.truncate(FileExtensionIndex);

                FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);
            }

            if (!FoundSoundFile)
            {
                FilePath.truncate(FilePath.scan_filename());

                TempSoundFontFilePath = "";
                TempSoundFontFilePath.add_byte(FilePath[FilePath.length() - 1]);
                FilePath.truncate(FilePath.length() - 1);

                size_t FileNameIndex = FilePath.scan_filename();

                if (FileNameIndex != pfc::infinite_size)
                {
                    FilePath += TempSoundFontFilePath;
                    FilePath.add_string(&FilePath[FileNameIndex], FilePath.length() - FileNameIndex - 1);

                    FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);
                }
            }
        }

        if (FoundSoundFile)
        {
            bool IsDLS = TempSoundFontFilePath.toLower().endsWith(".dls");

            _SoundFonts.push_back({ TempSoundFontFilePath.c_str(), 1.0f, _Container.GetBankOffset(), false, IsDLS });

            HasNonDefaultSoundFonts = true;
        }
    }

    // Finally, add the default sound font.
    {
        if (!defaultSoundFontFilePath.isEmpty())
        {
            bool IsDLS = defaultSoundFontFilePath.toLower().endsWith(".dls");

            _SoundFonts.push_back({ defaultSoundFontFilePath.c_str(), _BASSMIDIVolume, 0, false, IsDLS });
        }
    }

    for (const auto & sf : _SoundFonts)
    {
        if (sf.IsDLS())
        {
            HasDLS = true;
            break;
        }
    }

    // Force the use of a sound font player if an embedded or named sound font was found.
    if ((_PlayerType != PlayerTypes::FluidSynth) && HasNonDefaultSoundFonts && !_SoundFonts.empty())
    {
        _PlayerType = (FluidSynth::API::Exists() && HasDLS) ? PlayerTypes::FluidSynth : PlayerTypes::BASSMIDI;
    }

    // Show which sound fonts we'll be using in the console.
    if ((_PlayerType == PlayerTypes::BASSMIDI) || (_PlayerType == PlayerTypes::FluidSynth))
    {
        if (_PlayerType == PlayerTypes::FluidSynth)
            std::reverse(_SoundFonts.begin(), _SoundFonts.end());

        for (const auto & sf : _SoundFonts)
            Log.AtInfo().Format(STR_COMPONENT_BASENAME " is using sound font \"%s\" with bank offset %d.", sf.FilePath().c_str(), sf.BankOffset());
    }
}

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

        try
        {
            if (filesystem::g_exists(soundFontPath, abortHandler))
                return true;
        }
        catch(...) {};
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
