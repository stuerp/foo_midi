
/** $VER: Process.cpp (2026.04.10) P. Stuer **/

#include "pch.h"

#include "MIDIContainer.h"
#include "MIDIProcessor.h"

#include <Encoding.h>

void ProcessContainer(midi::container_t & container, bool asStream);
void ProcessStream(const midi::container_t & container, const std::vector<midi::message_t> & stream, const midi::sysex_table_t & sysExMap, const std::vector<uint8_t> & portNumbers, bool skipNormalEvents = false);
void ProcessTracks(const midi::container_t & container);

std::vector<uint8_t> ReadFile(const fs::path & filePath);

/// <summary>
/// Examines the specified file.
/// </summary>
void ExamineFile(const fs::path & filePath, const std::map<std::string, std::string> & args)
{
    try
    {
        std::vector<uint8_t> Data = ReadFile(filePath);

        midi::container_t Container;
        midi::processor_options_t Options;

        if (midi::processor_t::Process(Data, msc::UTF8ToWide(filePath.string().c_str()).c_str(), Container, Options))
            ProcessContainer(Container, args.contains("AsStream"));
        else
            ::puts("File format not recognized.");
    }
    catch (std::exception & e)
    {
        ::printf("%s\n", e.what());
    }
}

/// <summary>
/// Reads a file and returns its contents as a byte vector.
/// </summary>
std::vector<uint8_t> ReadFile(const fs::path & filePath)
{
    std::ifstream Stream(filePath, std::ios::binary | std::ios::ate);

    if (!Stream.is_open())
        throw std::runtime_error(std::format("Failed to open \"{}\" for reading)", filePath.string().c_str()));

    const std::streamsize Size = Stream.tellg();

    Stream.seekg(0, std::ios::beg);

    std::vector<uint8_t> Data((size_t) Size);

    if (!Stream.read((char *) Data.data(), Size))
        throw std::runtime_error("Failed to read file.");

    return Data;
}

/// <summary>
/// Processes a container.
/// </summary>
void ProcessContainer(midi::container_t & container, bool asStream)
{
//  ReadIns("GM1-GM2.ins");

    if (asStream)
    {
        {
            const uint32_t SubsongCount = (uint32_t) container.GetSubSongCount();
            const uint32_t SubsongIndex = 0;

            const uint32_t Format = container.GetFormat();
            const uint32_t TrackCount = (Format == 2) ? 1 : container.GetTrackCount();

            ::printf("MIDI Format %d, %d tracks, %d subsongs\n", Format, TrackCount, SubsongCount);

            for (uint32_t i = 0; i < TrackCount; ++i)
            {
                uint32_t ChannelCount = container.GetChannelCount(SubsongIndex);
                uint32_t Duration = container.GetDuration(SubsongIndex, false);
                uint32_t DurationInMS = container.GetDuration(SubsongIndex, true);

                ::printf("Track %2d: %d channels, %8d ticks, %8.2fs\n", i + 1, ChannelCount, Duration, (float) DurationInMS / 1000.0f);

                midi::metadata_table_t MetaData;

                container.GetMetaData(SubsongIndex, MetaData);

                for (size_t j = 0; j < MetaData.GetCount(); ++j)
                {
                    const midi::metadata_item_t & Item = MetaData[j];

                    ::printf("- %8d %s: \"%s\"\n", Item.Timestamp, Item.Name.c_str(), msc::TextToUTF8(Item.Value.c_str()).c_str());
                }
            }
        }

        uint32_t LoopBegin = container.GetLoopBeginTimestamp(0);
        uint32_t LoopEnd   =  container.GetLoopEndTimestamp(0);

        ::printf("Loop Begin: %d ticks\n", LoopBegin);
        ::printf("Loop End  : %d ticks\n", LoopEnd);

        std::vector<midi::message_t> Stream;
        midi::sysex_table_t SysExMap;
        std::vector<uint8_t> PortNumbers;

        container.SerializeAsStream(0, Stream, SysExMap, PortNumbers, LoopBegin, LoopEnd, 0);

        ProcessStream(container, Stream, SysExMap, PortNumbers);
    }
    else
        ProcessTracks(container);
}
