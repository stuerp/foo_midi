
/** $VER: MIDIPresetFilter.cpp (2023.06.04) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPresetFilter.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
MIDIPresetFilter::MIDIPresetFilter(const pfc::list_base_const_t<metadb_handle_ptr> & itemList, const char * midiPreset)
{
    _MIDIPreset = midiPreset ? midiPreset : "";

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
bool MIDIPresetFilter::apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo)
{
    t_size index;

    // Find the database entry.
    if (!_Handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, location, index))
        return false;

    if (_MIDIPreset.get_length())
        fileInfo.info_set(TagMIDIPreset, _MIDIPreset);
    else
        fileInfo.info_remove(TagMIDIPreset);

    return true;
}
