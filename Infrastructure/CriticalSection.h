
/** $VER: CriticalSection.h (2025.06.21) P. Stuer - RAII wrapper for a Windows CRITICAL_SECTION. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class critical_section_t
{
public:
    critical_section_t() noexcept
    {
        ::InitializeCriticalSection(&_cs);
    }

    ~critical_section_t() noexcept
    {
        ::DeleteCriticalSection(&_cs);
    }

    _Acquires_lock_(_cs)
    void Enter() noexcept
    {
        ::EnterCriticalSection(&_cs);
    }

    _Releases_lock_(_cs)
    void Leave() noexcept
    {
        ::LeaveCriticalSection(&_cs);
    }

    CRITICAL_SECTION * Handle() noexcept
    {
        return &_cs;
    }

private:
    CRITICAL_SECTION _cs;
};

class critical_section_lock_t
{
public:
    explicit critical_section_lock_t(critical_section_t & cs) noexcept : _cs(cs)
    {
        _cs.Enter();
    }

    ~critical_section_lock_t() noexcept
    {
        _cs.Leave();
    }

    // Non-copyable
    critical_section_lock_t(const critical_section_lock_t &) = delete;
    critical_section_lock_t & operator=(const critical_section_lock_t &) = delete;

private:
    critical_section_t & _cs;
};
