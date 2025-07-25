 
/** $VER: InputDecoderSoundfonts.cpp (2025.07.23) - Soundfont support functions for the InputDecoder **/

#include "pch.h"

#include "InputDecoder.h"

#include "Support.h"
#include "Log.h"

#include <fstream>

#pragma hdrstop

static fs::path GetSoundfontFilePath(const fs::path & filePath, abort_callback & abortHandler) noexcept;
static bool WriteSoundfontFile(const std::vector<uint8_t> data, bool isDLS, fs::path & filePath) noexcept;

/// <summary>
/// Gets the soundfonts and adjusts the player type, if necessary.
/// </summary>
void  InputDecoder::GetSoundfonts(const fs::path & defaultSoundfontFilePath, abort_callback & abortHandler)
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

            fs::path FilePath;

            if (WriteSoundfontFile(Data, IsDLS, FilePath))
            {
                Soundfonts.push_back(soundfont_t(FilePath, 1.f, _Container.GetBankOffset(), true, IsDLS));

                HasNonDefaultSoundfonts = true;
            }
        }
    }
        
    // Then, add the soundfont named like the MIDI file, if present.
    {
        auto SoundfontFilePath = GetSoundfontFilePath(_FilePath.c_str(), abortHandler);

        if (!SoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(SoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            Soundfonts.push_back(soundfont_t(SoundfontFilePath, 1.f, _Container.GetBankOffset(), false, IsDLS));

            HasNonDefaultSoundfonts = true;
        }
    }

    // Finally, add the default soundfont.
    {
        if (!defaultSoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(defaultSoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            Soundfonts.push_back(soundfont_t(defaultSoundfontFilePath, _BASSMIDIGain, 0, false, IsDLS));
        }
    }

    // Create the final soundfont list.
    std::string Message;

    for (const auto & sf : Soundfonts)
    {
        if (IsOneOf(sf.FilePath.extension().string().c_str(), { ".sflist", ".json" }))
        {
            if (filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
            {
                int Count = 0;

                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is reading soundfont list \"%s\".", sf.FilePath.string().c_str());

                for (const auto & iter : LoadSoundfontList(sf.FilePath))
                {
                    if (filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
                    {
                        if (sf.IsDLS)
                            HasDLS = true;

                        _Soundfonts.push_back(iter);
                        ++Count;
                    }
                    else
                        Message += ::FormatText("Soundfont \"%s\" does not exist.\n", iter.FilePath.string().c_str()).c_str();
                }

                Log.AtInfo().Write(STR_COMPONENT_BASENAME " read %d soundfonts from the list.", Count);
            }
            else
                Message += ::FormatText("Soundfont list \"%s\" does not exist.", sf.FilePath.string().c_str()).c_str();
        }
        else
        if (IsOneOf(sf.FilePath.extension().string().c_str(), { ".sf2", ".sf3", ".sf2pack", ".sfogg", ".dls" }))
        {
            if (filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
            {
                if (sf.IsDLS)
                    HasDLS = true;

                _Soundfonts.push_back(sf);
            }
            else
                Message += ::FormatText("Soundfont \"%s\" does not exist.", sf.FilePath.string().c_str()).c_str();
        }
    }

    if (!Message.empty())
        popup_message::g_show(Message.c_str(), STR_COMPONENT_BASENAME, popup_message::icon_error);

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
fs::path GetSoundfontFilePath(const fs::path & filePath, abort_callback & abortHandler) noexcept
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

    for (const char * & FileExtension : FileExtensions)
    {
        auto SoundfontPath = filePath;

        SoundfontPath.replace_extension(FileExtension);

        try
        {
            if (filesystem::g_exists(SoundfontPath.string().c_str(), abortHandler))
                return SoundfontPath;
        }
        catch(...) {};
    }

    return {};
}

/// <summary>
/// Writes a buffer containing soundfont data to a temporary file.
/// </summary>
bool WriteSoundfontFile(const std::vector<uint8_t> soundfont, bool isDLS, fs::path & filePath) noexcept
{
    char TempPath[MAX_PATH] = {};

    if (::GetTempPathA(_countof(TempPath), TempPath) == 0)
        return false;

    char TempFilePath[MAX_PATH] = {};

    if (::GetTempFileNameA(TempPath, "SF-", 0, TempFilePath) == 0)
        return false;

    fs::path FilePath = TempFilePath;

    // BASSMIDI requires SF2 and SF3 sound fonts with an .sf2 or .sf3 extension. FluidSynth also supports DLS.
    FilePath.replace_extension(isDLS ? ".dls" : ".sf2");

    try
    {
        std::ofstream Stream(FilePath, std::ios::binary);

        if (Stream)
        {
            Stream.write((const char *) soundfont.data(), (std::streamsize) soundfont.size());

            Stream.close();

            filePath = FilePath;
        }
    }
    catch (...) { }

    ::DeleteFileA(TempFilePath);

    return true;
}
