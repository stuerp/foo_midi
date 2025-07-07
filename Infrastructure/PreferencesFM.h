
/** $VER: PreferencesFM.h (2025.07.07) P. Stuer **/

#pragma once

#include <string>
#include <vector>

#pragma warning(disable: 4820)

struct bank_t
{
    int Id;
    std::wstring Name;

    bank_t() noexcept : Id(-1), Name() { }
    bank_t(const bank_t & other) noexcept : Id(other.Id), Name(other.Name) { }
    bank_t(int number, const std::wstring & name) noexcept : Id(number), Name(name) { }

    bank_t & operator =(const bank_t & other) noexcept
    {
        Id = other.Id;
        Name = other.Name;

        return *this;
    }

    bool operator ==(const bank_t & b) const noexcept { return Id == b.Id; }
    bool operator !=(const bank_t & b) const noexcept { return !operator ==(b); }
    bool operator < (const bank_t & b) const noexcept { return Name < b.Name; }
    bool operator > (const bank_t & b) const noexcept { return Name > b.Name; }
};

struct emulator_t
{
    int Id;
    std::wstring Name;

    emulator_t() noexcept : Id(-1), Name() { }
    emulator_t(const emulator_t & other) noexcept : Id(other.Id), Name(other.Name) { }
    emulator_t(int number, const std::wstring & name) noexcept : Id(number), Name(name) { }

    emulator_t & operator =(const emulator_t & other) noexcept
    {
        Id = other.Id;
        Name = other.Name;

        return *this;
    }

    bool operator ==(const bank_t & b) const noexcept { return Id == b.Id; }
    bool operator !=(const bank_t & b) const noexcept { return !operator ==(b); }
    bool operator < (const bank_t & b) const noexcept { return Name < b.Name; }
    bool operator > (const bank_t & b) const noexcept { return Name > b.Name; }
};

extern const std::vector<emulator_t> _ADLEmulators;
extern const std::vector<emulator_t> _OPNEmulators;

extern const std::vector<std::string> _MT32EmuSets;
