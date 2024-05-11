
/** $VER: MIDISysExDumps.h (2024.05.11) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <MIDIProcessor.h>

class MIDISysExDumps
{
public:
    MIDISysExDumps() noexcept { }

    MIDISysExDumps(const MIDISysExDumps & other) { operator=(other); }

    MIDISysExDumps & operator=(const MIDISysExDumps & other)
    {
        Items = other.Items;

        return *this;
    };

    MIDISysExDumps(MIDISysExDumps &&) = delete;
    MIDISysExDumps & operator=(MIDISysExDumps &&) = delete;

    virtual ~MIDISysExDumps() { };

    void Serialize(const char * filePath, pfc::string8 & text) noexcept;
    void Deserialize(const char * text, const char * filePath) noexcept;

    void Merge(MIDIContainer & container, abort_callback & abortHandler);

private:
    static void CreateRelativeFromAbsolutePath(const char * filePath, const char * directoryPath, pfc::string_base & relativePath);
    static void CreateAbsoluteFromRelativePath(const char * relativePath, const char * directoryPath, pfc::string_base & filePath);

public:
    pfc::array_t<pfc::string8> Items;
};
