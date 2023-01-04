
/** $VER: FileHasher.h (2023.01.03) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/file_info.h>
#include <sdk/playable_location.h>

#include "Configuration.h"

class FileHasher : public metadb_index_client
{
public:
    FileHasher() noexcept { };

    FileHasher(const FileHasher&) = delete;
    FileHasher(const FileHasher&&) = delete;
    FileHasher& operator=(const FileHasher&) = delete;
    FileHasher& operator=(FileHasher&&) = delete;

    virtual ~FileHasher() { };

    metadb_index_hash transform(const file_info & fileInfo, const playable_location & location) override;
};

const GUID GUIDTagMIDIHash = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };
