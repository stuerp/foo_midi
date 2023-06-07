#include "MIDIProcessor.h"

#include <string.h>

static inline bool it_equal(std::vector<uint8_t>::const_iterator it1, const char * it2, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (it1[(int)i] != it2[(int)i])
            return false;
    }

    return true;
}

static inline uint32_t toInt32LE(const uint8_t * data)
{
    return static_cast<uint32_t>(data[0]) | static_cast<uint32_t>(data[1] << 8) | static_cast<uint32_t>(data[2] << 16) | static_cast<uint32_t>(data[3] << 24);
}

static inline uint32_t toInt32LE(std::vector<uint8_t>::const_iterator data)
{
    return static_cast<uint32_t>(data[0]) | static_cast<uint32_t>(data[1] << 8) | static_cast<uint32_t>(data[2] << 16) | static_cast<uint32_t>(data[3] << 24);
}

bool MIDIProcessor::IsRIFF(std::vector<uint8_t> const & data)
{
    if (data.size() < 20)
        return false;

    if (::memcmp(&data[0], "RIFF", 4) != 0)
        return false;

    uint32_t Size = (uint32_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));

    if ((Size < 12) || (data.size() < Size + 8))
        return false;

    if (::memcmp(&data[8], "RMID", 4) != 0 || ::memcmp(&data[12], "data", 4) != 0)
        return false;

    uint32_t DataSize = toInt32LE(&data[16]);

    if ((DataSize < 18) || data.size() < DataSize + 20 || Size < DataSize + 12)
        return false;

    std::vector<uint8_t> Data;

    Data.assign(data.begin() + 20, data.begin() + 20 + 18);

    return IsSMF(Data);
}
/*
bool MIDIProcessor::GetTrackCountFromRIFF(std::vector<uint8_t> const & data, size_t & trackCount)
{
    trackCount = 0;

    uint32_t Size = (uint32_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));

    std::vector<uint8_t>::const_iterator it = data.begin() + 12;

    std::vector<uint8_t>::const_iterator body_end = data.begin() + 8 + Size;

    std::vector<uint8_t> extra_buffer;

    while (it != body_end)
    {
        if (body_end - it < 8)
            return false;

        uint32_t ChunkSize = (uint32_t)(it[4] | (it[5] << 8) | (it[6] << 16) | (it[7] << 24));

        if ((uint32_t) (body_end - it) < ChunkSize)
            return false;

        if (it_equal(it, "data", 4))
        {
            std::vector<uint8_t> Data;

            Data.assign(it + 8, it + 8 + ChunkSize);

            return GetTrackCount(Data, trackCount);
        }
        else
        {
            it += ChunkSize;

            if (ChunkSize & 1 && it != body_end)
                ++it;
        }
    }

    return false;
}
*/
static const char * riff_tag_mappings[][2] =
{
    { "IALB", "album" },
    { "IARL", "archival_location" },
    { "IART", "artist" },
    { "ITRK", "tracknumber" },
    { "ICMS", "commissioned" },
    { "ICMP", "composer" },
    { "ICMT", "comment" },
    { "ICOP", "copyright" },
    { "ICRD", "creation_date" },
    { "IENG", "engineer" },
    { "IGNR", "genre" },
    { "IKEY", "keywords" },
    { "IMED", "medium" },
    { "INAM", "title" },
    { "IPRD", "product" },
    { "ISBJ", "subject" },
    { "ISFT", "software" },
    { "ISRC", "source" },
    { "ISRF", "source_form" },
    { "ITCH", "technician" }
};

bool MIDIProcessor::ProcessRIFF(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    uint32_t Size = (uint32_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));

    std::vector<uint8_t>::const_iterator it = data.begin() + 12;

    std::vector<uint8_t>::const_iterator body_end = data.begin() + 8 + Size;

    bool found_data = false;
    bool found_info = false;

    MIDIMetaData MetaData;

    std::vector<uint8_t> extra_buffer;

    while (it != body_end)
    {
        if (body_end - it < 8)
            return false;

        uint32_t chunk_size = toInt32LE(it + 4);

        if ((uint32_t) (body_end - it) < chunk_size)
            return false;

        if (it_equal(it, "data", 4))
        {
            if (found_data)
                return false; /*throw exception_io_data( "Multiple RIFF data chunks found" );*/

            std::vector<uint8_t> Data;

            Data.assign(it + 8, it + 8 + chunk_size);

            if (!ProcessSMF(Data, container))
                return false;

            found_data = true;

            it += 8 + chunk_size; if (chunk_size & 1 && it != body_end) ++it;
        }
        else
        if (it_equal(it, "DISP", 4))
        {
            uint32_t type = toInt32LE(it + 8);

            if (type == 1)
            {
                extra_buffer.resize(chunk_size - 4 + 1);
                std::copy(it + 12, it + 8 + chunk_size, extra_buffer.begin());
                extra_buffer[chunk_size - 4] = '\0';
                MetaData.AddItem(MIDIMetaDataItem(0, "display_name", (const char *) &extra_buffer[0]));
            }

            it += 8 + chunk_size; if (chunk_size & 1 && it != body_end) ++it;
        }
        else
        if (it_equal(it, "LIST", 4))
        {
            std::vector<uint8_t>::const_iterator chunk_end = it + 8 + chunk_size;

            if (it_equal(it + 8, "INFO", 4))
            {
                if (found_info)
                    return false; /*throw exception_io_data( "Multiple RIFF LIST INFO chunks found" );*/

                if (chunk_end - it < 12)
                    return false;

                it += 12;

                while (it != chunk_end)
                {
                    if (chunk_end - it < 4)
                        return false;

                    uint32_t field_size = toInt32LE(it + 4);

                    if ((uint32_t) (chunk_end - it) < 8 + field_size)
                        return false;

                    std::string field;

                    field.assign(it, it + 4);

                    for (size_t i = 0; i < _countof(riff_tag_mappings); ++i)
                    {
                        if (!memcmp(&it[0], riff_tag_mappings[i][0], 4))
                        {
                            field = riff_tag_mappings[i][1];
                            break;
                        }
                    }

                    extra_buffer.resize(field_size + 1);
                    std::copy(it + 8, it + 8 + field_size, extra_buffer.begin());
                    extra_buffer[field_size] = '\0';

                    it += 8 + field_size;
                    MetaData.AddItem(MIDIMetaDataItem(0, field.c_str(), (const char *) &extra_buffer[0]));

                    if (field_size & 1 && it != chunk_end) ++it;
                }

                found_info = true;
            }
            else
                return false; /* unknown LIST chunk */

            it = chunk_end;
            if (chunk_size & 1 && it != body_end) ++it;
        }
        else
        {
            it += chunk_size;
            if (chunk_size & 1 && it != body_end) ++it;
        }

        if (found_data && found_info)
            break;
    }

    container.SetExtraMetaData(MetaData);

    return true;
}
