
/** $VER: RIFF.h (2025.04.23) P. Stuer **/

#pragma once

#include <stdint.h>

namespace riff
{

struct chunk_header_t
{
    uint32_t Id;
    uint32_t Size;
};

}
