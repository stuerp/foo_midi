
/** $VER: Stream.h (2025.09.06) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>

#include "Exception.h"

#pragma warning(disable: 4820)

namespace msc
{

/// <summary>
/// Implements a stream.
/// </summary>
class stream_t
{
public:
    virtual ~stream_t() { }

    virtual void Close() noexcept = 0;

    /// <summary>
    /// Reads a number of bytes.
    /// </summary>
    virtual void Read(void * data, uint64_t size) = 0;

    /// <summary>
    /// Writes a number of bytes.
    /// </summary>
    virtual void Write(const void * data, uint64_t size) = 0;

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint64_t size) = 0;

    /// <summary>
    /// Gets the current offset.
    /// </summary>
    virtual uint64_t Offset() const = 0;

    /// <summary>
    /// Moves to the specified offset.
    /// </summary>
    virtual void Offset(uint64_t size) = 0;
};

/// <summary>
/// Implements a stream.
/// </summary>
class file_stream_t : public stream_t
{
public:
    file_stream_t() noexcept : _hFile(INVALID_HANDLE_VALUE)
    {
    }

    virtual ~file_stream_t()
    {
        Close();
    }

    bool Open(const fs::path & filePath, bool forWriting = false);

    virtual void Close() noexcept;
    virtual void Read(void * data, uint64_t size);
    virtual void Write(const void * data, uint64_t size);
    virtual void Skip(uint64_t size);
    virtual uint64_t Offset() const;
    virtual void Offset(uint64_t size);

protected:
    HANDLE _hFile;
};

/// <summary>
/// Implements a stream.
/// </summary>
class memory_stream_t : public stream_t
{
public:
    memory_stream_t() noexcept : _hFile(INVALID_HANDLE_VALUE), _hMap(), _Data(), _Curr(), _Tail()
    {
    }

    bool Open(const fs::path & filePath, uint64_t offset, uint64_t size, bool forWriting = false);

    bool Open(const uint8_t * data, uint64_t size);

    virtual void Close() noexcept;

    virtual void Read(void * data, uint64_t size)
    {
        if (_Curr + (ptrdiff_t) size > _Tail)
            throw exception("Insufficient data");

        ::memcpy(data, _Curr, (size_t) size);
        _Curr += (ptrdiff_t) size;
    }

    virtual void Write(const void * data, uint64_t size)
    {
        if (_Curr + (ptrdiff_t) size > _Tail)
            throw exception("Insufficient data");

        ::memcpy(_Curr, data, (size_t) size);
        _Curr += (ptrdiff_t) size;
    }

    /// <summary>
    /// Skips the specified number of bytes.
    /// </summary>
    virtual void Skip(uint64_t size)
    {
        if (_Curr + (ptrdiff_t) size > _Tail)
            throw exception("Insufficient data");

        _Curr += (ptrdiff_t) size;
    }

    /// <summary>
    /// Moves to the specified offset.
    /// </summary>
    virtual uint64_t Offset() const
    {
        return (uint64_t) (_Curr - _Data);
    }

    /// <summary>
    /// Moves to the specified offset.
    /// </summary>
    virtual void Offset(uint64_t size)
    {
        if (_Data + (ptrdiff_t) size > _Tail)
            throw exception("Invalid offset");

        _Curr = _Data + (ptrdiff_t) size;
    }

protected:
    HANDLE _hFile;
    HANDLE _hMap;

    uint8_t * _Data;
    uint8_t * _Curr;
    uint8_t * _Tail;
};

}
