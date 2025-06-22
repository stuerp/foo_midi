
/** $VER: PresetFilter.h (2025.06.22) **/

#pragma once

#include <sdk/foobar2000-lite.h>
#include <sdk/file_info_filter.h>

#include "Configuration.h"

/// <summary>
/// Provides direct control over which part of file_info gets altered during a tag update uperation. To be used with metadb_io_v2::update_info_async().
/// </summary>
class preset_filter_t : public file_info_filter
{
public:
    preset_filter_t() = delete;

    preset_filter_t(const preset_filter_t & p_in) = delete;
    preset_filter_t(const preset_filter_t &&) = delete;
    preset_filter_t & operator=(const preset_filter_t &) = delete;
    preset_filter_t & operator=(preset_filter_t &&) = delete;

    virtual ~preset_filter_t() { };

    preset_filter_t(const pfc::list_base_const_t<metadb_handle_ptr> & list, const char * midiPreset);

    bool apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo) override;

private:
    pfc::string _Preset;
    metadb_handle_list _Handles;
};
