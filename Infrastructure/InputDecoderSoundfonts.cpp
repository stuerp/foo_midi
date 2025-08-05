 
/** $VER: InputDecoderSoundfonts.cpp (2025.08.04) - Soundfont support functions for the InputDecoder **/

#include "pch.h"

#include "InputDecoder.h"

#include "Support.h"
#include "Log.h"

#include <fstream>
#include <unordered_set>

#pragma hdrstop

static fs::path GetSoundfontFilePath(const fs::path & filePath, abort_callback & abortHandler) noexcept;
static bool WriteSoundfontFile(const fs::path & filePath, const std::vector<uint8_t> soundfont) noexcept;
static void AddSoundFont(const soundfont_t & sf, std::unordered_set<fs::path> & uniqueLists, std::vector<soundfont_t> & soundfonts, bool & hasDLS, std::string & report) noexcept;

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
            const bool IsDLS = (::memcmp(Data.data() + 8, "DLS ", 4) == 0);

            // BASSMIDI requires SF2 and SF3 sound fonts with an .sf2 or .sf3 extension. FluidSynth also supports DLS.
            const unique_path_t FilePath(IsDLS ? ".dls" : ".sf2");

            if (!FilePath.IsEmpty() && WriteSoundfontFile(FilePath.Path(), Data))
            {
                Soundfonts.push_back(soundfont_t(FilePath.Path(), 0.f, _Container.GetBankOffset(), true, IsDLS));

                HasNonDefaultSoundfonts = true;
            }
            else
                Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to write embedded soundfont to temporary file.");
        }
    }

    // Then, add the soundfont named like the MIDI file, if present.
    {
        const auto SoundfontFilePath = GetSoundfontFilePath(_FilePath.c_str(), abortHandler);

        if (!SoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(SoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            Soundfonts.push_back(soundfont_t(SoundfontFilePath, 0.f, _Container.GetBankOffset(), false, IsDLS));

            HasNonDefaultSoundfonts = true;
        }
    }

    // Next, add the soundfont named like the directory of the MIDI file, if present.
    {
        const fs::path FilePath = _FilePath.c_str();

        const auto SoundfontFilePath = GetSoundfontFilePath(FilePath.parent_path() / FilePath.parent_path().stem(), abortHandler);

        if (!SoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(SoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            Soundfonts.push_back(soundfont_t(SoundfontFilePath, 0.f, _Container.GetBankOffset(), false, IsDLS));

            HasNonDefaultSoundfonts = true;
        }
    }

    // Finally, add the default soundfont.
    {
        if (!defaultSoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(defaultSoundfontFilePath.extension().string().c_str(), ".dls") == 0);
            const float Gain = (_PlayerType == PlayerType::BASSMIDI) ? _BASSMIDIGain : 0.f;

            Soundfonts.push_back(soundfont_t(defaultSoundfontFilePath, Gain, 0, false, IsDLS));
        }
    }

    // Create the final soundfont list.
    std::string Report;
    std::unordered_set<fs::path> UniqueLists;

    for (const auto & sf : Soundfonts)
        AddSoundFont(sf, UniqueLists, _Soundfonts, HasDLS, Report);

    if (!Report.empty())
        popup_message::g_show(Report.c_str(), STR_COMPONENT_BASENAME, popup_message::icon_error);

    // Force the use of a soundfont player if an embedded or named soundfont was found.
    if ((_PlayerType != PlayerType::FluidSynth) && HasNonDefaultSoundfonts && !_Soundfonts.empty())
    {
        _PlayerType = (FluidSynth::API::Exists() && HasDLS) ? PlayerType::FluidSynth : PlayerType::BASSMIDI;
    }

    // Show which soundfonts we'll be using in the console.
    if ((_PlayerType == PlayerType::BASSMIDI) || (_PlayerType == PlayerType::FluidSynth))
    {
        // Both BASSMIDI and FluidSynth use a soundfont stack. The stack is searched from the top to the bottom for a matching preset. See https://www.fluidsynth.org/api/group__soundfont__management.html#ga0ba0bc9d4a19c789f9969cd22d22bf66
        // Except, BASSMIDI adds soundfonts to the bottom of the stack; FluidSynth to the top of the stack.
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
        try
        {
            auto SoundfontPath = filePath;

            // See https://github.com/stuerp/foo_midi/issues/114
            SoundfontPath.replace_extension(FileExtension);

            if (filesystem::g_exists(SoundfontPath.string().c_str(), abortHandler))
                return SoundfontPath;
        }
        catch (std::exception & e)
        {
            Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to replace file extension of \"%s\" with \"%s\": %s.", filePath.string().c_str(), FileExtension, e.what());
        }
    }

    return {};
}

/// <summary>
/// Writes a buffer containing soundfont data to a temporary file.
/// </summary>
bool WriteSoundfontFile(const fs::path & filePath, const std::vector<uint8_t> soundfont) noexcept
{
    try
    {
        std::ofstream Stream(filePath, std::ios::binary);

        if (!Stream.is_open())
            return false;

        Stream.write((const char *) soundfont.data(), (std::streamsize) soundfont.size());

        Stream.close();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

/// <summary>
/// Adds only existing soundfonts to the final list. Also handles recursively loading Soundfont list in soundfont lists.
/// </summary>
static void AddSoundFont(const soundfont_t & sf, std::unordered_set<fs::path> & uniqueLists, std::vector<soundfont_t> & soundfonts, bool & hasDLS, std::string & report) noexcept
{
    if (IsOneOf(sf.FilePath.extension().string().c_str(), { ".sflist", ".json" }))
    {
        // Prevent recursion with nested soundfont lists.
        {
            auto AbsolutePath = fs::absolute(sf.FilePath);

            if (uniqueLists.find(AbsolutePath) != uniqueLists.end())
                return;

            uniqueLists.insert(AbsolutePath);
        }

        if (!filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
        {
            report += ::FormatText("Soundfont list \"%s\" does not exist.", sf.FilePath.string().c_str()).c_str();

            return;
        }

        int Count = 0;

        Log.AtInfo().Write(STR_COMPONENT_BASENAME " is reading soundfont list \"%s\".", sf.FilePath.string().c_str());

        for (const auto & iter : LoadSoundfontList(sf.FilePath))
        {
            AddSoundFont(iter, uniqueLists, soundfonts, hasDLS, report);
            ++Count;
        }

        Log.AtInfo().Write(STR_COMPONENT_BASENAME " read %d soundfonts from the list.", Count);
    }
    else
    if (IsOneOf(sf.FilePath.extension().string().c_str(), { ".sf2", ".sf3", ".sf2pack", ".sfogg", ".dls" }))
    {
        if (!filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
        {
            report += ::FormatText("Soundfont \"%s\" does not exist.", sf.FilePath.string().c_str()).c_str();

            return;
        }

        if (sf.IsDLS)
            hasDLS = true;

        soundfonts.push_back(sf);
    }
}
