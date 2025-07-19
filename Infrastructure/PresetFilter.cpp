
/** $VER: PresetFilter.cpp (2025.06.22) **/

#include "pch.h"

#include "PresetFilter.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
preset_filter_t::preset_filter_t(const pfc::list_base_const_t<metadb_handle_ptr> & itemList, const char * midiPreset)
{
    if (midiPreset != nullptr)
        _Preset = midiPreset;

    pfc::array_t<t_size> Order;

    {
        Order.set_size(itemList.get_count());

        order_helper::g_fill(Order.get_ptr(), Order.get_size());

        itemList.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, Order.get_ptr());
    }

    {
        _Handles.set_count(Order.get_size());

        for (size_t i = 0; i < Order.get_size(); ++i)
            _Handles[i] = itemList[Order[i]];
    }
}

/// <summary>
/// Called by metadb_io_v2::update_info_async().
/// </summary>
bool preset_filter_t::apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo)
{
    // Find the database entry.
    t_size index;

    if (!_Handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, location, index))
        return false;

    if (!_Preset.empty())
        fileInfo.meta_set(TagPreset, _Preset);
    else
        fileInfo.meta_remove_field(TagPreset);

    return true;
}
