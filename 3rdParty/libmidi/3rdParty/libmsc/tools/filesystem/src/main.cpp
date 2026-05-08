
/** $VER: main.cpp (2025.09.01) P. Stuer - A test bed for Windows UTF-8 development and ghc::filesystem **/

#include "pch.h"

#include <format>

int __cdecl wmain(int argc, wchar_t * argv[])
{
    ::SetConsoleOutputCP(CP_UTF8);

    ::printf("ANSI code page: %d\nOEM code page: %d\nConsole output code page: %d\n", ::GetACP(), ::GetOEMCP(), ::GetConsoleOutputCP());

    fs::path FilePath = R"(D:\Music\Game\Ã¥Â¤Â§Ã¥Â¯Å’Ã§Â¿Â 4\21. Ã¦Â¬Â¢Ã¤Â¹ÂÃ¥Â¥Â½Ã¦â€”Â¶Ã¥â€¦â€°.mid)";

    ::printf("%s\n", FilePath.string().c_str());

    FilePath.replace_extension(".sf2");

    ::printf("%s\n", FilePath.string().c_str());

    ::puts("Ã°Å¸Ëœâ€š Ã¢Ë†Æ’y Ã¢Ë†â‚¬x Ã‚Â¬(x Ã¢â€°Âº y)");

    try
    {
        FilePath = R"(C:\Windows\notepad.exe)";

        msc::handle_t FileHandle = ::CreateFileA(FilePath.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0);

        if (FileHandle.IsValid())
            throw msc::win32_exception(std::format("Failed to open file \"{}\"", FilePath.string()));
    }
    catch (const msc::win32_exception & e)
    {
        std::fprintf(stderr, "%s\n", e.what());
    }
}
