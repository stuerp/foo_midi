
/** $VER: main.cpp (2026.05.03) P. Stuer **/

#include "pch.h"

void ExamineFile(const fs::path & filePath, const std::map<std::string, std::string> & args);

static void ProcessDirectory(const fs::path & directoryPath);
static void ProcessFile(const fs::path & filePath);

const std::vector<fs::path> Filters = { ".mmd", ".mid", ".g36", ".rmi", ".mxmf", ".xmf", ".mmf", ".tst" };

std::map<std::string, std::string> Arguments;

int main(int argc, const char ** argv)
{
    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    if (argc < 2)
    {
        ::printf("Insufficient arguments.\n");

        return -1;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            if (::_stricmp(argv[i], "-stream") == 0)
                Arguments["AsStream"] = "";
        }

        Arguments["midifile"] = argv[i];
    }

    if (!::fs::exists(Arguments["midifile"]))
    {
        ::printf("Failed to access \"%s\": path does not exist.\n", Arguments["midifile"].c_str());

        return -1;
    }

    fs::path Path = fs::canonical(Arguments["midifile"]);

    if (fs::is_directory(Path))
        ProcessDirectory(Path);
    else
        ProcessFile(Path);

    return 0;
}

/// <summary>
/// Returns true if the string matches one of the list.
/// </summary>
static bool IsOneOf(const fs::path & item, const std::vector<fs::path> & list) noexcept
{
    for (const auto & Item : list)
    {
        if (::_stricmp(item.string().c_str(), Item.string().c_str()) == 0)
            return true;
    }

    return false;
}

/// <summary>
///
/// </summary>
static void ProcessDirectory(const fs::path & directoryPath)
{
    ::printf("\"%s\"\n", directoryPath.string().c_str());

    for (const auto & Entry : fs::directory_iterator(directoryPath))
    {
        if (Entry.is_directory())
        {
            ProcessDirectory(Entry.path());
        }
        else
        if (IsOneOf(Entry.path().extension(), Filters))
        {
            ProcessFile(Entry.path());
        }
    }
}

/// <summary>
///
/// </summary>
static void ProcessFile(const fs::path & filePath)
{
    fs::path FilePath = filePath;

    FilePath.replace_extension(".log");

    (void) _chmod(FilePath.string().c_str(), _S_IWRITE | _S_IREAD);

    FILE * fp = nullptr;

    if ((::freopen_s(&fp, FilePath.string().c_str(), "w", stdout) != 0) || (fp == nullptr))
        return;

    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    auto FileSize = fs::file_size(filePath);

    ::printf("\n\"%s\", %" PRIu64 " bytes\n", filePath.string().c_str(), (uint64_t) FileSize);

    ExamineFile(filePath, Arguments);

    ::fflush(fp);

    ::fclose(fp);

    if ((::freopen_s(&fp, "CON", "w", stdout) != 0) || (fp == nullptr))
        return;
}
