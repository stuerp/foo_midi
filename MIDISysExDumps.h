
/** $VER: MIDISysExDumps.h (2023.06.04) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>

#include <libmidi/MIDIProcessor.h>

class MIDISysExDumps
{
public:
    MIDISysExDumps() noexcept { }
    MIDISysExDumps(const MIDISysExDumps & p_in) : Items(p_in.Items) { }
    virtual ~MIDISysExDumps() { };

    void Serialize(const char * filePath, pfc::string8 & text);
    void Deserialize(const char * text, const char * filePath);

    void Merge(MIDIContainer & container, abort_callback & abortHandler);

private:
    static void CreateRelativeFromAbsolutePath(const char * filePath, const char * directoryPath, pfc::string_base & relativePath);
    static void CreateAbsoluteFromRelativePath(const char * relativePath, const char * directoryPath, pfc::string_base & filePath);

public:
    pfc::array_t<pfc::string8> Items;
};
