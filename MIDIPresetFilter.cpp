
/** $VER: MIDIPresetFilter.cpp (2023.01.04) **/

#pragma warning(disable: 26446 26481 26493)

#include "MIDIPresetFilter.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
MIDIPresetFilter::MIDIPresetFilter(const pfc::list_base_const_t<metadb_handle_ptr> & list, const char * midiPreset)
{
    _MIDIPreset = midiPreset ? midiPreset : "";

    pfc::array_t<t_size> Order;

    Order.set_size(list.get_count());

    order_helper::g_fill(Order.get_ptr(), Order.get_size());

    list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, Order.get_ptr());

    _Handles.set_count(Order.get_size());

    for (size_t i = 0; i < Order.get_size(); ++i)
        _Handles[i] = list[Order[i]];
}

bool MIDIPresetFilter::apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo)
{
    t_size index;

    if (_Handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, location, index))
    {
        if (_MIDIPreset.get_length())
            fileInfo.info_set(TagMIDIPreset, _MIDIPreset);
        else
            fileInfo.info_remove(TagMIDIPreset);

        return true;
    }

    return false;
}
