
/** $VER: Channels.h (2025.06.21) P. Stuer - Thread-safe wrapper for the MIDI channel configuration **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>

#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>

#include "CriticalSection.h"

extern cfg_var_modern::cfg_blob CfgEnabledChannels;

#pragma warning(disable: 4820) // x bytes padding added after data member
class channels_t
{
public:
    channels_t() noexcept :_Data(0xFF), _Version()
    {
    }

    channels_t(const channels_t &) = delete;
    channels_t & operator=(const channels_t &) = delete;
    channels_t(channels_t &&) = delete;
    channels_t & operator=(channels_t &&) = delete;

    virtual ~channels_t() { }

    void Get(uint16_t * data, size_t size, uint64_t & version)
    {
        critical_section_lock_t Lock(_CriticalSection);

        ::memcpy(data, CfgEnabledChannels.get()->data(), size);
        version = _Version.load();
    }

    void Initialize()
    {
        critical_section_lock_t Lock(_CriticalSection);

        ::memcpy(_Data, CfgEnabledChannels.get()->data(), sizeof(_Data));
        ++_Version;
    }

    void Apply()
    {
        CfgEnabledChannels.set(_Data, sizeof(_Data));
        ++_Version;
    }

    bool HasChanged(uint64_t version) const noexcept
    {
        return (version != _Version.load());
    }

    void Reset() noexcept
    {
        All();
    }

    bool IsEnabled(uint32_t portNumber, uint32_t channelNumber) const
    {
        if (portNumber >= MaxPorts)
            throw std::out_of_range("Port number must be less than 128");

        const int64_t Mask = 1ll << channelNumber;

        return ((_Data[portNumber] & Mask) != 0);
    }

    /// <summary>
    /// Toggles the specified MIDI channel for the specified port.
    /// </summary>
    void Toggle(uint32_t portNumber, uint32_t channelNumber)
    {
        if (portNumber >= MaxPorts)
            throw std::out_of_range("Port number must be less than 128");

        const int64_t Mask = 1ll << channelNumber;

        if (_Data[portNumber] & Mask)
            _Data[portNumber] &= ~Mask;
        else
            _Data[portNumber] |= Mask;
    }

    /// <summary>
    /// Enables all MIDI channels on all ports.
    /// </summary>
    void All() noexcept
    {
        ::memset(_Data, 0xFF, sizeof(_Data));
    }

    /// <summary>
    /// Disables all MIDI channels on all ports.
    /// </summary>
    void None() noexcept
    {
        ::memset(_Data, 0x00, sizeof(_Data));
    }

    /// <summary>
    /// Enables MIDI channel 1 through 10 on all ports.
    /// </summary>
    void OnlyLow() noexcept
    {
        for (size_t i = 0; i < MaxPorts; ++i)
            _Data[i] = 0x03FF;
    }

    /// <summary>
    /// Enables MIDI channel 11 through 16 on all ports.
    /// </summary>
    void OnlyHigh() noexcept
    {
        for (size_t i = 0; i < MaxPorts; ++i)
            _Data[i] = 0xFC00;
    }

    uint64_t Version() const noexcept
    {
        return _Version;
    }

private:
    static const size_t MaxPorts = 128;

    uint16_t _Data[MaxPorts];

    std::atomic<uint64_t> _Version;
    critical_section_t _CriticalSection;
};
#pragma warning(default: 4820)

extern channels_t CfgChannels;
