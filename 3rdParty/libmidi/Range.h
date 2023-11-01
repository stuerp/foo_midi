
/** $VER: Range.h (2023.11.01) P. Stuer **/

#pragma once

#include <stdint.h>

/// <summary>
/// Represents a range.
/// </summary>
class Range
{
public:
    Range() noexcept { Clear(); }

    Range(const Range &) = delete;
    Range(Range && other) noexcept { Set(other._Begin, other._End); }
    Range & operator=(const Range &) = delete;
    Range & operator=(Range && other) noexcept { Set(other._Begin, other._End); return *this; }

    virtual ~Range() noexcept { }

    uint32_t Begin() const noexcept { return _Begin; }
    uint32_t End() const noexcept { return _End; }

    void SetBegin(uint32_t begin) noexcept { _Begin = begin; }
    void SetEnd(uint32_t end) noexcept { _End = end; }
    void Set(uint32_t begin, uint32_t end) noexcept { _Begin = begin; _End = end; }

    void Clear() noexcept { Set(~0u, ~0u); }

    uint32_t Size() const noexcept { return _End - _Begin; }

    bool HasBegin() const noexcept { return _Begin != ~0u; }
    bool HasEnd() const noexcept { return _End != ~0u; }

    bool IsSet() const noexcept { return  HasBegin() && HasEnd(); }
    bool IsEmpty() const noexcept { return !IsSet(); }

private:
    uint32_t _Begin;
    uint32_t _End;
};
