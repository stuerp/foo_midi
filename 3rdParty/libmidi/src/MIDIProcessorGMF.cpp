
/** $VER: MIDIProcessorGMF.cpp (2025.07.22) Game Music Format (http://www.vgmpf.com/Wiki/index.php?title=GMF) **/

#include "pch.h"

#include "MIDIProcessor.h"

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
        uint16_t Tempo = (uint16_t) (((uint16_t) data[4] << 8) | data[5]);
        uint32_t ScaledTempo = (uint32_t) Tempo * 100000;

        track_t Track;

        uint8_t Data[10] = { StatusCode::MetaData, MetaDataType::SetTempo, (uint8_t) (ScaledTempo >> 16), (uint8_t) (ScaledTempo >>  8), (uint8_t)  ScaledTempo };

        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, 5));

        // Roland MT-32 Owner's Manual: Reset all MT-32 parameters.
        Data[0] = StatusCode::SysEx;
        Data[1] = 0x41; // Roland manufacturer Id
        Data[2] = 0x10; // Device Id
        Data[3] = 0x16; // Model Id (MT-32)
        Data[4] = 0x12; // Command Id: Send (DT1)
        Data[5] = 0x7F; // All parameter reset
        Data[6] = 0x00;
        Data[7] = 0x00;
        Data[8] = 0x01; // Checksum
        Data[9] = StatusCode::SysExEnd;

        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, 10));

        Data[0] = StatusCode::MetaData;
        Data[1] = MetaDataType::EndOfTrack;

        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, 2));

        container.AddTrack(Track);
    }

    auto it = data.begin() + 7;

    return ProcessSMFTrack(it, data.end(), container);
}

}
