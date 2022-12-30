
/** $VER: MIDISysExDumps.cpp (2022.12.30) **/

#include "MIDISysExDumps.h"

#include "Fields.h"

void MIDISysExDumps::serialize(const char * filePath, pfc::string8 & p_out)
{
    if (filePath == nullptr)
        return;

    pfc::string8_fast p_relative;

    p_out.reset();

    for (unsigned i = 0; i < Items.get_count(); ++i)
    {
        relative_path_create(Items[i], filePath, p_relative);

        if (i)
            p_out += "\n";

        p_out += p_relative;
    }
}

void MIDISysExDumps::unserialize(const char * p_in, const char * filePath)
{
    if ((p_in == nullptr) || (filePath == nullptr))
        return;

    pfc::string8_fast p_relative, p_absolute;

    const char * end = p_in + ::strlen(p_in);

    while (p_in < end)
    {
        const char * LineFeed = ::strchr(p_in, '\n');

        if (LineFeed == nullptr)
            LineFeed = end;

        p_relative.set_string(p_in, (t_size)(LineFeed - p_in));

        relative_path_parse(p_relative, filePath, p_absolute);

        Items.append_single(p_absolute);

        p_in = LineFeed;

        while (*p_in == '\n')
            ++p_in;
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

            t_filesize FileSize = File->get_size_ex(abortHandler);

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

void MIDISysExDumps::relative_path_create(const char * filePath, const char * directoryPath, pfc::string_base & p_out)
{
    if ((filePath ==nullptr) || (directoryPath == nullptr))
        return;

    t_size p_file_fn = pfc::scan_filename(filePath);
    t_size p_base_path_fn = pfc::scan_filename(directoryPath);

    if (p_file_fn == p_base_path_fn && !strncmp(filePath, directoryPath, p_file_fn))
    {
        p_out = filePath + p_file_fn;
    }
    else
    if (p_file_fn > p_base_path_fn && !strncmp(filePath, directoryPath, p_base_path_fn) && pfc::is_path_separator(filePath[p_base_path_fn - 1]))
    {
        p_out = filePath + p_base_path_fn;
    }
    else
    if (p_base_path_fn > p_file_fn && !strncmp(filePath, directoryPath, p_file_fn) && pfc::is_path_separator(directoryPath[p_file_fn - 1]))
    {
        p_out.reset();

        t_size p_base_path_search = p_file_fn;

        while (p_base_path_search < p_base_path_fn)
        {
            if (pfc::is_path_separator(directoryPath[++p_base_path_search]))
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

