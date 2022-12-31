
/** $VER: MIDIPresetFilter.cpp (2022.12.31) **/

#pragma warning(disable: 26446 26481 26493)

#include "MIDIPresetFilter.h"

MIDIPresetFilter::MIDIPresetFilter(const pfc::list_base_const_t<metadb_handle_ptr> & list, const char * midiPreset)
{
    _MIDIPreset = midiPreset ? midiPreset : "";

    pfc::array_t<t_size> order;

    order.set_size(list.get_count());

    order_helper::g_fill(order.get_ptr(), order.get_size());

    list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, order.get_ptr());

    _Handles.set_count(order.get_size());

    for (t_size i = 0; i < order.get_size(); ++i)
        _Handles[i] = list[order[i]];
}
