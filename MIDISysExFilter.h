
/** $VER: MIDISysExFilter.h (2022.12.31) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/file_info_filter.h>

#include "Configuration.h"
#include "MIDISysExDumps.h"

class MIDISysExFilter : public file_info_filter
{
public:
    MIDISysExFilter() = delete;
    MIDISysExFilter(const MIDISysExFilter & p_in) = delete;
    MIDISysExFilter(const MIDISysExFilter &&) = delete;
    MIDISysExFilter & operator=(const MIDISysExFilter &) = delete;
    MIDISysExFilter & operator=(MIDISysExFilter &&) = delete;
    virtual ~MIDISysExFilter() { };

    MIDISysExFilter(const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const MIDISysExDumps & p_dumps);

    virtual bool apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo)
    {
        pfc::string8 FileExtension = pfc::string_extension(location->get_path());

        for (size_t i = 0; i < _SysExFileExtensionCount; ++i)
        {
            if (pfc::stricmp_ascii(FileExtension, _SysExFileExtensions[i]) == 0)
                return false;
        }

        t_size Index;

        if (_Handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, location, Index))
        {
            pfc::string8 Text;

            _SysExDumps.serialize(location->get_path(), Text);

            if (Text.get_length())
                fileInfo.info_set(field_syx, Text);
            else
                fileInfo.info_remove(field_syx);

            return true;
        }

        return false;
    }

private:
    MIDISysExDumps _SysExDumps;
    metadb_handle_list _Handles;
};
