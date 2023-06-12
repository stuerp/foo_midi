
/** $VER: ContextMenu.cpp (2023.06.04) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "ContextMenu.h"

static const char * ItemTexts[5] =
{
    "Convert to General MIDI",

    "Save synthesizer preset",
    "Remove synthesizer preset",

    "Assign SysEx dumps",
    "Clear SysEx dumps"
};

unsigned int ContextMenu::get_num_items() noexcept
{
    return _countof(ItemTexts);
}

void ContextMenu::get_item_name(unsigned int itemIndex, pfc::string_base & itemName) noexcept
{
    if (itemIndex >= _countof(ItemTexts))
        ::uBugCheck();

    itemName = ItemTexts[itemIndex];
}

bool ContextMenu::context_get_display(unsigned itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & itemList, pfc::string_base & out, unsigned &, const GUID &)
{
    if (itemIndex >= _countof(ItemTexts))
        uBugCheck();

    size_t MatchCount = 0, SysExMatchCount = 0;
    size_t ItemCount = itemList.get_count();

    for (size_t i = 0; i < ItemCount; ++i)
    {
        const playable_location& Location = itemList.get_item(i)->get_location();

        pfc::string8 Extension = pfc::string_extension(Location.get_path());

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

    if (((itemIndex != 3) && (MatchCount == ItemCount)) || ((itemIndex == 3) && ((MatchCount + SysExMatchCount) == ItemCount)))
    {
        out = ItemTexts[itemIndex];

        return true;
    }

    return false;
}

void ContextMenu::context_command(unsigned itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & itemList, const GUID &)
{
    if (itemIndex >= _countof(ItemTexts))
        uBugCheck();

    switch (itemIndex)
    {
        case 0: 
        {
            std::vector<uint8_t> Data;
            abort_callback_dummy AbortHandler;

            for (size_t i = 0, ItemCount = itemList.get_count(); i < ItemCount; ++i)
            {
                const playable_location & Location = itemList.get_item(i)->get_location();

                pfc::string8 FilePath = Location.get_path();

                FilePath += ".mid";

                {
                    service_ptr_t<file> File;

                    filesystem::g_open(File, Location.get_path(), filesystem::open_mode_read, AbortHandler);

                    t_filesize FileSize = File->get_size_ex(AbortHandler);

                    Data.resize((size_t)FileSize);

                    File->read_object(&Data[0], (t_size)FileSize, AbortHandler);

                    {
                        MIDIContainer Container;

                        if (MIDIProcessor::Process(Data, pfc::string_extension(Location.get_path()), Container))
                        {
                            Data.resize(0);

                            Container.SerializeAsSMF(Data);

                            filesystem::g_open(File, FilePath, filesystem::open_mode_write_new, AbortHandler);

                            File->write_object(&Data[0], Data.size(), AbortHandler);
                        }
                    }
                }
            }
            break;
        }

        case 1:
        case 2:
        {
            pfc::string8 PresetText;

            if (itemIndex == 1)
            {
                MIDIPreset Preset;

                Preset.Serialize(PresetText);
            }

            // Update the tags of the specified tracks.
            {
                static_api_ptr_t<metadb_io_v2> TagIO;

                service_ptr_t<MIDIPresetFilter> Filter = new service_impl_t<MIDIPresetFilter>(itemList, PresetText);

                TagIO->update_info_async(itemList, Filter, core_api::get_main_window(), 0, 0);
            }
            break;
        }

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

                service_ptr_t<MIDISysExFilter> Filter = new service_impl_t<MIDISysExFilter>(itemList, Dumps);

                TagIO->update_info_async(itemList, Filter, core_api::get_main_window(), 0, 0);
            }
            break;
        }
    }
}

static contextmenu_item_factory_t<ContextMenu> _ContextMenuFactory;
