
/** $VER: Soundfont.cpp (2025.07.22) P. Stuer - Soundfont support routines for wavetable players **/

#include "pch.h"

#include "Soundfont.h"
#include "SoundfontCache.h"

#include "Resource.h"
#include "Encoding.h"
#include "Log.h"

#include <json-builder.h>

#include <fstream>

static std::vector<soundfont_t> ProcessJSON(const fs::path & filePath, const json_value * json);
static std::vector<soundfont_t> ProcessText(const fs::path & filePath);
static void ProcessPatchMappings(const json_value * patchMappings, uint32_t channel, std::vector<BASS_MIDI_FONTEX> & out, BASS_MIDI_FONTEX & fontex);
static const json_value * FindObject(const json_value * object, const char * name);

/// <summary>
/// Loads a soundfont list (*.sflist, *.json)
/// </summary>
std::vector<soundfont_t> LoadSoundfontList(const fs::path & filePath)
{
    if (!fs::exists(filePath))
        return { };

    std::uintmax_t Size = fs::file_size(filePath);

    std::vector<char> Data(Size + 1);

    FILE * fp = nullptr;

    errno_t rc = ::fopen_s(&fp, (const char *) filePath.string().c_str(), "r");

    if ((rc != 0) || (fp == nullptr))
        throw std::exception(::FormatText("Failed to open soundfont list \"%s\": error %d", (const char *) filePath.string().c_str(), rc).c_str());

    ::fread(Data.data(), Data.size(), 1, fp);

    ::fclose(fp);

    Data[Size] = '\0';

    size_t Offset = 0;

    // Handle Unicode byte order markers.
    if (Size >= 2)
    {
        if ((Data[0] == 0xFF && Data[1] == 0xFE) || (Data[0] == 0xFE && Data[1] == 0xFF))
            throw std::exception("UTF-16 encoding is not supported at this time");

        if (Size >= 3 && (Data[0] == 0xEF) && (Data[1] == 0xBB) && (Data[2] == 0xBF))
        {
            Offset = 3;
            Size  -= 3;
        }
    }

    std::string ErrorMessage; ErrorMessage.resize(1024);

    json_settings Settings = { 0 };

    Settings.value_extra = json_builder_extra;

    json_value * JSON = ::json_parse_ex(&Settings, &Data[Offset], Size, ErrorMessage.data());

    if (JSON != nullptr)
    {
        auto List = ProcessJSON(filePath, JSON);

        ::json_builder_free(JSON);

        return List;
    }
    else
    {
        auto List = ProcessText(filePath);

        return List;
    }
}

/// <summary>
/// Processes the file as a flat text file.
/// </summary>
static std::vector<soundfont_t> ProcessText(const fs::path & filePath)
{
    std::vector<soundfont_t> Items;

    std::ifstream File(filePath);

    if (File)
    {
        std::string Line;

        while (std::getline(File, Line))
        {
            // Skip empty lines and lines containing only whitespace.
            if (std::all_of(Line.begin(), Line.end(), [](unsigned char c) { return std::isspace(c); }))
                continue;

            soundfont_t sf = { };

            if (fs::path(Line).is_relative())
                sf.FilePath = filePath.parent_path() / Line;
            else
                sf.FilePath = Line;

            Items.push_back(sf);
        }
    }

    return Items;
}

/// <summary>
/// Processes the JSON object.
/// </summary>
static std::vector<soundfont_t> ProcessJSON(const fs::path & filePath, const json_value * json)
{
    if (json == nullptr)
        return { };

    // Check the structure of the object.
    {
        if (json->type != json_object)
            throw std::exception("Base JSON item is not an object");

        if (json->u.object.length != 1)
            throw std::exception("Base JSON object can only be one object");

        if (::strcmp(json->u.object.values[0].name, "soundFonts") != 0)
            throw std::exception("Base JSON object must be a soundFonts object");
    }

    auto Soundfonts = json->u.object.values[0].value;

    {
        if (Soundfonts->type != json_array)
            throw std::exception("soundFonts object is not an array");

        for (uint32_t i = 0; i < Soundfonts->u.array.length; ++i)
        {
            auto Values = Soundfonts->u.array.values[i];

            if (Values->type != json_object)
                throw std::exception("Array item is not an object");

            {
                auto FileName = FindObject(Values, "fileName");

                if (FileName->type == json_none)
                    throw std::exception(::FormatText("Soundfont object %d has no fileName property", i + 1).c_str());

                if (FileName->type != json_string)
                    throw std::exception(::FormatText("fileName property of soundFont object %d is not a string", i + 1).c_str());
            }

            {
                auto Gain = FindObject(Values, "gain");

                if ((Gain->type != json_none) && (Gain->type != json_integer) && (Gain->type != json_double))
                    throw std::exception(::FormatText("gain property of soundFont object %d is not a number", i + 1).c_str());
            }

            {
                auto Channels = FindObject(Values, "channels");

                if (Channels->type != json_none)
                {
                    if (Channels->type != json_array)
                        throw std::exception(::FormatText("channels property of soundFont object %d is not an array", i + 1).c_str());

                    for (uint32_t j = 0; j < Channels->u.array.length; ++j)
                    {
                        json_value * channel = Channels->u.array.values[j];

                        if (channel->type != json_integer)
                            throw std::exception(::FormatText("Channel %d of channels property of soundFont object %d is not an integer", j + 1, i + 1).c_str());

                        if (channel->u.integer < 1 || channel->u.integer > 48)
                            throw std::exception(::FormatText("Channel %d of channels property of soundFont object %d is out of range. Value 1 thru 48 are valid", j + 1, i + 1).c_str());
                    }
                }
            }

            {
                auto PatchMappings = FindObject(Values, "patchMappings");

                if (PatchMappings->type != json_none)
                {
                    if (PatchMappings->type != json_array)
                        throw std::exception(::FormatText("patchMappings property of soundFont %d is not an array", i + 1).c_str());

                    for (uint32_t j = 0; j < PatchMappings->u.array.length; ++j)
                    {
                        uint32_t SrcCount = 0;
                        uint32_t DstCount = 0;

                        json_value * Mapping = PatchMappings->u.array.values[j];

                        if (Mapping->type != json_object)
                            throw std::exception(::FormatText("patchMapping %d of soundFont %d is not an object", j + 1, i + 1).c_str());

                        for (uint32_t k = 0; k < Mapping->u.object.length; ++k)
                        {
                            auto Name  = Mapping->u.object.values[k].name;
                            auto Value = Mapping->u.object.values[k].value;

                            if (Value->type != json_object)
                                throw std::exception(::FormatText("patchMapping %d of soundFont %d must only contain a source and destination object", j + 1, i + 1).c_str());

                            if (::strcmp(Name, "source") == 0)
                                ++SrcCount;
                            else
                            if (::strcmp(Name, "destination") == 0)
                                ++DstCount;

                            uint32_t BankCount = 0;
                            uint32_t ProgramCount = 0;

                            for (uint32_t l = 0; l < Value->u.object.length; ++l)
                            {
                                auto Name2  = Value->u.object.values[l].name;
                                auto Value2 = Value->u.object.values[l].value;

                                if ((::strcmp(Name2, "bank") != 0) && (::strcmp(Name2, "program") != 0))
                                    throw std::exception(::FormatText("invalid property %s", Name2).c_str());

                                if (Value2->type != json_integer)
                                    throw std::exception(::FormatText("invalid property value type").c_str());

                                int Min =   0;
                                int Max = 128;

                                if (::strcmp(Name2, "bank") == 0)
                                    ++BankCount;

                                if (::strcmp(Name2, "program") == 0)
                                {
                                    ++ProgramCount;
                                    Max = (::strcmp(Name, "destination") == 0) ? 65535 : 127;
                                }

                                if (Value2->u.integer < Min || Value2->u.integer > Max)
                                    throw std::exception(::FormatText("invalid value for property %s", Name2).c_str());
                            }

                            if (BankCount == 0)
                                throw std::exception(::FormatText("bank property not found").c_str());

                            if (BankCount > 1)
                                throw std::exception(::FormatText("More than one bank property").c_str());

                            if (ProgramCount == 0)
                                throw std::exception(::FormatText("program property not found").c_str());

                            if (ProgramCount > 1)
                                throw std::exception(::FormatText("More than one program property").c_str());
                        }

                        if (SrcCount > 1)
                            throw std::exception(::FormatText("More than one source property in patchMapping %d of soundFont %d", j + 1, i + 1).c_str());

                        if (DstCount == 0)
                            throw std::exception(::FormatText("destination property not found in patchMapping %d of soundFont %d", j + 1, i + 1).c_str());

                        if (DstCount > 1)
                            throw std::exception(::FormatText("More than one destination property in patchMapping %d of soundFont %d", j + 1, i + 1).c_str());
                    }
                }
            }
        }
    }

    std::vector<soundfont_t> Items;

    for (uint32_t i = 0; i < Soundfonts->u.array.length; ++i)
    {
        auto JSONSoundfont = Soundfonts->u.array.values[i];

        soundfont_t sf;

        {
            auto FileName = ::FindObject(JSONSoundfont, "fileName");

            if (fs::path(FileName->u.string.ptr).is_relative())
                sf.FilePath = filePath.parent_path() / FileName->u.string.ptr;
            else
                sf.FilePath = FileName->u.string.ptr;
        }

        {
            auto Gain = ::FindObject(JSONSoundfont, "gain");

            if (Gain->type != json_none)
            {
                double Value = 0.0;

                if (Gain->type == json_integer)
                    Value = (double) Gain->u.integer;
                else
                if (Gain->type == json_double)
                    Value = Gain->u.dbl;

                sf.Gain = (float) Value;
            }
        }

        {
            BASS_MIDI_FONTEX fex
            {
                .font     = NULL,
                .spreset  = -1,
                .sbank    = -1,
                .dpreset  = -1,
                .dbank    =  0,
                .dbanklsb =  0
            };

            auto JSONChannels      = ::FindObject(JSONSoundfont, "channels");
            auto JSONPatchMappings = ::FindObject(JSONSoundfont, "patchMappings");

            // Simplest case, whole bank loading
            if ((JSONChannels->type == json_none) && (JSONPatchMappings->type == json_none))
            {
                sf.FontEx.push_back(fex);
            }
            else
            {
                if (JSONPatchMappings->type == json_none)
                {
                    for (uint32_t k = 0; k < JSONChannels->u.array.length; ++k)
                    {
                        fex.dbanklsb = (int) JSONChannels->u.array.values[k]->u.integer;

                        sf.FontEx.push_back(fex);
                    }
                }
                else
                if (JSONChannels->type == json_none)
                    ProcessPatchMappings(JSONPatchMappings, 0, sf.FontEx, fex);
                else
                {
                    for (uint32_t k = 0; k < JSONChannels->u.array.length; ++k)
                        ProcessPatchMappings(JSONPatchMappings, (uint32_t) JSONChannels->u.array.values[k]->u.integer, sf.FontEx, fex);
                }
            }
        }

        Items.push_back(sf);
    }

    return Items;
}

/// <summary>
/// Converts the patch mappings to BASS_MIDI_FONTEX items.
/// </summary>
static void ProcessPatchMappings(const json_value * patchMappings, uint32_t channel, std::vector<BASS_MIDI_FONTEX> & out, BASS_MIDI_FONTEX & fontex)
{
    for (uint32_t i = 0; i < patchMappings->u.array.length; ++i)
    {
        auto Mapping = patchMappings->u.array.values[i];

        auto Src        = FindObject(Mapping, "source");
        auto SrcBank    = FindObject(Src, "bank");
        auto SrcProgram = FindObject(Src, "program");

        auto Dst        = FindObject(Mapping, "destination");
        auto DstBank    = FindObject(Dst, "bank");
        auto DstProgram = FindObject(Dst, "program");

        fontex.spreset  = (SrcProgram->type == json_none) ? -1 : (int) SrcProgram->u.integer;
        fontex.sbank    = (SrcBank->type    == json_none) ? -1 : (int) SrcBank->u.integer;

        fontex.dpreset  = (DstProgram->type == json_none) ? -1 : (int) DstProgram->u.integer;
        fontex.dbank    = (DstBank->type    == json_none) ?  0 : (int) DstBank->u.integer;
        fontex.dbanklsb = (int) channel;

        out.push_back(fontex);
    }
}

/// <summary>
/// Finds an object with the specified name.
/// </summary>
static const json_value * FindObject(const json_value * object, const char * name)
{
    if (object->type != json_object)
        return &json_value_none;

    for (uint32_t i = 0; i < object->u.object.length; ++i)
    {
        if (::strcmp(object->u.object.values[i].name, name) == 0)
            return object->u.object.values[i].value;
    }

    return &json_value_none;
}
