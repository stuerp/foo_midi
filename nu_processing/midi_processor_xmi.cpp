#include "midi_processor.h"

bool midi_processor::is_xmi( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[4];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "FORM", 4 ) ) return false;
		p_file->seek( 8, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "XDIR", 4 ) ) return false;
		p_file->seek( 0x1E, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "XMID", 4 ) ) return false;
		return true;
	}
	catch (exception_io_data_truncation &)
	{
		return false;
	}
	catch (exception_io_seek_out_of_range &)
	{
		return false;
	}
}

const t_uint8 midi_processor::xmi_default_tempo[5] = {0xFF, 0x51, 0x20, 0x8D, 0xB7};

unsigned midi_processor::decode_xmi_delta( file::ptr & p_file, abort_callback & p_abort )
{
	unsigned delta = 0;
	unsigned bytes_read = 1;
	t_uint8 byte;
	p_file->read_object_t( byte, p_abort );
	if ( !( byte & 0x80 ) )
	{
		do
		{
			delta += byte;
			p_file->read_object_t( byte, p_abort );
		}
		while ( !( byte & 0x80 ) && !p_file->is_eof( p_abort ) );
	}
	p_file->seek_ex( -1, file::seek_from_current, p_abort );
	return delta;
}

struct iff_chunk
{
	t_uint8 m_id[4];
	t_uint8 m_type[4];
	file::ptr m_data;
	pfc::array_t<iff_chunk> m_sub_chunks;

	iff_chunk()
	{
		memset( m_id, 0, sizeof( m_id ) );
		memset( m_type, 0, sizeof( m_type ) );
	}

	iff_chunk( const iff_chunk & p_in )
	{
		memcpy( m_id, p_in.m_id, sizeof( m_id ) );
		memcpy( m_type, p_in.m_type, sizeof( m_type ) );
		m_data = p_in.m_data;
		m_sub_chunks = p_in.m_sub_chunks;
	}

	const iff_chunk & find_sub_chunk( const char * p_id, unsigned index = 0 ) const
	{
		for ( t_size i = 0; i < m_sub_chunks.get_count(); ++i )
		{
			if ( !memcmp( p_id, m_sub_chunks[ i ].m_id, 4 ) )
			{
				if ( index ) --index;
				if ( !index ) return m_sub_chunks[ i ];
			}
		}
		throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );
	}

	unsigned get_chunk_count( const char * p_id ) const
	{
		unsigned chunk_count = 0;
		for ( t_size i = 0; i < m_sub_chunks.get_count(); ++i )
		{
			if ( !memcmp( p_id, m_sub_chunks[ i ].m_id, 4 ) )
			{
				++chunk_count;
			}
		}
		return chunk_count;
	}
};

struct iff_stream
{
	pfc::array_t<iff_chunk> m_chunks;

	const iff_chunk & find_chunk( const char * p_id ) const
	{
		for ( t_size i = 0; i < m_chunks.get_count(); ++i )
		{
			if ( !memcmp( p_id, m_chunks[ i ].m_id, 4 ) )
			{
				return m_chunks[ i ];
			}
		}
		throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );
	}
};

static void read_iff_chunk( file::ptr & p_file, iff_chunk & p_out, bool first_chunk, abort_callback & p_abort )
{
	p_file->read_object_t( p_out.m_id, p_abort );
	t_uint32 chunk_size;
	p_file->read_bendian_t( chunk_size, p_abort );
	bool is_cat_chunk = !memcmp( p_out.m_id, "CAT ", 4 );
	bool is_form_chunk = !memcmp( p_out.m_id, "FORM", 4 );
	if ( ( first_chunk && is_form_chunk ) || ( !first_chunk && is_cat_chunk ) )
	{
		file::ptr chunk_body = reader_limited::g_create( p_file, p_file->get_position( p_abort ), chunk_size, p_abort );
		chunk_body->read_object_t( p_out.m_type, p_abort );
		while ( !chunk_body->is_eof( p_abort ) )
		{
			iff_chunk chunk;
			read_iff_chunk( chunk_body, chunk, is_cat_chunk, p_abort );
			p_out.m_sub_chunks.append_single( chunk );
		}
		chunk_body->seek_ex( 0, file::seek_from_eof, p_abort );
		if ( chunk_size & 1 ) p_file->skip( 1, p_abort );
	}
	else if ( !is_form_chunk && !is_cat_chunk )
	{
		p_out.m_data = reader_limited::g_create( p_file, p_file->get_position( p_abort ), chunk_size, p_abort );
		p_file->skip( chunk_size, p_abort );
		if ( chunk_size & 1 ) p_file->skip( 1, p_abort );
	}
	else
	{
		if ( first_chunk ) throw exception_io_data( pfc::string_formatter() << "Found " << pfc::string8( (const char *)p_out.m_id, 4 ) << " chunk instead of FORM" );
		else throw exception_io_data( "Found multiple FORM chunks" );
	}
}

static void read_iff_stream( file::ptr & p_file, iff_stream & p_out, abort_callback & p_abort )
{
	bool first_chunk = true;
	while ( !p_file->is_eof( p_abort ) )
	{
		iff_chunk chunk;
		read_iff_chunk( p_file, chunk, first_chunk, p_abort );
		p_out.m_chunks.append_single( chunk );
		first_chunk = false;
	}
}

void midi_processor::process_xmi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	iff_stream xmi_file;
	read_iff_stream( p_file, xmi_file, p_abort );

	const iff_chunk & form_chunk = xmi_file.find_chunk( "FORM" );
	if ( memcmp( form_chunk.m_type, "XDIR", 4 ) ) throw exception_io_data( "XMI IFF not XDIR type" );

	const iff_chunk & cat_chunk = xmi_file.find_chunk( "CAT " );
	if ( memcmp( cat_chunk.m_type, "XMID", 4 ) ) throw exception_io_data( "XMI CAT chunk not XMID type" );

	const iff_chunk & xmid_form_chunk = cat_chunk.find_sub_chunk( "FORM" );
	if ( memcmp( xmid_form_chunk.m_type, "XMID", 4 ) ) throw exception_io_data( "XMI nested FORM chunk not XMID type" );

	unsigned track_count = xmid_form_chunk.get_chunk_count( "EVNT" );

	p_out.initialize( track_count > 1 ? 2 : 0, 0x100 );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, xmi_default_tempo, _countof( xmi_default_tempo ) ) );
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	for ( unsigned i = 0; i < track_count; ++i )
	{
		const iff_chunk & event_chunk = xmid_form_chunk.find_sub_chunk( "EVNT", i );
		file::ptr event_body = event_chunk.m_data;
		event_body->reopen( p_abort );

		midi_track track;

		unsigned current_timestamp = 0;

		unsigned last_event_timestamp = 0;

		pfc::array_t<t_uint8> buffer;
		buffer.set_count( 3 );

		while ( !event_body->is_eof( p_abort ) )
		{
			unsigned delta = decode_xmi_delta( event_body, p_abort );
			current_timestamp += delta;

			if ( current_timestamp > last_event_timestamp )
			{
				last_event_timestamp = current_timestamp;
			}

			event_body->read_object_t( buffer[ 0 ], p_abort );
			if ( buffer[ 0 ] == 0xFF )
			{
				event_body->read_object_t( buffer[ 1 ], p_abort );
				unsigned meta_count = decode_delta( event_body, p_abort );
				buffer.grow_size( meta_count + 2 );
				event_body->read_object( buffer.get_ptr() + 2, meta_count, p_abort );
				if ( buffer[ 1 ] == 0x2F && current_timestamp < last_event_timestamp )
				{
					current_timestamp = last_event_timestamp;
				}
				track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), meta_count + 2 ) );
				if ( buffer[ 1 ] == 0x2F ) break;
			}
			else if ( buffer[ 0 ] == 0xF0 )
			{
				unsigned system_exclusive_count = decode_delta( event_body, p_abort );
				buffer.grow_size( system_exclusive_count + 1 );
				event_body->read_object( buffer.get_ptr() + 1, system_exclusive_count, p_abort );
				track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), system_exclusive_count + 1 ) );
			}
			else if ( buffer[ 0 ] >= 0x80 && buffer[ 0 ] <= 0xEF )
			{
				unsigned bytes_read = 1;
				event_body->read_object_t( buffer[ 1 ], p_abort );
				midi_event::event_type type = (midi_event::event_type)( ( buffer[ 0 ] >> 4 ) - 8 );
				unsigned channel = buffer[ 0 ] & 0x0F;
				if ( type != midi_event::program_change && type != midi_event::channel_aftertouch )
				{
					event_body->read_object_t( buffer[ 2 ], p_abort );
					bytes_read = 2;
				}
				track.add_event( midi_event( current_timestamp, type, channel, buffer.get_ptr() + 1, bytes_read ) );
				if ( type == midi_event::note_on )
				{
					buffer[ 2 ] = 0x00;
					unsigned note_length = decode_delta( event_body, p_abort );
					unsigned note_end_timestamp = current_timestamp + note_length;
					if ( note_end_timestamp > last_event_timestamp ) last_event_timestamp = note_end_timestamp;
					track.add_event( midi_event( note_end_timestamp, type, channel, buffer.get_ptr() + 1, bytes_read ) );
				}
			}
			else throw exception_io_data( "Unexpected XMI status code" );
		}

		p_out.add_track( track );
	}
}
