
/** $VER: ContextMenu.h (2023.01.04) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/contextmenu.h>

#include <libmidi/MIDIProcessor.h>

#include "Configuration.h"

#include "MIDIPreset.h"
#include "MIDIPresetFilter.h"
#include "MIDISysExDumps.h"
#include "MIDISysExFilter.h"

class ContextMenu : public contextmenu_item_simple
{
public:
    ContextMenu() noexcept { };

    ContextMenu(const ContextMenu&) = delete;
    ContextMenu(const ContextMenu&&) = delete;
    ContextMenu& operator=(const ContextMenu&) = delete;
    ContextMenu& operator=(ContextMenu&&) = delete;

    virtual ~ContextMenu() { };

#pragma region("contextmenu_item_simple")
    virtual unsigned get_num_items() noexcept;
    virtual void get_item_name(unsigned n, pfc::string_base & itemName) noexcept;

    virtual bool get_item_description(unsigned n, pfc::string_base & out)
    {
        if (n > 4)
            ::uBugCheck();

        static const char * descriptions[5] =
        {
            "Converts the selected files into General MIDI files in the same path as the source.",
            "Applies the current synthesizer setup to this track for future playback.",
            "Removes a saved synthesizer preset from this track.",
            "Assigns the selected SysEx dumps to the selected MIDI files.",
            "Clears all assigned SysEx dumps from the selected MIDI files."
        };

        out = descriptions[n];

        return true;
    }

    virtual GUID get_item_guid(unsigned n) noexcept
    {
        static const GUID guids[5] =
        {
            { 0x70985c72, 0xe77e, 0x4bbb, { 0xbf, 0x11, 0x3c, 0x90, 0x2b, 0x27, 0x39, 0x9d } },
            { 0xeb3f3ab4, 0x60b3, 0x4579, { 0x9f, 0xf8, 0x38, 0xda, 0xc0, 0x91, 0x2c, 0x82 } },
            { 0x5bcb6efe, 0x2eb5, 0x4331, { 0xb9, 0xc1, 0x92, 0x4b, 0x77, 0xba, 0xcc, 0x10 } },
            { 0xd0e4a166, 0x10c, 0x41f0, { 0xad, 0x5a, 0x51, 0x84, 0x44, 0xa3, 0x92, 0x9c } },
            { 0x2aa8c082, 0x5d84, 0x4982, { 0xb4, 0x5d, 0xde, 0x51, 0xcb, 0x75, 0xff, 0xf2 } }
        };

        if (n > 4)
            ::uBugCheck();

        return guids[n];
    }

    virtual bool context_get_display(unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out, unsigned &, const GUID &);

    virtual void context_command(unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, const GUID &);
#pragma endregion

#pragma region("contextmenu_item_v2")
    GUID get_parent()
    {
        return contextmenu_groups::utilities;
    }
#pragma endregion
};
