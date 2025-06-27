
/** $VER: CLAPEventList.h (2025.06.27) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <vector>
#include <memory>

#include <clap/clap.h>

namespace CLAP
{

/// <summary>
/// Implements an event list in the format expected by the CLAP interface.
/// </summary>
struct EventList : clap_input_events
{
public:
    EventList()
    {
        this->ctx = this;

        this->size = [](const clap_input_events * self) -> uint32_t
        {
            const auto * This = static_cast<const EventList *>(self->ctx);

            return (uint32_t) This->Events.size();
        };

        this->get = [](const clap_input_events * self, uint32_t index) -> const clap_event_header *
        {
            const auto * This = static_cast<const EventList *>(self->ctx);

            return (index < This->Events.size()) ? This->Events[index].get() : nullptr;
        };
    }

    void Add(uint8_t status, uint8_t data1, uint8_t data2, uint32_t time = 0)
    {
        auto evt = std::make_unique<clap_event_midi_t>();

        evt->header.size     = sizeof(evt);
        evt->header.time     = time;
        evt->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        evt->header.type     = CLAP_EVENT_MIDI;
        evt->header.flags    = 0;

        evt->port_index = 0;
        evt->data[0] = status;
        evt->data[1] = data1;
        evt->data[2] = data2;

        Events.push_back(std::unique_ptr<clap_event_header>((clap_event_header *) evt.release()));
    }

    void Add(const uint8_t * data, size_t size_, uint32_t portNumber, uint32_t time = 0)
    {
        auto evt = std::make_unique<clap_event_midi_sysex>();

        evt->header.size     = sizeof(evt);
        evt->header.time     = time;
        evt->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        evt->header.type     = CLAP_EVENT_MIDI_SYSEX;
        evt->header.flags    = 0;

        evt->port_index = 0;
        evt->buffer = data;
        evt->size   = (uint32_t) size_;

        Events.push_back(std::unique_ptr<clap_event_header>((clap_event_header *) evt.release()));
    }

    void Clear() noexcept
    {
        Events.clear();
    }

public:
    std::vector<std::unique_ptr<clap_event_header>> Events;
};

}
