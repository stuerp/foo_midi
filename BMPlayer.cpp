#include "BMPlayer.h"

#include <string>

#pragma comment(lib, "../../../bass/c/bass.lib")
#pragma comment(lib, "../../../bass/c/bassmidi.lib")

#define SF2PACK

static const t_uint8 sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const t_uint8 sysex_gm2_reset[]= { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const t_uint8 sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const t_uint8 sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool is_gs_reset(const unsigned char * data, unsigned size)
{
	if ( size != _countof( sysex_gs_reset ) ) return false;

	if ( memcmp( data, sysex_gs_reset, 5 ) != 0 ) return false;
	if ( memcmp( data + 7, sysex_gs_reset + 7, 2 ) != 0 ) return false;
	if ( ( ( data[ 5 ] + data[ 6 ] + 1 ) & 127 ) != data[ 9 ] ) return false;
	if ( data[ 10 ] != sysex_gs_reset[ 10 ] ) return false;

	return true;
}

static void CALLBACK fb_close(void * user)
{
	delete (file::ptr*) user;
}

static QWORD CALLBACK fb_length(void * user)
{
	try
	{
		return (*((file::ptr*)user))->get_size_ex( abort_callback_dummy() );
	}
	catch (...)
	{
		return 0;
	}
}

static DWORD CALLBACK fb_read(void * buffer, DWORD length, void * user)
{
	try
	{
		return (*((file::ptr*)user))->read( buffer, length, abort_callback_dummy() );
	}
	catch (...)
	{
		return 0;
	}
}

static BOOL CALLBACK fb_seek(QWORD offset, void * user)
{
	try
	{
		(*((file::ptr*)user))->seek( offset, abort_callback_dummy() );
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
}

static BASS_FILEPROCS fb_fileprocs = { fb_close, fb_length, fb_read, fb_seek };

static void * fb_open( const char * path )
{
	file::ptr * f = NULL;
	try
	{
		f = new file::ptr;
		pfc::string8 m_path, m_canonical_path;
		if ( !strstr( path, "://" ) )
		{
			m_path = "file://";
			m_path += path;
			path = m_path.get_ptr();
		}
		filesystem::g_get_canonical_path( path, m_canonical_path );
		filesystem::g_open( *f, m_canonical_path, filesystem::open_mode_read, abort_callback_dummy() );
		return (void *) f;
	}
	catch (std::exception & e)
	{
		OutputDebugStringA( e.what() );
		delete f;
		return NULL;
	}
}

class soundfont_map : public pfc::thread
{
	critical_section m_lock;

	struct soundfont_info
	{
		unsigned m_reference_count;
		HSOUNDFONT m_handle;
		LARGE_INTEGER m_release_time;
	};

	pfc::map_t<std::string, soundfont_info> m_map;

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
				for ( pfc::map_t<std::string, soundfont_info>::const_iterator it = m_map.first(); it.is_valid(); )
				{
					pfc::map_t<std::string, soundfont_info>::const_iterator it_copy = it++;
					if ( it_copy->m_value.m_reference_count == 0 && now.QuadPart >= it_copy->m_value.m_release_time.QuadPart )
					{
						BASS_MIDI_FontFree( it_copy->m_value.m_handle );
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
		for ( pfc::map_t<std::string, soundfont_info>::const_iterator it = m_map.first(); it.is_valid(); ++it )
		{
			BASS_MIDI_FontFree( it->m_value.m_handle );
		}
	}

	HSOUNDFONT load_font( std::string p_path )
	{
		insync( m_lock );

		pfc::map_t<std::string, soundfont_info>::iterator it = m_map.find( p_path );

		if ( it.is_valid() )
		{
			++(it->m_value.m_reference_count);
			return it->m_value.m_handle;
		}
		else
		{
			void * fb_file = fb_open( p_path.c_str() );
			if ( !fb_file ) return NULL;

			HSOUNDFONT font = BASS_MIDI_FontInitUser( &fb_fileprocs, fb_file, 0 );
			if ( font )
			{
				m_map[ p_path ].m_reference_count = 1;
				m_map[ p_path ].m_handle = font;
			}
			return font;
		}
	}

	void free_font( HSOUNDFONT p_font )
	{
		insync( m_lock );

		for ( pfc::map_t<std::string, soundfont_info>::iterator it = m_map.first(); it.is_valid(); ++it )
		{
			if ( it->m_value.m_handle == p_font )
			{
				if ( --(it->m_value.m_reference_count) == 0 )
				{
					LARGE_INTEGER now;
					LARGE_INTEGER increment;
					QueryPerformanceCounter( &now );
					QueryPerformanceFrequency( &increment );
					now.QuadPart += increment.QuadPart * 2;
					it->m_value.m_release_time.QuadPart = now.QuadPart;
				}
				break;
			}
		}
	}

	void get_stats( t_filesize & total_sample_size, t_filesize & samples_loaded_size )
	{
		total_sample_size = 0;
		samples_loaded_size = 0;

		insync( m_lock );

		for ( pfc::map_t<std::string, soundfont_info>::iterator it = m_map.first(); it.is_valid(); ++it )
		{
			BASS_MIDI_FONTINFO info;
			if ( BASS_MIDI_FontGetInfo( it->m_value.m_handle, &info ) )
			{
				total_sample_size += info.samsize;
				samples_loaded_size += info.samload;
			}
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

	bool check_initialized()
	{
		insync(lock);
		return initialized;
	}

	bool initialize()
	{
		insync(lock);
		if ( !initialized )
		{
#ifdef SF2PACK
			pfc::string8_fast filename;
			pfc::string8 path = core_api::get_my_full_path();
			path.truncate( path.scan_filename() );
			filename = path;
			puFindFile finder = uFindFirstFile( pfc::string_formatter() << path << "bass*.dll" );
			if ( finder )
			{
				pfc::stringcvt::string_wide_from_utf8_fast convert;
				do
				{
					filename.truncate( path.length() );
					filename += finder->GetFileName();
					convert.convert( filename );
					BASS_PluginLoad( ( const char * ) convert.get_ptr(), BASS_UNICODE );
				} while ( finder->FindNext() );
				delete finder;
			}
#endif
			BASS_SetConfig( BASS_CONFIG_UPDATEPERIOD, 0 );
			initialized = !!BASS_Init( 0, 44100, 0, core_api::get_main_window(), NULL );
			if ( initialized )
			{
				BASS_SetConfigPtr( BASS_CONFIG_MIDI_DEFFONT, NULL );
				BASS_SetConfig( BASS_CONFIG_MIDI_VOICES, 256 );
				g_map = new soundfont_map;
				g_map->start();
			}
		}
		return initialized;
	}
} g_initializer;

bool g_get_soundfont_stats( t_filesize & total_sample_size, t_filesize & samples_loaded_size )
{
	if ( g_initializer.check_initialized() )
	{
		g_map->get_stats( total_sample_size, samples_loaded_size );
		return true;
	}
	else return false;
}

BMPlayer::BMPlayer() : MIDIPlayer()
{
	_stream = 0;
	bSincInterpolation = false;

	if ( !g_initializer.initialize() ) throw exception_io_data( "Unable to initialize BASS" );
}

BMPlayer::~BMPlayer()
{
	shutdown();
}

void BMPlayer::setSincInterpolation(bool enable)
{
	bSincInterpolation = enable;

	shutdown();
}

void BMPlayer::send_event(DWORD b)
{
	if (!(b & 0x80000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned port = (b >> 24) & 0x7F;
		unsigned channel = b & 0x0F;
		unsigned command = b & 0xF0;
		unsigned event_length = ( command == 0xC0 || command == 0xD0 ) ? 2 : 3;
		channel += 16 * port;
		BASS_MIDI_StreamEvents( _stream, BASS_MIDI_EVENTS_RAW + 1 + channel, event, event_length );
		if ( command == 0xB0 && event[ 1 ] == 0 )
		{
			if ( synth_mode == mode_xg )
			{
				if ( event[ 2 ] == 127 ) drum_channels[ channel ] = 1;
				else drum_channels[ channel ] = 0;
			}
			else if ( synth_mode == mode_gm2 )
			{
				if ( event[ 2 ] == 120 ) drum_channels[ channel ] = 1;
				else if ( event[ 2 ] == 121 ) drum_channels[ channel ] = 0;
			}
		}
		else if ( command == 0xC0 )
		{
			unsigned channel_masked = channel & 0x0F;
			unsigned drum_channel = drum_channels[ channel ];
			if ( ( channel_masked == 9 && !drum_channel ) ||
				( channel_masked != 9 && drum_channel ) )
				BASS_MIDI_StreamEvent( _stream, channel, MIDI_EVENT_DRUMS, drum_channel );
		}
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
		BASS_MIDI_StreamEvents( _stream, BASS_MIDI_EVENTS_RAW, data, size );
		if ( ( size == _countof( sysex_gm_reset ) && !memcmp( data, sysex_gm_reset, _countof( sysex_gm_reset ) ) ) ||
			( size == _countof( sysex_gm2_reset ) && !memcmp( data, sysex_gm2_reset, _countof( sysex_gm2_reset ) ) ) ||
			is_gs_reset( data, size ) ||
			( size == _countof( sysex_xg_reset ) && !memcmp( data, sysex_xg_reset, _countof( sysex_xg_reset ) ) ) )
		{
			reset_drum_channels();
			synth_mode = ( size == _countof( sysex_xg_reset ) ) ? mode_xg :
			             ( size == _countof( sysex_gs_reset ) ) ? mode_gs :
			             ( data [4] == 0x01 )                   ? mode_gm :
			                                                      mode_gm2;
		}
		else if ( synth_mode == mode_gs && size == 11 &&
			data [0] == 0xF0 && data [1] == 0x41 && data [3] == 0x42 &&
			data [4] == 0x12 && data [5] == 0x40 && (data [6] & 0xF0) == 0x10 &&
			data [10] == 0xF7)
		{
			if (data [7] == 2)
			{
				// GS MIDI channel to part assign
				gs_part_to_ch [ port ][ data [6] & 15 ] = data [8];
			}
			else if ( data [7] == 0x15 )
			{
				// GS part to rhythm allocation
				unsigned int drum_channel = gs_part_to_ch [ port ][ data [6] & 15 ];
				if ( drum_channel < 16 )
				{
					drum_channel += 16 * port;
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
	if ( _stream ) return true;

	_stream = BASS_MIDI_StreamCreate( 48, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ), uSampleRate );
	if (!_stream)
	{
		return false;
	}
	if (sSoundFontName.length())
	{
		pfc::string_extension ext(sSoundFontName);
		if ( !pfc::stricmp_ascii( ext, "sf2" )
#ifdef SF2PACK
			|| !pfc::stricmp_ascii( ext, "sf2pack" )
#endif
			)
		{
			HSOUNDFONT font = g_map->load_font( sSoundFontName.get_ptr() );
			if ( !font )
			{
				shutdown();
				return false;
			}
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
					if ( !_fgetts( name, 1024, fl ) ) break;
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
					HSOUNDFONT font = g_map->load_font( pfc::stringcvt::string_utf8_from_os( cr ).get_ptr() );
					if ( !font )
					{
						fclose( fl );
						shutdown();
						return false;
					}
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
		HSOUNDFONT font = g_map->load_font( sFileSoundFontName.get_ptr() );
		if ( !font )
		{
			shutdown();
			return false;
		}
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

	synth_mode = mode_gm;

	return true;
}

void BMPlayer::reset_drum_channels()
{
	static const BYTE part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

	memset( drum_channels, 0, sizeof( drum_channels ) );
	drum_channels[ 9 ] = 1;
	drum_channels[ 25 ] = 1;
	drum_channels[ 41 ] = 1;

	for ( unsigned i = 0; i < 3; i++ )
		memcpy( gs_part_to_ch[ i ], part_to_ch, sizeof( gs_part_to_ch[ i ] ) );

	if ( _stream )
	{
		for ( unsigned i = 0; i < 48; ++i )
		{
			BASS_MIDI_StreamEvent( _stream, i, MIDI_EVENT_DRUMS, drum_channels[ i ] );
		}
	}
}
