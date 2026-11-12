
/** $VER: MIDIProcessorRMI.cpp (2025.09.07) **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "Encoding.h"
#include "Exception.h"

#include <map>

namespace midi
{

static const std::map<const std::string, const std::string> RIFFToTagMap =
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
    { "IENC", "encoding" },
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

static inline uint32_t toInt32LE(const uint8_t * data)
{
    return static_cast<uint32_t>(data[0]) | static_cast<uint32_t>(data[1] << 8) | static_cast<uint32_t>(data[2] << 16) | static_cast<uint32_t>(data[3] << 24);
}

static inline uint32_t toInt32LE(std::vector<uint8_t>::const_iterator data)
{
    return static_cast<uint32_t>(data[0]) | static_cast<uint32_t>(data[1] << 8) | static_cast<uint32_t>(data[2] << 16) | static_cast<uint32_t>(data[3] << 24);
}

static bool ProcessList(std::vector<uint8_t>::const_iterator data, ptrdiff_t size, container_t & container, metadata_table_t & MetaData) noexcept;
static bool GetCodePage(std::vector<uint8_t>::const_iterator data, ptrdiff_t size, uint32_t & codePage) noexcept;

/// <summary>
/// Returns true if the data contains a RIFF file.
/// </summary>
bool processor_t::IsRMI(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 20)
        return false;

    if (::memcmp(data.data(), "RIFF", 4) != 0)
        return false;

    uint32_t Size = toInt32LE(&data[4]);

    if ((Size < 12) || (data.size() < (size_t) Size + 8))
        return false;

    if (::memcmp(&data[8], "RMID", 4) != 0 || ::memcmp(&data[12], "data", 4) != 0)
        return false;

    uint32_t DataSize = toInt32LE(&data[16]);

    if ((DataSize < 18) || (data.size() < (size_t) DataSize + 20) || (Size < DataSize + 12))
        return false;

    std::vector<uint8_t> Data(data.begin() + 20, data.begin() + 20 + 18);

    return IsSMF(Data);
}

/// <summary>
/// Processes the data as an RIFF file and returns an intialized container.
/// </summary>
bool processor_t::ProcessRMI(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::RMI;

    bool HasDataChunk = false;
    bool HasINFOChunk = false;
    bool IsDLS = false;

    metadata_table_t MetaData;

    uint32_t Size = toInt32LE(&data[4]);

    if ((Size < 8) || (data.size() < (size_t) Size + 8))
        throw midi::exception("Insufficient RIFF data");

    const auto Tail = data.begin() + (ptrdiff_t) (8 + Size);

    auto it = data.begin() + 12; // Skip past "RIFF" <size> "RMID".

    while (it < Tail)
    {
        if (Tail - it < 8)
            break; // throw midi::exception("Insufficient RIFF data"); // 02 - Rock.rmi is malformed.

        const ptrdiff_t ChunkSize = (ptrdiff_t) toInt32LE(it + 4);

        if ((Tail - it) < ChunkSize)
            throw midi::exception("Insufficient RIFF data");

        const std::string ChunkId(it, it + 4);

        // Is it a "data" chunk?
        if (ChunkId == "data")
        {
            if (HasDataChunk)
                throw midi::exception("Multiple RIFF data chunks found");

            std::vector<uint8_t> Data(it + 8, it + 8 + ChunkSize);

            if (!ProcessSMF(Data, container))
                return false;

            HasDataChunk = true;
        }
        else
        // Is it a "DISP" chunk?
        if (ChunkId == "DISP")
        {
            const uint32_t Type = toInt32LE(it + 8);

            if (Type == CF_TEXT)
            {
                std::string DisplayName(it + 12, it + 12 + (ChunkSize - 4));

                MetaData.AddItem(metadata_item_t(0, "display_name", DisplayName.c_str()));
            }
        }
        else
        // Is it a "LIST" chunk?
        if (ChunkId == "LIST")
        {
            // Is it a "INFO" chunk?
            if ((::memcmp(&it[8], "INFO", 4) == 0) && !HasINFOChunk)
                HasINFOChunk = ProcessList(it + 12, ChunkSize - 4, container, MetaData);
        }
        else
        // Is it a "RIFF" chunk? According to the standard this should not be possible but it is how embedded soundfonts are implemented. Sloppy design...
        if (ChunkId == "RIFF")
        {
            const auto ChunkTail = it + 8 + ChunkSize;

            IsDLS = (::memcmp(&it[8], "DLS ", 4) == 0);

            if (IsDLS || (::memcmp(&it[8], "sfbk", 4) == 0) || (::memcmp(&it[8], "sfpk", 4) == 0))
            {
                std::vector<uint8_t> Data(it, ChunkTail);

                container.SoundFont = Data;
            }
        }
#ifdef _DEBUG
        else
        {
            ::OutputDebugStringW(msc::FormatText(L"Unknown chunk \"%s\", %zu bytes\n", ChunkId.c_str(), ChunkSize).c_str());
        }
#endif
        it += (ptrdiff_t) 8 + ChunkSize;

        if ((ChunkSize & 1) && (it < Tail))
            ++it;
    }

    // If an embedded DLS soundfont was found: assume bank offset 0 unless any bank change (CC0) is detected to a bank that is not 0 and not 127 (Drums).
    if (IsDLS)
    {
        container.BankOffset = 0;

        for (const auto & Track : container)
        {
            for (const auto & Event : Track)
            {
                if ((Event.Type == event_t::ControlChange) && (Event.Data[0] == midi::BankSelect) && (Event.Data[1] != 0) && (Event.Data[1] != 127))
                {
                    container.BankOffset = 1;
                    break;
                }
            }
        }
    }

    container.SetExtraMetaData(MetaData);

    return true;
}

/// <summary>
/// Processes a RIFF LIST chunk and update the metadata with it.
/// </summary>
bool ProcessList(std::vector<uint8_t>::const_iterator data, ptrdiff_t chunkSize, container_t & container, metadata_table_t & MetaData) noexcept
{
    // Determine which code page to use before we encounter any text chunks.
    uint32_t CodePage = ~0u;

    GetCodePage(data, chunkSize, CodePage);

    // Process all chunks in the list.
    bool FoundIALBChunk = false;
    std::string ProductName;

    const auto Tail = data + chunkSize;

    while (data != Tail)
    {
        if (Tail - data < 4)
            return false;

        const ptrdiff_t ChunkSize = (ptrdiff_t) toInt32LE(data + 4);

        if ((Tail - data) < ((ptrdiff_t) 8 + ChunkSize))
            return false;

        const std::string ChunkId(data, data + 4);
        const auto ChunkData = data + 8;

        if (ChunkId == "IENC")
        {
            // Skip
        }
        else
        if (ChunkId == "IPIC")
        {
            container.SetArtwork(std::vector<uint8_t>(ChunkData, ChunkData + ChunkSize));
        }
        else
        if (ChunkId == "DBNK")
        {
            if (ChunkSize == 2)
                container.BankOffset = std::clamp((ChunkData[1] << 8) | ChunkData[0], 0, 127);
        }
        else
        {
            if (ChunkId == "IALB")
                FoundIALBChunk = true;

            std::string Text;

            if (CodePage != ~0u)
                Text = msc::CodePageToUTF8(CodePage, (const char *) &ChunkData[0], (size_t) ChunkSize);
            else
                Text.assign(ChunkData, ChunkData + ChunkSize);

            if (ChunkId == "IPRD")
                ProductName = Text;

            const auto Item = RIFFToTagMap.find(ChunkId);

            const std::string TagName = (Item != RIFFToTagMap.end()) ? Item->second : ChunkId;

            MetaData.AddItem(metadata_item_t(0, TagName.c_str(), Text.c_str()));
        }

        data += (ptrdiff_t) 8 + ChunkSize;

        if ((ChunkSize & 1) && (data < Tail))
            ++data;
    }

    // Use the product name also as album name if no IALB chunk was found in the INFO list.
    if (!FoundIALBChunk && !ProductName.empty())
        MetaData.AddItem(metadata_item_t(0, "album", ProductName.c_str()));

    return true;
}

/// <summary>
/// Gets the code page from the IENC chunk, if present.
/// </summary>
bool GetCodePage(std::vector<uint8_t>::const_iterator data, ptrdiff_t chunkSize, uint32_t & codePage) noexcept
{
    const auto Tail = data + chunkSize;

    while (data != Tail)
    {
        if (Tail - data < 4)
            return false;

        const ptrdiff_t ChunkSize = (ptrdiff_t) toInt32LE(data + 4);

        if ((Tail - data) < (ptrdiff_t) (8 + ChunkSize))
            return false;

        const std::string ChunkId(data, data + 4);
        const auto ChunkData = data + 8;

        if (ChunkId == "IENC")
        {
            std::string Encoding(ChunkData, ChunkData + ChunkSize);

            msc::GetCodePageFromEncoding(Encoding, codePage);

            return true;
        }

        data += (ptrdiff_t) 8 + ChunkSize;

        if ((ChunkSize & 1) && (data < Tail))
            ++data;
    }

    return false;
}

}
