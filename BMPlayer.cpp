#include "BMPlayer.h"

#pragma comment(lib, "../../../bass/c/bass.lib")
#pragma comment(lib, "../../../bass/c/bassmidi.lib")

class Bass_Initializer
{
	critical_section lock;

	bool initialized;

public:
	Bass_Initializer() : initialized(false) { }

	~Bass_Initializer()
	{
		if ( initialized ) BASS_Free();
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
			}
		}
		return initialized;
	}
} g_initializer;

class soundfont_map
{
	critical_section m_lock;
	pfc::map_t<HSOUNDFONT, unsigned> m_map;

public:
	void add_font( HSOUNDFONT p_font )
	{
		insync( m_lock );

		if ( m_map.have_item( p_font ) ) ++m_map[ p_font ];
		else m_map[ p_font ] = 1;
	}

	void free_font( HSOUNDFONT p_font )
	{
		insync( m_lock );

		if ( --m_map[ p_font ] == 0 )
		{
			m_map.remove( m_map.find( p_font ) );
			BASS_MIDI_FontFree( p_font );
		}
	}

} g_map;

BMPlayer::BMPlayer()
{
	_stream = 0;
	uSamplesRemaining = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;

	reset_drums();

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

	if (uTimeCurrent > sample)
	{
		reset_drums();
	}

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
		int param2 = (b >> 16) & 0xFF;
		int param1 = (b >> 8) & 0xFF;
		int cmd = b & 0xF0;
		int chan = b & 0x0F;

		switch (cmd)
		{
		case 0x80:
			BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_NOTE, MAKEWORD( param1, 0 ) );
			break;
		case 0x90:
			BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_NOTE, MAKEWORD( param1, param2 ) );
			break;
		case 0xA0:
			break;
		case 0xB0:
			switch ( param1 )
			{
			case 0:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_BANK, param2 );
				if ( ( param2 == 127 || param2 == 120 ) && !drum_channels[ chan ] )
				{
					BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUMS, 1 );
					drum_channels[ chan ] = 1;
				}
				else if ( param2 == 121 && drum_channels[ chan ] )
				{
					BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUMS, 0 );
					drum_channels[ chan ] = 0;
				}
				break;

			case 1:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_MODULATION, param2 );
				break;

			case 5:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PORTATIME, param2 );
				break;

			case 7:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_VOLUME, param2 );
				break;

			case 10:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PAN, param2 );
				break;

			case 11:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_EXPRESSION, param2 );
				break;

			case 64:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_SUSTAIN, param2 );
				break;

			case 65:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PORTAMENTO, param2 );
				break;

			case 71:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_RESONANCE, param2 );
				break;

			case 72:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_RELEASE, param2 );
				break;

			case 73:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_ATTACK, param2 );
				break;

			case 74:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_CUTOFF, param2 );
				break;

			case 84:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PORTANOTE, param2 );
				break;

			case 91:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_REVERB, param2 );
				break;

			case 93:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_CHORUS, param2 );
				break;

			case 120:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_SOUNDOFF, 0 );
				break;

			case 121:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_RESET, 0 );
				is_nrpn = false;
				index_rpn = 16383;
				index_nrpn = 16383;
				rpn_finetune = 8192;
				break;

			case 123:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_NOTESOFF, 0 );
				break;

			case 126:
			case 127:
				BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_MODE, param1 == 126 ? 1 : 0 );
				break;

			case 100:
			case 101:
				is_nrpn = false;
				if ( param1 == 100 ) index_rpn = ( index_rpn & 0x3F80 ) + param2;
				else index_rpn = ( index_rpn & 0x7F ) + ( param2 << 7 );
				break;

			case 98:
			case 99:
				is_nrpn = true;
				if ( param1 == 98 ) index_nrpn = ( index_nrpn & 0x3F80 ) + param2;
				else index_nrpn = ( index_nrpn & 0x7F ) + ( param2 << 7 );
				break;

			case 96:
				if ( is_nrpn ) index_nrpn = ( index_nrpn + 1 ) & 0x3FFF;
				else index_rpn = ( index_rpn + 1 ) & 0x3FFF;
				break;

			case 97:
				if ( is_nrpn ) index_nrpn = ( index_nrpn - 1 ) & 0x3FFF;
				else index_rpn = ( index_rpn - 1 ) & 0x3FFF;
				break;

			case 38:
				if ( !is_nrpn && index_rpn == 1 )
				{
					rpn_finetune = ( rpn_finetune & 0x3F80 ) + param2;
					BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_FINETUNE, rpn_finetune );
				}
				break;

			case 6:
				if ( !is_nrpn )
				{
					switch ( index_rpn )
					{
					case 0:
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PITCHRANGE, param2 );
						break;

					case 1:
						rpn_finetune = ( rpn_finetune & 0x7F ) + ( param2 << 7 );
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_FINETUNE, rpn_finetune );
						break;

					case 2:
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_COARSETUNE, param2 );
						break;
					}
				}
				else
				{
					switch( index_nrpn )
					{
					case 0x121:
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_RESONANCE, param2 );
						break;

					case 0x166:
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_RELEASE, param2 );
						break;

					case 0x163:
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_ATTACK, param2 );
						break;

					case 0x120:
						BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_CUTOFF, param2 );
						break;

					default:
						switch ( index_nrpn >> 8 )
						{
						case 0x14:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_CUTOFF, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x15:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_RESONANCE, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x18:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_COARSETUNE, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x19:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_FINETUNE, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x1A:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_LEVEL, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x1C:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_PAN, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x1D:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_REVERB, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;

						case 0x1E:
							BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_DRUM_CHORUS, MAKEWORD( index_nrpn & 0x7F, param2 ) );
							break;
						}
					}
				}
			}
			break;
		case 0xC0:
			BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PROGRAM, param1 );
			break;
		case 0xD0:
			BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_CHANPRES, param1 );
			break;
		case 0xE0:
			BASS_MIDI_StreamEvent( _stream, chan, MIDI_EVENT_PITCH, ( param2 << 7 ) + param1 );
			break;
		case 0xF0:
			break;
		}
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size;
		mSysexMap.get_entry( n, data, size );
		if ( size == 11 &&
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
					if ( drum_channels[ drum_channel ] != data[ 8 ] )
					{
						BASS_MIDI_StreamEvent( _stream, drum_channel, MIDI_EVENT_DRUMS, data[ 8 ] );
						drum_channels [ drum_channel ] = data [8];
					}
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

void BMPlayer::shutdown()
{
	if ( _stream ) BASS_StreamFree( _stream );
	_stream = NULL;
	for ( unsigned i = 0; i < _soundFonts.get_count(); ++i )
	{
		g_map.free_font( _soundFonts[ i ] );
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
	reset_drums();
	if (sSoundFontName.length())
	{
		pfc::string_extension ext(sSoundFontName);
		if ( !pfc::stricmp_ascii( ext, "sf2" ) || !pfc::stricmp_ascii( ext, "sf2pack" ) )
		{
			HSOUNDFONT font = BASS_MIDI_FontInit( pfc::stringcvt::string_wide_from_utf8( sSoundFontName ), BASS_UNICODE );
			if ( !font )
			{
				shutdown();
				return false;
			}
			g_map.add_font( font );
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
					g_map.add_font( font );
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

	return true;
}

void BMPlayer::reset_drums()
{
	static const BYTE part_to_ch[16] = {9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15};

	memset( drum_channels, 0, sizeof( drum_channels ) );
	drum_channels [9] = 1;

	memcpy( gs_part_to_ch, part_to_ch, sizeof( gs_part_to_ch ) );

	if ( _stream )
	{
		for ( unsigned i = 0; i < 16; ++i )
		{
			BASS_MIDI_StreamEvent( _stream, i, MIDI_EVENT_DRUMS, drum_channels[ i ] );
		}
	}
}
