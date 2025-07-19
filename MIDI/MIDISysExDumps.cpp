
/** $VER: MIDISysExDumps.cpp (2024.05.09) **/

#include "pch.h"

#include "MIDISysExDumps.h"

#include <format>

void MIDISysExDumps::Serialize(const char * filePath, pfc::string & text) noexcept
{
    if (filePath == nullptr)
        return;

    pfc::string RelativePath;

    text.reset();

    for (size_t i = 0; i < Items.get_count(); ++i)
    {
        CreateRelativeFromAbsolutePath(Items[i], filePath, RelativePath);

        if (i > 0)
            text += "\n";

        text += RelativePath;
    }
}

void MIDISysExDumps::Deserialize(const char * text, const char * filePath) noexcept
{
    if ((text == nullptr) || (filePath == nullptr))
        return;

    pfc::string RelativePath, AbsolutePath;

    const char * end = text + ::strlen(text);

    while (text < end)
    {
        const char * LineFeed = ::strchr(text, '\n');

        if (LineFeed == nullptr)
            LineFeed = end;

        RelativePath.set_string(text, (t_size)(LineFeed - text));

        CreateAbsoluteFromRelativePath(RelativePath, filePath, AbsolutePath);

        Items.append_single(AbsolutePath);

        text = LineFeed;

        while (*text == '\n')
            ++text;
    }
}

/// <summary>
/// Merges these SysEx dumps into the specified container.
/// </summary>
void MIDISysExDumps::Merge(midi::container_t & container, abort_callback & abortHandler)
{
    std::vector<uint8_t> Object;

    for (size_t i = 0; i < Items.get_count(); ++i)
    {
        try
        {
            file::ptr File;

            filesystem::g_open(File, Items[i], filesystem::open_mode_read, abortHandler);

            const t_filesize FileSize = File->get_size_ex(abortHandler);

            Object.resize((size_t) FileSize);

            File->read_object(Object.data(), (t_size) FileSize, abortHandler);

            {
                midi::container_t Container;

                if (!midi::processor_t::Process(Object, nullptr, Container))
                    break;

                container.MergeTracks(Container);
            }
        }
        catch (const std::exception & e)
        {
            pfc::string PathName;

            filesystem::g_get_canonical_path(Items[i], PathName);

            throw exception_io_data(std::format("Error processing SysEx file \"{}\": {}.", PathName.c_str(), e.what()).c_str());
        }
    }
}

void MIDISysExDumps::CreateRelativeFromAbsolutePath(const char * filePath, const char * directoryPath, pfc::string_base & relativePath)
{
    if ((filePath ==nullptr) || (directoryPath == nullptr))
        return;

    const t_size FileNameIndex = pfc::scan_filename(filePath);
    const t_size BasePathIndex = pfc::scan_filename(directoryPath);

    if ((FileNameIndex == BasePathIndex) && (::strncmp(filePath, directoryPath, FileNameIndex) == 0))
    {
        relativePath = filePath + FileNameIndex;
    }
    else
    if ((FileNameIndex > BasePathIndex) && (::strncmp(filePath, directoryPath, BasePathIndex) == 0) && pfc::is_path_separator((unsigned int)filePath[BasePathIndex - 1]))
    {
        relativePath = filePath + BasePathIndex;
    }
    else
    if ((BasePathIndex > FileNameIndex) && (::strncmp(filePath, directoryPath, FileNameIndex) == 0) && pfc::is_path_separator((unsigned int)directoryPath[FileNameIndex - 1]))
    {
        relativePath.reset();

        t_size p_base_path_search = FileNameIndex;

        while (p_base_path_search < BasePathIndex)
        {
            if (pfc::is_path_separator((unsigned int)directoryPath[++p_base_path_search]))
            {
                relativePath += "..\\";
            }
        }

        relativePath += filePath + FileNameIndex;
    }
    else
    {
        relativePath = filePath;
    }
}

void MIDISysExDumps::CreateAbsoluteFromRelativePath(const char * relativePath, const char * directoryPath, pfc::string_base & absolutePath)
{
    if ((relativePath == nullptr) || (directoryPath == nullptr))
        return;

    if (::strstr(relativePath, "://"))
    {
        absolutePath = relativePath;
    }
    else
    {
        pfc::string t = directoryPath;

        t.truncate(t.scan_filename());
        t += relativePath;

        filesystem::g_get_canonical_path(t, absolutePath);
    }
}
