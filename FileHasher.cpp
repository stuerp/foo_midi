
/** $VER: FileHasher.cpp (2023.01.03) **/

#pragma warning(disable: 5045 26481 26485)

#include "FileHasher.h"

metadb_index_hash FileHasher::transform(const file_info & fileInfo, const playable_location & location)
{
    const metadb_index_hash hash_null = 0;

    if (!IsMIDIFileExtension(pfc::string_extension(location.get_path())))
        return hash_null;

    hasher_md5_state HasherState;
    static_api_ptr_t<hasher_md5> Hasher;

    t_uint32 SubsongIndex = location.get_subsong();

    Hasher->initialize(HasherState);

    Hasher->process(HasherState, &SubsongIndex, sizeof(SubsongIndex));

    const char * Info = fileInfo.info_get(TagMIDIHash);

    if (Info)
        Hasher->process_string(HasherState, Info);
    else
        Hasher->process_string(HasherState, location.get_path());

#define HASH_STRING(s)  Info = fileInfo.info_get(s); if (Info) Hasher->process_string(HasherState, Info);

    HASH_STRING(TagMIDIFormat);
    HASH_STRING(TagMIDITrackCount);
    HASH_STRING(TagMIDIChannelCount);
    HASH_STRING(TagMIDITicks);
    HASH_STRING(TagMIDIType);
    HASH_STRING(TagMIDILoopStart);
    HASH_STRING(TagMIDILoopEnd);
    HASH_STRING(TagMIDILoopStartInMs);
    HASH_STRING(TagMIDILoopEndInMs);

    return from_md5(Hasher->get_result(HasherState));
}
