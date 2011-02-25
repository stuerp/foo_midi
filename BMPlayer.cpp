#include "BMPlayer.h"

#pragma comment(lib, "../../../bass/c/bass.lib")
#pragma comment(lib, "../../../bass/c/bassmidi.lib")

static const char sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const char sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const char sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

class soundfont_map : public pfc::thread
{
	critical_section m_lock;

	struct soundfont_info
	{
		unsigned m_reference_count;
		LARGE_INTEGER m_release_time;
	};

	pfc::map_t<HSOUNDFONT, soundfont_info> m_map;

	bool thread_running;

	virtual void threadProc()
	{
		thread_running = true;

		while (thread_running)
		{
			LARGE_INTEGER now;
			QueryPerformanceCounter( &now );
			{
				insync( m_lock );
				for ( pfc::map_t<HSOUNDFONT, soundfont_info>::const_iterator it = m_map.first(); it.is_valid(); )
				{
					pfc::map_t<HSOUNDFONT, soundfont_info>::const_iterator it_copy = it++;
					if ( it_copy->m_value.m_reference_count == 0 && now.QuadPart >= it_copy->m_value.m_release_time.QuadPart )
					{
						BASS_MIDI_FontFree( it_copy->m_key );
						m_map.remove( it_copy );
					}
				}
			}
			Sleep( 250 );
		}
	}

public:
	~soundfont_map()
	{
		thread_running = false;
		waitTillDone();
		for ( pfc::map_t<HSOUNDFONT, soundfont_info>::const_iterator it = m_map.first(); it.is_valid(); ++it )
		{
			BASS_MIDI_FontFree( it->m_key );
		}
	}

	void add_font( HSOUNDFONT p_font )
	{
		insync( m_lock );

		if ( m_map.have_item( p_font ) ) ++m_map[ p_font ].m_reference_count;
		else m_map[ p_font ].m_reference_count = 1;
	}

	void free_font( HSOUNDFONT p_font )
	{
		insync( m_lock );

		if ( --m_map[ p_font ].m_reference_count == 0 )
		{
			LARGE_INTEGER now;
			LARGE_INTEGER increment;
			QueryPerformanceCounter( &now );
			QueryPerformanceFrequency( &increment );
			now.QuadPart += increment.QuadPart * 2;
			m_map[ p_font ].m_release_time = now;

			if ( !isActive() ) start();
		}
	}
};

soundfont_map * g_map = NULL;

class Bass_Initializer
{
	critical_section lock;

	bool initialized;

public:
	Bass_Initializer() : initialized(false) { }

	~Bass_Initializer()
	{
		if ( initialized )
		{
			delete g_map;
			BASS_Free();
		}
	}

	bool initialize()
	{
		insync(lock);
		if ( !initialized )
		{
			initialized = !!BASS_Init( 0, 44100, 0, core_api::get_main_window(), NULL );
			if ( initialized )
			{
				BASS_SetConfigPtr( BASS_CONFIG_MIDI_DEFFONT, NULL );
				BASS_SetConfig( BASS_CONFIG_MIDI_VOICES, 256 );
				g_map = new soundfont_map;
			}
		}
		return initialized;
	}
} g_initializer;

BMPlayer::BMPlayer()
{
	_stream = 0;
	uSamplesRemaining = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;

	if ( !g_initializer.initialize() ) throw exception_io_data( "Unable to initialize BASS" );
}

BMPlayer::~BMPlayer()
{
	shutdown();
}

void BMPlayer::setSampleRate(unsigned rate)
{
	if (mStream.get_count())
	{
		for (UINT i = 0; i < mStream.get_count(); i++)
		{
			mStream[i].m_timestamp = MulDiv(mStream[i].m_timestamp, rate, uSampleRate);
		}
	}

	if (uTimeCurrent)
	{
		uTimeCurrent = MulDiv(uTimeCurrent, rate, uSampleRate);
	}

	if (uTimeEnd)
	{
		uTimeEnd = MulDiv(uTimeEnd, rate, uSampleRate);
	}

	if (uTimeLoopStart)
	{
		uTimeLoopStart = MulDiv(uTimeLoopStart, rate, uSampleRate);
	}

	uSampleRate = rate;

	shutdown();
}

bool BMPlayer::Load(const midi_container & midi_file, unsigned loop_mode, bool clean_flag)
{
	assert(!mStream.get_count());

	midi_file.serialize_as_stream( mStream, mSysexMap, clean_flag );

	if (mStream.get_count())
	{
		uStreamPosition = 0;
		uTimeCurrent = 0;

		uLoopMode = loop_mode;

		if (uLoopMode & loop_mode_enable)
		{
			uTimeLoopStart = midi_file.get_timestamp_loop_start( true );
			unsigned uTimeLoopEnd = midi_file.get_timestamp_loop_end( true );
			uTimeEnd = midi_file.get_timestamp_end( true );

			if ( uTimeLoopStart != ~0 || uTimeLoopEnd != ~0 )
			{
				uLoopMode |= loop_mode_force;
			}

			if ( uTimeLoopStart != ~0 )
			{
				for ( unsigned i = 0; i < mStream.get_count(); ++i )
				{
					if ( mStream[ i ].m_timestamp >= uTimeLoopStart )
					{
						uStreamLoopStart = i;
						break;
					}
				}
			}
			else uStreamLoopStart = ~0;

			if ( uTimeLoopEnd != ~0 )
			{
				uTimeEnd = uTimeLoopEnd;
			}

			if (!(uLoopMode & loop_mode_force)) uTimeEnd += 1000;
			else
			{
				UINT i;
				unsigned char note_on[128 * 16];
				memset( note_on, 0, sizeof( note_on ) );
				for (i = 0; i < mStream.get_count(); i++)
				{
					if (mStream[ i ].m_timestamp > uTimeEnd) break;
					UINT ev = mStream[ i ].m_event & 0xFF0000F0;
					if ( ev == 0x90 || ev == 0x80 )
					{
						UINT ch = mStream[ i ].m_event & 0x0F;
						UINT note = ( mStream[ i ].m_event >> 8 ) & 0x7F;
						UINT on = ( ev == 0x90 ) && ( mStream[ i ].m_event & 0xFF0000 );
						note_on [ ch * 128 + note ] = on;
					}
				}
				mStream.set_count( i );
				for ( UINT j = 0; j < 128 * 16; j++ )
				{
					if ( note_on[ j ] )
					{
						mStream.append_single( midi_stream_event( uTimeEnd, ( j >> 7 ) + ( j & 0x7F ) * 0x100 + 0x90 ) );
					}
				}
			}
		}
		else uTimeEnd = midi_file.get_timestamp_end( true ) + 1000;

		if (uSampleRate != 1000)
		{
			unsigned rate = uSampleRate;
			uSampleRate = 1000;
			setSampleRate(rate);
		}

		return true;
	}

	return false;
}

unsigned BMPlayer::Play(audio_sample * out, unsigned count)
{
	assert(mStream.get_count());

	if (!_stream && !startup()) return 0;

	DWORD done = 0;

	if ( uSamplesRemaining )
	{
		DWORD todo = uSamplesRemaining;
		if (todo > count) todo = count;
		render( out, todo );
		uSamplesRemaining -= todo;
		done += todo;
		uTimeCurrent += todo;
	}

	while (done < count)
	{
		DWORD todo = uTimeEnd - uTimeCurrent;
		if (todo > count - done) todo = count - done;

		DWORD time_target = todo + uTimeCurrent;
		UINT stream_end = uStreamPosition;

		while (stream_end < mStream.get_count() && mStream[stream_end].m_timestamp < time_target) stream_end++;

		if (stream_end > uStreamPosition)
		{
			for (; uStreamPosition < stream_end; uStreamPosition++)
			{
				midi_stream_event * me = mStream.get_ptr() + uStreamPosition;
				
				DWORD samples_todo = me->m_timestamp - uTimeCurrent;
				if ( samples_todo )
				{
					if ( samples_todo > count - done )
					{
						uSamplesRemaining = samples_todo - ( count - done );
						samples_todo = count - done;
					}
					render( out + done * 2, samples_todo );
					done += samples_todo;

					if ( uSamplesRemaining )
					{
						uTimeCurrent = me->m_timestamp;
						return done;
					}
				}

				send_event( me->m_event );

				uTimeCurrent = me->m_timestamp;
			}
		}

		if ( done < count )
		{
			DWORD samples_todo;
			if ( uStreamPosition < mStream.get_count() ) samples_todo = mStream[uStreamPosition].m_timestamp;
			else samples_todo = uTimeEnd;
			samples_todo -= uTimeCurrent;
			if ( samples_todo > count - done ) samples_todo = count - done;
			render( out + done * 2, samples_todo );
			done += samples_todo;
		}

		uTimeCurrent = time_target;

		if (uTimeCurrent >= uTimeEnd)
		{
			if ( uStreamPosition < mStream.get_count() )
			{
				for (; uStreamPosition < mStream.get_count(); uStreamPosition++)
				{
					send_event( mStream[ uStreamPosition ].m_event );
				}
			}

			if ((uLoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
			{
				if (uStreamLoopStart == ~0)
				{
					uStreamPosition = 0;
					uTimeCurrent = 0;
				}
				else
				{
					uStreamPosition = uStreamLoopStart;
					uTimeCurrent = uTimeLoopStart;
				}
			}
			else break;
		}
	}

	return done;
}

void BMPlayer::Seek(unsigned sample)
{
	if (sample >= uTimeEnd)
	{
		if ((uLoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
		{
			while (sample >= uTimeEnd) sample -= uTimeEnd - uTimeLoopStart;
		}
		else
		{
			sample = uTimeEnd;
		}
	}

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		shutdown();
	}

	if (!_stream && !startup()) return;

	uTimeCurrent = sample;

	pfc::array_t<midi_stream_event> filler;

	UINT stream_start = uStreamPosition;

	for (; uStreamPosition < mStream.get_count() && mStream[uStreamPosition].m_timestamp < uTimeCurrent; uStreamPosition++);

	uSamplesRemaining = mStream[uStreamPosition].m_timestamp - uTimeCurrent;

	if (uStreamPosition > stream_start)
	{
		filler.set_size( uStreamPosition - stream_start );
		memcpy(filler.get_ptr(), &mStream[stream_start], sizeof(midi_stream_event) * (uStreamPosition - stream_start));

		UINT i, j;
		midi_stream_event * me = filler.get_ptr();

		for (i = 0, stream_start = uStreamPosition - stream_start; i < stream_start; i++)
		{
			midi_stream_event & e = me[i];
			if ((e.m_event & 0xFF0000F0) == 0x90 && (e.m_event & 0xFF0000)) // note on
			{
				if ((e.m_event & 0x0F) == 9) // hax
				{
					e.m_event = 0;
					continue;
				}
				DWORD m = (e.m_event & 0xFF0F) | 0x80; // note off
				DWORD m2 = e.m_event & 0xFFFF; // also note off
				for (j = i + 1; j < stream_start; j++)
				{
					midi_stream_event & e2 = me[j];
					if ((e2.m_event & 0xFF00FFFF) == m || e2.m_event == m2)
					{
						// kill 'em
						e.m_event = 0;
						e2.m_event = 0;
						break;
					}
				}
			}
		}

		for (i = 0, j = 0; i < stream_start; i++)
		{
			if (me[i].m_event)
			{
				if (i != j) me[j] = me[i];
				j++;
			}
		}

		if (!j) return;

		for (i = 0; i < j; i++)
		{
			send_event( me[i].m_event );
		}
	}
}

void BMPlayer::send_event(DWORD b)
{
	if (!(b & 0xFF000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned channel = b & 0x0F;
		unsigned command = b & 0xF0;
		unsigned event_length = ( command == 0xC0 || command == 0xD0 ) ? 2 : 3;
		BASS_MIDI_StreamEvents( _stream, BASS_MIDI_EVENTS_RAW, event, event_length );
		if ( command == 0xB0 && event[ 1 ] == 0 )
		{
			if ( event[ 2 ] == 127 ) drum_channels[ channel ] = 1;
			else if ( event[ 2 ] == 121 ) drum_channels[ channel ] = 0;
		}
		else if ( command == 0xC0 && drum_channels[ channel ] )
		{
			BASS_MIDI_StreamEvent( _stream, channel, MIDI_EVENT_DRUMS, 1 );
		}
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size;
		mSysexMap.get_entry( n, data, size );
		BASS_MIDI_StreamEvents( _stream, BASS_MIDI_EVENTS_RAW, data, size );
		if ( ( size == _countof( sysex_gm_reset ) && !memcmp( data, sysex_gm_reset, _countof( sysex_gm_reset ) ) ) ||
			( size == _countof( sysex_gs_reset ) && !memcmp( data, sysex_gs_reset, _countof( sysex_gs_reset ) ) ) ||
			( size == _countof( sysex_xg_reset ) && !memcmp( data, sysex_xg_reset, _countof( sysex_xg_reset ) ) ) )
		{
			reset_drum_channels();
		}
		else if ( size == 11 &&
			data [0] == 0xF0 && data [1] == 0x41 && data [3] == 0x42 &&
			data [4] == 0x12 && data [5] == 0x40 && (data [6] & 0xF0) == 0x10 &&
			data [10] == 0xF7)
		{
			if (data [7] == 2)
			{
				// GS MIDI channel to part assign
				gs_part_to_ch [ data [6] & 15 ] = data [8];
			}
			else if ( data [7] == 0x15 )
			{
				// GS part to rhythm allocation
				unsigned int drum_channel = gs_part_to_ch [ data [6] & 15 ];
				if ( drum_channel < 16 )
				{
					drum_channels [ drum_channel ] = data [8];
				}
			}
		}
	}
}

void BMPlayer::render(audio_sample * out, unsigned count)
{
	BASS_ChannelGetData( _stream, out, BASS_DATA_FLOAT | ( count * sizeof( audio_sample ) * 2 ) );
}

void BMPlayer::setSoundFont( const char * in )
{
	sSoundFontName = in;
	shutdown();
}

void BMPlayer::setFileSoundFont( const char * in )
{
	sFileSoundFontName = in;
	shutdown();
}

void BMPlayer::shutdown()
{
	if ( _stream ) BASS_StreamFree( _stream );
	_stream = NULL;
	for ( unsigned i = 0; i < _soundFonts.get_count(); ++i )
	{
		g_map->free_font( _soundFonts[ i ] );
	}
	_soundFonts.set_count( 0 );
}

bool BMPlayer::startup()
{
	_stream = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE, uSampleRate );
	if (!_stream)
	{
		return false;
	}
	if (sSoundFontName.length())
	{
		pfc::string_extension ext(sSoundFontName);
		if ( !pfc::stricmp_ascii( ext, "sf2" ) )
		{
			HSOUNDFONT font = BASS_MIDI_FontInit( pfc::stringcvt::string_wide_from_utf8( sSoundFontName ), BASS_UNICODE );
			if ( !font )
			{
				shutdown();
				return false;
			}
			g_map->add_font( font );
			_soundFonts.append_single( font );
		}
		else if ( !pfc::stricmp_ascii( ext, "sflist" ) )
		{
			FILE * fl = _tfopen( pfc::stringcvt::string_os_from_utf8( sSoundFontName ), _T("r, ccs=UTF-8") );
			if ( fl )
			{
				TCHAR path[1024], name[1024], temp[1024];
				pfc::stringcvt::convert_utf8_to_wide( path, 1024, sSoundFontName, sSoundFontName.scan_filename() );
				while ( !feof( fl ) )
				{
					_fgetts( name, 1024, fl );
					name[1023] = 0;
					TCHAR * cr = _tcschr( name, '\n' );
					if ( cr ) *cr = 0;
					if ( isalpha( name[0] ) && name[1] == ':' )
					{
						cr = name;
					}
					else
					{
						_tcscpy_s( temp, 1024, path );
						_tcscat_s( temp, 1024, name );
						cr = temp;
					}
					HSOUNDFONT font = BASS_MIDI_FontInit( cr, BASS_UNICODE );
					if ( !font )
					{
						fclose( fl );
						shutdown();
						return false;
					}
					g_map->add_font( font );
					_soundFonts.append_single( font );
				}
				fclose( fl );
			}
			else
			{
				return false;
			}
		}
	}

	if ( sFileSoundFontName.length() )
	{
		HSOUNDFONT font = BASS_MIDI_FontInit( pfc::stringcvt::string_wide_from_utf8( sFileSoundFontName ), BASS_UNICODE );
		if ( !font )
		{
			shutdown();
			return false;
		}
		g_map->add_font( font );
		_soundFonts.append_single( font );
	}

	pfc::array_t< BASS_MIDI_FONT > fonts;
	for ( unsigned i = 0, j = _soundFonts.get_count(); i < j; ++i )
	{
		BASS_MIDI_FONT sf;
		sf.font = _soundFonts[ j - i - 1 ];
		sf.preset = -1;
		sf.bank = 0;
		fonts.append_single( sf );
	}
	BASS_MIDI_StreamSetFonts( _stream, fonts.get_ptr(), fonts.get_count() );

	reset_drum_channels();

	return true;
}

void BMPlayer::reset_drum_channels()
{
	static const BYTE part_to_ch[16] = {9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15};

	memset( drum_channels, 0, sizeof( drum_channels ) );
	drum_channels[ 9 ] = 1;

	memcpy( gs_part_to_ch, part_to_ch, sizeof( gs_part_to_ch ) );

	if ( _stream )
	{
		for ( unsigned i = 0; i < 16; ++i )
		{
			BASS_MIDI_StreamEvent( _stream, i, MIDI_EVENT_DRUMS, drum_channels[ i ] );
		}
	}
}
