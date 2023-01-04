
/** $VER: MIDISysExDumps.cpp (2022.12.31) **/

#pragma warning(disable: 26446 26481 26493)

#include "MIDISysExDumps.h"

void MIDISysExDumps::Serialize(const char * filePath, pfc::string8 & text)
{
    if (filePath == nullptr)
        return;

    pfc::string8 RelativePath;

    text.reset();

    for (size_t i = 0; i < Items.get_count(); ++i)
    {
        CreateRelativePath(Items[i], filePath, RelativePath);

        if (i)
            text += "\n";

        text += RelativePath;
    }
}

void MIDISysExDumps::Deserialize(const char * text, const char * filePath)
{
    if ((text == nullptr) || (filePath == nullptr))
        return;

    pfc::string8 RelativePath, AbsolutePath;

    const char * end = text + ::strlen(text);

    while (text < end)
    {
        const char * LineFeed = ::strchr(text, '\n');

        if (LineFeed == nullptr)
            LineFeed = end;

        RelativePath.set_string(text, (t_size)(LineFeed - text));

        relative_path_parse(RelativePath, filePath, AbsolutePath);

        Items.append_single(AbsolutePath);

        text = LineFeed;

        while (*text == '\n')
            ++text;
    }
}

/// <summary>
/// Merges these SysEx dumps into the specified container.
/// </summary>
void MIDISysExDumps::Merge(midi_container & container, abort_callback & abortHandler)
{
    std::vector<uint8_t> Data;

    for (size_t i = 0; i < Items.get_count(); ++i)
    {
        try
        {
            file::ptr File;

            filesystem::g_open(File, Items[i], filesystem::open_mode_read, abortHandler);

            const t_filesize FileSize = File->get_size_ex(abortHandler);

            Data.resize(FileSize);

            File->read_object(&Data[0], FileSize, abortHandler);

            {
                midi_container Container;

                if (!midi_processor::process_syx_file(Data, Container))
                    break;

                container.merge_tracks(Container);
            }
        }
        catch (const std::exception & e)
        {
            pfc::string8 path;

            filesystem::g_get_canonical_path(Items[i], path);

            pfc::string8 temp = "Error processing dump ";

            temp += path;
            temp += ": ";
            temp += e.what();

            throw exception_io_data(temp);
        }
    }
}

void MIDISysExDumps::CreateRelativePath(const char * filePath, const char * directoryPath, pfc::string_base & p_out)
{
    if ((filePath ==nullptr) || (directoryPath == nullptr))
        return;

    const t_size p_file_fn = pfc::scan_filename(filePath);
    const t_size p_base_path_fn = pfc::scan_filename(directoryPath);

    if (p_file_fn == p_base_path_fn && !strncmp(filePath, directoryPath, p_file_fn))
    {
        p_out = filePath + p_file_fn;
    }
    else
    if ((p_file_fn > p_base_path_fn) && (::strncmp(filePath, directoryPath, p_base_path_fn) == 0) && pfc::is_path_separator((unsigned int)filePath[p_base_path_fn - 1]))
    {
        p_out = filePath + p_base_path_fn;
    }
    else
    if ((p_base_path_fn > p_file_fn) && (::strncmp(filePath, directoryPath, p_file_fn) == 0) && pfc::is_path_separator((unsigned int)directoryPath[p_file_fn - 1]))
    {
        p_out.reset();

        t_size p_base_path_search = p_file_fn;

        while (p_base_path_search < p_base_path_fn)
        {
            if (pfc::is_path_separator((unsigned int)directoryPath[++p_base_path_search]))
            {
                p_out += "..\\";
            }
        }

        p_out += filePath + p_file_fn;
    }
    else
    {
        p_out = filePath;
    }
}

void MIDISysExDumps::relative_path_parse(const char * relativePath, const char * directoryPath, pfc::string_base & p_out)
{
    if ((relativePath == nullptr) || (directoryPath == nullptr))
        return;

    if (::strstr(relativePath, "://"))
    {
        p_out = relativePath;
    }
    else
    {
        pfc::string8 t = directoryPath;

        t.truncate(t.scan_filename());
        t += relativePath;

        filesystem::g_get_canonical_path(t, p_out);
    }
}

