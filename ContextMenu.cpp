
/** $VER: ContextMenu.cpp (2022.12.30) **/

#include <sdk/foobar2000-lite.h>
#include <sdk/contextmenu.h>

#include <midi_processing/midi_processor.h>

#include "Configuration.h"

#include "MIDIPreset.h"
#include "MIDIPresetFilter.h"
#include "MIDISysExDumps.h"
#include "MIDISysExFilter.h"

static const char * ItemNames[5] =
{
    "Convert to General MIDI",
    "Save synthesizer preset",
    "Remove synthesizer preset",
    "Assign SysEx dumps",
    "Clear SysEx dumps"
};

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
    virtual unsigned get_num_items() noexcept
    {
        return _countof(ItemNames);
    }

    virtual void get_item_name(unsigned n, pfc::string_base & itemName) noexcept
    {
        if (n > 4)
            ::uBugCheck();

        itemName = ItemNames[n];
    }

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

#pragma endregion


#pragma region("contextmenu_item_v2")
    GUID get_parent()
    {
        return contextmenu_groups::utilities;
    }
#pragma endregion

    virtual bool context_get_display(unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out, unsigned &, const GUID &)
    {
        if (n > 4)
            uBugCheck();

        size_t j, matches, matches_syx;

        size_t i = data.get_count();

        for (matches = 0, matches_syx = 0, j = 0; j < i; ++j)
        {
            const playable_location& Location = data.get_item(j)->get_location();

            pfc::string8 Extension = pfc::string_extension(Location.get_path());

            size_t FileExtensionIndex;
            size_t FileExtensionCount;

            for (FileExtensionIndex = (unsigned)((n == 0) ? 3 : 0), FileExtensionCount = _FileExtensionCount; FileExtensionIndex < FileExtensionCount; ++FileExtensionIndex)
            {
                if (pfc::stricmp_ascii(Extension, _FileExtensions[FileExtensionIndex]) == 0)
                {
                    ++matches;
                    break;
                }
            }

            if (FileExtensionIndex < FileExtensionCount)
                continue;

            if (n == 3)
            {
                for (size_t SyxExtensionIndex = 0, SyxExtensionCount = _SyxExtensionCount; SyxExtensionIndex < SyxExtensionCount; ++SyxExtensionIndex)
                {
                    if (!pfc::stricmp_ascii(Extension, _SyxExtension[SyxExtensionIndex]))
                    {
                        ++matches_syx;
                        break;
                    }
                }
            }
        }

        if ((n != 3 && matches == i) || (n == 3 && (matches + matches_syx) == i))
        {
            out = ItemNames[n];

            return true;
        }

        return false;
    }

    virtual void context_command(unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, const GUID &)
    {
        if (n > 4)
            uBugCheck();

        if (!n)
        {
        //  unsigned i = data.get_count();

            abort_callback_dummy m_abort;
            std::vector<uint8_t> file_data;

            for (t_size i = 0, j = data.get_count(); i < j; ++i)
            {
                const playable_location & loc = data.get_item(i)->get_location();
                pfc::string8 out_path = loc.get_path();
                out_path += ".mid";

                service_ptr_t<file> p_file;
                filesystem::g_open(p_file, loc.get_path(), filesystem::open_mode_read, m_abort);

                t_filesize size = p_file->get_size_ex(m_abort);

                file_data.resize(size);
                p_file->read_object(&file_data[0], size, m_abort);

                midi_container midi_file;

                if (!midi_processor::process_file(file_data, pfc::string_extension(loc.get_path()), midi_file))
                    continue;

                file_data.resize(0);
                midi_file.serialize_as_standard_midi_file(file_data);

                filesystem::g_open(p_file, out_path, filesystem::open_mode_write_new, m_abort);
                p_file->write_object(&file_data[0], file_data.size(), m_abort);
            }
        }
        else
        if (n < 3)
        {
            const char * p_midi_preset;
            pfc::string8 preset_serialized;

            if (n == 1)
            {
                MIDIPreset thePreset;
                thePreset.serialize(preset_serialized);
                p_midi_preset = preset_serialized.get_ptr();
            }
            else
                p_midi_preset = 0;

            static_api_ptr_t<metadb_io_v2> p_imgr;
            service_ptr_t<MIDIPresetFilter> p_filter = new service_impl_t<MIDIPresetFilter>(data, p_midi_preset);

            p_imgr->update_info_async(data, p_filter, core_api::get_main_window(), 0, 0);
        }
        else
        {
            MIDISysExDumps Dumps;

            if (n == 3)
            {
                for (t_size i = 0; i < data.get_count(); ++i)
                {
                    const char * path = data[i]->get_path();

                    if (IsSysExFileExtension(pfc::string_extension(path)))
                    {
                        Dumps.Items.append_single(path);
                    }
                }
            }

            static_api_ptr_t<metadb_io_v2> p_imgr;
            service_ptr_t<MIDISysExFilter> p_filter = new service_impl_t<MIDISysExFilter>(data, Dumps);

            p_imgr->update_info_async(data, p_filter, core_api::get_main_window(), 0, 0);
        }
    }
};

static contextmenu_item_factory_t<ContextMenu> ContextMenuFactory;
