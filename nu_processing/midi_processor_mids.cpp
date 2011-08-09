#include "midi_processor.h"

bool midi_processor::is_mids( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[ 4 ];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "RIFF", 4 ) ) return false;
		p_file->seek( 8, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "MIDS", 4 ) ) return false;
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "fmt ", 4 ) ) return false;
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

void midi_processor::process_mids( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	unsigned char buffer[ 4 ];
	p_file->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "RIFF", 4 ) ) throw exception_io_data( "MIDS not a RIFF file" );
	t_uint32 file_size;
	p_file->read_lendian_t( file_size, p_abort );
	file::ptr riff_body = reader_limited::g_create( p_file, 8, file_size, p_abort );

	riff_body->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "MIDS", 4 ) ) throw exception_io_data( "Not a RIFF MIDS file" );

	riff_body->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "fmt ", 4 ) ) throw exception_io_data( "MIDS missing RIFF fmt chunk" );

	t_uint32 fmt_size;
	riff_body->read_lendian_t( fmt_size, p_abort );

	t_uint32 time_format = 1;
	t_uint32 max_buffer = 0;
	t_uint32 flags = 0;

	if ( fmt_size >= 4 )
	{
		riff_body->read_lendian_t( time_format, p_abort );
		fmt_size -= 4;
	}
	if ( fmt_size >= 4 )
	{
		riff_body->read_lendian_t( max_buffer, p_abort );
		fmt_size -= 4;
	}
	if ( fmt_size >= 4 )
	{
		riff_body->read_lendian_t( flags, p_abort );
		fmt_size -= 4;
	}

	riff_body->skip( fmt_size, p_abort );
	if ( fmt_size & 1 ) riff_body->skip( 1, p_abort );

	p_out.initialize( 0, time_format );

	riff_body->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "data", 4 ) ) throw exception_io_data( "MIDS missing RIFF data chunk" );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	t_uint32 data_size;
	riff_body->read_lendian_t( data_size, p_abort );

	file::ptr data_body = reader_limited::g_create( riff_body, riff_body->get_position( p_abort ), data_size, p_abort );

	t_uint32 segment_count;
	data_body->read_lendian_t( segment_count, p_abort );

	bool is_eight_byte = !!(flags & 1);

	data_body->seek( 4, p_abort );

	midi_track track;

	unsigned current_timestamp = 0;

	for ( unsigned i = 0; i < segment_count; ++i )
	{
		t_uint32 segment_size;
		data_body->skip( 4, p_abort );
		data_body->read_lendian_t( segment_size, p_abort );
		file::ptr segment_body = reader_limited::g_create( data_body, data_body->get_position( p_abort ), segment_size, p_abort );
		while ( !segment_body->is_eof( p_abort ) )
		{
			t_uint32 delta;
			t_uint32 event;
			data_body->read_lendian_t( delta, p_abort );
			current_timestamp += delta;
			if ( !is_eight_byte ) data_body->skip( 4, p_abort );
			data_body->read_lendian_t( event, p_abort );
			if ( event >> 24 == 0x01 )
			{
				t_uint8 buffer[ 5 ] = { 0xFF, 0x51 };
				buffer[ 2 ] = (t_uint8)( event >> 16 );
				buffer[ 3 ] = (t_uint8)( event >> 8 );
				buffer[ 4 ] = (t_uint8)event;
				p_out.add_track_event( 0, midi_event( current_timestamp, midi_event::extended, 0, buffer, sizeof( buffer ) ) );
			}
			else if ( !( event >> 24 ) )
			{
				unsigned event_code = ( event & 0xF0 ) >> 4;
				if ( event_code >= 0x8 && event_code <= 0xE )
				{
					unsigned bytes_to_write = 1;
					t_uint8 buffer[2];
					buffer[ 0 ] = (t_uint8)( event >> 8 );
					if ( event_code != 0xC && event_code != 0xD )
					{
						buffer[ 1 ] = (t_uint8)( event >> 16 );
						bytes_to_write = 2;
					}
					track.add_event( midi_event( current_timestamp, (midi_event::event_type)( event_code - 8 ), event & 0x0F, buffer, bytes_to_write ) );
				}
			}
		}
	}

	track.add_event( midi_event( current_timestamp, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );

	p_out.add_track( track );
}
