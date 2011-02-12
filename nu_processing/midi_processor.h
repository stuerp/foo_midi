#ifndef _MIDI_PROCESSORS_H_
#define _MIDI_PROCESSORS_H_

#include <foobar2000.h>

#include "midi_container.h"

class midi_processor
{
	static const t_uint8 end_of_track[2];

	static const t_uint8 hmp_default_tempo[5];

	static const t_uint8 hmi_loop_start[11];
	static const t_uint8 hmi_loop_end[9];

	static const t_uint8 xmi_default_tempo[5];

	static const t_uint8 mus_default_tempo[5];
	static const t_uint8 mus_controllers[15];

	static unsigned decode_delta( file::ptr & p_file, abort_callback & p_abort );
	static unsigned decode_hmp_delta( file::ptr & p_file, abort_callback & p_abort );
	static unsigned decode_xmi_delta( file::ptr & p_file, abort_callback & p_abort );

	static bool is_standard_midi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_riff_midi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_hmp( file::ptr & p_file, abort_callback & p_abort );
	static bool is_hmi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_xmi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_mus( file::ptr & p_file, abort_callback & p_abort );
	static bool is_mids( file::ptr & p_file, abort_callback & p_abort );

	static void process_standard_midi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_riff_midi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_hmp( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_hmi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_xmi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_mus( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_mids( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );

public:
	static void process_file( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
};

#endif
