
/** $VER: MIDISyxFilter.h (2022.12.30) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <sdk/file_info_filter.h>

#include "MIDISysExDumps.h"

class midi_syx_filter : public file_info_filter
{
    MIDISysExDumps m_dumps;

    metadb_handle_list m_handles;

public:
    midi_syx_filter(const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const MIDISysExDumps & p_dumps)
    {
        m_dumps = p_dumps;

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
        pfc::string8 m_ext = pfc::string_extension(p_location->get_path());

        for (unsigned j = 0, k = _countof(_SyxExtension); j < k; ++j)
        {
            if (!pfc::stricmp_ascii(m_ext, _SyxExtension[j]))
                return false;
        }

        t_size index;

        if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, p_location, index))
        {
            pfc::string8 m_dump_serialized;

            m_dumps.serialize(p_location->get_path(), m_dump_serialized);

            if (m_dump_serialized.get_length())
                p_info.info_set(field_syx, m_dump_serialized);
            else
                p_info.info_remove(field_syx);

            return true;
        }
        else
        {
            return false;
        }
    }
};
