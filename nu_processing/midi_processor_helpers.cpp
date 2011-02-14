#include "midi_processor.h"

#include "../helpers/file_cached.h"

const t_uint8 midi_processor::end_of_track[2] = {0xFF, 0x2F};
const t_uint8 midi_processor::loop_start[11] = {0xFF, 0x06, 'l', 'o', 'o', 'p', 'S', 't', 'a', 'r', 't'};
const t_uint8 midi_processor::loop_end[9] =    {0xFF, 0x06, 'l', 'o', 'o', 'p', 'E', 'n', 'd'};

unsigned midi_processor::decode_delta( service_ptr_t<file> & p_file, abort_callback & p_abort )
{
	unsigned delta = 0;
	unsigned char byte;
	do
	{
		p_file->read_object_t( byte, p_abort );
		delta = ( delta << 7 ) + ( byte & 0x7F );
	}
	while ( byte & 0x80 );
	return delta;
}

void midi_processor::process_file( file::ptr & p_file, const char * p_extension, midi_container & p_out, abort_callback & p_abort )
{
	file::ptr temp;
	file_cached::g_create( temp, p_file, p_abort, 4096 );
	if ( is_standard_midi( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_standard_midi( temp, p_out, p_abort );
	}
	else if ( is_riff_midi( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_riff_midi( temp, p_out, p_abort );
	}
	else if ( is_hmp( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_hmp( temp, p_out, p_abort );
	}
	else if ( is_hmi( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_hmi( temp, p_out, p_abort );
	}
	else if ( is_xmi( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_xmi( temp, p_out, p_abort );
	}
	else if ( is_mus( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_mus( temp, p_out, p_abort );
	}
	else if ( is_mids( temp, p_abort ) )
	{
		temp->reopen( p_abort );
		process_mids( temp, p_out, p_abort );
	}
	else if ( is_lds( temp, p_extension, p_abort ) )
	{
		temp->reopen( p_abort );
		process_lds( temp, p_out, p_abort );
	}
	else throw exception_io_unsupported_format();
}
