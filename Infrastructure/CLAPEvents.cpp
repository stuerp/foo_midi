
/** $VER: CLAPEventList.cpp (2025.10.06) P. Stuer **/

#include "pch.h"

#include "CLAPEvents.h"
#include "Log.h"

namespace CLAP
{

/// <summary>
/// Implements an input event list for the CLAP plug-in.
/// </summary>
InputEvents::InputEvents()
{
    this->ctx = this;

    // Returns the number of events in the list
    this->size = [](const clap_input_events_t * self) -> uint32_t
    {
        const auto * This = static_cast<const InputEvents *>(self->ctx);

        return (uint32_t) This->Events.size();
    };

    // Gets an event from the list.
    this->get = [](const clap_input_events_t * self, uint32_t index) -> const clap_event_header *
    {
        const auto * This = static_cast<const InputEvents *>(self->ctx);

        return (index < This->Events.size()) ? This->Events[index] : nullptr;
    };
}

/// <summary>
/// Adds a MIDI message.
/// </summary>
void InputEvents::Add(uint8_t status, uint8_t data1, uint8_t data2, uint16_t portNumber, uint32_t time)
{
    // Clunky in C++ but otherwise the Address Saninitzer goes nuts.
    auto evt = (clap_event_midi_t *) std::malloc(sizeof(clap_event_midi_t));

    if (evt == nullptr)
        return;

    evt->header.size     = sizeof(clap_event_midi_t);
    evt->header.time     = time;
    evt->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    evt->header.type     = CLAP_EVENT_MIDI;
    evt->header.flags    = 0;

    evt->port_index = portNumber;
    evt->data[0] = status;
    evt->data[1] = data1;
    evt->data[2] = data2;

    Events.push_back((clap_event_header_t *) evt);
}

/// <summary>
/// Adds a SysEx message.
/// </summary>
void InputEvents::Add(const uint8_t * data, uint32_t size_, uint16_t portNumber, uint32_t time)
{
    // Clunky in C++ but otherwise the Address Saninitzer goes nuts.
    auto evt = (clap_event_midi_sysex_t *) std::malloc(sizeof(clap_event_midi_sysex_t));

    if (evt == nullptr)
        return;

    evt->header.size     = sizeof(clap_event_midi_sysex_t);
    evt->header.time     = time;
    evt->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    evt->header.type     = CLAP_EVENT_MIDI_SYSEX;
    evt->header.flags    = 0;

    evt->port_index = portNumber;
    evt->buffer = data;
    evt->size   = size_;

    Events.push_back((clap_event_header_t *) evt);
}

OutputEvents::OutputEvents()
{
    this->ctx = this;

    // Receives events from the plug-in. Return false if the event could not be pushed.
    this->try_push = [](const clap_output_events_t * list, const clap_event_header_t * event) -> bool
    {
    #ifdef DEBUG
        switch (event->type)
        {
            case CLAP_EVENT_NOTE_ON:
            case CLAP_EVENT_NOTE_OFF:
            case CLAP_EVENT_NOTE_CHOKE:
            case CLAP_EVENT_NOTE_END:
            case CLAP_EVENT_NOTE_EXPRESSION:        // Represents a note expression. Uses clap_event_note_expression.
            case CLAP_EVENT_PARAM_VALUE:            // PARAM_VALUE sets the parameter's value; uses clap_event_param_value.
            case CLAP_EVENT_PARAM_MOD:              // PARAM_MOD sets the parameter's modulation amount; uses clap_event_param_mod.
            case CLAP_EVENT_PARAM_GESTURE_BEGIN:    // Indicates that the user started or finished adjusting a knob.
            case CLAP_EVENT_PARAM_GESTURE_END:
            case CLAP_EVENT_TRANSPORT:              // Update the transport info; clap_event_transport.
                break;

            case CLAP_EVENT_MIDI:
            {
                const auto * me = (clap_event_midi_t *) event;

                Log.AtTrace().Write(STR_COMPONENT_BASENAME " received MIDI message %02X %02X %02X %02X.", me->data[0], me->data[1], me->data[2], me->data[2]);
                break;
            }

            case CLAP_EVENT_MIDI_SYSEX:
            {
                const auto * sx = (clap_event_midi_sysex_t *) event;

                std::string Buffer; Buffer.resize(((size_t) sx->size * 3) + 1); // 3 characters + terminator

                for (size_t i = 0; i < sx->size; ++i)
                    ::sprintf_s(Buffer.data(), 4, "%02X ", sx->buffer[i]);

                Log.AtTrace().Write(STR_COMPONENT_BASENAME " received MIDI SysEx message %s.", Buffer.c_str());
                break;
            }

            case CLAP_EVENT_MIDI2:
            {
                Log.AtTrace().Write(STR_COMPONENT_BASENAME " received MIDI 2 message.");
                break;
            }

            default:
            {
                Log.AtWarn().Write(STR_COMPONENT_BASENAME, " received unsupported CLAP event %d.", event->type);
            }
        }
    #endif
        return true;
    };
}

}
