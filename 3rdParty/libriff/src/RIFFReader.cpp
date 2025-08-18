
/** $VER: RIFFReader.cpp (2025.04.30) P. Stuer **/

#include "pch.h"

#include "RIFFReader.h"

namespace riff
{

bool reader_t::Open(stream_t * stream, option_t options)
{
    _Stream = stream;
    _Options = options;

    return true;
}

void reader_t::Close() noexcept
{
    if (_Stream != nullptr)
    {
        _Stream->Close();
        _Stream = nullptr;
    }
}

bool reader_t::ReadHeader(uint32_t & formType)
{
    Read(&_Header, sizeof(_Header));

    if ((_Header.Id != FOURCC_RIFF) || (_Header.Size < sizeof(formType)))
        throw riff::exception("Invalid RIFF chunk");

    Read(&formType, sizeof(formType));

    return true;
}

}
