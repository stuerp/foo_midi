
/** $VER: Encoding.h (2025.09.01) P. Stuer **/

#pragma once

namespace msc
{

std::wstring UTF8ToWide(const char * text, size_t size) noexcept;
std::string WideToUTF8(const wchar_t * wide, size_t size) noexcept;

std::wstring CodePageToWide(uint32_t codePage, const char * text, size_t size) noexcept;
std::string CodePageToUTF8(uint32_t codePage, const char * text, size_t size) noexcept;

std::wstring TextToWide(const char * text, size_t size = 0) noexcept;
std::string TextToUTF8(const char * text, size_t size = 0) noexcept;

bool GetCodePageFromEncoding(const std::string & encoding, uint32_t & codePage) noexcept;

inline std::wstring UTF8ToWide(const char * text, size_t size) noexcept
{
    return CodePageToWide(CP_UTF8, text, size);
}

inline std::string WideToUTF8(const std::wstring & text) noexcept
{
    return WideToUTF8(text.c_str(), text.length());
}

inline std::wstring CodePageToWide(uint32_t codePage, const std::string & text) noexcept
{
    return CodePageToWide(codePage, text.c_str(), text.length());
}

inline std::wstring UTF8ToWide(const std::string & text) noexcept
{
    return CodePageToWide(CP_UTF8, text.c_str(), text.length());
}

std::string FormatText(const char * format, ...) noexcept;
std::wstring FormatText(const wchar_t * format, ...) noexcept;

bool IsEUCJP(const char * text, size_t size) noexcept;
bool IsShiftJIS(const char * text, size_t size) noexcept;
bool IsUTF8(const char * text, size_t size) noexcept;
bool IsASCII(const char * text, size_t size) noexcept;

}
