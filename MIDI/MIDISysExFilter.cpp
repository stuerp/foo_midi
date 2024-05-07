
/** $VER: MIDISysExFilter.cpp (2023.06.12) **/

#include "framework.h"

#include "MIDISysExFilter.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
MIDISysExFilter::MIDISysExFilter(const pfc::list_base_const_t<metadb_handle_ptr> & list, const MIDISysExDumps & sysexDumps)
{
    _SysExDumps = sysexDumps;

    pfc::array_t<t_size> Order;

    {
        Order.set_size(list.get_count());

        order_helper::g_fill(Order.get_ptr(), Order.get_size());

        list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, Order.get_ptr());
    }

    {
        _Handles.set_count(Order.get_size());

        for (t_size n = 0; n < Order.get_size(); ++n)
            _Handles[n] = list[Order[n]];
    }
}

bool MIDISysExFilter::apply_filter(metadb_handle_ptr location, t_filestats, file_info & fileInfo)
{
    pfc::string8 FileExtension = pfc::string_extension(location->get_path());

    for (size_t i = 0; i < _SysExFileExtensionCount; ++i)
    {
        if (pfc::stricmp_ascii(FileExtension, _SysExFileExtensions[i]) == 0)
            return false;
    }

    t_size Index;

    if (!_Handles.bsearch_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, location, Index))
        return false;

    pfc::string8 Text;

    _SysExDumps.Serialize(location->get_path(), Text);

    if (Text.get_length())
        fileInfo.info_set(TagMIDISysExDumps, Text);
    else
        fileInfo.info_remove(TagMIDISysExDumps);

    return true;
}
