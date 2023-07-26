#include "MIDIProcessor.h"

#include <Windows.h>

#include <string.h>

struct IFFChunk
{
    uint8_t Id[4];
    uint8_t Type[4];
    std::vector<uint8_t> _Data;
    std::vector<IFFChunk> _SubChunks;

    IFFChunk()
    {
        memset(Id, 0, sizeof(Id));
        memset(Type, 0, sizeof(Type));
    }

    IFFChunk(const IFFChunk & in)
    {
        memcpy(Id, in.Id, sizeof(Id));
        memcpy(Type, in.Type, sizeof(Type));
        _Data = in._Data;
        _SubChunks = in._SubChunks;
    }

    const IFFChunk & FindSubChunk(const char * id, uint32_t index = 0) const
    {
        for (std::size_t i = 0; i < _SubChunks.size(); ++i)
        {
            if (!memcmp(id, _SubChunks[i].Id, 4))
            {
                if (index)
                    --index;

                if (!index)
                    return _SubChunks[i];
            }
        }

        return *this; // throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );
    }

    /// <summary>
    /// Gets the number of chunks with the specified id.
    /// </summary>
    uint32_t GetChunkCount(const char * id) const
    {
        uint32_t ChunkCount = 0;

        for (std::size_t i = 0; i < _SubChunks.size(); ++i)
        {
            if (!memcmp(id, _SubChunks[i].Id, 4))
                ++ChunkCount;
        }

        return ChunkCount;
    }
};

struct IFFStream
{
    std::vector<IFFChunk> _Chunks;

    IFFChunk fail;

    /// <summary>
    /// Finds the first chunk with the specified id.
    /// </summary>
    const IFFChunk & FindChunk(const char * id) const
    {
        for (std::size_t i = 0; i < _Chunks.size(); ++i)
        {
            if (!memcmp(id, _Chunks[i].Id, 4))
                return _Chunks[i];
        }

        return fail; // throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );
    }
};

bool MIDIProcessor::IsXMI(std::vector<uint8_t> const & data)
{
    if (data.size() < 0x22)
        return false;

    if (data[ 0] != 'F' || data[ 1] != 'O' || data[ 2] != 'R' || data[ 3] != 'M' ||
        data[ 8] != 'X' || data[ 9] != 'D' || data[10] != 'I' || data[11] != 'R' ||
        data[30] != 'X' || data[31] != 'M' || data[32] != 'I' || data[33] != 'D')
        return false;

    return true;
}

uint32_t MIDIProcessor::DecodeVariableLengthQuantityXMI(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end)
{
    uint32_t Quantity = 0;

    if (it == end)
        return 0;

    uint8_t Byte = *it++;

    if (!(Byte & 0x80))
    {
        do
        {
            Quantity += Byte;

            if (it == end)
                break;

            Byte = *it++;
        }
        while (!(Byte & 0x80) && it != end);
    }

    --it;

    return Quantity;
}

static bool ReadChunk(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, IFFChunk & chunk, bool isFirstChunk)
{
    if (end - it < 8)
        return false;

    std::copy(it, it + 4, chunk.Id);
    it += 4;

    uint32_t ChunkSize = (uint32_t) (it[0] << 24) | (it[1] << 16) | (it[2] << 8) | it[3];

    if ((size_t) (end - it) < ChunkSize)
        return false;

    it += 4;

    bool IsCATChunk = !memcmp(chunk.Id, "CAT ", 4);
    bool IsFORMChunk = !memcmp(chunk.Id, "FORM", 4);

    std::size_t ChunkSizeLimit = (size_t) (end - it);

    if (ChunkSize > ChunkSizeLimit) ChunkSize = (uint32_t)
        ChunkSizeLimit;

    if ((isFirstChunk && IsFORMChunk) || (!isFirstChunk && IsCATChunk))
    {
        if (end - it < 4)
            return false;

        // Read the sub-chunks of a FORM or CAT chunk.
        auto ChunkEnd = it + ChunkSize;

        std::copy(it, it + 4, chunk.Type);
        it += 4;

        while (it < ChunkEnd)
        {
            IFFChunk SubChunk;

            if (!ReadChunk(it, ChunkEnd, SubChunk, IsCATChunk))
                return false;

            chunk._SubChunks.push_back(SubChunk);
        }

        it = ChunkEnd;

        if (ChunkSize & 1 && it != end)
            ++it;
    }
    else
    if (!IsFORMChunk && !IsCATChunk)
    {
        chunk._Data.assign(it, it + ChunkSize);
        it += ChunkSize;

        if (ChunkSize & 1 && it != end)
            ++it;
    }
    else
    {
        /*if ( first_chunk ) throw exception_io_data( pfc::string_formatter() << "Found " << pfc::string8( (const char *)p_out.m_id, 4 ) << " chunk instead of FORM" );
        else throw exception_io_data( "Found multiple FORM chunks" );*/
        return false;
    }

    return true;
}

static bool ReadStream(std::vector<uint8_t> const & data, IFFStream & stream)
{
    auto it = data.begin(), end = data.end();

    bool IsFirstChunk = true;

    while (it != end)
    {
        IFFChunk Chunk;

        if (ReadChunk(it, end, Chunk, IsFirstChunk))
        {
            stream._Chunks.push_back(Chunk);
            IsFirstChunk = false;
        }
        else
        if (IsFirstChunk)
            return false;
        else
            break;
    }

    return true;
}

bool MIDIProcessor::GetTrackCountFromXMI(std::vector<uint8_t> const & data, size_t & trackCount)
{
    trackCount = 0;

    IFFStream Stream;

    if (!ReadStream(data, Stream))
        return false;

    const IFFChunk & form_chunk = Stream.FindChunk("FORM");

    if (::memcmp(form_chunk.Type, "XDIR", 4))
        return SetLastErrorCode(MIDIError::XMIFORMXDIRNotFound);

    const IFFChunk & cat_chunk = Stream.FindChunk("CAT ");

    if (::memcmp(cat_chunk.Type, "XMID", 4))
        return SetLastErrorCode(MIDIError::XMICATXMIDNotFound);

    trackCount = cat_chunk.GetChunkCount("FORM");

    return true;
}

bool MIDIProcessor::ProcessXMI(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    SetLastErrorCode(MIDIError::None);

    IFFStream Stream;

    if (!ReadStream(data, Stream))
        return false;

    const FOURCC FOURCC_FORM = mmioFOURCC('F', 'O', 'R', 'M');

    const IFFChunk & FORMChunk = Stream.FindChunk("FORM");

    if (memcmp(FORMChunk.Type, "XDIR", 4))
        return SetLastErrorCode(MIDIError::XMIFORMXDIRNotFound);

    const IFFChunk & CATChunk = Stream.FindChunk("CAT ");

    if (memcmp(CATChunk.Type, "XMID", 4))
        return SetLastErrorCode(MIDIError::XMICATXMIDNotFound);

    uint32_t TrackCount = CATChunk.GetChunkCount("FORM");

    container.Initialize(TrackCount > 1 ? 2u : 0u, 60);

    for (uint32_t i = 0; i < TrackCount; ++i)
    {
        const IFFChunk & SubFORMChunk = CATChunk.FindSubChunk("FORM", i);

        if (memcmp(SubFORMChunk.Type, "XMID", 4))
            return SetLastErrorCode(MIDIError::XMIFORMXMIDNotFound);

        const IFFChunk & EVNTChunk = SubFORMChunk.FindSubChunk("EVNT");

        if (memcmp(EVNTChunk.Id, "EVNT", 4))
            return SetLastErrorCode(MIDIError::XMIEVNTChunkNotFound);

        std::vector<uint8_t> const & Data = EVNTChunk._Data;

        MIDITrack Track;

        bool IsTempoSet = false;

        uint32_t CurrentTimestamp = 0;
        uint32_t LastEventTimestamp = 0;

        std::vector<uint8_t> Buffer;

        Buffer.resize(3);

        auto it = Data.begin(), end = Data.end();

        while (it != end)
        {
            uint32_t Delta = DecodeVariableLengthQuantityXMI(it, end);

            CurrentTimestamp += Delta;

            if (CurrentTimestamp > LastEventTimestamp)
                LastEventTimestamp = CurrentTimestamp;

            if (it == end)
                return SetLastErrorCode(MIDIError::InsufficientData);

            Buffer[0] = *it++;

            if (Buffer[0] == StatusCodes::MetaData)
            {
                if (it == end)
                    return SetLastErrorCode(MIDIError::InsufficientData);

                Buffer[1] = *it++;

                long Size = 0;

                if (Buffer[1] == MetaDataTypes::EndOfTrack)
                {
                    if (LastEventTimestamp > CurrentTimestamp)
                        CurrentTimestamp = LastEventTimestamp;
                }
                else
                {
                    Size = DecodeVariableLengthQuantity(it, end);

                    if (Size < 0)
                        return SetLastErrorCode(MIDIError::InvalidMetaDataMessage);

                    if (end - it < Size)
                        return SetLastErrorCode(MIDIError::InsufficientData);

                    Buffer.resize((size_t) (Size + 2));
                    std::copy(it, it + Size, Buffer.begin() + 2);
                    it += Size;

                    if ((Buffer[1] == MetaDataTypes::SetTempo) && (Size == 3))
                    {
                        uint32_t Tempo = (uint32_t) (Buffer[2] * 0x10000 + Buffer[3] * 0x100 + Buffer[4]);
                        uint32_t PpQN = (Tempo * 3) / 25000;

                        Tempo = Tempo * 60 / PpQN;

                        Buffer[2] = (uint8_t) (Tempo / 0x10000);
                        Buffer[3] = (uint8_t) (Tempo /   0x100);
                        Buffer[4] = (uint8_t) (Tempo);

                        if (CurrentTimestamp == 0)
                            IsTempoSet = true;
                    }
                }

                Track.AddEvent(MIDIEvent(CurrentTimestamp, MIDIEvent::Extended, 0, &Buffer[0], (size_t) (Size + 2)));

                if (Buffer[1] == MetaDataTypes::EndOfTrack)
                    break;
            }
            else
            if (Buffer[0] == StatusCodes::SysEx)
            {
                long Size = DecodeVariableLengthQuantity(it, end);

                if (Size < 0)
                    return SetLastErrorCode(MIDIError::InvalidSysExMessage);

                if (end - it < Size)
                    return SetLastErrorCode(MIDIError::InsufficientData);

                Buffer.resize((size_t) (Size + 1));
                std::copy(it, it + Size, Buffer.begin() + 1);
                it += Size;

                Track.AddEvent(MIDIEvent(CurrentTimestamp, MIDIEvent::Extended, 0, &Buffer[0], (size_t) (Size + 1)));
            }
            else
            if (Buffer[0] >= StatusCodes::NoteOff && Buffer[0] <= StatusCodes::ActiveSensing)
            {
                if (it == end)
                    return SetLastErrorCode(MIDIError::InsufficientData);

                Buffer.resize(3);

                Buffer[1] = *it++;
                uint32_t BytesRead = 1;

                MIDIEvent::EventType Type = (MIDIEvent::EventType) ((Buffer[0] >> 4) - 8);
                uint32_t Channel = (uint32_t) (Buffer[0] & 0x0F);

                if ((Type != MIDIEvent::ProgramChange) && (Type != MIDIEvent::ChannelAftertouch))
                {
                    if (it == end)
                        return SetLastErrorCode(MIDIError::InsufficientData);

                    Buffer[2] = *it++;
                    BytesRead = 2;
                }

                Track.AddEvent(MIDIEvent(CurrentTimestamp, Type, Channel, &Buffer[1], BytesRead));

                if (Type == MIDIEvent::NoteOn)
                {
                    Buffer[2] = 0x00;

                    int Length = DecodeVariableLengthQuantity(it, end);

                    if (Length < 0)
                        return SetLastErrorCode(MIDIError::XMIInvalidNoteMessage);

                    uint32_t Timestamp = CurrentTimestamp + Length;

                    if (Timestamp > LastEventTimestamp)
                        LastEventTimestamp = Timestamp;

                    Track.AddEvent(MIDIEvent(Timestamp, Type, Channel, &Buffer[1], BytesRead));
                }
            }
            else
                return SetLastErrorCode(MIDIError::UnknownStatusCode);
        }

        if (!IsTempoSet)
            Track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, XMIDefaultTempo, _countof(XMIDefaultTempo)));

        container.AddTrack(Track);
    }

    return true;
}

const uint8_t MIDIProcessor::XMIDefaultTempo[5] = { StatusCodes::MetaData, MetaDataTypes::SetTempo, 0x07, 0xA1, 0x20 };
