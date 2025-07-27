
/** $VER: Support.cpp (2025.07.26) - Support functions **/

#pragma once

bool IsOneOf(const std::wstring & item, const std::vector<std::wstring> & list) noexcept;
bool IsOneOf(const char * item, const std::vector<const char *> & list) noexcept;

/// <summary>
/// Creates a unique file name with an optional file extension. The caller takes responsibility for deleting the file after use if necessary.
/// </summary>
class unique_path_t
{
public:
    unique_path_t() noexcept : unique_path_t(fs::path())
    {
    }

    unique_path_t(const fs::path & extension) noexcept
    {
        char TempPath[MAX_PATH] = {};

        if (::GetTempPathA(_countof(TempPath), TempPath) == 0)
            return;

        GUID guid;

        if (!SUCCEEDED(::CoCreateGuid(&guid)))
            return;

        char FileName[MAX_PATH];

        if (-1 == ::sprintf_s(FileName, sizeof(FileName), "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]))
            return;

        _Path = fs::path(TempPath) / FileName;

        _Path.replace_extension(extension);
    }

    fs::path Path() const noexcept { return _Path; }

    bool IsEmpty() const noexcept { return _Path.empty(); }

private:
    fs::path _Path;
};
