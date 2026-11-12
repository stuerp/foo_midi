
/** $VER: RAII.h (2026.07.31) P. Stuer - RAII wrappers **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>
#include <CommCtrl.h>

#include <string>
#include <filesystem>
#include <utility> // std::exchange

namespace fs = std::filesystem;

#include "CriticalSection.h"

#pragma once

namespace msc
{

/// <summary>
/// Implements a RAII wrapper for HANDLE. 
/// </summary>
class handle_t
{
public:
    explicit handle_t(HANDLE handle) noexcept : _Handle(handle) { }

    // Move-only type
    handle_t(const handle_t & other) = delete;
    handle_t & operator=(const handle_t & other) = delete;

    handle_t(handle_t && other) noexcept : _Handle(other._Handle)
    {
        other._Handle = INVALID_HANDLE_VALUE;
    }

    handle_t & operator=(handle_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _Handle = other._Handle;
            other._Handle = INVALID_HANDLE_VALUE;
        }

        return *this;
    }

    ~handle_t() noexcept
    {
        Reset();
    }

    HANDLE Get() const noexcept
    {
        return _Handle;
    }

    operator HANDLE() const noexcept
    {
        return _Handle;
    }

    explicit operator bool() const noexcept
    {
        return _Handle != INVALID_HANDLE_VALUE;
    }

    bool IsValid() const noexcept
    {
        return _Handle != INVALID_HANDLE_VALUE;
    }

    void Reset() noexcept
    {
        if (_Handle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(_Handle);
            _Handle = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE _Handle = INVALID_HANDLE_VALUE;
};

/// <summary>
/// Implements a synchronization lock. 
/// </summary>
class lock_t
{
public:
    lock_t(critical_section_t & cs) : _cs(cs)
    {
        _cs.Enter();
    }

    lock_t(const lock_t &) = delete;
    lock_t & operator=(const lock_t &) = delete;
    lock_t(lock_t &&) = delete;
    lock_t & operator=(lock_t &&) = delete;

    ~lock_t()
    {
        _cs.Leave();
    }
     
private:
    critical_section_t & _cs;
};

/// <summary>
/// Implements a RAII wrapper for HMODULE.
/// </summary>
class module_t
{
public:
    explicit module_t(const std::wstring filePath) noexcept : _hModule(::LoadLibraryExW(filePath.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE)) { }

    explicit module_t(HMODULE hModule) noexcept : _hModule(hModule) { }

    // Move-only type
    module_t(const module_t & other) = delete;
    module_t & operator=(const module_t & other) = delete;

    module_t(module_t && other) noexcept : _hModule(other.Release()) { }

    module_t & operator=(module_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _hModule = other.Release();
        }

        return *this;
    }

    ~module_t()
    {
        Reset();
    }

    HMODULE Get() const noexcept
    {
        return _hModule;
    }

    operator HMODULE() const noexcept
    {
        return _hModule;
    }

    explicit operator bool() const noexcept
    {
        return _hModule != nullptr;
    }

    HMODULE Release() noexcept
    {
        return std::exchange(_hModule, nullptr);
    }

    bool Reset() noexcept
    {
        if (_hModule == nullptr)
            return true;

        const BOOL Result = ::FreeLibrary(_hModule);

        _hModule = nullptr;

        return Result != FALSE;
    }

private:
    HMODULE _hModule = nullptr;
};

/// <summary>
/// Implements a RAII wrapper for HIMAGELIST.
/// </summary>
class imagelist_t
{
public:
    imagelist_t() noexcept : _hImageList(nullptr) { }
    imagelist_t(HIMAGELIST hImageList) noexcept : _hImageList(hImageList) { }

    // Move-only type
    imagelist_t(const imagelist_t & other) = delete;
    imagelist_t & operator=(const imagelist_t & other) = delete;

    imagelist_t(imagelist_t && other) noexcept : _hImageList(other.Release()) { }

    imagelist_t & operator =(HIMAGELIST hImageList) noexcept
    {
        Reset();

        _hImageList = hImageList;

        return *this;
    }

    imagelist_t & operator =(imagelist_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _hImageList = other.Release();
        }

        return *this;
    }

    ~imagelist_t()
    {
        Reset();
    }

    HIMAGELIST Get() const noexcept
    {
        return _hImageList;
    }

    operator HIMAGELIST() const noexcept
    {
        return _hImageList;
    }

    explicit operator bool() const noexcept
    {
        return _hImageList != nullptr;
    }

    HIMAGELIST Release() noexcept
    {
        return std::exchange(_hImageList, nullptr);
    }

    bool Reset() noexcept
    {
        if (_hImageList == nullptr)
            return true;

        const BOOL Result = ::ImageList_Destroy(_hImageList);

        _hImageList = nullptr;

        return Result != FALSE;
    }

private:
    HIMAGELIST _hImageList = nullptr;
};

/// <summary>
/// Implements a RAII wrapper for HICON.
/// </summary>
class icon_t
{
public:
    icon_t() noexcept : _hIcon(nullptr) { }
    icon_t(HICON hIcon) noexcept : _hIcon(hIcon) { } // Take ownership

    // Move-only type
    icon_t(const icon_t & other) = delete;
    icon_t & operator=(const icon_t & other) = delete;

    icon_t(icon_t && other) noexcept : _hIcon(other.Release()) { }

    icon_t & operator =(HICON hIcon) noexcept
    {
        Reset();

        _hIcon = hIcon;

        return *this;
    }

    icon_t & operator =(icon_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _hIcon = other.Release();
        }

        return *this;
    }

    ~icon_t()
    {
        Reset();
    }

    HICON Get() const noexcept
    {
        return _hIcon;
    }

    operator HICON() const noexcept
    {
        return _hIcon;
    }

    explicit operator bool() const noexcept
    {
        return _hIcon != nullptr;
    }

    HICON Release() noexcept
    {
        return std::exchange(_hIcon, nullptr);
    }

    bool Reset() noexcept
    {
        if (_hIcon == nullptr)
            return true;

        const BOOL Result = ::DestroyIcon(_hIcon);

        _hIcon = nullptr;

        return Result != FALSE;
    }

private:
    HICON _hIcon = nullptr;
};

/// <summary>
/// Implements a RAII wrapper for HBRUSH.
/// </summary>
class brush_t
{
public:
    explicit brush_t() : _hBrush(nullptr) { }

    explicit brush_t(COLORREF color) : _hBrush(::CreateSolidBrush(color))
    {
        if (_hBrush == nullptr)
            throw std::runtime_error("CreateSolidBrush() failed");
    }

    brush_t(int style, COLORREF color) : _hBrush(::CreateHatchBrush(style, color))
    {
        if (_hBrush == nullptr)
            throw std::runtime_error("CreateHatchBrush() failed");
    }

    explicit brush_t(HBRUSH hBrush) noexcept : _hBrush(hBrush) { } // Take ownership

    // Move-only type
    brush_t(const brush_t & other) = delete;
    brush_t & operator=(const brush_t & other) = delete;

    brush_t(brush_t && other) noexcept : _hBrush(std::exchange(other._hBrush, nullptr)) { }
    brush_t & operator =(brush_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _hBrush = std::exchange(other._hBrush, nullptr);
        }

        return *this;
    }

    ~brush_t() noexcept
    {
        Reset();
    }

    HBRUSH Get() const noexcept
    {
        return _hBrush;
    }

    operator HBRUSH() const noexcept
    {
        return _hBrush;
    }

    operator HGDIOBJ() const noexcept
    {
        return (HGDIOBJ) _hBrush;
    }

    explicit operator bool() const noexcept
    {
        return _hBrush != nullptr;
    }

    bool Reset() noexcept
    {
        if (_hBrush == nullptr)
            return true;

        const BOOL Result = ::DeleteObject(_hBrush);

        _hBrush = nullptr;

        return Result != FALSE;
    }

private:
    HBRUSH _hBrush = nullptr;
};

inline HGDIOBJ SelectObject(HDC hDC, const brush_t & brush) noexcept
{
    return ::SelectObject(hDC, brush.Get());
}

/// <summary>
/// Implements a RAII wrapper for HPEN.
/// </summary>
class pen_t
{
public:
    explicit pen_t() : _hPen(nullptr) { }

    explicit pen_t(int width, COLORREF color) : _hPen(::CreatePen(PS_SOLID, width, color))
    {
        if (_hPen == nullptr)
            throw std::runtime_error("CreatePen() failed");
    }

    explicit pen_t(HPEN hPen) noexcept : _hPen(hPen) { } // Take ownership

    // Move-only type
    pen_t(const pen_t & other) = delete;
    pen_t & operator=(const pen_t & other) = delete;

    pen_t(pen_t && other) noexcept : _hPen(std::exchange(other._hPen, nullptr)) { }
    pen_t & operator =(pen_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _hPen = std::exchange(other._hPen, nullptr);
        }

        return *this;
    }

    ~pen_t() noexcept
    {
        Reset();
    }

    HPEN Get() const noexcept
    {
        return _hPen;
    }

    operator HPEN() const noexcept
    {
        return _hPen;
    }

    operator HGDIOBJ() const noexcept
    {
        return (HGDIOBJ) _hPen;
    }

    explicit operator bool() const noexcept
    {
        return _hPen != nullptr;
    }

    bool Reset() noexcept
    {
        if (_hPen == nullptr)
            return true;

        const BOOL Result = ::DeleteObject(_hPen);

        _hPen = nullptr;

        return Result != FALSE;
    }

private:
    HPEN _hPen = nullptr;
};

inline HGDIOBJ SelectObject(HDC hDC, const pen_t & pen) noexcept
{
    return ::SelectObject(hDC, pen.Get());
}

}
