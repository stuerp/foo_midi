#include <foobar2000.h>

#include "BMPlayer.h"

#include <string>

#include <map>
#include <time.h>

#undef USE_STD_THREAD
#ifdef USE_STD_THREAD
#include <thread>
#endif

#define SF2PACK

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

static const uint8_t sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t sysex_gm2_reset[]= { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool is_gs_reset(const unsigned char * data, unsigned long size)
{
	if ( size != _countof( sysex_gs_reset ) ) return false;

	if ( memcmp( data, sysex_gs_reset, 5 ) != 0 ) return false;
	if ( memcmp( data + 7, sysex_gs_reset + 7, 2 ) != 0 ) return false;
	if ( ( ( data[ 5 ] + data[ 6 ] + 1 ) & 127 ) != data[ 9 ] ) return false;
	if ( data[ 10 ] != sysex_gs_reset[ 10 ] ) return false;

	return true;
}

struct Cached_SoundFont
{
    unsigned long ref_count;
    time_t time_released;
    HSOUNDFONT handle;
    Cached_SoundFont() : handle( 0 ) { }
};

static critical_section Cache_Lock;

static std::map<std::string, Cached_SoundFont> Cache_List;

static bool Cache_Running = false;

static void cache_run();

#ifdef USE_STD_THREAD
static std::thread * Cache_Thread = NULL;
#else
class Cache_Thread : public pfc::thread
{
public:
	virtual void threadProc()
	{
		cache_run();
	}

	~Cache_Thread()
	{
		Cache_Running = false;
		waitTillDone();
	}
};

static Cache_Thread * Cache_Thread_Instance = NULL;
#endif

static void cache_init()
{
#ifdef USE_STD_THREAD
    Cache_Thread = new std::thread( cache_run );
#else
	Cache_Thread_Instance = new Cache_Thread;
	Cache_Thread_Instance->start();
#endif
}

static void cache_deinit()
{
#ifdef USE_STD_THREAD
    Cache_Running = false;
    Cache_Thread->join();
    delete Cache_Thread;
	Cache_Thread = NULL;
#else
	delete Cache_Thread_Instance;
	Cache_Thread_Instance = NULL;
#endif
    
    for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
    {
        BASS_MIDI_FontFree( it->second.handle );
    }
}

static HSOUNDFONT cache_open( const char * path )
{
    HSOUNDFONT font = NULL;
    
    insync( Cache_Lock );

    Cached_SoundFont & entry = Cache_List[ path ];
    
    if ( !entry.handle )
    {
        font = BASS_MIDI_FontInit( pfc::stringcvt::string_os_from_utf8( path ), BASS_UNICODE );
        if ( font )
        {
            entry.handle = font;
            entry.ref_count = 1;
        }
        else
        {
            Cache_List.erase( path );
        }
    }
    else
	{
        font = entry.handle;
		++entry.ref_count;
	}
    
    return font;
}

static void cache_close( HSOUNDFONT handle )
{
    insync( Cache_Lock );
    
    for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
    {
        if ( it->second.handle == handle )
        {
            if ( --it->second.ref_count == 0 )
                time( &it->second.time_released );
            break;
        }
    }
}

static void cache_get_stats( uint64_t & total_sample_size, uint64_t & samples_loaded_size )
{
	insync( Cache_Lock );

	total_sample_size = 0;
	samples_loaded_size = 0;

	for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
	{
		BASS_MIDI_FONTINFO info;
		if ( BASS_MIDI_FontGetInfo( it->second.handle, &info ) )
		{
			total_sample_size += info.samsize;
			samples_loaded_size += info.samload;
		}
	}
}

static void cache_run()
{
    Cache_Running = true;
    
    while ( Cache_Running )
    {
        time_t now;
        time( &now );
        
		{
			insync( Cache_Lock );

			for ( auto it = Cache_List.begin(); it != Cache_List.end(); )
			{
				if ( it->second.ref_count == 0 )
				{
					if ( difftime( it->second.time_released, now ) >= 10.0 )
					{
						BASS_MIDI_FontFree( it->second.handle );
						it = Cache_List.erase( it );
						continue;
					}
				}
				++it;
			}
		}
        
        Sleep( 250 );
    }
}

static class Bass_Initializer
{
    critical_section lock;

	bool initialized;
    
    std::string base_path;

public:
	Bass_Initializer() : initialized(false)
    {
    }

	~Bass_Initializer()
	{
		if ( initialized )
		{
            cache_deinit();
			BASS_Free();
		}
	}

	bool check_initialized()
	{
		insync( lock );
		return initialized;
	}
    
    void set_base_path()
    {
        base_path = core_api::get_my_full_path();
        size_t slash = base_path.find_last_of( '\\' );
        base_path.erase( base_path.begin() + slash + 1, base_path.end() );
    }
    
    void load_plugin(const char * name)
    {
        std::string full_path = base_path;
        full_path += name;
		BASS_PluginLoad( (const char *) pfc::stringcvt::string_os_from_utf8( full_path.c_str() ).get_ptr(), BASS_UNICODE );
    }

	bool initialize()
	{
		insync( lock );
		if ( !initialized )
		{
#ifdef SF2PACK
            set_base_path();
            load_plugin( "bassflac.dll" );
            load_plugin( "basswv.dll" );
            load_plugin( "bassopus.dll" );
            load_plugin( "bass_mpc.dll" );
#endif
			BASS_SetConfig( BASS_CONFIG_UPDATEPERIOD, 0 );
			initialized = !!BASS_Init( 0, 44100, 0, NULL, NULL );
			if ( initialized )
			{
				BASS_SetConfigPtr( BASS_CONFIG_MIDI_DEFFONT, NULL );
				BASS_SetConfig( BASS_CONFIG_MIDI_VOICES, 256 );
                cache_init();
			}
		}
		return initialized;
	}
} g_initializer;

bool g_get_soundfont_stats( uint64_t & total_sample_size, uint64_t & samples_loaded_size )
{
       if ( g_initializer.check_initialized() )
       {
               cache_get_stats( total_sample_size, samples_loaded_size );
               return true;
       }
       else return false;
}

BMPlayer::BMPlayer() : MIDIPlayer()
{
	_stream = 0;
	bSincInterpolation = false;

	if ( !g_initializer.initialize() ) throw std::runtime_error( "Unable to initialize BASS" );
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

void BMPlayer::send_event(uint32_t b)
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
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
        std::size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
		BASS_MIDI_StreamEvents( _stream, BASS_MIDI_EVENTS_RAW, data, (unsigned int) size );
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

void BMPlayer::render(float * out, unsigned long count)
{
	BASS_ChannelGetData( _stream, out, BASS_DATA_FLOAT | (unsigned int) ( count * sizeof( float ) * 2 ) );
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
	for ( unsigned long i = 0; i < _soundFonts.size(); ++i )
	{
		cache_close( _soundFonts[i] );
    }
	_soundFonts.resize( 0 );
}

bool BMPlayer::startup()
{
	if ( _stream ) return true;

	_stream = BASS_MIDI_StreamCreate( 48, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ), (unsigned int) uSampleRate );
	if (!_stream)
	{
		return false;
	}
	if (sSoundFontName.length())
	{
        std::string ext;
        size_t dot = sSoundFontName.find_last_of('.');
        if (dot != std::string::npos) ext.assign( sSoundFontName.begin() +  dot + 1, sSoundFontName.end() );
		if ( !stricmp_utf8( ext.c_str(), "sf2" )
#ifdef SF2PACK
			|| !stricmp_utf8( ext.c_str(), "sf2pack" )
#endif
			)
		{
			HSOUNDFONT font = cache_open( sSoundFontName.c_str() );
			if ( !font )
			{
				shutdown();
				return false;
			}
			_soundFonts.push_back( font );
		}
		else if ( !stricmp_utf8( ext.c_str(), "sflist" ) )
		{
			FILE * fl = _tfopen( pfc::stringcvt::string_os_from_utf8( sSoundFontName.c_str() ), _T("r, ccs=UTF-8") );
			if ( fl )
			{
                std::string path, temp;
				pfc::stringcvt::string_utf8_from_os converter;
                TCHAR name[32768];
                size_t slash = sSoundFontName.find_last_of('\\');
                if ( slash != std::string::npos ) path.assign( sSoundFontName.begin(), sSoundFontName.begin() + slash + 1 );
				while ( !feof( fl ) )
				{
					if ( !_fgetts( name, 32767, fl ) ) break;
					name[32767] = 0;
					TCHAR * cr = _tcschr( name, _T('\n') );
					if ( cr ) *cr = 0;
                    cr = _tcschr( name, _T('\r') );
                    if ( cr ) *cr = 0;
					converter.convert( name );
					if ( ( isalpha( name[0] ) && name[1] == _T(':') ) || name[0] == '\\' )
					{
						temp = converter;
					}
					else
					{
                        temp = path;
                        temp += converter;
					}
					HSOUNDFONT font = cache_open( temp.c_str() );
					if ( !font )
					{
						fclose( fl );
						shutdown();
						return false;
					}
					_soundFonts.push_back( font );
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
		HSOUNDFONT font = cache_open( sFileSoundFontName.c_str() );
		if ( !font )
		{
			shutdown();
			return false;
		}
		_soundFonts.push_back( font );
	}

    std::vector< BASS_MIDI_FONT > fonts;
	for ( unsigned long i = 0, j = _soundFonts.size(); i < j; ++i )
	{
		BASS_MIDI_FONT sf;
		sf.font = _soundFonts[ j - i - 1 ];
		sf.preset = -1;
		sf.bank = 0;
		fonts.push_back( sf );
	}
	BASS_MIDI_StreamSetFonts( _stream, &fonts[0], (unsigned int) fonts.size() );

	reset_drum_channels();

	synth_mode = mode_gm;

	return true;
}

void BMPlayer::reset_drum_channels()
{
	static const uint8_t part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

	memset( drum_channels, 0, sizeof( drum_channels ) );
	drum_channels[ 9 ] = 1;
	drum_channels[ 25 ] = 1;
	drum_channels[ 41 ] = 1;

	for ( unsigned long i = 0; i < 3; i++ )
		memcpy( gs_part_to_ch[ i ], part_to_ch, sizeof( gs_part_to_ch[ i ] ) );

	if ( _stream )
	{
		for ( unsigned i = 0; i < 48; ++i )
		{
			BASS_MIDI_StreamEvent( _stream, i, MIDI_EVENT_DRUMS, drum_channels[ i ] );
		}
	}
}
