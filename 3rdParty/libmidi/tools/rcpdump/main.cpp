
/** $VER: main.cpp (2025.03.21) P. Stuer **/

#include "pch.h"

static void ProcessDirectory(const WCHAR * directoryPath, const WCHAR * searchPattern);
static void ProcessFile(const WCHAR * filePath, uint64_t fileSize);

int rcpmain(int argc, wchar_t * argv[]);

const WCHAR * Filters[] = { L".rcp", L".r36", L".g18", L".g36", L".cm6", L".gsd" };

int wmain(int argc, wchar_t * argv[])
{
    if (argc < 2)
        return -1;

    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    if (!::PathFileExistsW(argv[1]))
    {
        ::printf("Failed to access \"%s\": path does not exist.\n", msc::WideToUTF8(argv[1]).c_str());
        return -1;
    }

    WCHAR DirectoryPath[MAX_PATH];

    if (::GetFullPathNameW(argv[1], _countof(DirectoryPath), DirectoryPath, nullptr) == 0)
    {
        ::printf("Failed to expand \"%s\": Error %u.\n", msc::WideToUTF8(argv[1]).c_str(), (uint32_t) ::GetLastError());
        return -1;
    }

    if (!::PathIsDirectoryW(argv[1]))
    {
        ::PathCchRemoveFileSpec(DirectoryPath, _countof(DirectoryPath));

        ProcessDirectory(DirectoryPath, ::PathFindFileNameW(argv[1]));
    }
    else
        ProcessDirectory(DirectoryPath, L"*.*");

    return 0;
}

static void ProcessDirectory(const WCHAR * directoryPath, const WCHAR * searchPattern)
{
    ::printf("\"%s\"\n", msc::WideToUTF8(directoryPath).c_str());

    WCHAR PathName[MAX_PATH];

    if (!SUCCEEDED(::PathCchCombineEx(PathName, _countof(PathName), directoryPath, searchPattern, PATHCCH_ALLOW_LONG_PATHS)))
        return;

    WIN32_FIND_DATA fd = {};

    HANDLE hFind = ::FindFirstFileW(PathName, &fd);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    BOOL Success = TRUE;

    if (::wcscmp(fd.cFileName, L".") == 0)
    {
        Success = ::FindNextFileW(hFind, &fd);

        if (Success && ::wcscmp(fd.cFileName, L"..") == 0)
            Success = ::FindNextFileW(hFind, &fd);
    }

    while (Success)
    {
        if (SUCCEEDED(::PathCchCombineEx(PathName, _countof(PathName), directoryPath, fd.cFileName, PATHCCH_ALLOW_LONG_PATHS)))
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                ProcessDirectory(PathName, searchPattern);
            else
            {
                uint64_t FileSize = (((uint64_t) fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;

                ProcessFile(PathName, FileSize);
            }
        }

        Success = ::FindNextFileW(hFind, &fd);
    }

    ::FindClose(hFind);
}

static void ProcessFile(const WCHAR * filePath, uint64_t fileSize)
{
    ::printf("\n\"%s\", %" PRIu64 " bytes\n", msc::WideToUTF8(filePath).c_str(), fileSize);

    const WCHAR * FileExtension;

    HRESULT hr = ::PathCchFindExtension(filePath, ::wcslen(filePath) + 1, &FileExtension);

    if (SUCCEEDED(hr))
    {
        for (const auto & Filter : Filters)
        {
            if (::_wcsicmp(FileExtension, Filter) == 0)
            {
                WCHAR DstFilePath[MAX_PATH];

                ::wcscpy_s(DstFilePath, _countof(DstFilePath), filePath);

                ::PathCchRenameExtension(DstFilePath, _countof(DstFilePath), L"mid");

                const WCHAR * DstFileName = ::PathFindFileNameW(DstFilePath);

                {
                    const wchar_t * argv[] = { L"rcp2mid.exe", /*L"-KeepMutedChannels",*/ filePath, DstFileName /* L"DEADBEEF.mid" */ };

                    rcpmain(_countof(argv), (wchar_t **) argv);
                }

                break;
            }
        }
    }
}
