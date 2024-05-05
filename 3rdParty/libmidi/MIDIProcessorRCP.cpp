
/** $VER: MIDIProcessorRCP.cpp (2024.05.05) Recomposer Format (http://www.vgmpf.com/Wiki/index.php?title=GMF) **/

#include "MIDIProcessor.h"

bool MIDIProcessor::IsRCP(std::vector<uint8_t> const & data, const char * fileExtension)
{
    if (fileExtension == nullptr)
        return false;

    if (data.size() < 28)
        return false;

    if (::strncmp((const char *) data.data(), "RCM-PC98V2.0(C)COME ON MUSIC", 28) == 0)
    {
        if (::_stricmp(fileExtension, "rcp") == 0)
            return true;

        if (::_stricmp(fileExtension, "r36") == 0)
            return true;

        return false;
    }

    if (data.size() < 31)
        return false;

    if (::strncmp((const char *) data.data(), "COME ON MUSIC RECOMPOSER RCP3.0", 31) == 0)
    {
        if (::_stricmp(fileExtension, "g18") == 0)
            return true;

        if (::_stricmp(fileExtension, "g36") == 0)
            return true;

        return false;
    }

    return false;
}

#include <windows.h>

bool MIDIProcessor::ProcessRCP(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    const uint8_t * Data = data.data();
    size_t Offset = 32;

    char Title[64];

    ::memcpy(Title, Data + Offset, _countof(Title));
    Offset += _countof(Title);

    for (size_t n = _countof(Title) - 1; n >= 0; --n)
    {
        if (Title[n] == ' ')
            Title[n] = '\0';
        else
        if (Title[n] != '\0')
            break;
    }

    WCHAR Wide[256];

    int Result = ::MultiByteToWideChar(932, 0, Title, (int) _countof(Title), Wide, (int) _countof(Wide)); // Shift-JIS to Unicode
    Wide[Result] = 0;
    ::OutputDebugStringW(Wide);
    ::OutputDebugStringW(L"\n");

    for (int i = 0; i < 12; ++i)
    {
        char Memo[28];

        ::memcpy(Memo, Data + Offset, _countof(Memo));
        Offset += _countof(Memo);

        for (size_t n = _countof(Memo) - 1; n >= 0; --n)
        {
            if (Memo[n] == ' ')
                Memo[n] = '\0';
            else
            if (Memo[n] != '\0')
                break;
        }

        int Result = ::MultiByteToWideChar(932, 0, Memo, (int) _countof(Memo), Wide, (int) _countof(Wide)); // Shift-JIS to Unicode
        Wide[Result] = 0;
    ::OutputDebugStringW(Wide);
    ::OutputDebugStringW(L"\n");
    }

    return true;
}
