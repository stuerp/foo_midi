
/** $VER: RIFFWriter.cpp (2025.09.06) P. Stuer **/

#include "pch.h"

#include "RIFFWriter.h"
#include "Exception.h"

namespace riff
{

bool writer_t::Open(msc::stream_t * stream, Options options)
{
    _Stream = stream;
    _Options = options;

    return true;
}

void writer_t::Close() noexcept
{
    if (_Stream != nullptr)
    {
        _Stream->Close();
        _Stream = nullptr;
    }
}

}
