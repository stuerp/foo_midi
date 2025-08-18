
/** $VER: Encoding.h (2024.08.17) P. Stuer **/

#pragma once

#include "pch.h"

namespace riff
{

std::string WideToUTF8(const wchar_t * wide, size_t size);
std::wstring TextToWide(const char * text, size_t size = 0);

std::wstring CodePageToWide(uint32_t codePage, const char * text, size_t size);
std::string CodePageToUTF8(uint32_t codePage, const char * text, size_t size);

std::string TextToUTF8(const char * text, size_t size = 0);

bool GetCodePageFromEncoding(const std::string & encoding, uint32_t & codePage) noexcept;

inline std::wstring UTF8ToWide(const char * text, size_t size)
{
    return CodePageToWide(CP_UTF8, text, size);
}

inline std::string WideToUTF8(const std::wstring & text)
{
    return WideToUTF8(text.c_str(), text.length());
}

inline std::wstring CodePageToWide(uint32_t codePage, const std::string & text)
{
    return CodePageToWide(codePage, text.c_str(), text.length());
}

inline std::wstring UTF8ToWide(const std::string & text)
{
    return CodePageToWide(CP_UTF8, text.c_str(), text.length());
}

std::string FormatText(const char * format, ...);
std::wstring FormatText(const wchar_t * format, ...);

bool IsEUCJP(const char * text, size_t size);
bool IsShiftJIS(const char * text, size_t size);
bool IsUTF8(const char * text, size_t size);
bool IsASCII(const char * text, size_t size);

}
