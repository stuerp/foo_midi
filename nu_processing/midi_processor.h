#ifndef _MIDI_PROCESSORS_H_
#define _MIDI_PROCESSORS_H_

#include <foobar2000.h>

#include "midi_container.h"

class midi_processor
{
	static const t_uint8 end_of_track[2];
	static const t_uint8 loop_start[11];
	static const t_uint8 loop_end[9];

	static const t_uint8 hmp_default_tempo[5];

	static const t_uint8 xmi_default_tempo[5];

	static const t_uint8 mus_default_tempo[5];
	static const t_uint8 mus_controllers[15];

	static const t_uint8 lds_default_tempo[5];

	static int decode_delta( file::ptr & p_file, abort_callback & p_abort );
	static unsigned decode_hmp_delta( file::ptr & p_file, abort_callback & p_abort );
	static unsigned decode_xmi_delta( file::ptr & p_file, abort_callback & p_abort );

	static bool is_standard_midi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_riff_midi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_hmp( file::ptr & p_file, abort_callback & p_abort );
	static bool is_hmi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_xmi( file::ptr & p_file, abort_callback & p_abort );
	static bool is_mus( file::ptr & p_file, abort_callback & p_abort );
	static bool is_mids( file::ptr & p_file, abort_callback & p_abort );
	static bool is_lds( file::ptr & p_file, const char * p_extension, abort_callback & p_abort );
	static bool is_gmf( file::ptr & p_file, abort_callback & p_abort );
	static bool is_syx( file::ptr & p_file, abort_callback & p_abort );

	static void process_standard_midi_track( file::ptr & p_file, midi_container & p_out, bool needs_end_marker, abort_callback & p_abort );

	static void process_standard_midi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_riff_midi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_hmp( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_hmi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_xmi( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_mus( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_mids( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_lds( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_gmf( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
	static void process_syx( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );

public:
	static void process_file( file::ptr & p_file, const char * p_extension, midi_container & p_out, abort_callback & p_abort );

	static void process_syx_file( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort );
};

#endif
