
/** $VER: ContextMenu.cpp (2023.05.28) **/

#pragma warning(disable: 5045 26481 26485)

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

bool ContextMenu::context_get_display(unsigned itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out, unsigned &, const GUID &)
{
    if (itemIndex >= _countof(ItemTexts))
        uBugCheck();

    size_t j, matches, matches_syx;

    size_t TrackCount = data.get_count();

    for (matches = 0, matches_syx = 0, j = 0; j < TrackCount; ++j)
    {
        const playable_location& Location = data.get_item(j)->get_location();

        pfc::string8 Extension = pfc::string_extension(Location.get_path());

        size_t FileExtensionIndex;
        size_t FileExtensionCount;

        for (FileExtensionIndex = (unsigned int)((itemIndex == 0) ? 3 : 0), FileExtensionCount = _FileExtensionCount; FileExtensionIndex < FileExtensionCount; ++FileExtensionIndex)
        {
            if (pfc::stricmp_ascii(Extension, _FileExtensions[FileExtensionIndex]) == 0)
            {
                ++matches;
                break;
            }
        }

        if (FileExtensionIndex < FileExtensionCount)
            continue;

        if (itemIndex == 3)
        {
            for (size_t SyxExtensionIndex = 0, SyxExtensionCount = _SysExFileExtensionCount; SyxExtensionIndex < SyxExtensionCount; ++SyxExtensionIndex)
            {
                if (pfc::stricmp_ascii(Extension, _SysExFileExtensions[SyxExtensionIndex]) == 0)
                {
                    ++matches_syx;
                    break;
                }
            }
        }
    }

    if (((itemIndex != 3) && matches == TrackCount) || ((itemIndex) == 3 && (matches + matches_syx) == TrackCount))
    {
        out = ItemTexts[itemIndex];

        return true;
    }

    return false;
}

void ContextMenu::context_command(unsigned itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & trackList, const GUID &)
{
    if (itemIndex >= _countof(ItemTexts))
        uBugCheck();

    switch (itemIndex)
    {
        case 0: 
        {
            std::vector<uint8_t> Data;
            abort_callback_dummy AbortHandler;

            for (size_t i = 0, TrackCount = trackList.get_count(); i < TrackCount; ++i)
            {
                const playable_location & Location = trackList.get_item(i)->get_location();

                pfc::string8 FilePath = Location.get_path();

                FilePath += ".mid";

                service_ptr_t<file> File;

                filesystem::g_open(File, Location.get_path(), filesystem::open_mode_read, AbortHandler);

                t_filesize FileSize = File->get_size_ex(AbortHandler);

                Data.resize((size_t)FileSize);

                File->read_object(&Data[0], (t_size)FileSize, AbortHandler);

                MIDIContainer Container;

                if (MIDIProcessor::Process(Data, pfc::string_extension(Location.get_path()), Container))
                {
                    Data.resize(0);

                    Container.serialize_as_standard_midi_file(Data);

                    filesystem::g_open(File, FilePath, filesystem::open_mode_write_new, AbortHandler);

                    File->write_object(&Data[0], Data.size(), AbortHandler);
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

                service_ptr_t<MIDIPresetFilter> Filter = new service_impl_t<MIDIPresetFilter>(trackList, PresetText);

                TagIO->update_info_async(trackList, Filter, core_api::get_main_window(), 0, 0);
            }
            break;
        }

        case 3:
        case 4:
        {
            MIDISysExDumps Dumps;

            if (itemIndex == 3)
            {
                for (size_t i = 0; i < trackList.get_count(); ++i)
                {
                    const char * FilePath = trackList[i]->get_path();

                    if (IsSysExFileExtension(pfc::string_extension(FilePath)))
                        Dumps.Items.append_single(FilePath);
                }
            }

            // Update the tags of the specified tracks.
            {
                static_api_ptr_t<metadb_io_v2> TagIO;

                service_ptr_t<MIDISysExFilter> Filter = new service_impl_t<MIDISysExFilter>(trackList, Dumps);

                TagIO->update_info_async(trackList, Filter, core_api::get_main_window(), 0, 0);
            }
            break;
        }
    }
}

static contextmenu_item_factory_t<ContextMenu> _ContextMenuFactory;
