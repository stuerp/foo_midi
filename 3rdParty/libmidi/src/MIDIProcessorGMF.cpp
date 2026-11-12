
/** $VER: MIDIProcessorGMF.cpp (2026.05.20) Game Music Format (http://www.vgmpf.com/Wiki/index.php?title=GMF) **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "SysEx.h"

namespace midi
{

bool processor_t::IsGMF(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 32)
        return false;

    if (data[0] != 'G' || data[1] != 'M' || data[2] != 'F' || data[3] != 1)
        return false;

    return true;
}

bool processor_t::ProcessGMF(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::GMF;

    container.Initialize(0, 0xC0);

    // Add a director's track.
    {
        track_t Track;

        const uint16_t Tempo = (uint16_t) (((uint16_t) data[4] << 8) | data[5]);
        const uint32_t ScaledTempo = (uint32_t) Tempo * 100'000;

        uint8_t Data[5] = { StatusCode::MetaData, MetaDataType::SetTempo, (uint8_t) (ScaledTempo >> 16), (uint8_t) (ScaledTempo >> 8), (uint8_t) ScaledTempo };

        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, 5));

        Track.AddEvent(event_t(0, event_t::Extended, 0, sysex_t::MT32Reset, _countof(sysex_t::MT32Reset)));

        Data[0] = StatusCode::MetaData;
        Data[1] = MetaDataType::EndOfTrack;

        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, 2));

        container.AddTrack(Track);
    }

    auto it = data.begin() + 7;

    return ProcessSMFTrack(it, data.end(), container);
}

}
