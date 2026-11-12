
/** $VER: CriticalSection.h (2025.09.14) P. Stuer **/

#pragma once

namespace msc
{

class critical_section_t
{
public:
    critical_section_t() noexcept
    {
        ::InitializeCriticalSection(&_cs);
    }

    critical_section_t(const critical_section_t &) = delete;
    critical_section_t & operator=(const critical_section_t &) = delete;
    critical_section_t(critical_section_t &&) = delete;
    critical_section_t & operator=(critical_section_t &&) = delete;

    ~critical_section_t() noexcept
    {
        ::DeleteCriticalSection(&_cs);
    }

    void Enter() noexcept
    {
        ::EnterCriticalSection(&_cs);
    }

    bool TryEnter() noexcept
    {
        return ::TryEnterCriticalSection(&_cs) != 0;
    }

    void Leave() noexcept
    {
        ::LeaveCriticalSection(&_cs);
    }

    operator CRITICAL_SECTION *()
    {
        return &_cs;
    }

private:
    CRITICAL_SECTION _cs;
};

}
