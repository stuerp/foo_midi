
/** $VER: MIDISyxDumps.h (2022.12.30) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>

#include <midi_processing/midi_processor.h>

class MIDISysExDumps
{
public:
    MIDISysExDumps() noexcept { }
    MIDISysExDumps(const MIDISysExDumps & p_in) noexcept : Items(p_in.Items) { }

    void serialize(const char * filePath, pfc::string8 & p_out);
    void unserialize(const char * p_in, const char * filePath);

    void Merge(midi_container & container, abort_callback & abortHandler);

private:

    static void relative_path_create(const char * filePath, const char * directoryPath, pfc::string_base & p_out);
    static void relative_path_parse(const char * relativePath, const char * directoryPath, pfc::string_base & p_out);

public:
    pfc::array_t<pfc::string8> Items;
};
