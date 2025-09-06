 
/** $VER: InputDecoderSoundfonts.cpp (2025.09.06) - Soundfont support functions for the InputDecoder **/

#include "pch.h"

#include "InputDecoder.h"

#include <libsf.h>
#include <unordered_set>

#include "Log.h"

#pragma hdrstop

static fs::path GetSoundfontFilePath(const fs::path & filePath, abort_callback & abortHandler) noexcept;
static void AddSoundFont(const soundfont_t & sf, std::unordered_set<std::string> & uniqueLists, std::vector<soundfont_t> & soundfonts, bool & hasDLS, std::string & report) noexcept;

/// <summary>
/// Gets the soundfonts and adjusts the player type, if necessary.
/// </summary>
void  InputDecoder::GetSoundfonts(const fs::path & defaultSoundfontFilePath, abort_callback & abortHandler)
{
    std::vector<soundfont_t> Soundfonts;

    const bool ConvertDLS = (((_PlayerType == PlayerType::BASSMIDI) && CfgBASSMIDIUseDLS) || ((_PlayerType == PlayerType::FluidSynth) && CfgFluidSynthUseDLS));

    /** IMPORTANT: The following sequence of adding soundfonts is optimal for BASSMIDI. For FluidSynth, we'll reverse the order later. **/

    // First, add the embedded soundfont, if present.
    {
        const auto & Data = _Container.SoundFont;

        if (Data.size() > 12)
        {
            const bool IsDLS = (::memcmp(Data.data() + 8, "DLS ", 4) == 0);

            if (IsDLS && ConvertDLS)
            {
                const msc::unique_path_t TempFilePath(".sf2");

                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is converting embedded DLS collection to SF2 bank \"%s\".", TempFilePath.Path().string().c_str());

                sf::dls::collection_t Collection;

                ReadDLS(Collection, Data);

                sf::bank_t Bank;

                Bank.ConvertFrom(Collection);

                if (!TempFilePath.IsEmpty() && WriteSF2(Bank, TempFilePath.Path()))
                    Soundfonts.push_back(soundfont_t(TempFilePath.Path(), 0.f, _Container.BankOffset, true, false));
                else
                    Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to write embedded soundfont to temporary file.");
            }
            else
            {
                // BASSMIDI requires SF2 and SF3 sound fonts with an .sf2 or .sf3 extension. FluidSynth also supports DLS.
                const msc::unique_path_t TempFilePath(IsDLS ? ".dls" : ".sf2");

                if (!TempFilePath.IsEmpty() && WriteSF2(Data, TempFilePath.Path()))
                    Soundfonts.push_back(soundfont_t(TempFilePath.Path(), 0.f, _Container.BankOffset, true, IsDLS));
                else
                    Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to write embedded soundfont to temporary file.");
            }
        }
    }

    // Then, add the soundfont named like the MIDI file, if present.
    try
    {
        const auto SoundfontFilePath = GetSoundfontFilePath(_FilePath.c_str(), abortHandler);

        if (!SoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(SoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            if (IsDLS && ConvertDLS)
            {
                const msc::unique_path_t TempFilePath(".sf2");

                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is converting DLS collection \"%s\" to SF2 bank \"%s\".", SoundfontFilePath.string().c_str(), TempFilePath.Path().string().c_str());

                sf::dls::collection_t Collection;

                ReadDLS(Collection, SoundfontFilePath);

                sf::bank_t Bank;

                Bank.ConvertFrom(Collection);

                WriteSF2(Bank, TempFilePath.Path());

                Soundfonts.push_back(soundfont_t(TempFilePath.Path(), 0.f, 0, false, false));
            }
            else
                Soundfonts.push_back(soundfont_t(SoundfontFilePath, 0.f, 0, false, IsDLS));
        }
    }
    catch (std::exception & e)
    {
        Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to look for soundfont named like MIDI file: %s.", e.what());
    }

    // Next, add the soundfont named like the directory of the MIDI file, if present.
    try
    {
        const fs::path FilePath = _FilePath.c_str();

        const auto SoundfontFilePath = GetSoundfontFilePath(FilePath.parent_path() / FilePath.parent_path().stem(), abortHandler);

        if (!SoundfontFilePath.empty())
        {
            const bool IsDLS = (::_stricmp(SoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            if (IsDLS && ConvertDLS)
            {
                const msc::unique_path_t TempFilePath(".sf2");

                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is converting DLS collection \"%s\" to SF2 bank \"%s\".", SoundfontFilePath.string().c_str(), TempFilePath.Path().string().c_str());

                sf::dls::collection_t Collection;

                ReadDLS(Collection, SoundfontFilePath);

                sf::bank_t Bank;

                Bank.ConvertFrom(Collection);

                WriteSF2(Bank, TempFilePath.Path());

                Soundfonts.push_back(soundfont_t(TempFilePath.Path(), 0.f, 0, false, false));
            }
            else
                Soundfonts.push_back(soundfont_t(SoundfontFilePath, 0.f, 0, false, IsDLS));
        }
    }
    catch (std::exception & e)
    {
        Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to look for soundfont named like directory containing the MIDI file: %s.", e.what());
    }

    // Finally, add the default soundfont.
    {
        if (!defaultSoundfontFilePath.empty())
        {
            const float Gain = (_PlayerType == PlayerType::BASSMIDI) ? _BASSMIDIGain : 0.f;

            const bool IsDLS = (::_stricmp(defaultSoundfontFilePath.extension().string().c_str(), ".dls") == 0);

            if (IsDLS && ConvertDLS)
            {
                const msc::unique_path_t TempFilePath(".sf2");

                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is converting DLS collection \"%s\" to SF2 bank \"%s\".", defaultSoundfontFilePath.string().c_str(), TempFilePath.Path().string().c_str());

                sf::dls::collection_t Collection;

                ReadDLS(Collection, defaultSoundfontFilePath);

                sf::bank_t Bank;

                Bank.ConvertFrom(Collection);

                WriteSF2(Bank, TempFilePath.Path());

                Soundfonts.push_back(soundfont_t(TempFilePath.Path(), Gain, 0, false, false));
            }
            else
                Soundfonts.push_back(soundfont_t(defaultSoundfontFilePath, Gain, 0, false, IsDLS));
        }
    }

    // Create the final soundfont list.
    {
        std::unordered_set<std::string> UniqueLists;
        std::string Report;
        bool HasDLS = false;

        for (const auto & sf : Soundfonts)
            AddSoundFont(sf, UniqueLists, _Soundfonts, HasDLS, Report);

        if (!Report.empty())
            popup_message::g_show(Report.c_str(), STR_COMPONENT_BASENAME, popup_message::icon_error);
    }

    // Show which sound fonts we'll be using in the console.
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
        fs::path SoundfontPath = filePath;

        SoundfontPath.replace_extension(FileExtension);

        if (filesystem::g_exists(SoundfontPath.string().c_str(), abortHandler))
            return SoundfontPath;
    }

    return {};
}

/// <summary>
/// Adds only existing soundfonts to the final list. Also handles recursively loading Soundfont list in soundfont lists.
/// </summary>
static void AddSoundFont(const soundfont_t & sf, std::unordered_set<std::string> & uniqueLists, std::vector<soundfont_t> & soundfonts, bool & hasDLS, std::string & report) noexcept
{
    if (msc::IsOneOf(sf.FilePath.extension().string().c_str(), { ".sflist", ".json" }))
    {
        // Prevent recursion with nested soundfont lists.
        {
            auto AbsolutePath = fs::absolute(sf.FilePath);

            if (uniqueLists.find(AbsolutePath.string().c_str()) != uniqueLists.end())
                return;

            uniqueLists.insert(AbsolutePath.string().c_str());
        }

        if (!filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
        {
            report += msc::FormatText("Soundfont list \"%s\" does not exist.", sf.FilePath.string().c_str()).c_str();

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
    if (msc::IsOneOf(sf.FilePath.extension().string().c_str(), { ".sf2", ".sf3", ".sf2pack", ".sfogg", ".dls" }))
    {
        if (!filesystem::g_exists(sf.FilePath.string().c_str(), fb2k::noAbort))
        {
            report += msc::FormatText("Soundfont \"%s\" does not exist.", sf.FilePath.string().c_str()).c_str();

            return;
        }

        if (sf.IsDLS)
            hasDLS = true;

        soundfonts.push_back(sf);
    }
}

/// <summary>
/// Reads a DLS collection from memory.
/// </summary>
bool ReadDLS(sf::dls::collection_t & collection, const std::vector<uint8_t> & dlsData) noexcept
{
    try
    {
        msc::memory_stream_t ms;

        if (ms.Open(dlsData.data(), dlsData.size()))
        {
            sf::dls::reader_t dr;

            if (dr.Open(&ms, riff::reader_t::option_t::None))
            {
                dr.Process(collection, sf::dls::reader_options_t(true));
            }
        }

        return true;
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to read DLS collection: %s.", e.what());

        return false;
    }
}

/// <summary>
/// Reads a DLS collection from a file.
/// </summary>
bool ReadDLS(sf::dls::collection_t & collection, const fs::path & filePath) noexcept
{
    try
    {
        msc::file_stream_t fs;

        if (fs.Open(filePath, false))
        {
            sf::dls::reader_t dr;

            if (dr.Open(&fs, riff::reader_t::option_t::None))
            {
                dr.Process(collection, sf::dls::reader_options_t(true));
            }
        }

        return true;
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to read DLS collection: %s.", e.what());

        return false;
    }
}

/// <summary>
/// Writes a SF2 bank to a file.
/// </summary>
bool WriteSF2(const sf::bank_t & bank, const fs::path & filePath) noexcept
{
    try
    {
        msc::file_stream_t fs;

        if (fs.Open(filePath, true))
        {
            sf::writer_t sw;

            if (sw.Open(&fs, riff::writer_t::Options::PolyphoneCompatible))
            {
                sw.Process(bank);
            }
        }

        return true;
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to write SF2 bank: %s.", e.what());

        return false;
    }
}

/// <summary>
/// Writes a buffer containing SF2 data to a file.
/// </summary>
bool WriteSF2(const std::vector<uint8_t> & sf2Data, const fs::path & filePath) noexcept
{
    try
    {
        std::ofstream Stream(filePath, std::ios::binary);

        if (!Stream.is_open())
            return false;

        Stream.write((const char *) sf2Data.data(), (std::streamsize) sf2Data.size());

        Stream.close();

        return true;
    }
    catch (...)
    {
        return false;
    }
}
