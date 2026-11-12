
/** $VER: main.cpp (2026.05.17) P. Stuer **/

#include "pch.h"

#include <MIDI2.h>

using namespace midi2;

static void ProcessDirectory(const fs::path & directoryPath) noexcept;
static void ProcessFile(const fs::path & filePath) noexcept;
static std::vector<uint8_t> ReadFile(const fs::path & filePath);
static void ExamineFile(const fs::path & filePath) noexcept;

static std::map<std::string, std::string> Arguments;

int main(int argc, const char ** argv)
{
    ::SetConsoleOutputCP(CP_UTF8);

    if (argc < 2)
    {
        ::printf("Insufficient arguments.\n");

        return -1;
    }

    const char * FileName = argv[1];

    if (!::fs::exists(FileName))
    {
        ::printf("Failed to access \"%s\": path does not exist.\n", FileName);

        return -1;
    }

    fs::path Path = fs::canonical(FileName);

    if (fs::is_directory(Path))
        ProcessDirectory(Path);
    else
        ProcessFile(Path);

    return 0;
}

/// <summary>
/// Processes all entries of a directory.
/// </summary>
static void ProcessDirectory(const fs::path & directoryPath) noexcept
{
    ::printf("\"%s\"\n", directoryPath.string().c_str());

    for (const auto & Entry : fs::directory_iterator(directoryPath))
    {
        if (Entry.is_directory())
        {
            ProcessDirectory(Entry.path());
        }
        else
        {
            ProcessFile(Entry.path());
        }
    }
}

/// <summary>
/// Processes a file.
/// </summary>
static void ProcessFile(const fs::path & filePath) noexcept
{
    fs::path FilePath = filePath;

    FilePath.replace_extension(".log");

    (void) _chmod(FilePath.string().c_str(), _S_IWRITE | _S_IREAD);

    FILE * fp = nullptr;

    if ((::freopen_s(&fp, FilePath.string().c_str(), "w", stdout) != 0) || (fp == nullptr))
    {
        if ((::freopen_s(&fp, "CON", "w", stdout) != 0) || (fp == nullptr))
            return;

        ::puts("Failed to open log file.");

        return;
    }

    ::printf("\xEF\xBB\xBF"); // UTF-8 BOM

    auto FileSize = fs::file_size(filePath);

    ::printf("\n\"%s\", %" PRIu64 " bytes\n", filePath.string().c_str(), (uint64_t) FileSize);

    ExamineFile(filePath);

    ::fflush(fp);

    ::fclose(fp);

    (void) ::freopen_s(&fp, "CON", "w", stdout);
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
/// Examines the specified file.
/// </summary>
void ExamineFile(const fs::path & filePath) noexcept
{
    try
    {
        std::vector<uint8_t> Data = ReadFile(filePath);

        if (Data.size() < 8)
        {
            ::puts("Failed to examine file: File is too small");

            return;
        }

        if (::memcmp(Data.data(), "SMF2CLIP", 8) != 0)
        {
            ::puts("Failed to examine file: MIDI 2.0 signature not found");

            return;
        }

        ::puts("00000000 SMF2CLIP");

        size_t i = 1;
        size_t Offset = 8;

        const auto Packets = ump_t::FromBytes(Data.data() + 8, Data.size() - 8);

        for (const auto & Packet : Packets)
        {
            ::printf("%5zu %08zX", i, Offset);

            switch (Packet.GetMessageType())
            {
                // 7.2 Utility Messages
                case MessageType::Utility:
                {
                    const auto Message = utility_message_t(Packet);

                    ::printf(" Utility, ");

                    const uint16_t Status = Message.Status();

                    switch (Status)
                    {
                        case 0x00: ::printf("No Operation"); break;
                        case 0x01: ::printf("Jitter Reduction Clock, %u clock ticks", Message.Payload()); break;
                        case 0x02: ::printf("Jitter Reduction Timestamp, %u clock ticks", Message.Payload()); break;
                        case 0x03: ::printf("Delta Clockstamp, %u Ticks Per Quarter Note", Message.Payload()); break;
                        case 0x04: ::printf("Delta Clockstamp, %u Ticks Since Last Event", Message.Payload()); break;
                    }
                    break;
                }

                // 7.6 System Messages
                case MessageType::System:
                {
                    const auto Message = system_message_t(Packet);

                    const uint8_t Group = Message.Group();
                    const uint16_t Status = Message.Status();

                    switch (Status)
                    {
                        case 0xF1: ::printf(" System Common, Group %u, MIDI Time Code", Group); break;
                        case 0xF2: ::printf(" System Common, Group %u, Song Position Pointer", Group); break;
                        case 0xF3: ::printf(" System Common, Group %u, Song Select", Group); break;
                        case 0xF6: ::printf(" System Common, Group %u, Tune Request", Group); break;

                        case 0xF8: ::printf(" System Real Time, Group %u, Timing Clock", Group); break;
                        case 0xFA: ::printf(" System Real Time, Group %u, Start", Group); break;
                        case 0xFB: ::printf(" System Real Time, Group %u, Continue", Group); break;
                        case 0xFC: ::printf(" System Real Time, Group %u, Stop", Group); break;
                        case 0xFE: ::printf(" System Real Time, Group %u, Active Sensing", Group); break;
                        case 0xFF: ::printf(" System Real Time, Group %u, Reset", Group); break;
                    }
                    break;
                }

                // 7.3 MIDI 1.0 Channel Voice Messages
                case MessageType::MIDI1ChannelVoice:
                {
                    const auto Message = midi1_channel_voice_message_t(Packet);

                    const uint8_t Group = Message.Group();
                    const uint16_t Status = Message.Status();

                    const char * Description = "<Unknown>";

                    switch (Status)
                    {
                        case 0x80: Description = "Note Off"; break;
                        case 0x90: Description = "Note On"; break;
                        case 0xA0: Description = "Poly Pressure"; break;
                        case 0xB0: Description = "Control Change"; break;
                        case 0xC0: Description = "Program Change"; break;
                        case 0xD0: Description = "Channel Pressure"; break;
                        case 0xE0: Description = "Pitch Bend"; break;
                    }

                    ::printf(" MIDI 1.0 Channel Voice, Group %u, %s", Group, Description);
                    break;
                }

                // 7.7 System Exclusive (7-bit) Messages
                case MessageType::Data:
                {
                    const auto Message = sysex7_message_t(Packet);

                    const uint8_t Group = Message.Group();
                    const uint16_t Status = Message.Status();
                    const uint8_t Size = Message.Size();

                    ::printf(" System Exclusive (7-bit), Group %u, Status %u, %u bytes,", Group, Status, Size);

                    if (Size != 0)
                    {
                        auto d = &Packet.Data[2];

                        for (uint8_t i = 0; i < Size; ++i)
                            ::printf(" %02X", *d++);
                    }
                    break;
                }

                // 7.4 MIDI 2.0 Channel Voice Messages
                case MessageType::MIDI2ChannelVoice:
                {
                    const auto Message = midi2_channel_voice_message_t(Packet);

                    const uint8_t Group = Message.Group();
                    const uint16_t Status = Message.Status();

                    const uint16_t Channel = Message.Channel();

                    std::string Description = "<Unknown>";

                    switch (Status)
                    {
                        case 0x00: Description = "Registered Per-Note Controller"; break;
                        case 0x10: Description = "Assignable Per-Note Controller"; break;
                        case 0x20: Description = "Registered Controller (RPN)"; break;
                        case 0x30: Description = "Assignable Controller (NRPN)"; break;
                        case 0x40: Description = "Relative Registered Controller (RPN)"; break;
                        case 0x50: Description = "Relative Assignable Controller (NRPN)"; break;
                        case 0x60: Description = "Per-Note Pitch Bend"; break;

                        case 0x80: Description = "Note Off"; break;
                        case 0x90: Description = "Note On"; break;
                        case 0xA0: Description = "Poly Pressure"; break;
                        case 0xB0: Description = "Control"; break;
                        case 0xC0:
                        {
                            Description.resize(256);
                            ::sprintf_s(Description.data(), Description.size(), "Program Change: Bank Valid: %u, Program 0x%02X, Bank MSB 0x%02X, Bank LSB 0x%02X", Packet.Data[3], Packet.Data[4], Packet.Data[6], Packet.Data[7]);
                            break;
                        }
                        case 0xD0: Description = "Channel Pressure"; break;
                        case 0xE0: Description = "Pitch Bend"; break;

                        case 0xF0: Description = "Per-Note Management"; break;
                    }

                    ::printf(" MIDI 2.0 Channel Voice, Group %u, Channel %u, %s", Group, Channel, Description.c_str());
                    break;
                }

                // 7.8 System Exclusive (8-bit) / Mixed Data Set Messages
                case MessageType::ExtendedData:
                {
                    const auto Message = message_t(Packet);

                    const uint16_t Status = Message.Status();

                    if (Status < 4)
                    {
                        // System Exclusive (8-bit) Message
                        const auto Message = sysex8_message_t(Packet);

                        const uint8_t Group = Message.Group();
                        const uint8_t Size = Message.Size();
                        const uint8_t StreamId = Message.StreamId();

                        ::printf(" System Exclusive (8-bit), Group %u, Status %u, %u bytes, Stream Id %u,", Group, Status, Size, StreamId);

                        if (Size != 0)
                        {
                            auto d = (uint8_t * const) &Packet.Data[2];

                            for (uint8_t i = 0; i < Size; ++i)
                                ::printf(" %02X", *d++);
                        }
                    }
                    else
                    {
                        // Mixed Data Set Message
                        const auto Message = mixed_data_set_message_t(Packet);

                        const uint8_t Group = Message.Group();
                        const uint8_t Id = Message.Id();

                        if (Status == 8)
                        {
                            ::printf(" Mixed Data Set Header, Group %u, Id %u, %u bytes, %u chunks, Chunk %u, Manufacturer %u, Device %u, Sub-Id 1 %u, Sub-Id 2 %u\n", Group, Id,
                                Message.ChunkSize(), Message.ChunkCount(), Message.ChunkNumber(), Message.ManufacturerId(), Message.DeviceId(), Message.SubId1(), Message.SubId2());
                        }
                        else
                            ::printf(" Mixed Data Set Payload, Group %u, Id %u\n", Group, Id);
                    }
                    break;
                }

                // 7.5 Flex Data Messages
                case MessageType::FlexData:
                {
                    const auto Message = flex_data_message_t(Packet);

                    const uint8_t Group = Message.Group();
                    const uint16_t Status = Message.Status();

                    const uint8_t Format = Message.Format();
                    const uint8_t Address = Message.Address();
                    const uint8_t Channel = Message.Channel();
                    const uint8_t StatusBank = Message.StatusBank();

                    std::string  Description = "<Unknown>";

                    switch ((StatusBank << 8) | Status)
                    {
                        case 0x0000:
                        {
                            Description = "Set Tempo";

                            const uint32_t Tempo = (uint32_t) (Packet.Data[4] << 24) | (uint32_t) (Packet.Data[5] << 16) | (uint32_t) (Packet.Data[6] << 8) | (uint32_t) Packet.Data[7];

                            char Text[128];

                            ::sprintf_s(Text, _countof(Text), ", %.3fμs / Quarter Note, %u BPM", Tempo * 10. / 1'000., (uint32_t) (60'000'000. / (Tempo * 10. / 1'000.)));

                            Description += Text;
                            break;
                        }

                        case 0x0001:
                        {
                            Description = "Set Time Signature";

                            char Text[128];

                            ::sprintf_s(Text, _countof(Text), ", %u/%u, %u of 1/32 notes", Packet.Data[4], Packet.Data[5], Packet.Data[6]);

                            Description += Text;
                            break;
                        }

                        case 0x0002:
                        {
                            Description = "Set Metronome";
                            break;
                        }

                        case 0x0005:
                        {
                            const int8_t SharpsFlats = (Packet.Data[4] > 4);
                            const uint8_t TonicNote = (Packet.Data[4] & 0x0F);

                            Description = "Set Key Signature ";

                            const std::string SharpNotes[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
                            const std::string FlatNotes [12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    
                            const std::string * Notes = (SharpsFlats < 0) ? FlatNotes : SharpNotes;
    
                            Description += Notes[TonicNote];
                            break;
                        }

                        case 0x0006:
                        {
                            Description = "Set Chord Name";
                            break;
                        }

                        default:
                        {
                            const char * TextFormat = "<Unknown>";

                            switch ((StatusBank << 8) | Status)
                            {
                                case 0x0100: TextFormat = "Unknown Metadata Text Event"; break;
                                case 0x0101: TextFormat = "Project Name"; break;
                                case 0x0102: TextFormat = "Composition Name"; break;
                                case 0x0103: TextFormat = "MIDI Clip Name"; break;
                                case 0x0104: TextFormat = "Copyright Notice"; break;
                                case 0x0105: TextFormat = "Composer Name"; break;
                                case 0x0106: TextFormat = "Lyricist Name"; break;
                                case 0x0107: TextFormat = "Arranger Name"; break;
                                case 0x0108: TextFormat = "Publisher Name"; break;
                                case 0x0109: TextFormat = "Primary Performer Name"; break;
                                case 0x010A: TextFormat = "Accompanying Performer Name"; break;
                                case 0x010B: TextFormat = "Recording Data"; break;
                                case 0x010C: TextFormat = "Recording Location"; break;

                                case 0x0200: TextFormat = "Unknown Performance Text Event"; break;
                                case 0x0201: TextFormat = "Lyrics"; break;
                                case 0x0202: TextFormat = "Lyrics Language"; break;
                                case 0x0203: TextFormat = "Ruby"; break;
                                case 0x0204: TextFormat = "Ruby Language"; break;
                            }

                            Description = "Text, \"" + std::string(TextFormat) + "\", \"";

                            auto p = (const char *) &Packet.Data[4];

                            for (size_t i = 0; (i < 12) && (*p != '\0'); ++i)
                                Description += *p++;

                            Description += '\"';
                            break;
                        }
                    }

                    ::printf(" Flex Data, Group %u, Format %u, Address %u, Channel %u, Status Bank %u, Status %u, %s", Group, Format, Address, Channel, StatusBank, Status, Description.c_str());
                    break;
                }

                // 7.1 Stream Messages
                case MessageType::Stream:
                {
                    const auto Message = stream_message_t(Packet);

                    const uint8_t Format  = Message.Format();
                    const uint16_t Status = Message.Status();

                    const char * Description = "<Unknown>";

                    switch (Status)
                    {
                        case 0x00: Description = "Endpoint Discovery"; break;                   // 7.1.1
                        case 0x01: Description = "Endpoint Info Notification"; break;           // 7.1.2
                        case 0x02: Description = "Device Identify Notification"; break;         // 7.1.3
                        case 0x03: Description = "Endpoint Name"; break;                        // 7.1.4
                        case 0x04: Description = "Product Instance Id Notification"; break;     // 7.1.5
                        case 0x05: Description = "Stream Configuration Request"; break;         // 7.1.6.2
                        case 0x06: Description = "Stream Configuration Notification"; break;    // 7.1.6.3

                        case 0x10: Description = "Function Block Discovery"; break;             // 7.1.7
                        case 0x11: Description = "Function Block Info Notification"; break;     // 7.1.8
                        case 0x12: Description = "Function Block Name Notification"; break;     // 7.1.9

                        case 0x20: Description = "Start of Clip"; break;                        // 7.1.10
                        case 0x21: Description = "End of Clip"; break;                          // 7.1.11
                    }

                    ::printf(" Stream, Format %u, %s", Format, Description);
                    break;
                }

                default:
                    ::printf(" Unknown message type: %u", Packet.GetMessageType());
            }

            ::putchar('\n');

            ++i;
            Offset += Packet.Size;
        }
    }
    catch (std::exception & e)
    {
        ::printf("%s\n", e.what());
    }
}
