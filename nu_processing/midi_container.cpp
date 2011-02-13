#include "midi_container.h"

midi_event::midi_event( const midi_event & p_in )
{
	m_timestamp = p_in.m_timestamp;
	m_channel = p_in.m_channel;
	m_type = p_in.m_type;
	m_data_count = p_in.m_data_count;
	memcpy( m_data, p_in.m_data, m_data_count );
	m_ext_data = p_in.m_ext_data;
}

midi_event::midi_event( unsigned p_timestamp, event_type p_type, unsigned p_channel, const t_uint8 * p_data, t_size p_data_count )
{
	m_timestamp = p_timestamp;
	m_type = p_type;
	m_channel = p_channel;
	if ( p_data_count <= max_static_data_count )
	{
		m_data_count = p_data_count;
		memcpy( m_data, p_data, p_data_count );
	}
	else
	{
		m_data_count = max_static_data_count;
		memcpy( m_data, p_data, max_static_data_count );
		m_ext_data.set_data_fromptr( p_data + max_static_data_count, p_data_count - max_static_data_count );
	}
}

unsigned midi_event::get_data_count() const
{
	return m_data_count + m_ext_data.get_count();
}

void midi_event::copy_data( t_uint8 * p_out, unsigned p_offset, unsigned p_count ) const
{
	unsigned max_count = m_data_count + m_ext_data.get_count() - p_offset;
	p_count = min( p_count, max_count );
	if ( p_offset < max_static_data_count )
	{
		unsigned max_count = max_static_data_count - p_offset;
		unsigned count = min( max_count, p_count );
		memcpy( p_out, m_data + p_offset, count );
		p_offset -= count;
		p_count -= count;
		p_out += count;
	}
	memcpy( p_out, m_ext_data.get_ptr(), p_count );
}

midi_track::midi_track(const midi_track & p_in)
{
	m_events = p_in.m_events;
}

void midi_track::add_event( const midi_event & p_event )
{
	t_size index = m_events.get_count();

	if ( index )
	{
		midi_event & event = m_events[ index - 1 ];
		if ( event.m_type == midi_event::extended && event.get_data_count() >= 2 &&
			event.m_data[ 0 ] == 0xFF && event.m_data[ 1 ] == 0x2F )
		{
			--index;
			if ( event.m_timestamp < p_event.m_timestamp )
			{
				event.m_timestamp = p_event.m_timestamp;
			}
		}

		while ( index > 0 )
		{
			if ( m_events[ index - 1 ].m_timestamp <= p_event.m_timestamp ) break;
			--index;
		}
	}

	m_events.insert_multi( p_event, index, 1 );
}

t_size midi_track::get_count() const
{
	return m_events.get_count();
}

const midi_event & midi_track::operator [] ( t_size p_index ) const
{
	return m_events[ p_index ];
}

tempo_entry::tempo_entry(unsigned p_timestamp, unsigned p_tempo)
{
	m_timestamp = p_timestamp;
	m_tempo = p_tempo;
}

void tempo_map::add_tempo( unsigned p_tempo, unsigned p_timestamp )
{
	t_size index = m_entries.get_count();

	while ( index > 0 )
	{
		if ( m_entries[ index - 1 ].m_timestamp < p_timestamp ) break;
		--index;
	}

	if ( index && m_entries[ index - 1 ].m_timestamp == p_timestamp )
	{
		m_entries[ index - 1 ].m_tempo = p_tempo;
	}
	else
	{
		m_entries.insert_multi( tempo_entry( p_timestamp, p_tempo ), index, 1 );
	}
}

const unsigned tempo_map::timestamp_to_ms( unsigned p_timestamp, unsigned p_dtx ) const
{
	unsigned timestamp_ms = 0;
	unsigned timestamp = 0;
	t_size tempo_index = 0;
	unsigned current_tempo = 500000;
	t_size tempo_count = m_entries.get_count();

	p_dtx *= 1000;

	while ( tempo_index < tempo_count && timestamp + p_timestamp >= m_entries[ tempo_index ].m_timestamp )
	{
		unsigned delta = m_entries[ tempo_index ].m_timestamp - timestamp;
		timestamp_ms += MulDiv( current_tempo, delta, p_dtx );
		current_tempo = m_entries[ tempo_index ].m_tempo;
		++tempo_index;
		timestamp += delta;
		p_timestamp -= delta;
	}

	timestamp_ms += MulDiv( current_tempo, p_timestamp, p_dtx );

	return timestamp_ms;
}

system_exclusive_entry::system_exclusive_entry(const system_exclusive_entry & p_in)
{
	m_offset = p_in.m_offset;
	m_length = p_in.m_length;
}

system_exclusive_entry::system_exclusive_entry(t_size p_offset, t_size p_length)
{
	m_offset = p_offset;
	m_length = p_length;
}

unsigned system_exclusive_table::add_entry( const t_uint8 * p_data, t_size p_size )
{
	for ( unsigned i = 0; i < m_entries.get_count(); ++i )
	{
		const system_exclusive_entry & entry = m_entries[ i ];
		if ( p_size == entry.m_length && !memcmp( p_data, m_data.get_ptr() + entry.m_offset, p_size ) )
			return i;
	}
	system_exclusive_entry entry( m_data.get_count(), p_size );
	m_data.append_fromptr( p_data, p_size );
	m_entries.append_single( entry );
	return m_entries.get_count() - 1;
}

void system_exclusive_table::get_entry( unsigned p_index, const t_uint8 * & p_data, t_size & p_size )
{
	const system_exclusive_entry & entry = m_entries[ p_index ];
	p_data = m_data.get_ptr() + entry.m_offset;
	p_size = entry.m_length;
}

midi_stream_event::midi_stream_event(unsigned p_timestamp, unsigned p_event)
{
	m_timestamp = p_timestamp;
	m_event = p_event;
}

midi_meta_data_item::midi_meta_data_item(const midi_meta_data_item & p_in)
{
	m_timestamp = p_in.m_timestamp;
	m_name = p_in.m_name;
	m_value = p_in.m_value;
}

midi_meta_data_item::midi_meta_data_item(unsigned p_timestamp, const char * p_name, const char * p_value)
{
	m_timestamp = p_timestamp;
	m_name = p_name;
	m_value = p_value;
}

void midi_meta_data::add_item( const midi_meta_data_item & p_item )
{
	m_data.append_single( p_item );
}

void midi_meta_data::append( const midi_meta_data & p_data )
{
	m_data.append( p_data.m_data );
}

bool midi_meta_data::get_item( const char * p_name, midi_meta_data_item & p_out ) const
{
	for ( unsigned i = 0; i < m_data.get_count(); ++i )
	{
		const midi_meta_data_item & item = m_data[ i ];
		if ( !pfc::stricmp_ascii( p_name, item.m_name ) )
		{
			p_out = item;
			return true;
		}
	}
	return false;
}

t_size midi_meta_data::get_count() const
{
	return m_data.get_count();
}

const midi_meta_data_item & midi_meta_data::operator [] ( t_size p_index ) const
{
	return m_data[ p_index ];
}

void midi_container::encode_delta( pfc::array_t<t_uint8> & p_out, unsigned delta )
{
	unsigned shift = 7 * 4;
	while ( shift && !( delta >> shift ) )
	{
		shift -= 7;
	}
	while (shift > 0)
	{
		p_out.append_single( (unsigned char)( ( ( delta >> shift ) & 0x7F ) | 0x80 ) );
		shift -= 7;
	}
	p_out.append_single( (unsigned char)( delta & 0x7F ) );
}

void midi_container::initialize( unsigned p_form, unsigned p_dtx )
{
	m_form = p_form;
	m_dtx = p_dtx;
}

void midi_container::add_track( const midi_track & p_track )
{
	unsigned i;

	m_tracks.append_single( p_track );

	for ( i = 0; i < p_track.get_count(); ++i )
	{
		const midi_event & event = p_track[ i ];
		if ( event.m_type == midi_event::extended && event.get_data_count() >= 5 &&
			event.m_data[ 0 ] == 0xFF && event.m_data[ 1 ] == 0x51 )
		{
			unsigned tempo = ( event.m_data[ 2 ] << 16 ) + ( event.m_data[ 3 ] << 8 ) + event.m_data[ 4 ];
			m_tempo_map.add_tempo( tempo, event.m_timestamp );
		}
		else if ( event.m_type == midi_event::note_on || event.m_type == midi_event::note_off )
		{
			m_channel_mask |= 1 << event.m_channel;
		}
	}

	if ( i && p_track[ i - 1 ].m_timestamp > m_timestamp_end )
	{
		m_timestamp_end = p_track[ i - 1 ].m_timestamp;
	}
}

void midi_container::add_track_event( t_size p_track_index, const midi_event & p_event )
{
	midi_track & track = m_tracks[ p_track_index ];

	track.add_event( p_event );

	if ( p_event.m_type == midi_event::extended && p_event.get_data_count() >= 5 &&
		p_event.m_data[ 0 ] == 0xFF && p_event.m_data[ 1 ] == 0x51 )
	{
		unsigned tempo = ( p_event.m_data[ 2 ] << 16 ) + ( p_event.m_data[ 3 ] << 8 ) + p_event.m_data[ 4 ];
		m_tempo_map.add_tempo( tempo, p_event.m_timestamp );
	}
	else if ( p_event.m_type == midi_event::note_on || p_event.m_type == midi_event::note_off )
	{
		m_channel_mask |= 1 << p_event.m_channel;
	}
}

void midi_container::set_extra_meta_data( const midi_meta_data & p_data )
{
	m_extra_meta_data = p_data;
}

void midi_container::serialize_as_stream( pfc::array_t<midi_stream_event> & p_stream, system_exclusive_table & p_system_exclusive, bool clean_emidi ) const
{
	unsigned current_timestamp = 0;
	pfc::array_t<t_uint8> data;
	pfc::array_t<t_size> track_positions;
	t_size track_count = m_tracks.get_count();

	track_positions.append_multi( (t_size)0, track_count );

	if ( clean_emidi )
	{
		for ( unsigned i = 0; i < track_count; ++i )
		{
			bool skip_track = false;
			const midi_track & track = m_tracks[ i ];
			for ( unsigned j = 0; j < track.get_count(); ++j )
			{
				const midi_event & event = track[ j ];
				if ( event.m_type == midi_event::control_change &&
					( event.m_data[ 0 ] == 110 || event.m_data[ 0 ] == 111 ) )
				{
					if ( event.m_data[ 0 ] == 110 )
					{
						if ( event.m_data[ 1 ] != 0 && event.m_data[ 1 ] != 127 )
						{
							skip_track = true;
							break;
						}
					}
					else
					{
						if ( event.m_data[ 1 ] == 0 )
						{
							skip_track = true;
							break;
						}
					}
				}
			}
			if ( skip_track )
			{
				track_positions[ i ] = track.get_count();
			}
		}
	}

	for (;;)
	{
		unsigned next_timestamp = ~0;
		t_size next_track = 0;
		for ( unsigned i = 0; i < track_count; ++i )
		{
			if ( track_positions[ i ] >= m_tracks[ i ].get_count() ) continue;
			if ( m_tracks[ i ][ track_positions[ i ] ].m_timestamp < next_timestamp )
			{
				next_timestamp = m_tracks[ i ][ track_positions[ i ] ].m_timestamp;
				next_track = i;
			}
		}
		if ( next_timestamp == ~0 ) break;

		const midi_event & event = m_tracks[ next_track ][ track_positions[ next_track ] ];
		unsigned timestamp_ms = m_tempo_map.timestamp_to_ms( event.m_timestamp, m_dtx );
		if ( event.m_type != midi_event::extended )
		{
			unsigned event_code = ( ( event.m_type + 8 ) << 4 ) + event.m_channel;
			if ( event.m_data_count >= 1 ) event_code += event.m_data[ 0 ] << 8;
			if ( event.m_data_count >= 2 ) event_code += event.m_data[ 1 ] << 16;
			p_stream.append_single( midi_stream_event( timestamp_ms, event_code ) );
		}
		else
		{
			t_size data_count = event.get_data_count();
			if ( data_count >= 3 && event.m_data[ 0 ] == 0xF0 )
			{
				data.grow_size( data_count );
				event.copy_data( data.get_ptr(), 0, data_count );
				if ( data[ data_count - 1 ] == 0xF7 )
				{
					unsigned system_exclusive_index = p_system_exclusive.add_entry( data.get_ptr(), data_count );
					p_stream.append_single( midi_stream_event( timestamp_ms, system_exclusive_index | 0x80000000 ) );
				}
			}
		}

		track_positions[ next_track ]++;
	}
}

void midi_container::serialize_as_standard_midi_file( pfc::array_t<t_uint8> & p_midi_file ) const
{
	if ( !m_tracks.get_count() ) return;

	pfc::array_t<t_uint8> data;

	p_midi_file.append_fromptr( "MThd", 4 );
	p_midi_file.append_multi( (unsigned char)0, 3 );
	p_midi_file.append_single( (unsigned char)6 );
	p_midi_file.append_single( (unsigned char)0 );
	p_midi_file.append_single( (unsigned char)m_form );
	p_midi_file.append_single( (unsigned char)(m_tracks.get_count() >> 8) );
	p_midi_file.append_single( (unsigned char)m_tracks.get_count() );
	p_midi_file.append_single( (unsigned char)(m_dtx >> 8) );
	p_midi_file.append_single( (unsigned char)m_dtx );

	for ( unsigned i = 0; i < m_tracks.get_count(); ++i )
	{
		const midi_track & track = m_tracks[ i ];
		unsigned last_timestamp = 0;
		unsigned char last_event_code = 0xFF;
		t_size length_offset;

		p_midi_file.append_fromptr( "MTrk", 4 );

		length_offset = p_midi_file.get_count();
		p_midi_file.append_multi( (unsigned char)0, 4 );

		for ( unsigned j = 0; j < track.get_count(); ++j )
		{
			const midi_event & event = track[ j ];
			encode_delta( p_midi_file, event.m_timestamp - last_timestamp );
			last_timestamp = event.m_timestamp;
			if ( event.m_type != midi_event::extended )
			{
				const unsigned char event_code = ( ( event.m_type + 8 ) << 4 ) + event.m_channel;
				if ( event_code != last_event_code )
				{
					p_midi_file.append_single( event_code );
					last_event_code = event_code;
				}
				p_midi_file.append_fromptr( event.m_data, event.m_data_count );
			}
			else
			{
				t_size data_count = event.get_data_count();
				if ( data_count >= 1 )
				{
					if ( event.m_data[ 0 ] == 0xF0 )
					{
						--data_count;
						p_midi_file.append_single( (unsigned char)0xF0 );
						encode_delta( p_midi_file, data_count );
						data.grow_size( data_count );
						event.copy_data( data.get_ptr(), 1, data_count );
						p_midi_file.append_fromptr( data.get_ptr(), data_count );
					}
					else if ( event.m_data[ 0 ] == 0xFF && data_count >= 2 )
					{
						data_count -= 2;
						p_midi_file.append_single( (unsigned char)0xFF );
						p_midi_file.append_single( event.m_data[ 1 ] );
						encode_delta( p_midi_file, data_count );
						data.grow_size( data_count );
						event.copy_data( data.get_ptr(), 2, data_count );
						p_midi_file.append_fromptr( data.get_ptr(), data_count );
					}
				}
			}
		}

		t_size track_length = p_midi_file.get_count() - length_offset - 4;
		p_midi_file[ length_offset + 0 ] = (unsigned char)( track_length >> 24 );
		p_midi_file[ length_offset + 1 ] = (unsigned char)( track_length >> 16 );
		p_midi_file[ length_offset + 2 ] = (unsigned char)( track_length >> 8 );
		p_midi_file[ length_offset + 3 ] = (unsigned char)track_length;
	}
}

const unsigned midi_container::get_timestamp_end(bool ms /* = false */) const
{
	if ( !ms ) return m_timestamp_end;
	else return m_tempo_map.timestamp_to_ms( m_timestamp_end, m_dtx );
}

const unsigned midi_container::get_format() const
{
	return m_form;
}

const unsigned midi_container::get_track_count() const
{
	return m_tracks.get_count();
}

const unsigned midi_container::get_channel_count() const
{
	unsigned count = 0;
	for (unsigned i = 0, j = 1; i < 16; ++i, j <<= 1)
	{
		if ( m_channel_mask & j ) ++count;
	}
	return count;
}

const unsigned midi_container::get_timestamp_loop_start(bool ms /* = false */) const
{
	if ( !ms ) return m_timestamp_loop_start;
	else if ( m_timestamp_loop_start != ~0 ) return m_tempo_map.timestamp_to_ms( m_timestamp_loop_start, m_dtx );
	else return ~0;
}

const unsigned midi_container::get_timestamp_loop_end(bool ms /* = false */) const
{
	if ( !ms ) return m_timestamp_loop_end;
	else if ( m_timestamp_loop_end != ~0 ) return m_tempo_map.timestamp_to_ms( m_timestamp_loop_end, m_dtx );
	else return ~0;
}

void midi_container::get_meta_data( midi_meta_data & p_out )
{
	pfc::string8_fast temp;
	pfc::stringcvt::string_utf8_from_ansi convert;

	pfc::array_t<t_uint8> data;

	bool type_found = false;
	bool type_non_gm_found = false;

	for ( unsigned i = 0; i < m_tracks.get_count(); ++i )
	{
		const midi_track & track = m_tracks[ i ];
		for ( unsigned j = 0; j < track.get_count(); ++j )
		{
			const midi_event & event = track[ j ];
			if ( event.m_type == midi_event::extended )
			{
				t_size data_count = event.get_data_count();
				if ( !type_non_gm_found && data_count >= 1 && event.m_data[ 0 ] == 0xF0 )
				{
					unsigned char test = 0;
					unsigned char test2 = 0;
					if ( data_count > 1 ) test  = event.m_data[ 1 ];
					if ( data_count > 3 ) test2 = event.m_data[ 3 ];

					const char * type = NULL;

					switch( test )
					{
					case 0x7E:
						type_found = true;
						break;
					case 0x43:
						type = "XG";
						break;
					case 0x42:
						type = "X5";
						break;
					case 0x41:
						if ( test2 == 0x42 ) type = "GS";
						else if ( test2 == 0x16 ) type = "MT-32";
						else if ( test2 == 0x14 ) type = "D-50";
					}

					if ( type )
					{
						type_found = true;
						type_non_gm_found = true;
						p_out.add_item( midi_meta_data_item( m_tempo_map.timestamp_to_ms( event.m_timestamp, m_dtx ), "type", type ) );
					}
				}
				else if ( data_count >= 2 && event.m_data[ 0 ] == 0xFF )
				{
					data_count -= 2;
					switch ( event.m_data[ 1 ] )
					{
					case 6:
						data.grow_size( data_count );
						event.copy_data( data.get_ptr(), 2, data_count );
						convert.convert( (const char *)data.get_ptr(), data_count );
						p_out.add_item( midi_meta_data_item( m_tempo_map.timestamp_to_ms( event.m_timestamp, m_dtx ), "track_marker", convert ) );
						break;

					case 2:
						data.grow_size( data_count );
						event.copy_data( data.get_ptr(), 2, data_count );
						convert.convert( (const char *)data.get_ptr(), data_count );
						p_out.add_item( midi_meta_data_item( m_tempo_map.timestamp_to_ms( event.m_timestamp, m_dtx ), "copyright", convert ) );
						break;

					case 1:
						data.grow_size( data_count );
						event.copy_data( data.get_ptr(), 2, data_count );
						convert.convert( (const char *)data.get_ptr(), data_count );
						temp = "track_text_";
						temp += pfc::format_int( i, 2 );
						p_out.add_item( midi_meta_data_item( m_tempo_map.timestamp_to_ms( event.m_timestamp, m_dtx ), temp, convert ) );
						break;

					case 3:
					case 4:
						data.grow_size( data_count );
						event.copy_data( data.get_ptr(), 2, data_count );
						convert.convert( (const char *)data.get_ptr(), data_count );
						temp = "track_name_";
						temp += pfc::format_int( i, 2 );
						p_out.add_item( midi_meta_data_item( m_tempo_map.timestamp_to_ms( event.m_timestamp, m_dtx ), temp, convert ) );
						break;
					}
				}
			}
		}
	}

	if ( type_found && !type_non_gm_found )
	{
		p_out.add_item( midi_meta_data_item( 0, "type", "GM" ) );
	}

	p_out.append( m_extra_meta_data );
}

void midi_container::scan_for_loops( bool p_xmi_loops, bool p_marker_loops )
{
	pfc::array_t<t_uint8> data;

	m_timestamp_loop_start = ~0;
	m_timestamp_loop_end = ~0;

	if ( p_xmi_loops )
	{
		for ( unsigned i = 0; i < m_tracks.get_count(); ++i )
		{
			const midi_track & track = m_tracks[ i ];
			for ( unsigned j = 0; j < track.get_count(); ++j )
			{
				const midi_event & event = track[ j ];
				if ( event.m_type == midi_event::control_change &&
					( event.m_data[ 0 ] == 0x74 || event.m_data[ 0 ] == 0x75 ) )
				{
					if ( event.m_data[ 0 ] == 0x74 )
					{
						if ( m_timestamp_loop_start == ~0 || m_timestamp_loop_start > event.m_timestamp )
						{
							m_timestamp_loop_start = event.m_timestamp;
						}
					}
					else
					{
						if ( m_timestamp_loop_end == ~0 || m_timestamp_loop_end < event.m_timestamp )
						{
							m_timestamp_loop_end = event.m_timestamp;
						}
					}
				}
			}
		}
	}

	if ( p_marker_loops )
	{
		for ( unsigned i = 0; i < m_tracks.get_count(); ++i )
		{
			const midi_track & track = m_tracks[ i ];
			for ( unsigned j = 0; j < track.get_count(); ++j )
			{
				const midi_event & event = track[ j ];
				if ( event.m_type == midi_event::extended &&
					event.get_data_count() >= 9 &&
					event.m_data[ 0 ] == 0xFF && event.m_data[ 1 ] == 0x06 )
				{
					unsigned data_count = event.get_data_count() - 2;
					data.grow_size( data_count );
					event.copy_data( data.get_ptr(), 2, data_count );

					if ( data_count == 9 && !pfc::stricmp_ascii_ex( (const char *)data.get_ptr(), 9, "loopStart", 9 ) )
					{
						if ( m_timestamp_loop_start == ~0 || m_timestamp_loop_start > event.m_timestamp )
						{
							m_timestamp_loop_start = event.m_timestamp;
						}
					}
					else if ( data_count == 7 && !pfc::stricmp_ascii_ex( (const char *)data.get_ptr(), 7, "loopEnd", 7 ) )
					{
						if ( m_timestamp_loop_end == ~0 || m_timestamp_loop_end < event.m_timestamp )
						{
							m_timestamp_loop_end = event.m_timestamp;
						}
					}
				}
			}
		}
	}
}
