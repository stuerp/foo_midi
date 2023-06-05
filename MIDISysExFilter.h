
/** $VER: MIDISysExFilter.h (2023.04.01) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/file_info_filter.h>

#include "Configuration.h"
#include "MIDISysExDumps.h"

// Provides direct control over which part of file_info gets altered during a tag update uperation. To be used with metadb_io_v2::update_info_async().
class MIDISysExFilter : public file_info_filter
{
public:
    MIDISysExFilter() = delete;

    MIDISysExFilter(const MIDISysExFilter & p_in) = delete;
    MIDISysExFilter(const MIDISysExFilter &&) = delete;
    MIDISysExFilter & operator=(const MIDISysExFilter &) = delete;
    MIDISysExFilter & operator=(MIDISysExFilter &&) = delete;

    virtual ~MIDISysExFilter() { };

    MIDISysExFilter(const pfc::list_base_const_t<metadb_handle_ptr> & list, const MIDISysExDumps & dumps);

    bool apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo) override;

private:
    MIDISysExDumps _SysExDumps;
    metadb_handle_list _Handles;
};
