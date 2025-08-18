
/** $VER: RIFFReader.h (2025.04.30) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>

#include "RIFF.h"
#include "FOURCC.h"

#include "Stream.h"
#include "Encoding.h"
#include "Exception.h"

namespace riff
{

#pragma warning(disable: 4820)

/// <summary>
/// Implements a reader for a RIFF file.
/// </summary>
class reader_t
{
public:
    reader_t() noexcept : _Stream(), _Options(), _Header()
    {
    }

    virtual ~reader_t()
    {
    }

    enum option_t
    {
        None = 0,
        OnlyMandatory = 1
    };

    bool Open(stream_t * stream, option_t options);

    virtual void Close() noexcept;

    virtual bool ReadHeader(uint32_t & formType);

    template <typename T> bool ReadChunks(T&& processChunk);
    template <typename T> bool ReadChunks(uint32_t chunkSize, T&& processChunk);

    /// <summary>
    /// Reads a value.
    /// </summary>
    template <typename T> void Read(T & data)
    {
        _Stream->Read(&data, sizeof(T));
    }

    /// <summary>
    /// Reads a number of bytes into a buffer.
    /// </summary>
    virtual void Read(void * data, uint32_t size)
    {
        _Stream->Read(data, size);
    }

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint32_t size)
    {
        _Stream->Skip(size);
    }

    /// <summary>
    /// Move to the specified offset.
    /// </summary>
    virtual void Offset(uint32_t size)
    {
        _Stream->Offset(size);
    }

    /// <summary>
    /// Skips the current chunk.
    /// </summary>
    virtual void SkipChunk(const chunk_header_t & ch)
    {
        Skip(ch.Size);
    }

protected:
    virtual bool HasMandatory() noexcept
    {
        return false;
    }

protected:
    stream_t * _Stream;
    option_t _Options;
    chunk_header_t _Header;
};

/// <summary>
/// Reads chunks.
/// </summary>
template <typename T> bool reader_t::ReadChunks(T&& readChunk)
{
    return ReadChunks(_Header.Size - sizeof(_Header.Id), readChunk);
}

/// <summary>
/// Reads chunks.
/// </summary>
template <typename T> bool reader_t::ReadChunks(uint32_t chunkSize, T&& readChunk)
{
    while (chunkSize != 0)
    {
        chunk_header_t ch;

        {
            if (chunkSize < sizeof(ch))
                throw exception(FormatText("Failed to read chunk header: need %u bytes, have %u bytes", sizeof(ch), chunkSize));

            Read(&ch, sizeof(ch));
            chunkSize -= sizeof(ch);
        }

        {
            if (chunkSize < ch.Size)
                throw exception(FormatText("Failed to read chunk data: need %u bytes, have %u bytes", ch.Size, chunkSize));

            readChunk(ch);
            chunkSize -= ch.Size;

            if (ch.Size & 1)
            {
                if (chunkSize < 1)
                    throw exception("Failed to skip pad byte: insufficient data");

                Skip(1);
                --chunkSize;
            }
        }
    }

    return true;
}

}
