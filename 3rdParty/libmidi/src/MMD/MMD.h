
/** $VER: MMD.h (2026.05.08) P. Stuer - Based on Valley Bell's mmd2mid (https://github.com/ValleyBell/MidiConverters). **/

#pragma once

#include <cstdint>
#include <vector>

#include "Stream.h"

namespace mmd
{

struct track_t
{
    track_t() : Offset(), Length(), LoopOffset(), LoopLength(), MaxLoopExpansions(), Transpose(), Channel() {}

    uint32_t Offset;
    uint32_t Length;

    uint32_t LoopOffset;
    uint32_t LoopLength;
    uint16_t MaxLoopExpansions;     // Number of times to take the loops of this track

    int8_t Transpose;
    uint8_t Channel;
};

class converter_t
{
public:
    converter_t() : _Tempo(), _Transpose(), _Title(), _SysEx() { }

    bool ToSMF(const uint8_t * srcData, uint32_t srcSize, std::vector<uint8_t> & dstData) noexcept;

private:
    static void AdjustTracks(std::vector<track_t> & tracks, uint32_t minLoopTicks) noexcept;

    bool GetLoops(const uint8_t * data, uint32_t size, track_t & track) noexcept;
    bool ConvertTrack(const uint8_t * data, uint32_t size, track_t & track, stream_t & ms, uint8_t trackNumber) noexcept;

public:
    uint8_t _Tempo;
    int8_t _Transpose;
    const char * _Title;
    const uint8_t * _SysEx[8];
};

}
