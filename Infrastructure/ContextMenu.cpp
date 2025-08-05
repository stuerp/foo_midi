
/** $VER: ContextMenu.cpp (2025.07.02) **/

#include "pch.h"

#include "ContextMenu.h"
#include "InputDecoder.h"

#include <pfc/string-conv-lite.h>
#include <filesystem.h>

#include "Configuration.h"

#include "Preset.h"
#include "PresetFilter.h"

#include "MIDISysExDumps.h"
#include "MIDISysExFilter.h"

unsigned int ContextMenu::get_num_items() noexcept
{
    return _countof(Names);
}

void ContextMenu::get_item_name(unsigned int index, pfc::string_base & name) noexcept
{
    if (index >= _countof(Names))
        ::uBugCheck();

    name = Names[index];
}

bool ContextMenu::get_item_description(unsigned index, pfc::string_base & description) noexcept
{
    static const char * Descriptions[] =
    {
        "Converts the selected files into General MIDI files in the same path as the source.",
        "Applies the current synthesizer preset to this track for future playback.",
        "Removes a saved synthesizer preset from this track.",
        "Assigns the selected SysEx dumps to the selected MIDI files.",
        "Clears all assigned SysEx dumps from the selected MIDI files.",
    };

    assert(_countof(Descriptions) == _countof(Names));

    if (index >= _countof(Descriptions))
        ::uBugCheck();

    description = Descriptions[index];

    return true;
}

GUID ContextMenu::get_item_guid(unsigned int index) noexcept
{
    static const GUID GUIDs[] =
    {
        { 0x70985c72, 0xe77e, 0x4bbb, { 0xbf, 0x11, 0x3c, 0x90, 0x2b, 0x27, 0x39, 0x9d } },
        { 0xeb3f3ab4, 0x60b3, 0x4579, { 0x9f, 0xf8, 0x38, 0xda, 0xc0, 0x91, 0x2c, 0x82 } },
        { 0x5bcb6efe, 0x2eb5, 0x4331, { 0xb9, 0xc1, 0x92, 0x4b, 0x77, 0xba, 0xcc, 0x10 } },
        { 0xd0e4a166, 0x010c, 0x41f0, { 0xad, 0x5a, 0x51, 0x84, 0x44, 0xa3, 0x92, 0x9c } },
        { 0x2aa8c082, 0x5d84, 0x4982, { 0xb4, 0x5d, 0xde, 0x51, 0xcb, 0x75, 0xff, 0xf2 } },
    };

    assert(_countof(GUIDs) == _countof(Names));

    if (index >= _countof(GUIDs))
        ::uBugCheck();

    return GUIDs[index];
}

bool ContextMenu::context_get_display(unsigned int itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & itemList, pfc::string_base & out, unsigned & flags, const GUID &)
{
    if (itemIndex >= _countof(Names))
        ::uBugCheck();

    size_t MatchCount = 0, SysExMatchCount = 0;
    size_t ItemCount = itemList.get_count();

    for (size_t i = 0; i < ItemCount; ++i)
    {
        const playable_location & Location = itemList.get_item(i)->get_location();

        auto Extension = pfc::string_extension(Location.get_path());

        size_t FileExtensionIndex;

        for (FileExtensionIndex = (size_t)((itemIndex == 0) ? 3 : 0); FileExtensionIndex < _FileExtensionCount; ++FileExtensionIndex)
        {
            if (pfc::stricmp_ascii(Extension, _FileExtensions[FileExtensionIndex]) == 0)
            {
                ++MatchCount;
                break;
            }
        }

        if (FileExtensionIndex < _FileExtensionCount)
            continue;

        if (itemIndex == 3)
        {
            for (size_t SysExExtensionIndex = 0; SysExExtensionIndex < _SysExFileExtensionCount; ++SysExExtensionIndex)
            {
                if (pfc::stricmp_ascii(Extension, _SysExFileExtensions[SysExExtensionIndex]) == 0)
                {
                    ++SysExMatchCount;
                    break;
                }
            }
        }
    }

    if (itemIndex == 5)
    {
        out = Names[itemIndex];
/*
        auto & Instance = CLAP::Host::GetInstance();

        flags = !Instance.HasGUI() ? FLAG_DISABLED_GRAYED : 0;
*/
        return true;
    }

    if ((itemIndex == 3) && ((MatchCount + SysExMatchCount) == ItemCount))
    {
        out = Names[itemIndex];

        return true;
    }

    if ((itemIndex != 3) && (MatchCount == ItemCount))
    {
        out = Names[itemIndex];

        return true;
    }

    return false;
}

void ContextMenu::context_command(unsigned int itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & itemList, const GUID &)
{
    if (itemIndex >= _countof(Names))
        ::uBugCheck();

    switch (itemIndex)
    {
        // Convert to Standard MIDI File (SMF)
        case 0: 
        {
            std::vector<uint8_t> Object;

            for (size_t i = 0, ItemCount = itemList.get_count(); i < ItemCount; ++i)
            {
                const playable_location & Location = itemList.get_item(i)->get_location();

                pfc::string FilePath = Location.get_path();

                FilePath += ".mid";

                {
                    service_ptr_t<file> File;

                    filesystem::g_open(File, Location.get_path(), filesystem::open_mode_read, fb2k::noAbort);

                    t_filesize FileSize = File->get_size_ex(fb2k::noAbort);

                    Object.resize((size_t) FileSize);

                    File->read_object(Object.data(), (t_size) FileSize, fb2k::noAbort);

                    {
                        midi::container_t Container;

                        bool Success = false;

                        try
                        {
                            const auto TmpFilePath = filesystem::g_get_native_path(Location.get_path());

                            Success = midi::processor_t::Process(Object, pfc::wideFromUTF8(TmpFilePath), Container);
                        }
                        catch (...)
                        {
                        }

                        if (Success)
                        {
                            Object.resize(0);

                            Container.SerializeAsSMF(Object);

                            filesystem::g_open(File, FilePath, filesystem::open_mode_write_new, fb2k::noAbort);

                            File->write_object(Object.data(), Object.size(), fb2k::noAbort);
                        }
                    }
                }
            }
            break;
        }

        // "Save synthesizer preset"
        // "Remove synthesizer preset"
        case 1:
        case 2:
        {
            pfc::string PresetText;

            if (itemIndex == 1)
            {
                preset_t Preset;

                Preset.Serialize(PresetText);
            }

            // Update the tags of the specified tracks.
            {
                static_api_ptr_t<metadb_io_v2> TagIO;

                auto Filter = new service_impl_t<preset_filter_t>(itemList, PresetText);

                TagIO->update_info_async(itemList, Filter, core_api::get_main_window(), 0, 0);
            }
            break;
        }


        // "Assign SysEx dumps"
        // "Clear SysEx dumps"
        case 3:
        case 4:
        {
            MIDISysExDumps Dumps;

            if (itemIndex == 3)
            {
                for (size_t i = 0; i < itemList.get_count(); ++i)
                {
                    const char * FilePath = itemList[i]->get_path();

                    if (IsSysExFileExtension(pfc::string_extension(FilePath)))
                        Dumps.Items.append_single(FilePath);
                }
            }

            // Update the tags of the specified tracks.
            {
                static_api_ptr_t<metadb_io_v2> TagIO;

                auto Filter = new service_impl_t<MIDISysExFilter>(itemList, Dumps);

                TagIO->update_info_async(itemList, Filter, core_api::get_main_window(), 0, 0);
            }
            break;
        }
    }
}

const char * ContextMenu::Names[5] =
{
    "Save as Standard MIDI File (SMF)",

    "Save synthesizer preset",
    "Remove synthesizer preset",

    "Assign SysEx dumps",
    "Clear SysEx dumps",
};

static contextmenu_item_factory_t<ContextMenu> _ContextMenuFactory;
