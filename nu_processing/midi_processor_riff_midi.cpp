#include "midi_processor.h"

bool midi_processor::is_riff_midi( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[4];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "RIFF", 4 ) ) return false;
		p_file->seek( 8, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "RMID", 4 ) ) return false;
		p_file->seek( 12, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( memcmp( buffer, "data", 4 ) ) return false;
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

static const char * riff_tag_mappings[][2] = 
{
	{ "IALB", "album" },
	{ "IARL", "archival_location" },
	{ "IART", "artist" },
	{ "ITRK", "tracknumber" },
	{ "ICMS", "commissioned" },
	{ "ICMP", "composer" },
	{ "ICMT", "comment" },
	{ "ICOP", "copyright" },
	{ "ICRD", "creation_date" },
	{ "IENG", "engineer" },
	{ "IGNR", "genre" },
	{ "IKEY", "keywords" },
	{ "IMED", "medium" },
	{ "INAM", "title" },
	{ "IPRD", "product" },
	{ "ISBJ", "subject" },
	{ "ISFT", "software" },
	{ "ISRC", "source" },
	{ "ISRF", "source_form" },
	{ "ITCH", "technician" }
};

void midi_processor::process_riff_midi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	unsigned char buffer[4];
	p_file->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "RIFF", 4 ) ) throw exception_io_data( "Not a RIFF file" );
	t_uint32 file_size;
	p_file->read_lendian_t( file_size, p_abort );
	file::ptr file_body = reader_limited::g_create( p_file, p_file->get_position( p_abort ), file_size, p_abort );

	file_body->read_object_t( buffer, p_abort );
	if ( memcmp( buffer, "RMID", 4 ) ) throw exception_io_data( "Not a RIFF MIDI file" );

	bool found_data = false;
	bool found_info = false;

	midi_meta_data meta_data;

	pfc::stringcvt::string_utf8_from_ansi convert;

	pfc::array_t<t_uint8> extra_buffer;

	while ( !file_body->is_eof( p_abort ) )
	{
		file_body->read_object_t( buffer, p_abort );
		t_uint32 chunk_size;
		file_body->read_lendian_t( chunk_size, p_abort );
		t_filesize chunk_position = file_body->get_position( p_abort );
		file::ptr chunk_body = reader_limited::g_create( file_body, chunk_position, chunk_size, p_abort );
		if ( !memcmp( buffer, "data", 4 ) )
		{
			if ( !found_data )
			{
				process_standard_midi( chunk_body, p_out, p_abort );
				found_data = true;
			}
			else throw exception_io_data( "Multiple RIFF data chunks found" );
		}
		else if ( !memcmp( buffer, "DISP", 4 ) )
		{
			t_uint32 type;
			chunk_body->read_lendian_t( type, p_abort );
			if ( type == 1 )
			{
				extra_buffer.grow_size( chunk_size - 4 );
				chunk_body->read_object( extra_buffer.get_ptr(), chunk_size - 4, p_abort );
				convert.convert( (const char *)extra_buffer.get_ptr(), chunk_size - 4 );
				meta_data.add_item( midi_meta_data_item( 0, "display_name", convert ) );
			}
			else chunk_body->skip( chunk_size, p_abort );
			if ( chunk_size & 1 && !chunk_body->is_eof( p_abort ) ) chunk_body->skip( 1, p_abort );
		}
		else if ( !memcmp( buffer, "LIST", 4 ) )
		{
			chunk_body->read_object_t( buffer, p_abort );
			if ( !memcmp( buffer, "INFO", 4 ) )
			{
				if ( !found_info )
				{
					while ( !chunk_body->is_eof( p_abort ) )
					{
						chunk_body->read_object_t( buffer, p_abort );
						t_uint32 field_size;
						chunk_body->read_lendian_t( field_size, p_abort );
						pfc::string8 field;
						field.set_string( (const char *)buffer, 4 );
						for ( unsigned i = 0; i < _countof(riff_tag_mappings); ++i )
						{
							if ( !memcmp( buffer, riff_tag_mappings[ i ][ 0 ], 4 ) )
							{
								field = riff_tag_mappings[ i ][ 1 ];
								break;
							}
						}
						extra_buffer.grow_size( field_size );
						chunk_body->read_object( extra_buffer.get_ptr(), field_size, p_abort );
						convert.convert( (const char *)extra_buffer.get_ptr(), field_size );
						meta_data.add_item( midi_meta_data_item( 0, field, convert ) );
						if ( field_size & 1 && !chunk_body->is_eof( p_abort ) ) chunk_body->skip( 1, p_abort );
					}
					found_info = true;
				}
				else throw exception_io_data( "Multiple RIFF LIST INFO chunks found" );
			}
		}
		if ( !file_body->is_eof( p_abort ) )
		{
			if ( chunk_size & 1 )
			{
				file_body->skip( 1, p_abort );
				++chunk_size;
			}
			chunk_position += chunk_size;
			if ( file_body->get_position( p_abort ) != chunk_position )
			{
				file_body->seek( chunk_position, p_abort );
			}
		}

		if ( found_data && found_info ) break;
	}

	p_out.set_extra_meta_data( meta_data );
}
