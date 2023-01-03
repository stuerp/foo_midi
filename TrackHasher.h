
/** $VER: TrackHasher.h (2023.01.02) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/file_info.h>
#include <sdk/playable_location.h>

#include "Configuration.h"

class TrackHasher : public metadb_index_client
{
public:
    TrackHasher() noexcept { };

    TrackHasher(const TrackHasher&) = delete;
    TrackHasher(const TrackHasher&&) = delete;
    TrackHasher& operator=(const TrackHasher&) = delete;
    TrackHasher& operator=(TrackHasher&&) = delete;

    virtual ~TrackHasher() { };

    virtual metadb_index_hash transform(const file_info & fileInfo, const playable_location & location)
    {
        const metadb_index_hash hash_null = 0;

        if (!IsMIDIFileExtension(pfc::string_extension(location.get_path())))
            return hash_null;

        hasher_md5_state HasherState;
        static_api_ptr_t<hasher_md5> Hasher;

        t_uint32 SubsongIndex = location.get_subsong();

        Hasher->initialize(HasherState);

        Hasher->process(HasherState, &SubsongIndex, sizeof(SubsongIndex));

        const char * Info = fileInfo.info_get(MetaDataHash);

        if (Info)
            Hasher->process_string(HasherState, Info);
        else
            Hasher->process_string(HasherState, location.get_path());

    #define HASH_STRING(s)  Info = fileInfo.info_get(s); if (Info) Hasher->process_string(HasherState, Info);

        HASH_STRING(MetaDataFormat);
        HASH_STRING(MetaDataTracks);
        HASH_STRING(MetaDataChannels);
        HASH_STRING(MetaDataTicks);
        HASH_STRING(MetaDataType);
        HASH_STRING(MetaDataLoopStart);
        HASH_STRING(MetaDataLoopEnd);
        HASH_STRING(MetaDataLoopStartInMS);
        HASH_STRING(MetaDataLoopEndInMS);

        return from_md5(Hasher->get_result(HasherState));
    }
};

const GUID GUIDTrackHasher = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };
