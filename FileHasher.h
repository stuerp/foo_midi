
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
