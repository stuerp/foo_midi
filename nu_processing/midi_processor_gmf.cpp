#include "midi_processor.h"

bool midi_processor::is_gmf( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[4];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		t_filesize size = p_file->get_size_ex( p_abort );
		if ( size < 0x20 || memcmp( buffer, "GMF\x01", 4 ) ) return false;
		return true;
	}
	catch (exception_io_data_truncation &)
	{
		return false;
	}
}

void midi_processor::process_gmf( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	t_uint8 buffer[10];
	p_file->read_object( buffer, 4, p_abort );
	t_filesize size = p_file->get_size_ex( p_abort );
	if ( size < 0x20 || memcmp( buffer, "GMF\x01", 4 ) ) throw exception_io_data( "Not a GMF file" );

	p_out.initialize( 0, 0xC0 );

	t_uint16 tempo;
	t_uint32 tempo_scaled;
	p_file->read_bendian_t( tempo, p_abort );
	tempo_scaled = tempo * 100000;

	midi_track track;

	buffer[0] = 0xFF;
	buffer[1] = 0x51;
	buffer[2] = tempo_scaled >> 16;
	buffer[3] = tempo_scaled >> 8;
	buffer[4] = tempo_scaled;

	track.add_event( midi_event( 0, midi_event::extended, 0, buffer, 5 ) );

	buffer[0] = 0xF0;
	buffer[1] = 0x41;
	buffer[2] = 0x10;
	buffer[3] = 0x16;
	buffer[4] = 0x12;
	buffer[5] = 0x7F;
	buffer[6] = 0x00;
	buffer[7] = 0x00;
	buffer[8] = 0x01;
	buffer[9] = 0xF7;

	track.add_event( midi_event( 0, midi_event::extended, 0, buffer, 10 ) );

	buffer[0] = 0xFF;
	buffer[1] = 0x2F;

	track.add_event( midi_event( 0, midi_event::extended, 0, buffer, 2 ) );

	p_out.add_track( track );

	file::ptr file_limited = reader_limited::g_create( p_file, 7, size - 7, p_abort );
	process_standard_midi_track( file_limited, p_out, false, p_abort );
}
