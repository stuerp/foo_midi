#include "midi_processor.h"

bool midi_processor::is_standard_midi( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[4];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "MThd", 4 ) ) return false;
		p_file->read_object_t( buffer, p_abort );
		if ( buffer[ 0 ] != 0 || buffer[ 1 ] != 0 || buffer[ 2 ] != 0 || buffer[ 3 ] != 6 ) return false;
		p_file->seek( 14, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "MTrk", 4 ) ) return false;
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

void midi_processor::process_standard_midi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	unsigned char buffer[4];
	p_file->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "MThd", 4 ) ) throw exception_io_data("Bad MIDI header signature");
	p_file->read_object_t( buffer, p_abort );
	if ( buffer[ 0 ] != 0 || buffer[ 1 ] != 0 || buffer[ 2 ] != 0 || buffer[ 3 ] != 6 ) throw exception_io_data("Bad MIDI header size");
	t_uint16 form;
	t_uint16 track_count_16;
	t_uint16 dtx;
	t_size track_count;
	p_file->read_bendian_t( form, p_abort );
	if ( form > 1 ) throw exception_io_data();
	p_file->read_bendian_t( track_count_16, p_abort );
	p_file->read_bendian_t( dtx, p_abort );
	track_count = track_count_16;

	p_out.initialize( form, dtx );

	for ( t_size i = 0; i < track_count; ++i )
	{
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "MTrk", 4 ) ) throw exception_io_data("Bad MIDI track signature");
		t_uint32 track_size;
		p_file->read_bendian_t( track_size, p_abort );
		t_filesize track_data_offset = p_file->get_position( p_abort );
		file::ptr file_limited = reader_limited::g_create( p_file, track_data_offset, track_size, p_abort );

		midi_track track;
		unsigned current_timestamp = 0;
		unsigned char last_event_code = 0xFF;

		pfc::array_t<t_uint8> buffer;
		buffer.grow_size( 3 );

		try
		{
			for (;;)
			{
				unsigned delta = decode_delta( file_limited, p_abort );
				current_timestamp += delta;
				unsigned char event_code;
				file_limited->read_object_t( event_code, p_abort );
				unsigned data_bytes_read = 0;
				if ( event_code < 0x80 )
				{
					if ( last_event_code == 0xFF ) throw exception_io_data("First MIDI track event short encoded");
					buffer[ data_bytes_read++ ] = event_code;
					event_code = last_event_code;
				}
				if ( event_code < 0xF0 )
				{
					last_event_code = event_code;
					if ( data_bytes_read < 1 )
					{
						file_limited->read_object_t( buffer[ 0 ], p_abort );
						++data_bytes_read;
					}
					switch ( event_code & 0xF0 )
					{
					case 0xC0:
					case 0xD0:
						break;
					default:
						file_limited->read_object_t( buffer[ data_bytes_read ], p_abort );
						++data_bytes_read;
					}
					track.add_event( midi_event( current_timestamp, (midi_event::event_type)(( event_code >> 4 ) - 8), event_code & 0x0F, buffer.get_ptr(), data_bytes_read ) );
				}
				else if ( event_code == 0xF0 )
				{
					unsigned data_count = decode_delta( file_limited, p_abort );
					buffer.grow_size( data_count + 1 );
					buffer[ 0 ] = 0xF0;
					file_limited->read_object( buffer.get_ptr() + 1, data_count, p_abort );
					track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), data_count + 1 ) );
				}
				else if ( event_code == 0xFF )
				{
					unsigned char meta_type;
					file_limited->read_object_t( meta_type, p_abort );
					unsigned data_count = decode_delta( file_limited, p_abort );
					buffer.grow_size( data_count + 2 );
					buffer[ 0 ] = 0xFF;
					buffer[ 1 ] = meta_type;
					file_limited->read_object( buffer.get_ptr() + 2, data_count, p_abort );
					track.add_event( midi_event( current_timestamp, midi_event::extended, 0, buffer.get_ptr(), data_count + 2 ) );

					if ( meta_type == 0x2F ) break;
				}
				else if ( event_code == 0xFB || event_code == 0xFC )
				{
					console::formatter() << "[foo_midi] MIDI " << ( ( event_code == 0xFC ) ? "stop" : "start" ) << " status code ignored";
				}
				else throw exception_io_data("Unhandled MIDI status code");
			}
		}
		catch (exception_io_data_truncation &)
		{
			throw exception_io_data_truncation("MIDI track truncated");
		}

		file_limited.release();

		track_data_offset += track_size;
		if ( p_file->get_position( p_abort ) != track_data_offset )
		{
			p_file->seek( track_data_offset, p_abort );
		}

		p_out.add_track( track );
	}
}
