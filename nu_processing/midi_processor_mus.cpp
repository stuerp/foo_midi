#include "midi_processor.h"

const t_uint8 midi_processor::mus_default_tempo[5] = {0xFF, 0x51, 0x09, 0xA3, 0x1A};

const t_uint8 midi_processor::mus_controllers[15] = {0,0,1,7,10,11,91,93,64,67,120,123,126,127,121};

bool midi_processor::is_mus( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[4];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		t_filesize size = p_file->get_size_ex( p_abort );
		if ( size < 0x20 || memcmp( buffer, "MUS\x1A", 4 ) ) return false;
		t_uint16 length;
		t_uint16 offset;
		t_uint16 instrument_count;
		p_file->read_lendian_t( length, p_abort );
		p_file->read_lendian_t( offset, p_abort );
		p_file->seek( 12, p_abort );
		p_file->read_lendian_t( instrument_count, p_abort );
		if ( offset >= 16 + instrument_count * 2 && offset < 16 + instrument_count * 4 && offset + length <= size ) return true;
		return false;
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

void midi_processor::process_mus( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	unsigned char buffer[ 4 ];
	p_file->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "MUS\x1A", 4 ) ) throw exception_io_data( "Not a MUS file" );
	
	t_uint16 length;
	t_uint16 offset;

	p_file->read_lendian_t( length, p_abort );
	p_file->read_lendian_t( offset, p_abort );

	p_out.initialize( 0, 0x59 );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, mus_default_tempo, _countof( mus_default_tempo ) ) );
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	midi_track track;

	unsigned current_timestamp = 0;

	t_uint8 velocity_levels[ 16 ] = { 0 };

	file::ptr track_body = reader_limited::g_create( p_file, offset, length, p_abort );

	while ( !track_body->is_eof( p_abort ) )
	{
		track_body->read_object_t( buffer[ 0 ], p_abort );
		if ( buffer[ 0 ] == 0x60 ) break;

		midi_event::event_type type;

		unsigned bytes_to_write;

		unsigned channel = buffer[ 0 ] & 0x0F;
		if ( channel == 0x0F ) channel = 9;
		else if ( channel >= 9 ) ++channel;

		switch ( buffer[ 0 ] & 0x70 )
		{
		case 0x00:
			type = midi_event::note_on;
			track_body->read_object_t( buffer[ 1 ], p_abort );
			buffer[ 2 ] = 0;
			bytes_to_write = 2;
			break;

		case 0x10:
			type = midi_event::note_on;
			track_body->read_object_t( buffer[ 1 ], p_abort );
			if ( buffer[ 1 ] & 0x80 )
			{
				track_body->read_object_t( buffer[ 2 ], p_abort );
				velocity_levels[ channel ] = buffer[ 2 ];
				buffer[ 1 ] &= 0x7F;
			}
			else
			{
				buffer[ 2 ] = velocity_levels[ channel ];
			}
			bytes_to_write = 2;
			break;

		case 0x20:
			type = midi_event::pitch_wheel;
			track_body->read_object_t( buffer[ 1 ], p_abort );
			buffer[ 2 ] = buffer[ 1 ] >> 1;
			buffer[ 1 ] <<= 7;
			bytes_to_write = 2;
			break;

		case 0x30:
			type = midi_event::control_change;
			track_body->read_object_t( buffer[ 1 ], p_abort );
			if ( buffer[ 1 ] >= 10 && buffer[ 1 ] <= 14 )
			{
				buffer[ 1 ] = mus_controllers[ buffer[ 1 ] ];
				buffer[ 2 ] = 1;
				bytes_to_write = 2;
			}
			else throw exception_io_data( "Unhandled MUS system event" );
			break;

		case 0x40:
			track_body->read_object_t( buffer[ 1 ], p_abort );
			if ( buffer[ 1 ] )
			{
				if ( buffer[ 1 ] < 10 )
				{
					type = midi_event::control_change;
					buffer[ 1 ] = mus_controllers[ buffer[ 1 ] ];
					track_body->read_object_t( buffer[ 2 ], p_abort );
					bytes_to_write = 2;
				}
				else throw exception_io_data( "Invalid MUS controller change event" );
			}
			else
			{
				type = midi_event::program_change;
				track_body->read_object_t( buffer[ 1 ], p_abort );
				bytes_to_write = 1;
			}
			break;

		default:
			throw exception_io_data( "Invalid MUS status code" );
		}

		track.add_event( midi_event( current_timestamp, type, channel, buffer + 1, bytes_to_write ) );

		if ( buffer[ 0 ] & 0x80 )
		{
			unsigned delta = decode_delta( track_body, p_abort );
			current_timestamp += delta;
		}
	}

	track.add_event( midi_event( current_timestamp, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );

	p_out.add_track( track );
}
