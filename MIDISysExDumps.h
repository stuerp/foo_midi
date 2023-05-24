
/** $VER: MIDISysExDumps.h (2022.12.31) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>

#include <libmidi/MIDIProcessor.h>

class MIDISysExDumps
{
public:
    MIDISysExDumps() noexcept { }
    MIDISysExDumps(const MIDISysExDumps & p_in) : Items(p_in.Items) { }
//  MIDISysExDumps(const MIDISysExDumps &&) = delete;
//  MIDISysExDumps & operator=(const MIDISysExDumps &) = delete;
//  MIDISysExDumps & operator=(MIDISysExDumps &&) = delete;
    virtual ~MIDISysExDumps() { };

    void Serialize(const char * filePath, pfc::string8 & text);
    void Deserialize(const char * text, const char * filePath);

    void Merge(MIDIContainer & container, abort_callback & abortHandler);

private:
    static void CreateRelativePath(const char * filePath, const char * directoryPath, pfc::string_base & p_out);
    static void relative_path_parse(const char * relativePath, const char * directoryPath, pfc::string_base & p_out);

public:
    pfc::array_t<pfc::string8> Items;
};
