#pragma once

#include <algorithm>
#include <cstdint>

// The VST bridge uses this same structure in a zero-initialized file mapping.
struct PlaybackDialogPosition
{
    alignas(8) volatile LONG64 Coordinates;
    volatile LONG Saved;

    void Save(HWND window) noexcept
    {
        RECT Bounds;
        if (!::IsWindowVisible(window) || ::IsIconic(window) || !::GetWindowRect(window, &Bounds))
            return;

        const uint64_t Packed = (static_cast<uint64_t>(static_cast<uint32_t>(Bounds.left)) << 32) |
            static_cast<uint32_t>(Bounds.top);
        ::InterlockedExchange64(&Coordinates, static_cast<LONG64>(Packed));
        ::InterlockedExchange(&Saved, TRUE);
    }

    bool Restore(HWND window) noexcept
    {
        if (!::InterlockedCompareExchange(&Saved, 0, 0))
            return false;

        const auto Packed = static_cast<uint64_t>(::InterlockedCompareExchange64(&Coordinates, 0, 0));
        POINT Position = { static_cast<LONG>(Packed >> 32), static_cast<LONG>(Packed & UINT32_MAX) };

        RECT Bounds;
        MONITORINFO Monitor = { sizeof(Monitor) };
        if (::GetWindowRect(window, &Bounds) &&
            ::GetMonitorInfoW(::MonitorFromPoint(Position, MONITOR_DEFAULTTONEAREST), &Monitor))
        {
            // Keep the dialog on screen when its size or the monitor layout has changed.
            const RECT & Work = Monitor.rcWork;
            const LONG Right = Work.right - (Bounds.right - Bounds.left);
            const LONG Bottom = Work.bottom - (Bounds.bottom - Bounds.top);
            Position.x = (std::max)(Work.left, (std::min)(Position.x, Right));
            Position.y = (std::max)(Work.top, (std::min)(Position.y, Bottom));
        }

        return ::SetWindowPos(window, nullptr, Position.x, Position.y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
    }
};
