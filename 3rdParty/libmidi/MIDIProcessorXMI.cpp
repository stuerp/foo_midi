#include "MIDIProcessor.h"

#include <string.h>

struct IFFChunk
{
    uint8_t m_id[4];
    uint8_t m_type[4];
    std::vector<uint8_t> m_data;
    std::vector<IFFChunk> m_sub_chunks;

    IFFChunk()
    {
        memset(m_id, 0, sizeof(m_id));
        memset(m_type, 0, sizeof(m_type));
    }

    IFFChunk(const IFFChunk & p_in)
    {
        memcpy(m_id, p_in.m_id, sizeof(m_id));
        memcpy(m_type, p_in.m_type, sizeof(m_type));
        m_data = p_in.m_data;
        m_sub_chunks = p_in.m_sub_chunks;
    }

    const IFFChunk & find_sub_chunk(const char * p_id, uint32_t index = 0) const
    {
        for (std::size_t i = 0; i < m_sub_chunks.size(); ++i)
        {
            if (!memcmp(p_id, m_sub_chunks[i].m_id, 4))
            {
                if (index) --index;
                if (!index) return m_sub_chunks[i];
            }
        }
        /*throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );*/
        return *this;
    }

    uint32_t get_chunk_count(const char * p_id) const
    {
        uint32_t chunk_count = 0;
        for (std::size_t i = 0; i < m_sub_chunks.size(); ++i)
        {
            if (!memcmp(p_id, m_sub_chunks[i].m_id, 4))
            {
                ++chunk_count;
            }
        }
        return chunk_count;
    }
};

struct IFFStream
{
    std::vector<IFFChunk> m_chunks;

    IFFChunk fail;

    const IFFChunk & find_chunk(const char * p_id) const
    {
        for (std::size_t i = 0; i < m_chunks.size(); ++i)
        {
            if (!memcmp(p_id, m_chunks[i].m_id, 4))
            {
                return m_chunks[i];
            }
        }
        /*throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );*/
        return fail;
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

static bool ReadChunk(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, IFFChunk & p_out, bool first_chunk)
{
    if (end - it < 8)
        return false;

    std::copy(it, it + 4, p_out.m_id);
    it += 4;

    uint32_t chunk_size = (uint32_t) (it[0] << 24) | (it[1] << 16) | (it[2] << 8) | it[3];

    if ((size_t) (end - it) < chunk_size)
        return false;

    it += 4;
    bool is_cat_chunk = !memcmp(p_out.m_id, "CAT ", 4);
    bool is_form_chunk = !memcmp(p_out.m_id, "FORM", 4);

    std::size_t chunk_size_limit = (size_t) (end - it);

    if (chunk_size > chunk_size_limit) chunk_size = (uint32_t)
        chunk_size_limit;

    if ((first_chunk && is_form_chunk) || (!first_chunk && is_cat_chunk))
    {
        if (end - it < 4) return false;
        std::vector<uint8_t>::const_iterator chunk_end = it + chunk_size;
        std::copy(it, it + 4, p_out.m_type);
        it += 4;
        while (it < chunk_end)
        {
            IFFChunk chunk;
            if (!ReadChunk(it, chunk_end, chunk, is_cat_chunk)) return false;
            p_out.m_sub_chunks.push_back(chunk);
        }
        it = chunk_end;
        if (chunk_size & 1 && it != end) ++it;
    }
    else if (!is_form_chunk && !is_cat_chunk)
    {
        p_out.m_data.assign(it, it + chunk_size);
        it += chunk_size;
        if (chunk_size & 1 && it != end) ++it;
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
    std::vector<uint8_t>::const_iterator it = data.begin(), end = data.end();

    bool first_chunk = true;

    while (it != end)
    {
        IFFChunk chunk;

        if (ReadChunk(it, end, chunk, first_chunk))
        {
            stream.m_chunks.push_back(chunk);
            first_chunk = false;
        }
        else
            if (first_chunk)
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

    const IFFChunk & form_chunk = Stream.find_chunk("FORM");

    if (::memcmp(form_chunk.m_type, "XDIR", 4))
        return SetLastErrorCode(MIDIError::XMIFORMXDIRNotFound);

    const IFFChunk & cat_chunk = Stream.find_chunk("CAT ");

    if (::memcmp(cat_chunk.m_type, "XMID", 4))
        return SetLastErrorCode(MIDIError::XMICATXMIDNotFound);

    trackCount = cat_chunk.get_chunk_count("FORM");

    return true;
}

bool MIDIProcessor::ProcessXMI(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    SetLastErrorCode(MIDIError::None);

    IFFStream Stream;

    if (!ReadStream(data, Stream))
        return false;

    const IFFChunk & FORMChunk = Stream.find_chunk("FORM");

    if (memcmp(FORMChunk.m_type, "XDIR", 4))
        return SetLastErrorCode(MIDIError::XMIFORMXDIRNotFound);

    const IFFChunk & CATChunk = Stream.find_chunk("CAT ");

    if (memcmp(CATChunk.m_type, "XMID", 4))
        return SetLastErrorCode(MIDIError::XMICATXMIDNotFound);

    uint32_t TrackCount = CATChunk.get_chunk_count("FORM");

    container.Initialize(TrackCount > 1 ? 2u : 0u, 60);

    for (uint32_t i = 0; i < TrackCount; ++i)
    {
        const IFFChunk & FORMChunk = CATChunk.find_sub_chunk("FORM", i);

        if (memcmp(FORMChunk.m_type, "XMID", 4))
            return SetLastErrorCode(MIDIError::XMIFORMXMIDNotFound);

        const IFFChunk & EVNTChunk = FORMChunk.find_sub_chunk("EVNT");

        if (memcmp(EVNTChunk.m_id, "EVNT", 4))
            return SetLastErrorCode(MIDIError::XMIEVNTChunkNotFound);

        std::vector<uint8_t> const & Data = EVNTChunk.m_data;

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
