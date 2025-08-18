
/** $VER: RIFFWriter.h (2025.04.30) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>

#include "RIFF.h"
#include "FOURCC.h"

#include "Stream.h"
#include "Exception.h"

namespace riff
{

#pragma warning(disable: 4820)

/// <summary>
/// Implements a writer for a RIFF file.
/// </summary>
class writer_t
{
public:
    writer_t() noexcept : _Stream(), _Options()
    {
    }

    virtual ~writer_t()
    {
    }

    enum option_t
    {
        None = 0,

        OnlyMandatory       = 0x0001,
        PolyphoneCompatible = 0x0002,   // Writes a SoundFont file that can be read by Polyphone but that adheres not strictly to the SoundFont 2.04 specification.
    };

    bool Open(stream_t * stream, option_t options);

    virtual void Close() noexcept;

    template <typename T> uint32_t WriteChunks(uint32_t chunkId, uint32_t listType, T&& writeChunks);
    template <typename T> uint32_t WriteChunk(uint32_t chunkId, T&& writeChunk);

    /// <summary>
    /// Write a number of bytes.
    /// </summary>
    virtual uint32_t Write(const void * data, uint32_t size)
    {
        _Stream->Write(data, size);

        return size;
    }

    /// <summary>
    /// Writes a value.
    /// </summary>
    template <typename T> void Write(const T data)
    {
        _Stream->Write(&data, sizeof(T));
    }

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint32_t size)
    {
        _Stream->Skip(size);
    }

    /// <summary>
    /// Gets the current offset.
    /// </summary>
    virtual uint32_t GetOffset()
    {
        return (uint32_t) _Stream->Offset();
    }

    /// <summary>
    /// Move to the specified offset.
    /// </summary>
    virtual void SetOffset(uint32_t size)
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
    stream_t * _Stream;
    option_t _Options;

    struct marker_t
    {
        uint32_t Offset;
        uint32_t Size;
    };

    std::vector<marker_t> _Markers;
};

/// <summary>
/// Writes a chunk that can contain other chunks.
/// </summary>
template <typename T> uint32_t writer_t::WriteChunks(uint32_t chunkId, uint32_t listType, T&& writeChunks)
{
    _Markers.push_back({ GetOffset() + sizeof(uint32_t), 0 });

    size_t Index = _Markers.size() - 1;

    const uint32_t Header[] = { chunkId, 0, listType };

    Write(Header, sizeof(Header));

    uint32_t ChunkSize = sizeof(listType) + writeChunks();

    _Markers[Index].Size = ChunkSize;

    return sizeof(chunk_header_t) + ChunkSize;
}


/// <summary>
/// Writes a chunk.
/// </summary>
template <typename T> uint32_t writer_t::WriteChunk(uint32_t chunkId, T&& writeChunk)
{
    _Markers.push_back({ GetOffset() + sizeof(uint32_t), 0 });

    size_t Index = _Markers.size() - 1;

    const uint32_t Header[] = { chunkId, 0 };

    Write(Header, sizeof(Header));

    uint32_t ChunkSize = writeChunk();

    _Markers[Index].Size = ChunkSize;

    // Hack: Polyphone requires the chunk size to be even.
    if (_Options & option_t::PolyphoneCompatible)
        _Markers[Index].Size += (ChunkSize & 1);

    if (ChunkSize & 1)
    {
        constexpr const uint8_t Filler = 0;

        _Stream->Write(&Filler, sizeof(Filler));

        ++ChunkSize;
    }

    return sizeof(Header) + ChunkSize;
}

}
