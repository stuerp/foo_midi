
/** $VER: MIDIPresetFilter.h (2022.12.31) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/file_info_filter.h>

#include "Configuration.h"

class MIDIPresetFilter : public file_info_filter
{
public:
    MIDIPresetFilter() = delete;
    MIDIPresetFilter(const MIDIPresetFilter & p_in) = delete;
    MIDIPresetFilter(const MIDIPresetFilter &&) = delete;
    MIDIPresetFilter & operator=(const MIDIPresetFilter &) = delete;
    MIDIPresetFilter & operator=(MIDIPresetFilter &&) = delete;
    virtual ~MIDIPresetFilter() { };

    MIDIPresetFilter(const pfc::list_base_const_t<metadb_handle_ptr> & list, const char * midiPreset);

    virtual bool apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo)
    {
        t_size index;

        if (_Handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, location, index))
        {
            if (_MIDIPreset.get_length())
                fileInfo.info_set(field_preset, _MIDIPreset);
            else
                fileInfo.info_remove(field_preset);

            return true;
        }

        return false;
    }

private:
    pfc::string8 _MIDIPreset;

    metadb_handle_list _Handles;
};
