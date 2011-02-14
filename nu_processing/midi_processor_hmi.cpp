#include "midi_processor.h"

bool midi_processor::is_hmi( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[12];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "HMI-MIDISONG", 12 ) ) return false;
		return true;
	}
	catch (exception_io_data_truncation &)
	{
		return false;
	}
}

void midi_processor::process_hmi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	pfc::array_t<t_uint8> buffer;
	buffer.set_count( 12 );

	p_file->read_object( buffer.get_ptr(), 12, p_abort );
	if ( memcmp( buffer.get_ptr(), "HMI-MIDISONG", 12 ) ) throw exception_io_data( "Not an HMI file" );

	p_file->seek( 0xE4, p_abort );

	t_uint32 track_count;
	t_uint32 track_table_offset;

	p_file->read_lendian_t( track_count, p_abort );
	p_file->read_lendian_t( track_table_offset, p_abort );

	p_file->seek( track_table_offset, p_abort );

	pfc::array_t<t_uint32> track_offsets;
	track_offsets.set_count( track_count );
	for ( unsigned i = 0; i < track_count; ++i )
	{
		p_file->read_lendian_t( track_offsets[ i ], p_abort );
	}

	p_out.initialize( 1, 0xC0 );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, hmp_default_tempo, _countof( hmp_default_tempo ) ) );
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	for ( unsigned i = 0; i < track_count; ++i )
	{
		unsigned track_offset = track_offsets[ i ];
		unsigned track_length;
		if ( i + 1 < track_count )
		{
			track_length = track_offsets[ i + 1 ] - track_offset;
		}
		else
		{
			track_length = p_file->get_size_ex( p_abort ) - track_offset;
		}
		file::ptr track_body = reader_limited::g_create( p_file, track_offset, track_length, p_abort );

		buffer.grow_size( 13 );
		track_body->read_object( buffer.get_ptr(), 13, p_abort );
		if ( memcmp( buffer.get_ptr(), "HMI-MIDITRACK", 13 ) ) throw exception_io_data( "Invalid HMI track header" );

		midi_track track;
		unsigned current_timestamp = 0;
		unsigned char last_event_code = 0xFF;

		unsigned last_event_timestamp = 0;

		t_uint32 meta_offset;
		track_body->seek( 0x4B, p_abort );
		track_body->read_lendian_t( meta_offset, p_abort );
		if ( meta_offset )
		{
			track_body->seek( meta_offset, p_abort );
			p_file->read_object( buffer.get_ptr(), 2, p_abort );
			unsigned meta_size = buffer[ 1 ];
			buffer.grow_size( meta_size + 2 );
			p_file->read_object( buffer.get_ptr() + 2, meta_size, p_abort );
			while ( meta_size > 0 && buffer[ meta_size + 1 ] == ' ' ) --meta_size;
			buffer[ 0 ] = 0xFF;
			buffer[ 1 ] = 0x01;
			track.add_event( midi_event( 0, midi_event::extended, 0, buffer.get_ptr(), meta_size + 2 ) );
		}

		t_uint32 track_data_offset;
		track_body->seek( 0x57, p_abort );
		track_body->read_lendian_t( track_data_offset, p_abort );
		track_body->seek( track_data_offset, p_abort );

		while ( !track_body->is_eof( p_abort ) )
		{
			unsigned delta = decode_delta( track_body, p_abort );
			if ( delta > 0xFFFF )
			{
				current_timestamp = last_event_timestamp;
				console::formatter() << "[foo_midi] Large HMI delta detected, shunting.";
			}
			else
			{
				current_timestamp += delta;
				if ( current_timestamp > last_event_timestamp )
				{
					last_event_timestamp = current_timestamp;
				}
			}

			track_body->read_object_t( buffer[ 0 ], p_abort );
			if ( buffer[ 0 ] == 0xFF )
			{
				last_event_code = 0xFF;
				track_body->read_object_t( buffer[ 1 ], p_abort );
				unsigned meta_count = decode_delta( track_body, p_abort );
				buffer.grow_size( meta_count + 2 );
				track_body->read_object( buffer.get_ptr() + 2, meta_count, p_abort );
				if ( buffer[ 1 ] == 0x2F && last_event_timestamp > current_timestamp )
				{
					current_timestamp = last_event_timestamp;
				}
				track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), meta_count + 2 ) );
				if ( buffer[ 1 ] == 0x2F ) break;
			}
			else if ( buffer[ 0 ] == 0xF0 )
			{
				last_event_code = 0xFF;
				unsigned system_exclusive_count = decode_delta( track_body, p_abort );
				buffer.grow_size( system_exclusive_count + 1 );
				track_body->read_object( buffer.get_ptr() + 1, system_exclusive_count, p_abort );
				track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), system_exclusive_count + 1 ) );
			}
			else if ( buffer[ 0 ] == 0xFE )
			{
				last_event_code = 0xFF;
				track_body->read_object_t( buffer[ 1 ], p_abort );
				if ( buffer[ 1 ] == 0x10 )
				{
					track_body->skip( 2, p_abort );
					track_body->read_object_t( buffer[ 2 ], p_abort );
					track_body->skip( buffer[ 2 ] + 4, p_abort );
				}
				else if ( buffer[ 1 ] == 0x12 )
				{
					track_body->skip( 2, p_abort );
				}
				else if ( buffer[ 1 ] == 0x13 )
				{
					track_body->skip( 10, p_abort );
				}
				else if ( buffer[ 1 ] == 0x14 )
				{
					track_body->skip( 2, p_abort );
					p_out.add_track_event( 0, midi_event( current_timestamp, midi_event::extended, 0, loop_start, _countof( loop_start ) ) );
				}
				else if ( buffer[ 1 ] == 0x15 )
				{
					track_body->skip( 6, p_abort );
					p_out.add_track_event( 0, midi_event( current_timestamp, midi_event::extended, 0, loop_end, _countof( loop_end ) ) );
				}
				else throw exception_io_data( "Unexpected HMI meta event" );
			}
			else if ( buffer[ 0 ] <= 0xEF )
			{
				unsigned bytes_read = 1;
				if ( buffer[ 0 ] >= 0x80 )
				{
					track_body->read_object_t( buffer[ 1 ], p_abort );
					last_event_code = buffer[ 0 ];
				}
				else
				{
					if ( last_event_code == 0xFF ) throw exception_io_data( "HMI used shortened event after Meta or SysEx message" );
					buffer[ 1 ] = buffer[ 0 ];
					buffer[ 0 ] = last_event_code;
				}
				midi_event::event_type type = (midi_event::event_type)( ( buffer[ 0 ] >> 4 ) - 8 );
				unsigned channel = buffer[ 0 ] & 0x0F;
				if ( type != midi_event::program_change && type != midi_event::channel_aftertouch )
				{
					track_body->read_object_t( buffer[ 2 ], p_abort );
					bytes_read = 2;
				}
				track.add_event( midi_event( current_timestamp, type, channel, buffer.get_ptr() + 1, bytes_read ) );
				if ( type == midi_event::note_on )
				{
					buffer[ 2 ] = 0x00;
					unsigned note_length = decode_delta( track_body, p_abort );
					unsigned note_end_timestamp = current_timestamp + note_length;
					if ( note_end_timestamp > last_event_timestamp ) last_event_timestamp = note_end_timestamp;
					track.add_event( midi_event( note_end_timestamp, midi_event::note_on, channel, buffer.get_ptr() + 1, bytes_read ) );
				}
			}
			else throw exception_io_data( "Unexpected HMI status code" );
		}

		p_out.add_track( track );
	}
}
