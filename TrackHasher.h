
/** $VER: TrackHasher.h (2022.12.31) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <sdk/metadb_index.h>
#include <sdk/hasher_md5.h>

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

    virtual metadb_index_hash transform(const file_info & info, const playable_location & location)
    {
        const metadb_index_hash hash_null = 0;

        if (!IsFileExtensionSupported(pfc::string_extension(location.get_path())))
            return hash_null;

        hasher_md5_state hasher_state;
        static_api_ptr_t<hasher_md5> hasher;

        t_uint32 subsong = location.get_subsong();

        hasher->initialize(hasher_state);

        hasher->process(hasher_state, &subsong, sizeof(subsong));

        const char * str = info.info_get(field_hash);

        if (str)
            hasher->process_string(hasher_state, str);
        else
            hasher->process_string(hasher_state, location.get_path());

    #define HASH_STRING(s)      \
	str = info.info_get(s); \
	if(str) hasher->process_string(hasher_state, str);

        HASH_STRING(field_format);
        HASH_STRING(field_tracks);
        HASH_STRING(field_channels);
        HASH_STRING(field_ticks);
        HASH_STRING(field_type);
        HASH_STRING(field_loop_start);
        HASH_STRING(field_loop_end);
        HASH_STRING(field_loop_start_ms);
        HASH_STRING(field_loop_end_ms);

        return from_md5(hasher->get_result(hasher_state));
    }
};

const GUID GUIDTrackHasher = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };
