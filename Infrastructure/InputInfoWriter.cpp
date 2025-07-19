 
/** $VER: InputInfoWriter.cpp (2025.07.15) **/

#include "pch.h"

#include "InputDecoder.h"

/// <summary>
/// Sets the tags for the specified file.
/// </summary>
void InputDecoder::retag_set_info(t_uint32, const file_info & fileInfo, abort_callback & abortHandler) const
{
    if (_IsSysExFile)
        throw pfc::exception("You cannot tag SysEx files.");

    file_info_impl fi(fileInfo);

    // Move the SysEx dumps from the information field to a tag.
    {
        // Remove all instances of the tag.
        fi.meta_remove_field(TagMIDISysExDumps);

        // Get the value from the information field.
        const char * SysExDumps = fi.info_get(TagMIDISysExDumps);

        // Add the tag.
        if (SysExDumps != nullptr)
            fi.meta_set(TagMIDISysExDumps, SysExDumps);
    }

    // Update the metadb.
    {
        file::ptr Stream;

        filesystem::g_open_tempmem(Stream, abortHandler);

        tag_processor::write_apev2(Stream, fi, abortHandler);

        Stream->seek(0, abortHandler);

        pfc::array_t<t_uint8> Tags;

        Tags.set_count((t_size) Stream->get_size_ex(abortHandler));

        Stream->read_object(Tags.get_ptr(), Tags.get_count(), abortHandler);

        static_api_ptr_t<metadb_index_manager>()->set_user_data(GUIDTagMIDIHash, _Hash, Tags.get_ptr(), Tags.get_count());
    }
}
