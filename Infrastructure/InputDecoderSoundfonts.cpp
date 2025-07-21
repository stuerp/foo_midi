 
/** $VER: InputDecoderSoundfonts.cpp (2025.07.21) - Soundfont support functions for the InputDecoder **/

#include "pch.h"

#include "InputDecoder.h"

#include "Support.h"
#include "Log.h"

#pragma hdrstop

static bool GetSoundfontFilePath(const pfc::string & filePath, pfc::string & soundFontPath, abort_callback & abortHandler) noexcept;
static bool WriteSoundfontFile(const std::vector<uint8_t> data, bool isDLS, std::string & filePath) noexcept;

/// <summary>
/// Gets the soundfonts and adjusts the player type, if necessary.
/// </summary>
void  InputDecoder::GetSoundfonts(const pfc::string & defaultSoundfontFilePath, abort_callback & abortHandler)
{
    std::vector<soundfont_t> Soundfonts;

    /** IMPORTANT: The following sequence of adding soundfonts is optimal for BASSMIDI. For FluidSynth, we'll reverse the order later. **/
    bool HasNonDefaultSoundfonts = false; // True if an embedded or named soundfont is found.
    bool HasDLS = false;

    // First, add the embedded soundfont, if present.
    {
        const auto & Data = _Container.GetSoundfontData();

        if (Data.size() > 12)
        {
            bool IsDLS = (::memcmp(Data.data() + 8, "DLS ", 4) == 0);

            std::string FilePath;

            if (WriteSoundfontFile(Data, IsDLS, FilePath))
            {
                Soundfonts.push_back({ FilePath, 1.f, _Container.GetBankOffset(), true, IsDLS });

                HasNonDefaultSoundfonts = true;
            }
        }
    }
        
    // Then, add the soundfont named like the MIDI file, if present.
    {
        pfc::string FilePath = _FilePath;
        pfc::string TempSoundfontFilePath;

        bool FoundSoundFile = GetSoundfontFilePath(FilePath, TempSoundfontFilePath, abortHandler);

        if (!FoundSoundFile)
        {
            size_t FileExtensionIndex = FilePath.find_last('.');

            if (FileExtensionIndex > FilePath.scan_filename())
            {
                FilePath.truncate(FileExtensionIndex);

                FoundSoundFile = GetSoundfontFilePath(FilePath, TempSoundfontFilePath, abortHandler);
            }

            if (!FoundSoundFile)
            {
                FilePath.truncate(FilePath.scan_filename());

                TempSoundfontFilePath = "";
                TempSoundfontFilePath.add_byte(FilePath[FilePath.length() - 1]);
                FilePath.truncate(FilePath.length() - 1);

                size_t FileNameIndex = FilePath.scan_filename();

                if (FileNameIndex != pfc::infinite_size)
                {
                    FilePath += TempSoundfontFilePath;
                    FilePath.add_string(&FilePath[FileNameIndex], FilePath.length() - FileNameIndex - 1);

                    FoundSoundFile = GetSoundfontFilePath(FilePath, TempSoundfontFilePath, abortHandler);
                }
            }
        }

        if (FoundSoundFile)
        {
            bool IsDLS = TempSoundfontFilePath.toLower().endsWith(".dls");

            Soundfonts.push_back({ TempSoundfontFilePath.c_str(), 1.0f, _Container.GetBankOffset(), false, IsDLS });

            HasNonDefaultSoundfonts = true;
        }
    }

    // Finally, add the default soundfont.
    {
        if (!defaultSoundfontFilePath.isEmpty())
        {
            bool IsDLS = defaultSoundfontFilePath.toLower().endsWith(".dls");

            Soundfonts.push_back({ defaultSoundfontFilePath.c_str(), _BASSMIDIGain, 0, false, IsDLS });
        }
    }

    // Create the final soundfont list.
    for (const auto & sf : Soundfonts)
    {
        if (IsOneOf(sf.FilePath.extension().string().c_str(), { ".sflist", ".json" }))
        {
            Log.AtInfo().Write(STR_COMPONENT_BASENAME " is reading soundfont list \"%s\".", sf.FilePath.string().c_str());

            for (const auto & iter : LoadSoundfontList(sf.FilePath))
                _Soundfonts.push_back(iter);
        }
        else
        if (IsOneOf(sf.FilePath.extension().string().c_str(), { ".sf2", ".sf3", ".sf2pack", ".sfogg", ".dls" }))
            _Soundfonts.push_back(sf);
    }

    for (const auto & sf : _Soundfonts)
    {
        if (sf.IsDLS)
        {
            HasDLS = true;
            break;
        }
    }

    // Force the use of a soundfont player if an embedded or named soundfont was found.
    if ((_PlayerType != PlayerType::FluidSynth) && HasNonDefaultSoundfonts && !_Soundfonts.empty())
    {
        _PlayerType = (FluidSynth::API::Exists() && HasDLS) ? PlayerType::FluidSynth : PlayerType::BASSMIDI;
    }

    // Show which soundfonts we'll be using in the console.
    if ((_PlayerType == PlayerType::BASSMIDI) || (_PlayerType == PlayerType::FluidSynth))
    {
        // FluidSynth uses a soundfont stack. The stack is search from the top to the bottom for a matching preset. See https://www.fluidsynth.org/api/group__soundfont__management.html#ga0ba0bc9d4a19c789f9969cd22d22bf66
        if (_PlayerType == PlayerType::FluidSynth)
            std::reverse(_Soundfonts.begin(), _Soundfonts.end());

        for (const auto & sf : _Soundfonts)
            Log.AtInfo().Write(STR_COMPONENT_BASENAME " is using soundfont \"%s\" (Preferred bank offset %d).", sf.FilePath.string().c_str(), sf.BankOffset);
    }
}

/// <summary>
/// Gets the path name of a soundfont file with the same base name as the specified MIDI file, if any.
/// </summary>
bool GetSoundfontFilePath(const pfc::string & filePath, pfc::string & soundfontPath, abort_callback & abortHandler) noexcept
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

    soundfontPath = filePath;

    const char * Period = ::strrchr(soundfontPath.c_str(), '.');

    if (Period == nullptr)
        return false;

    for (const char * & FileExtension : FileExtensions)
    {
        soundfontPath.truncate((Period - soundfontPath.c_str()) + (size_t) 1);
        soundfontPath += FileExtension;

        try
        {
            if (filesystem::g_exists(soundfontPath, abortHandler))
                return true;
        }
        catch(...) {};
    }

    return false;
}

/// <summary>
/// Writes a buffer containing soundfont data to a temporary file.
/// </summary>
bool WriteSoundfontFile(const std::vector<uint8_t> data, bool isDLS, std::string & filePath) noexcept
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
