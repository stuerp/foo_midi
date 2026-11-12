
/** $VER: Win32.h (2026.07.22) P. Stuer **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <SDKDDKVer.h>
#include <windows.h>

#include <guiddef.h>

#include <vector>
#include <utility> // std::pair

namespace msc
{
    struct rect_t
    {
        RECT Value {};

        rect_t() = default;

        explicit rect_t(const RECT & other) noexcept : Value(other) { }

        rect_t(LONG left, LONG top, LONG right, LONG bottom) noexcept : Value{ left, top, right, bottom } { }

        rect_t & operator=(const RECT & other) noexcept
        {
            Value = other;

            return *this;
        }

        operator RECT &() noexcept { return Value; }
        operator const RECT &() const noexcept { return Value; }

        RECT * operator &() noexcept { return &Value; }
        const RECT * operator &() const noexcept { return &Value; }

        bool operator ==(const rect_t & other) const noexcept
        {
            return (Value.left == other.Value.left) && (Value.top == other.Value.top) && (Value.right == other.Value.right) && (Value.bottom == other.Value.bottom);
        }

        bool operator ==(const RECT & other) const noexcept
        {
            return (Value.left == other.left) && (Value.top == other.top) && (Value.right == other.right) && (Value.bottom == other.bottom);
        }

        bool operator !=(const rect_t & other) const noexcept { return !(*this == other); }
        bool operator !=(const RECT & other) const noexcept { return !(*this == other); }

        auto operator <=>(const rect_t & other) const noexcept
        {
            if (auto Result = (Value.left <=> other.Value.left); Result != 0)
                return Result;

            if (auto Result = (Value.top <=> other.Value.top); Result != 0)
                return Result;

            if (auto Result = (Value.right <=> other.Value.right); Result != 0)
                return Result;

            return (Value.bottom <=> other.Value.bottom);
        }

        LONG Width() const noexcept
        {
            return (Value.right - Value.left);
        }

        LONG Height() const noexcept
        {
            return (Value.bottom - Value.top);
        }

        bool IsEmpty() const noexcept
        {
            return (::IsRectEmpty(&Value) != 0);
        }

        void Offset(LONG dx, LONG dy) noexcept
        {
            ::OffsetRect(&Value, dx, dy);
        }

        void Inflate(LONG dx, LONG dy) noexcept
        {
            ::InflateRect(&Value, dx, dy);
        }

        bool Contains(POINT pt) const noexcept
        {
            return (::PtInRect(&Value, pt) != 0);
        }
    };

    std::wstring GUIDToWide(const GUID & id) noexcept;

    inline std::string GUIDToUTF8(const GUID & id) noexcept
    {
        return WideToUTF8(GUIDToWide(id));
    }

    GUID WideToGUID(const std::wstring & text) noexcept;

    inline GUID UTF8ToGUID(const std::string & text) noexcept
    {
        return WideToGUID(UTF8ToWide(text));
    }

    std::wstring GetFullPath(const std::wstring & fileName) noexcept;
    bool OpenFileDialog(HWND hParent, const std::vector<std::pair<std::wstring_view, std::wstring_view>> & filters, uint32_t selectedExtension, const std::wstring_view & defaultExtension, const std::wstring_view & title, const std::wstring_view & directoryPath, std::wstring & filePath) noexcept;
}

// std::hash specialization
namespace std
{
    template<>
    struct hash<msc::rect_t>
    {
        size_t operator()(const msc::rect_t & r) const noexcept
        {
            const size_t h1 = std::hash<LONG>()(r.Value.left);
            const size_t h2 = std::hash<LONG>()(r.Value.top);
            const size_t h3 = std::hash<LONG>()(r.Value.right);
            const size_t h4 = std::hash<LONG>()(r.Value.bottom);

            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
}
