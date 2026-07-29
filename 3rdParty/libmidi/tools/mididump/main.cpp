
/** $VER: main.cpp (2026.05.17) P. Stuer **/

#include "pch.h"

void ExamineFile(const fs::path & filePath, const std::map<std::string, std::string> & args);

static void ProcessDirectory(const fs::path & directoryPath);
static void ProcessFile(const fs::path & filePath);

const std::unordered_set<fs::path> Filters = { ".midi2", ".mmd", ".mid", ".g36", ".rmi", ".mxmf", ".xmf", ".mmf", ".tst" };

std::map<std::string, std::string> Arguments;

int main(int argc, const char ** argv)
{
    ::SetConsoleOutputCP(CP_UTF8);

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

        Arguments["FileName"] = argv[i];
    }

    if (!::fs::exists(Arguments["FileName"]))
    {
        ::printf("Failed to access \"%s\": path does not exist.\n", Arguments["FileName"].c_str());

        return -1;
    }

    fs::path Path = fs::canonical(Arguments["FileName"]);

    if (fs::is_directory(Path))
        ProcessDirectory(Path);
    else
        ProcessFile(Path);

    return 0;
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
        if (Filters.contains(Entry.path().extension()))
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
    {
        if ((::freopen_s(&fp, "CON", "w", stdout) != 0) || (fp == nullptr))
            return;

        ::puts("Failed to open log file.");

        return;
    }

    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    auto FileSize = fs::file_size(filePath);

    ::printf("\n\"%s\", %" PRIu64 " bytes\n", filePath.string().c_str(), (uint64_t) FileSize);

    ExamineFile(filePath, Arguments);

    ::fflush(fp);

    ::fclose(fp);

    (void) ::freopen_s(&fp, "CON", "w", stdout);
}
