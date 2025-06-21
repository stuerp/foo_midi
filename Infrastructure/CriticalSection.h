
/** $VER: CriticalSection.h (2025.06.21) P. Stuer - RAII wrapper for a Windows CRITICAL_SECTION. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class CriticalSection
{
public:
    CriticalSection() noexcept
    {
        ::InitializeCriticalSection(&_cs);
    }

    ~CriticalSection() noexcept
    {
        ::DeleteCriticalSection(&_cs);
    }

    void Enter() noexcept
    {
        ::EnterCriticalSection(&_cs);
    }

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

class CriticalSectionLock
{
public:
    explicit CriticalSectionLock(CriticalSection & cs) noexcept : _cs(cs)
    {
        _cs.Enter();
    }

    ~CriticalSectionLock() noexcept
    {
        _cs.Leave();
    }

    // Non-copyable
    CriticalSectionLock(const CriticalSectionLock &) = delete;
    CriticalSectionLock & operator=(const CriticalSectionLock &) = delete;

private:
    CriticalSection & _cs;
};
