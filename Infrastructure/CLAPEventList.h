
/** $VER: CLAPEventList.h (2025.06.28) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <vector>
#include <memory>

#include <clap/clap.h>

#include "Resource.h"
#include "Encoding.h"

namespace CLAP
{

/// <summary>
/// Implements an input event list for the CLAP plug-in.
/// </summary>
struct InputEvents : clap_input_events
{
public:
    InputEvents();

    void Add(uint8_t status, uint8_t data1, uint8_t data2, uint32_t time = 0);
    void Add(const uint8_t * data, size_t size_, uint32_t portNumber, uint32_t time = 0);

    void Clear() noexcept
    {
        Events.clear();
    }

public:
    std::vector<std::unique_ptr<clap_event_header>> Events;
};

/// <summary>
/// Implements an output event list for the CLAP plug-in.
/// </summary>
struct OutputEvents : clap_output_events
{
public:
    OutputEvents();

public:
    std::vector<std::unique_ptr<clap_event_header>> Events;
};

}
