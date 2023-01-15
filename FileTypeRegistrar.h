
/** $VER: FileTypeRegistrar.h (2022.12.31) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/input_file_type.h>

#include "Configuration.h"

/// <summary>
/// Registers media file types that can be opened through "open file" dialogs or associated with foobar2000 application in the Windows shell.
/// </summary>
class FileTypeRegistrar : public input_file_type
{
public:
    FileTypeRegistrar() noexcept { }

    FileTypeRegistrar(const FileTypeRegistrar&) = delete;
    FileTypeRegistrar(const FileTypeRegistrar&&) = delete;
    FileTypeRegistrar& operator=(const FileTypeRegistrar&) = delete;
    FileTypeRegistrar& operator=(FileTypeRegistrar&&) = delete;

    virtual ~FileTypeRegistrar() noexcept { }

private:
    unsigned int get_count() noexcept override
    {
        return 2;
    }

    bool get_name(unsigned int index, pfc::string_base & name) override
    {
        if (index > 1)
            return false;

        name = (index == 0) ? "MIDI files" : "SysEx dump files";

        return true;
    }

    bool get_mask(unsigned int index, pfc::string_base & mask) override
    {
        if (index > 1)
            return false;

        mask.reset();

        const size_t Count = (index == 0) ? _FileExtensionCount : _SysExFileExtensionCount;
        const char * const * Extensions = (index == 0) ? _FileExtensions : _SysExFileExtensions;

        if (Extensions == nullptr)
            return false;

        for (size_t i = 0; i < Count; ++i)
        {
            if (i > 0)
                mask.add_byte(';');

            mask << "*." << Extensions[i];
        }

        return true;
    }

    bool is_associatable(unsigned) noexcept override
    {
        return true;
    }
};
