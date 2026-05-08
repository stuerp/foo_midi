
/** $VER: Cakewalk.cpp (2025.07.26) P. Stuer - Reads an Instrument Definition file **/

#include "pch.h"

#include "Cakewalk.h"

#include <map>

#include <algorithm>
#include <cctype>

#include <iostream>
#include <fstream>
#include <sstream>
#include <ranges>

#pragma warning(disable: 4820)

namespace StringUtils
{
    static std::string Trim(const std::string & str)
    {
        auto Start = str.find_first_not_of(" \t\r\n");
        auto End   = str.find_last_not_of(" \t\r\n");

        return (Start == std::string::npos) ? "" : str.substr(Start, End - Start + 1);
    }

    static bool IsComment(const std::string & line)
    {
        const std::string Trimmed = Trim(line);

        return Trimmed.empty() || Trimmed[0] == ';';
    }

    static bool IsSectionHeader(const std::string & line)
    {
        const std::string Trimmed = Trim(line);

        return !Trimmed.empty() && Trimmed.front() == '.';
    }

    static std::string ExtractSectionName(const std::string & line)
    {
        const std::string Trimmed = Trim(line);

        return Trimmed.substr(1, Trimmed.length() - 1);
    }

    static bool IsSubsectionHeader(const std::string & line)
    {
        const std::string Trimmed = Trim(line);

        return !Trimmed.empty() && Trimmed.front() == '[' && Trimmed.back() == ']';
    }

    static std::string ExtractSubsectionName(const std::string & line)
    {
        const std::string Trimmed = Trim(line);

        return Trimmed.substr(1, Trimmed.length() - 2); // Strip [ and ]
    }
}

/* MIDI 2.0
struct RegisteredController {
    uint16_t bank;     // 0–16383
    uint16_t index;    // 0–16383
    std::string name;
    uint32_t value;    // 32-bit resolution
};

std::map<std::pair<uint16_t, uint16_t>, RegisteredController> registeredControllers;

void InsParser::parseRegisteredControllerEntry(const std::string& line) {
    std::regex rcPattern(R"((\d+)\.(\d+)\s*=\s*(.+))");
    std::smatch match;
    if (std::regex_match(line, match, rcPattern)) {
        uint16_t bank = std::stoi(match[1]);
        uint16_t index = std::stoi(match[2]);
        std::string name = StringUtils::trim(match[3]);
        registeredControllers[{bank, index}] = {bank, index, name, 0};
    }
}
void applyRelativeChange(uint16_t bank, uint16_t index, int32_t delta) {
    auto& rc = registeredControllers[{bank, index}];
    rc.value = std::clamp<int64_t>(rc.value + delta, 0, UINT32_MAX);
}*/

class subsection_t
{
public:
    subsection_t() { }
    subsection_t(const std::string & name) : Name(name) { };

public:
    std::string Name;
    std::string BasedOn;
    std::map<std::string, std::string> Items;
};

class section_t
{
public:
    section_t() { }
    section_t(const std::string & name): Name(name) { };

public:
    std::string Name;

    std::map<std::string, subsection_t> Subsections;
};

class InsParser
{
public:
    bool Read(const fs::path & filePath);

public:
    std::map<std::string, section_t> Sections;

private:
    void ParseLine(const std::string & line);

    enum Mode
    {
        Bank,           // (CC#0 * 128) + CC#32
        DrumKit,
        Controller,
        RPN,            // (CC#101 * 128) + CC#100
        NRPN,           // (CC#99 * 128) * CC#98
        Instrument,
    };

private:
    Mode _Mode;

    section_t * _Section;
    subsection_t * _Subsection;

    std::map<std::string, std::string> * _Items;
};

bool InsParser::Read(const fs::path & filePath)
{
    std::ifstream Stream(filePath);

    if (!Stream.is_open())
        return false;

    std::string line;

    while (std::getline(Stream, line))
    {
        ParseLine(line);
    }

    return true;
}

void InsParser::ParseLine(const std::string & line)
{
    using namespace StringUtils;

    if (IsComment(line))
        return;

    if (IsSectionHeader(line))
    {
        std::string Name = ExtractSectionName(line);

        auto [Iter, IsInserted] = Sections.insert_or_assign(Name, section_t(Name));

        _Section = &Iter->second;

        if (Name == "Patch Names")
            _Mode = Bank;
        else
        if (Name == "Note Names")
            _Mode = DrumKit;
        else
        if (Name == "Controller Names")
            _Mode = Controller;
        else
        if (Name == "RPN Names")
            _Mode = RPN;
        else
        if (Name == "NRPN Names")
            _Mode = NRPN;
        else
        if (Name == "Instrument Definitions")
            _Mode = Instrument;
    }
    else
    if (IsSubsectionHeader(line))
    {
        std::string Name = ExtractSubsectionName(line);

        auto [Iter, IsInserted] = _Section->Subsections.insert_or_assign(Name, subsection_t(Name));

        _Subsection = &Iter->second;
        _Items = &(_Subsection->Items);
    }
    else
    {
        const char * Delimiter = ::strchr(line.c_str(), '=');

        if (Delimiter != nullptr)
        {
            std::string Key = Trim(line.substr(0, (size_t) (Delimiter - line.c_str())));
            std::string Value(Delimiter + 1);

            if (Key == "BasedOn")
                _Subsection->BasedOn = Trim(Value);
            else
                _Items->insert_or_assign(Trim(Key), Trim(Value));
        }
    }
}

/// <summary>
/// Reads a Cakewalk Instrument Definition file (.ins)
/// </summary>
void ReadIns(const fs::path & filePath)
{
    InsParser Parser;

    if (!Parser.Read(filePath))
        return;

    auto & Instruments = Parser.Sections["Instrument Definitions"];

    for (const auto & Instrument : Instruments.Subsections)
    {
        std::cout << "." << Instrument.second.Name << "\n";

        for (const auto & [Key, Value] : Instrument.second.Items)
        {
            std::cout << "  - " << Key << " = " << Value << "\n";
        }
    }
}
