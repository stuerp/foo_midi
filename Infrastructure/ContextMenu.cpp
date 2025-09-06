
/** $VER: ContextMenu.cpp (2025.09.01) **/

#include "pch.h"

#include "ContextMenu.h"
#include "InputDecoder.h"

#include <pfc/string-conv-lite.h>
#include <filesystem.h>

#include <libsf.h>

#include "Configuration.h"

#include "Preset.h"
#include "PresetFilter.h"

#include "MIDISysExDumps.h"
#include "MIDISysExFilter.h"

#pragma comment(lib, "comdlg32")

static void ConvertDLSToSF2(const std::vector<uint8_t> & dlsData, const std::string & filePath) noexcept;

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
    assert(_countof(Descriptions) == _countof(Names));

    if (index >= _countof(Descriptions))
        ::uBugCheck();

    description = Descriptions[index];

    return true;
}

GUID ContextMenu::get_item_guid(unsigned int index) noexcept
{
    assert(_countof(GUIDs) == _countof(Names));

    if (index >= _countof(GUIDs))
        ::uBugCheck();

    return GUIDs[index];
}

bool ContextMenu::context_get_display(unsigned int itemIndex, const pfc::list_base_const_t<metadb_handle_ptr> & itemList, pfc::string_base & out, unsigned & flags, const GUID &)
{
    if (itemIndex >= _countof(Names))
        ::uBugCheck();

    const size_t ItemCount = itemList.get_count();

    size_t MatchCount = 0;
    size_t SysExMatchCount = 0;
    size_t RMICount = 0;

    for (size_t i = 0; i < ItemCount; ++i)
    {
        const playable_location & Location = itemList.get_item(i)->get_location();

        const auto Extension = pfc::string_extension(Location.get_path());

        size_t FileExtensionIndex;

        for (FileExtensionIndex = (size_t)((itemIndex == 0) ? 3 : 0); FileExtensionIndex < _FileExtensionCount; ++FileExtensionIndex)
        {
            if (pfc::stricmp_ascii(Extension, _FileExtensions[FileExtensionIndex]) == 0)
            {
                ++MatchCount;

                if ((itemIndex == 5) && (pfc::stricmp_ascii(Extension, "RMI") == 0))
                    ++RMICount;
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

    if ((itemIndex == 3) && ((MatchCount + SysExMatchCount) == ItemCount))
    {
        out = Names[itemIndex];

        return true;
    }

    if (itemIndex == 5)
    {
        out = Names[itemIndex];

        return (RMICount != 0);
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

        // Save player preset
        // Remove player preset
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

        // Assign SysEx dumps.
        // Clear SysEx dumps.
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

        // Extract embedded sound font
        case 5:
        {
            std::vector<uint8_t> Object;

            for (size_t i = 0, ItemCount = itemList.get_count(); i < ItemCount; ++i)
            {
                const playable_location & Location = itemList.get_item(i)->get_location();

                try
                {
                    service_ptr_t<file> File;

                    filesystem::g_open(File, Location.get_path(), filesystem::open_mode_read, fb2k::noAbort);

                    t_filesize FileSize = File->get_size_ex(fb2k::noAbort);

                    Object.resize((size_t) FileSize);

                    File->read_object(Object.data(), (t_size) FileSize, fb2k::noAbort);

                    try
                    {
                        midi::container_t Container;

                        if (midi::processor_t::Process(Object, pfc::wideFromUTF8(Location.get_path()), Container, midi::DefaultOptions))
                        {
                            const auto & Data = Container.SoundFont;

                            if (Data.size() > 12)
                            {
                                const bool IsDLS = (::memcmp(Data.data() + 8, "DLS ", 4) == 0);

                                std::string FileTypes;

                                if (IsDLS)
                                {
                                    FileTypes = "DLS collection (.dls)";
                                    FileTypes.push_back('\0');
                                    FileTypes += "*.dls";
                                    FileTypes.push_back('\0');
                                }

                                FileTypes += "SoundFont bank (.sf2)";
                                FileTypes.push_back('\0');
                                FileTypes += "*.sf2";
                                FileTypes.push_back('\0');

                                FileTypes.push_back('\0');

                                fs::path FilePath = Location.get_path(); // This may throw an exception due to UTF-8 to Wide conversion.

                                std::string FileName = FilePath.stem().string().c_str();

                                FileName.resize(MAX_PATH);

                                OPENFILENAMEA ofn =
                                {
                                    .lStructSize     = sizeof(ofn),
                                    .lpstrFilter     = FileTypes.c_str(),
                                    .lpstrFile       = (LPSTR) FileName.c_str(),
                                    .nMaxFile        = (DWORD) FileName.size(),
                                    .lpstrInitialDir = foobar2000_io::afterProtocol(Location.get_path()),
                                    .lpstrTitle      = "Select sound font name",
                                    .Flags           = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT,
                                    .lpstrDefExt     = IsDLS ? ".dls" : ".sf2",
                                };

                                if (::GetSaveFileNameA(&ofn))
                                {
                                    if (IsDLS && (ofn.nFilterIndex == 2))
                                    {
                                        ConvertDLSToSF2(Data, FileName);
                                    }
                                    else
                                    {
                                        filesystem::g_open(File, FileName.c_str(), filesystem::open_mode_write_new, fb2k::noAbort);

                                        File->write_object(Data.data(), Data.size(), fb2k::noAbort);
                                    }
                                }
                            }
                        }
                    }
                    catch (const std::exception & e)
                    {
                        pfc::string Message = pfc::string("Failed to process MIDI file \"") + Location.get_path() + "\": ";

                        throw exception_io_data(Message + e.what());
                    }
                }
                catch (const std::exception & e)
                {
                    pfc::string Message = pfc::string("Failed to read MIDI file \"") + Location.get_path() + "\": ";

                    throw exception_io_data(Message + e.what());
                }
            }

            break;
        }
    }
}

/// <summary>
/// Converts the DLS collection into an SF bank and saves it.
/// </summary>
void ConvertDLSToSF2(const std::vector<uint8_t> & dlsData, const std::string & filePath) noexcept
{
    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is converting embedded DLS collection to SF2 bank \"%s\".", filePath.c_str());

    sf::dls::collection_t Collection;

    if (!ReadDLS(Collection, dlsData))
        return;

    try
    {
        sf::bank_t Bank;

        Bank.ConvertFrom(Collection);

        WriteSF2(Bank, filePath);
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to convert DLS collection: %s.", e.what());
    }
}

const char * ContextMenu::Names[6] =
{
    "Save as Standard MIDI File (SMF)",

    "Save player preset",
    "Remove player preset",

    "Assign SysEx dumps",
    "Clear SysEx dumps",

    "Extract embedded sound font"
};

const char * ContextMenu::Descriptions[6] =
{
    "Saves the selected files as Standard MIDI files in the same path as the source.",

    "Adds the current player preset to this track.",
    "Removes a saved player preset from this track.",

    "Assigns the selected SysEx dumps to the selected MIDI files.",
    "Clears all assigned SysEx dumps from the selected MIDI files.",

    "Extracts the sound font embedded in the selected files and save it to disk."
};

const GUID ContextMenu::GUIDs[] =
{
    { 0x70985c72, 0xe77e, 0x4bbb, { 0xbf, 0x11, 0x3c, 0x90, 0x2b, 0x27, 0x39, 0x9d } },

    { 0xeb3f3ab4, 0x60b3, 0x4579, { 0x9f, 0xf8, 0x38, 0xda, 0xc0, 0x91, 0x2c, 0x82 } },
    { 0x5bcb6efe, 0x2eb5, 0x4331, { 0xb9, 0xc1, 0x92, 0x4b, 0x77, 0xba, 0xcc, 0x10 } },

    { 0xd0e4a166, 0x010c, 0x41f0, { 0xad, 0x5a, 0x51, 0x84, 0x44, 0xa3, 0x92, 0x9c } },
    { 0x2aa8c082, 0x5d84, 0x4982, { 0xb4, 0x5d, 0xde, 0x51, 0xcb, 0x75, 0xff, 0xf2 } },

    { 0xe3e4b9fd, 0xdc92, 0x4c9f, { 0xac, 0x90, 0xf6, 0xf8, 0x48, 0xcc, 0x03, 0x3b } }
};

static contextmenu_item_factory_t<ContextMenu> _ContextMenuFactory;
