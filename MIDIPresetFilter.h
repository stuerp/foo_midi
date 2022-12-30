
/** $VER: MIDIPresetFilter.h (2022.12.30) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <sdk/file_info_filter.h>

#include "Fields.h"

class midi_preset_filter : public file_info_filter
{
public:
    midi_preset_filter(const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const char * p_midi_preset)
    {
        m_midi_preset = p_midi_preset ? p_midi_preset : "";

        pfc::array_t<t_size> order;

        order.set_size(p_list.get_count());

        order_helper::g_fill(order.get_ptr(), order.get_size());

        p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, order.get_ptr());

        m_handles.set_count(order.get_size());

        for (t_size n = 0; n < order.get_size(); ++n)
        {
            m_handles[n] = p_list[order[n]];
        }
    }

    virtual bool apply_filter(metadb_handle_ptr p_location, t_filestats, file_info & p_info)
    {
        t_size index;
        if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, p_location, index))
        {
            if (m_midi_preset.get_length())
                p_info.info_set(field_preset, m_midi_preset);
            else
                p_info.info_remove(field_preset);
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    pfc::string8 m_midi_preset;

    metadb_handle_list m_handles;
};
