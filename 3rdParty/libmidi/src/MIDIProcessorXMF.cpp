
/** $VER: MIDIProcessorXMF.cpp (2025.09.06) Extensible Music Format (https://www.midi.org/specifications/file-format-specifications/xmf-extensible-music-format/extensible-music-format-xmf-2) **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "Encoding.h"
#include "Exception.h"

#undef WINAPI

#ifdef _DEBUG
#define __TRACE
#endif

#include <libmidi.h>
#include <zlib.h>

namespace midi
{

enum StringFormatID
{
    ExtendedAsciiVisible = 0,       // Extended ASCII, visible to the user
    ExtendedAsciiHidden = 1,        // Extended ASCII, hidden from the user

    UnicodeVisible = 2,             // Unicode, visible to the user
    UnicodeHidden = 3,              // Unicode, hidden from the user

    CompressedUnicodeVisible = 4,   // Compressed Unicode, visible to the user
    CompressedUnicodeHidden = 5,    // Compressed Unicode, hidden from the user

    BinaryDataVisible = 6,          // Binary data, visible to the user
    BinaryDataHidden = 7,           // Binary data, hidden from the user
};

enum FieldSpecifierID
{
    XMFFileType = 0,                // XMF File Type including the revision ID (only valid in the root node).

    NodeName = 1,                   // Node name, unique
    NodeIDNumber = 2,               // Node ID number

    ResourceFormat = 3,             // Resource format (SMF, DLS, WAV, ...)

    FilenameOnDisk = 4,             // Filename on disk including filename extension.
    FilenameExtensionOnDisk = 5,    // Filename extention on disk

    MacOSFileTypeAndCreator = 6,    // MacOS file type and creator

    MimeType = 7,                   // MIME type

    Title = 8,                      // Title
    CopyrightNotice = 9,            // Copyright notice
    Comment = 10,                   // Comment

    Autostart = 11,                 // Node Name of the FileNode containing the SMF image to autostart when the XMF file loads
    Preload = 12,                   // Used to pre-load specific SMF and DLS file images.

    ContentDescription = 13,        // Used to characterize the set of resources needed to play an SMF file in the Mobile XMF document, (RP-42a, Mobile XMF Content Format Specification, December 2006)
    ID3Metadata = 14,               // Contains a block of ID3v2.4.0 Metadata (RP-47, ID3 Metadata for XMF Files)

    Custom = -1,
};

enum ResourceFormatID
{
    Standard = 0,
    MMAManufacturer = 1,
    Registered = 2,
    NonRegistered = 3,
};

enum StandardResourceFormatID
{
    StandardMidiFileType0 = 0,                  // Standard MIDI File (SMF), Type 0
    StandardMidiFileType1 = 1,                  // Standard MIDI File (SMF), Type 1

    DownloadableSoundsLevel1 = 2,               // Downloadable Sounds (DLS), Level 1
    DownloadableSoundsLevel2 = 3,               // Downloadable Sounds (DLS), Level 2
    DownloadableSoundsLevel2_1 = 4,             // Downloadable Sounds (DLS), Level 2.1

    MobileDownloadableSoundsInstrumentFile = 5, // 

    InvalidStandardResourceFormat = -1,
};

enum UnpackerID
{
    None = 0,
    MMAManufacturerUnpacker = 1,
    RegisteredUnpacker = 2,
    NonRegisteredUnpacker = 3,
};

enum StandardUnpackerID
{
    NoUnpacker = 0,
    Zlib = 1,

    InvalidUnpacker = -1,
};

enum ReferenceTypeID
{
    Invalid = 0,

    InLineResource = 1,
    InFileResource = 2,
    InFileNode = 3,

    ExternalResourceFile = 4,
    ExternalXMFResource = 5,                    // Recommend Practice (RP-039)

    XMFFileURIandNodeID = 6,                    // Recommend Practice (RP-039)
};

struct xmf_metadata_type_t
{
    uint32_t ID;
    StringFormatID StringFormat;
    std::string Language;                       // https://www.rfc-editor.org/rfc/rfc3282
};

struct xmf_international_content_t
{
    uint32_t ID;
    std::vector<uint8_t> Data;
};

// 3.2.1.1.1. FieldSpecifier Structure (RP-030)
struct xmf_field_specifier_t
{
    FieldSpecifierID FieldID;                   // 5.2.1. Standard FieldID Assignments (RP-030)
    std::string FieldName;
};

struct xmf_metadata_item_t
{
    xmf_field_specifier_t FieldSpecifier;

    StringFormatID UniversalContentsFormat;
    std::vector<uint8_t> UniversalContentsData;

    std::vector<xmf_international_content_t> _InternationalContents;
};

struct xmf_unpacker_t
{
    UnpackerID ID;
    StandardUnpackerID StandardUnpackerID;
    int ManufacturerID;
    int InternalUnpackerID;
    size_t UnpackedSize;
};

struct xmf_reference_type_t
{
    ReferenceTypeID ID;
    ptrdiff_t Offset;
    std::string Uri;
};

struct xmf_node_t
{
    size_t Size;        // Size of the complete node (in bytes)
    size_t HeaderSize;  // Relative offset to the node contents (in bytes)
    size_t ItemCount;   // 0 for a FileNode, or count for a FolderNode

    std::string Name;
    uint32_t ID;

    uint32_t XMFFileTypeID;
    uint32_t XMFFileTypeRevisionID;

    std::vector<xmf_metadata_item_t> MetaData;
    std::vector<xmf_unpacker_t> Unpackers;

    xmf_reference_type_t ReferenceType;

    std::vector<xmf_node_t> Children;

    std::vector<uint8_t> Unpack(const std::vector<uint8_t> & data);
};

struct xmf_file_t
{
    std::string XMFMetaFileVersion;

    uint32_t XMFFileTypeID;
    uint32_t XMFFileTypeRevisionID;

    uint32_t Size;
};

const size_t MagicSize = 4;

/// <summary>
/// Returns true if the byte vector contains XMF data.
/// </summary>
bool processor_t::IsXMF(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < MagicSize)
        return false;

    if (data[ 0] != 'X' || data[ 1] != 'M' || data[ 2] != 'F' || data[ 3] != '_')
        return false;

    return true;
}

/// <summary>
/// Processes a byte vector with XMF data.
/// </summary>
bool processor_t::ProcessXMF(std::vector<uint8_t> const & data, container_t & container)
{
    TRACE_INDENT();

    xmf_file_t File = { };
    metadata_table_t Metadata;

    auto Head = data.begin();
    auto Tail = data.end();

    auto Data = Head + MagicSize;

    // Read the XMFMetaFileVersion which indicates the version of the XMF Meta-File Format Specification being used.
    File.XMFMetaFileVersion = std::string(Data, Data + 4);
    Data += 4;

    Metadata.AddItem(metadata_item_t(0, "xmf_meta_file_version", File.XMFMetaFileVersion.c_str()));

    #ifdef __TRACE
    ::printf("%*sXMF File Version %s\n", __TRACE_LEVEL * 4, "", File.XMFMetaFileVersion.c_str());
    #endif

    // RP-043: XMF Meta File Format 2.0, September 2004
    if (std::atof(File.XMFMetaFileVersion.c_str()) >= 2.0f)
    {
        char s[16];

        // Read the XMFFileTypeID: XMF Type 2 file (Mobile XMF)
        File.XMFFileTypeID = (uint32_t) ((Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) | Data[3]);
        Data += 4;

        ::sprintf_s(s, _countof(s), "%d", File.XMFFileTypeID);

        Metadata.AddItem(metadata_item_t(0, "xmf_file_type", s));

        // Read the XMFFileTypeRevisionID: Version 1 of Mobile XMF spec
        File.XMFFileTypeRevisionID = (uint32_t) ((Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) | Data[3]);
        Data += 4;

        char t[16];

        ::sprintf_s(t, _countof(s), "%d", File.XMFFileTypeRevisionID);

        Metadata.AddItem(metadata_item_t(0, "xmf_file_type_revision", t));

        #ifdef __TRACE
        ::printf("%*sXMF File Type %s Revision %s\n", __TRACE_LEVEL * 4, "", s, t);
        #endif
    }

    File.Size = (uint32_t) DecodeVariableLengthQuantity(Data, Tail);

    // Read the MetadataTypesTable if present.
    const uint32_t TableSize = (uint32_t) DecodeVariableLengthQuantity(Data, Tail); // Total node length in bytes, including NodeContents

    if (TableSize != 0)
        throw midi::exception("XMF MetadataTypesTable is not yet supported");

    // Read the tree.
    {
        const auto TreeStart = DecodeVariableLengthQuantity(Data, Tail);
    //  const auto TreeEnd   = DecodeVariableLengthQuantity(Data, Tail);

        Data = data.begin() + TreeStart;

        ProcessNode(Head, Tail, Data, Metadata, container);

        container.SetExtraMetaData(Metadata);

        container.FileFormat = FileFormat::XMF;
    }

    TRACE_UNINDENT();

    return true;
}

/// <summary>
/// Processes a tree node.
/// </summary>
bool processor_t::ProcessNode(std::vector<uint8_t>::const_iterator & head, std::vector<uint8_t>::const_iterator tail, std::vector<uint8_t>::const_iterator & data, metadata_table_t & metadata, container_t & container)
{
    #ifdef __TRACE
    ::printf("%*sNode\n", __TRACE_LEVEL * 4, "");
    #endif

    TRACE_INDENT();

    const std::vector<uint8_t>::const_iterator HeaderHead = data;

    xmf_node_t Node = {};

    // Read the node header.
    Node.Size       = (size_t) DecodeVariableLengthQuantity(data, tail); // 3.2.1 NodeMetaData NodeLength
    Node.ItemCount  = (size_t) DecodeVariableLengthQuantity(data, tail); // 3.2.1 NodeMetaData NodeContainedItems
    Node.HeaderSize = (size_t) DecodeVariableLengthQuantity(data, tail); // 3.2.1 NodeMetaData NodeHeaderLength

    auto StandardResourceFormat = StandardResourceFormatID::InvalidStandardResourceFormat;

    // Read the metadata.
    {
//      const auto MetaDataHead = data;
        const size_t MetaDataSize = (size_t) DecodeVariableLengthQuantity(data, tail);
        const auto MetaDataTail = data + (ptrdiff_t) MetaDataSize;

        #ifdef __TRACE
        ::printf("%*sMetadata (%d bytes)\n", __TRACE_LEVEL * 4, "", (uint32_t) MetaDataSize);
        #endif

        TRACE_INDENT();

        while (data < MetaDataTail)
        {
            // 3.2.1.1 MetaDataItem
            xmf_metadata_item_t MetadataItem = {};

            {
                const ptrdiff_t Size = (ptrdiff_t) DecodeVariableLengthQuantity(data, tail);

                if (Size == 0)
                {
                    MetadataItem.FieldSpecifier.FieldID   = (FieldSpecifierID) DecodeVariableLengthQuantity(data, tail);
                }
                else
                {
                    MetadataItem.FieldSpecifier.FieldID   = FieldSpecifierID::Custom;
                    MetadataItem.FieldSpecifier.FieldName = std::string(data, data + Size);
                    data += Size;
                }
            }

            {
                // 3.2.1.1.2. FieldContents Structure (RP-30)
                const size_t FieldContentsCount = (size_t) DecodeVariableLengthQuantity(data, tail);

                if (FieldContentsCount == 0)
                {
                    std::string Name;
                    std::string Value;

                    // Interpret the universal field contents.
                    const size_t Size = (size_t) DecodeVariableLengthQuantity(data, tail);

                    if (Size > 0)
                    {
                        MetadataItem.UniversalContentsFormat = (StringFormatID) DecodeVariableLengthQuantity(data, tail); // 3.2.2.1. StringFormatTypeID Definitions (RP-030)
                        MetadataItem.UniversalContentsData.assign(data, data + (ptrdiff_t) Size - 1);
                        data += (ptrdiff_t) Size - 1;

                    //  const bool HiddenContents = (MetadataItem.UniversalContentsFormat & 1);

                        auto Head = MetadataItem.UniversalContentsData.begin();
                        auto Tail = MetadataItem.UniversalContentsData.end();

                        // 5.2.1. Standard FieldID Assignments (RP-030)
                        switch (MetadataItem.FieldSpecifier.FieldID)
                        {
                            // 5.2.2. Identifies the Type to which the XMF file conforms and the specification Revision level within that Type. Only valid in the RootNode.
                            case FieldSpecifierID::XMFFileType:
                            {
                                Node.XMFFileTypeID         = (uint32_t) DecodeVariableLengthQuantity(Head, Tail);
                                Node.XMFFileTypeRevisionID = (uint32_t) DecodeVariableLengthQuantity(Head, Tail);

                                Name = "XMF File Type";
                                Value = msc::FormatText("File Type %d Revision %d", Node.XMFFileTypeID, Node.XMFFileTypeRevisionID);
                                break;
                            }

                            case FieldSpecifierID::NodeName:
                            {
                                Node.Name = std::string(Head, Tail);

                                Name  = "Node Name";
                                Value = Node.Name;
                                break;
                            }

                            case FieldSpecifierID::NodeIDNumber:
                            {
                                Node.ID = (uint32_t) DecodeVariableLengthQuantity(Head, Tail);

                                Name  = "Node ID Number";
                                Value = msc::FormatText("%d", Node.ID);
                                break;
                            }

                            case FieldSpecifierID::ResourceFormat:
                            {
                                const auto ResourceFormat = (ResourceFormatID) DecodeVariableLengthQuantity(Head, MetadataItem.UniversalContentsData.end());

                                // 5.3.1. Standard ResourceFormatIDs (RP-030)
                                if (ResourceFormat == ResourceFormatID::Standard)
                                {
                                    StandardResourceFormat = (StandardResourceFormatID) DecodeVariableLengthQuantity(Head, MetadataItem.UniversalContentsData.end());

                                    switch (StandardResourceFormat)
                                    {
                                        case StandardMidiFileType0:                     Value = "SMF Type 0"; break;
                                        case StandardMidiFileType1:                     Value = "SMF Type 1"; break;

                                        case DownloadableSoundsLevel1:                  Value = "DLS Level 1"; break;
                                        case DownloadableSoundsLevel2:                  Value = "DLS Level 2"; break;
                                        case DownloadableSoundsLevel2_1:                Value = "DLS Level 2.1"; break;

                                        case MobileDownloadableSoundsInstrumentFile:    Value = "Mobile DLS"; break;

                                        case InvalidStandardResourceFormat:
                                        default:                                        Value = "Unknown";
                                    }
                                }
                                else
                                // 5.3.2. MMA Manufacturer ResourceFormatIDs (RP-030)
                                {
                                }

                                Name  = "Resource Format";
                                break;
                            }

                            case FieldSpecifierID::FilenameOnDisk:
                            {
                                Name  = "File Name";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::FilenameExtensionOnDisk:
                            {
                                Name  = "File Extension";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::MacOSFileTypeAndCreator:
                            {
                                Name  = "MacOS File Type and Creator";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::MimeType:
                            {
                                Name  = "MIME Type";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::Title:
                            {
                                Name  = "Title";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::CopyrightNotice:
                            {
                                Name  = "Copyright";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::Comment:
                            {
                                Name  = "Comment";
                                Value = std::string(Head, Tail);
                                break;
                            }

                            case FieldSpecifierID::Autostart:           // Type 0 and Type 1 XMF Files (SMF + DLS) (RP-031)
                            {
                                Name  = "Autostart";
                                Value = "";
                                break;
                            }

                            case FieldSpecifierID::Preload:             // Type 0 and Type 1 XMF Files (SMF + DLS) (RP-031)
                            {
                                Name  = "Preload";
                                Value = "";
                                break;
                            }

                            case FieldSpecifierID::ContentDescription:  // Mobile XMF Content Format Specification, December 2006 (RP-42a)
                            {
                                Name  = "Content Description";
                                Value = "";
                                break;
                            }

                            case FieldSpecifierID::ID3Metadata:         // ID3 Metadata for XMF Files (RP-47)
                            {
                                Name  = "ID3 Metadata";
                                Value = "";
                                break;
                            }


                            case FieldSpecifierID::Custom:
                            {
                                Name  = "Custom";
                                Value = "";
                                break;
                            }

                            default:
                            {
                                Name  = "Unknown";
                                Value = "";
                                break;
                            }
                        }
                    }

                    #ifdef __TRACE
                    ::printf("%*sUniversal metadata, %9d bytes, %-30s: %s\n", __TRACE_LEVEL * 4, "", (uint32_t) Size, Name.c_str(), Value.c_str());
                    #endif
                }
                else
                {
                    // Interpret the international field contents.
                    for (size_t i = 0; i < FieldContentsCount; ++i)
                    {
                        int MetaDataTypeID = DecodeVariableLengthQuantity(data, tail);

                        const size_t Size = (size_t) DecodeVariableLengthQuantity(data, tail);

                        #ifdef __TRACE
                        ::printf("%*sInternational metadata, %9zu bytes, ID %3d\n", __TRACE_LEVEL * 4, "", Size, MetaDataTypeID);
                        #endif

                        data += (ptrdiff_t) Size;
                    }
                }
            }

            Node.MetaData.push_back(MetadataItem);
        }

        TRACE_UNINDENT();
    }

    // Read the unpackers.
    {
//      const auto UnpackersHead = data;
        const size_t UnpackersLength = (size_t) DecodeVariableLengthQuantity(data, tail);
        const auto UnpackersTail = data + (ptrdiff_t) UnpackersLength;

        while (data < UnpackersTail)
        {
            xmf_unpacker_t Unpacker = {};

            Unpacker.ID = (UnpackerID) DecodeVariableLengthQuantity(data, tail);

            switch (Unpacker.ID)
            {
                case UnpackerID::None:
                {
                    Unpacker.StandardUnpackerID = (StandardUnpackerID) DecodeVariableLengthQuantity(data, tail);
                    break;
                }

                case UnpackerID::MMAManufacturerUnpacker:
                {
                    int ManufacturerID = *data++;

                    if (ManufacturerID == 0)
                    {
                        ManufacturerID <<= 8; ManufacturerID |= *data++;
                        ManufacturerID <<= 8; ManufacturerID |= *data++;
                    }

                    Unpacker.ManufacturerID     = ManufacturerID;
                    Unpacker.InternalUnpackerID = DecodeVariableLengthQuantity(data, tail);
                    break;
                }

                case UnpackerID::RegisteredUnpacker:
                case UnpackerID::NonRegisteredUnpacker:
                    throw midi::exception("Unsupported XMF compression algorithm");

                default:
                    throw midi::exception("Unknown XMF compression algorithm");
            }

            Unpacker.UnpackedSize = (size_t) DecodeVariableLengthQuantity(data, tail);

            Node.Unpackers.push_back(Unpacker);
        }
    }

    // Read the reference type.
    {
        data = HeaderHead + (ptrdiff_t) Node.HeaderSize;

        Node.ReferenceType.ID = (ReferenceTypeID) DecodeVariableLengthQuantity(data, tail);

        switch (Node.ReferenceType.ID)
        {
            case ReferenceTypeID::InLineResource:
            {
                Node.ReferenceType.Offset = data - head;
                break;
            }

            case ReferenceTypeID::InFileResource:
            case ReferenceTypeID::InFileNode:
            case ReferenceTypeID::ExternalResourceFile:
            case ReferenceTypeID::ExternalXMFResource:
            case ReferenceTypeID::XMFFileURIandNodeID:
                throw midi::exception("Unsupported XMF reference type");

            case ReferenceTypeID::Invalid:
            default:
                throw midi::exception("Unknown XMF reference type");
        }
    }

    // Read the content.
    {
        if (Node.ItemCount == 0)
        {
            // File node
            ptrdiff_t Size = (ptrdiff_t) (Node.Size - Node.HeaderSize - 1);

            std::vector<uint8_t> Data(data, data + Size);
            std::vector<uint8_t> UnpackedData;

            switch (StandardResourceFormat)
            {
                case StandardResourceFormatID::StandardMidiFileType0:
                case StandardResourceFormatID::StandardMidiFileType1:
                {
                    if (container.FileFormat == FileFormat::Unknown)
                    {
                        UnpackedData = Node.Unpack(Data);

                        ProcessSMF(UnpackedData, container);
                    }
                    break;
                }

                case StandardResourceFormatID::DownloadableSoundsLevel1:
                case StandardResourceFormatID::DownloadableSoundsLevel2:
                case StandardResourceFormatID::DownloadableSoundsLevel2_1:
                case StandardResourceFormatID::MobileDownloadableSoundsInstrumentFile:
                {
                    if (container.SoundFont.empty())
                    {
                        UnpackedData = Node.Unpack(Data);

                        container.SoundFont = UnpackedData;
                    }
                    break;
                }

                case StandardResourceFormatID::InvalidStandardResourceFormat:
                default:
                    ; // Not yet supported.
            }

            #ifdef __TRACE
            ::printf("%*s%d bytes\n", __TRACE_LEVEL * 4, "", (uint32_t) Size);
            #endif

            data += Size;
        }
        else
        {
            // Folder node
            switch (Node.ReferenceType.ID)
            {
                case ReferenceTypeID::InLineResource:
                case ReferenceTypeID::InFileResource:
                {
                    auto Data = head + Node.ReferenceType.Offset;

                    for (size_t i = 0; i < Node.ItemCount; ++i)
                    {
                        ProcessNode(head, tail, Data, metadata, container);
                    }
                    break;
                }

                case ReferenceTypeID::InFileNode:
                case ReferenceTypeID::ExternalResourceFile:
                case ReferenceTypeID::ExternalXMFResource:
                case ReferenceTypeID::XMFFileURIandNodeID:
                    throw midi::exception("Unsupported XMF reference type");

                case ReferenceTypeID::Invalid:
                default:
                    throw midi::exception("Unknown XMF reference type");
            }
        }
    }

    TRACE_UNINDENT();

    return true;
}

/// <summary>
/// Unpackes the specified data, if necessary.
/// </summary>
std::vector<uint8_t> xmf_node_t::Unpack(const std::vector<uint8_t> & data)
{
    if (Unpackers.size() == 0)
        return data;

    const auto & Unpacker = Unpackers[0];
    std::vector<uint8_t> UnpackedData;

    if (Unpacker.StandardUnpackerID == StandardUnpackerID::Zlib)
    {
        UnpackedData.resize(Unpacker.UnpackedSize);

        processor_t::Inflate(data, UnpackedData);
    }
    else
    if (Unpacker.InternalUnpackerID != 0)
    {
        if ((data.size() > 2) && (data[0] == 0x78) && (data[1] == 0xDA))
        {
            UnpackedData.resize(Unpacker.UnpackedSize);

            processor_t::InflateRaw(data, UnpackedData);
        }
        else
            throw midi::exception(msc::FormatText("Unable to unpack data using unknown compression algorithm 0x%02X from manufacturer 0x%06X",  Unpacker.InternalUnpackerID,  Unpacker.ManufacturerID));
    }

    return UnpackedData;
}

/// <summary>
/// Inflates a zlib deflated data stream.
/// </summary>
int processor_t::Inflate(const std::vector<uint8_t> & src, std::vector<uint8_t> & dst) noexcept
{
    z_stream Stream = { };

    Stream.total_in = Stream.avail_in = (uInt) src.size();
    Stream.next_in = (Bytef *) src.data();

    Stream.total_out = Stream.avail_out = (uInt) dst.size();
    Stream.next_out = (Bytef *) dst.data();

    int Status = ::inflateInit2(&Stream, MAX_WBITS);

    if (Status == Z_OK)
    {
        Status = ::inflate(&Stream, Z_FINISH);

        if (Status == Z_STREAM_END)
            Status = Z_OK;
    }

    ::inflateEnd(&Stream);

    return Status;
}

/// <summary>
/// Inflates a raw deflated data stream.
/// </summary>
int processor_t::InflateRaw(const std::vector<uint8_t> & src, std::vector<uint8_t> & dst) noexcept
{
    z_stream Stream = { };

    Stream.avail_in = (uInt) src.size();
    Stream.next_in = (Bytef *) src.data();

    Stream.avail_out = (uInt) dst.size();
    Stream.next_out = (Bytef *) dst.data();

    int Status = ::inflateInit(&Stream);

    if (Status == Z_OK)
    {
        Status = ::inflate(&Stream, Z_NO_FLUSH);

        if (Status == Z_STREAM_END)
            Status = Z_OK;
    }

    ::inflateEnd(&Stream);

    return Status;
}

}
