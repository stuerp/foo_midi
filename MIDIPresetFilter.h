
/** $VER: MIDIPresetFilter.h (2023.01.04) **/

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

    bool apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo) override;

private:
    pfc::string8 _MIDIPreset;

    metadb_handle_list _Handles;
};