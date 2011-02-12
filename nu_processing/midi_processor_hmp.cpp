#include "midi_processor.h"

const t_uint8 midi_processor::hmp_default_tempo[5] = {0xFF, 0x51, 0x18, 0x80, 0x00};

bool midi_processor::is_hmp( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[4];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "HMIM", 4 ) ) return false;
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "IDIP", 4 ) && memcmp( buffer, "IDIR", 4 ) ) return false;
		return true;
	}
	catch (exception_io_data_truncation &)
	{
		return false;
	}
}

unsigned midi_processor::decode_hmp_delta( file::ptr & p_file, abort_callback & p_abort )
{
	unsigned delta = 0;
	unsigned shift = 0;
	unsigned char byte;
	do
	{
		p_file->read_object_t( byte, p_abort );
		delta = delta + ( ( byte & 0x7F ) << shift );
		shift += 7;
	}
	while ( !( byte & 0x80 ) );
	return delta;
}

void midi_processor::process_hmp( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	unsigned char buffer[4];
	p_file->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "HMIM", 4 ) ) throw exception_io_data( "Not a HMP file" );
	p_file->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "IDIP", 4 ) && memcmp( buffer, "IDIR", 4 ) ) throw exception_io_data( "Not a HMP file" );

	bool is_funky = buffer[3] == 'R';

	t_uint8 track_count_8;
	t_uint16 dtx = 0xC0;

	p_file->seek( is_funky ? 0x1A : 0x30, p_abort );
	p_file->read_object_t( track_count_8, p_abort );

	if ( is_funky )
	{
		p_file->seek( 0x4C, p_abort );
		p_file->read_bendian_t( dtx, p_abort );
	}

	p_out.initialize( 1, dtx );

	p_file->seek( is_funky ? 0x1A : 0x30, p_abort );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, hmp_default_tempo, _countof( hmp_default_tempo ) ) );
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	p_file->read_object_t( buffer[ 0 ], p_abort );
	while ( !p_file->is_eof( p_abort ) )
	{
		if ( buffer[ 0 ] != 0xFF )
		{
			p_file->read_object_t( buffer[ 0 ], p_abort );
			continue;
		}
		p_file->read_object_t( buffer[ 1 ], p_abort );
		if ( buffer[ 1 ] != 0x2F )
		{
			buffer[ 0 ] = buffer[ 1 ];
			continue;
		}
		break;
	}

	p_file->skip( is_funky ? 3 : 5, p_abort );

	unsigned track_count = track_count_8;

	for ( unsigned i = 1; i < track_count; ++i )
	{
		t_uint16 track_size_16;
		t_uint32 track_size_32;

		if ( is_funky )
		{
			p_file->read_lendian_t( track_size_16, p_abort );
			track_size_32 = track_size_16 - 4;
			if ( p_file->get_size_ex( p_abort ) - p_file->get_position( p_abort ) < track_size_32 + 2 ) break;
			p_file->skip( 2, p_abort );
		}
		else
		{
			p_file->read_lendian_t( track_size_32, p_abort );
			track_size_32 -= 12;
			if ( p_file->get_size_ex( p_abort ) - p_file->get_position( p_abort ) < track_size_32 + 8 ) break;
			p_file->skip( 4, p_abort );
		}

		midi_track track;

		unsigned current_timestamp = 0;

		pfc::array_t<t_uint8> buffer;
		buffer.set_count( 3 );

		t_filesize track_data_offset = p_file->get_position( p_abort );
		file::ptr track_body = reader_limited::g_create( p_file, track_data_offset, track_size_32, p_abort );

		while ( !track_body->is_eof( p_abort ) )
		{
			unsigned delta = decode_hmp_delta( track_body, p_abort );
			current_timestamp += delta;
			track_body->read_object_t( buffer[ 0 ], p_abort );
			if ( buffer[ 0 ] == 0xFF )
			{
				track_body->read_object_t( buffer[ 1 ], p_abort );
				unsigned meta_count = decode_delta( track_body, p_abort );
				buffer.grow_size( meta_count + 2 );
				track_body->read_object( buffer.get_ptr() + 2, meta_count, p_abort );
				track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), meta_count + 2 ) );
				if ( buffer[ 1 ] == 0x2F ) break;
			}
			else if ( buffer[ 0 ] >= 0x80 && buffer[ 0 ] <= 0xEF )
			{
				unsigned bytes_read = 2;
				switch ( buffer[ 0 ] & 0xF0 )
				{
				case 0xC0:
				case 0xD0:
					bytes_read = 1;
				}
				track_body->read_object( buffer.get_ptr() + 1, bytes_read, p_abort );
				track.add_event( midi_event( current_timestamp, (midi_event::event_type)( ( buffer[ 0 ] >> 4 ) - 8 ), buffer[ 0 ] & 0x0F, buffer.get_ptr() + 1, bytes_read ) );
			}
			else throw exception_io_data( "Unexpected status code in HMP track" );
		}

		track_data_offset += track_size_32 + ( is_funky ? 0 : 4 );
		if ( p_file->get_position( p_abort ) < track_data_offset )
		{
			p_file->seek( track_data_offset, p_abort );
		}

		p_out.add_track( track );
	}
}
