#include "midi_processor.h"

bool midi_processor::is_syx( file::ptr & p_file, abort_callback & p_abort )
{
	try
	{
		unsigned char buffer[1];
		p_file->reopen( p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( buffer[ 0 ] != 0xF0 ) return false;
		p_file->seek_ex( -1, foobar2000_io::file::seek_from_eof, p_abort );
		p_file->read_object_t( buffer, p_abort );
		if ( buffer[ 0 ] != 0xF7 ) return false;
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

void midi_processor::process_syx( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	pfc::array_t<t_uint8> buffer;

	t_filesize size64 = p_file->get_size_ex( p_abort );

	if ( size64 > (1 << 24) ) throw exception_io_data("File too large");

	t_size size = ( t_size ) size64;

	buffer.set_count( size );

	p_file->read_object( buffer.get_ptr(), size, p_abort );

	const t_uint8 * ptr = buffer.get_ptr(), * end = ptr + size;

	p_out.initialize( 0, 1 );

	midi_track track;

	while ( ptr < end )
	{
		const t_uint8 * msg_end = ptr + 1;

		while ( *msg_end++ != 0xF7 );

		track.add_event( midi_event( 0, midi_event::extended, 0, ptr, msg_end - ptr ) );

		ptr = msg_end;
	}

	p_out.add_track( track );
}