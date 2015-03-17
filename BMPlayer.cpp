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
                uOutputDebugString( e.what() );
                delete f;
                return NULL;
        }
}

static HSOUNDFONT cache_open( const char * path )
{
    HSOUNDFONT font = NULL;
    
    insync( Cache_Lock );

    Cached_SoundFont & entry = Cache_List[ path ];
    
    if ( !entry.handle )
    {
		if (!stricmp_utf8_partial(path, "file://") && !stricmp_utf8(pfc::string_extension(path), "sfz"))
		{
			font = BASS_MIDI_FontInit(pfc::stringcvt::string_wide_from_utf8(path), 0);
		}
		else
		{
			void * fb_file = fb_open(path);
			if (!fb_file) return NULL;

			font = BASS_MIDI_FontInitUser(&fb_fileprocs, fb_file, 0);
		}

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
					if ( difftime( now, it->second.time_released ) >= 10.0 )
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
	bool already;
    
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
			if ( !already )
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
			BASS_SetConfig( BASS_CONFIG_UPDATETHREADS, 0 );
			initialized = !!BASS_Init( 0, 44100, 0, NULL, NULL );
			if ( !initialized )
				initialized = already = BASS_ErrorGetCode() == BASS_ERROR_ALREADY;
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
	memset(_stream, 0, sizeof(_stream));
	bSincInterpolation = false;
	bEffects = true;

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

void BMPlayer::setEffects(bool enable)
{
	bEffects = enable;

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
		unsigned event_length = (command >= 0xF8 && command <= 0xFF) ? 1 : (( command == 0xC0 || command == 0xD0 ) ? 2 : 3);
		if ( port > 2 ) port = 2;
		if ( bank_lsb_overridden && command == 0xB0 && event[1] == 0x20 ) return;
		BASS_MIDI_StreamEvents( _stream[port], BASS_MIDI_EVENTS_RAW, event, event_length );
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
        std::size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
		BASS_MIDI_StreamEvents( _stream[port], BASS_MIDI_EVENTS_RAW, data, (unsigned int) size );
		if ( port == 0 )
		{
			BASS_MIDI_StreamEvents( _stream[1], BASS_MIDI_EVENTS_RAW, data, (unsigned int)size );
			BASS_MIDI_StreamEvents( _stream[2], BASS_MIDI_EVENTS_RAW, data, (unsigned int)size );
		}
	}
}

void BMPlayer::render(float * out, unsigned long count)
{
	float buffer[1024];
	while (count)
	{
		unsigned long todo = count;
		if (todo > 512)
			todo = 512;
		memset(out, 0, todo * sizeof(float) * 2);
		for (int i = 0; i < 3; ++i)
		{
			BASS_ChannelGetData(_stream[i], buffer, BASS_DATA_FLOAT | (unsigned int)(todo * sizeof(float) * 2));
			for (unsigned long j = 0; j < todo * 2; ++j)
			{
				out[j] += buffer[j];
			}
		}
		out += todo * 2;
		count -= todo;
	}
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
	if ( _stream[2] ) BASS_StreamFree( _stream[2] );
	if ( _stream[1] ) BASS_StreamFree( _stream[1] );
	if ( _stream[0] ) BASS_StreamFree( _stream[0] );
	memset( _stream, 0, sizeof(_stream) );
	for ( unsigned long i = 0; i < _soundFonts.size(); ++i )
	{
		cache_close( _soundFonts[i] );
    }
	_soundFonts.resize( 0 );
}

void BMPlayer::compound_presets( std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels )
{
	if ( !in.size() )
	{
		BASS_MIDI_FONTEX fex = { 0, -1, -1, -1, 0, 0 };
		in.push_back( fex );
	}

	if ( channels.size() )
	{
		for ( auto pit = in.begin(); pit != in.end(); ++pit )
		{
			for ( auto it = channels.begin(); it != channels.end(); ++it )
			{
				bank_lsb_override[ *it - 1 ] = *it;

				int dbanklsb = (int) *it;
				pit->dbanklsb = dbanklsb;

				out.push_back( *pit );
			}
		}
	}
	else
	{
		for ( auto pit = in.begin(); pit != in.end(); ++pit )
		{
			out.push_back( *pit );
		}
	}
}

bool BMPlayer::load_font_item(std::vector<BASS_MIDI_FONTEX> & presetList, std::string in_path)
{
	std::string ext;
	size_t dot = in_path.find_last_of('.');
	if (dot != std::string::npos) ext.assign( in_path.begin() +  dot + 1, in_path.end() );
	if ( !stricmp_utf8( ext.c_str(), "sf2" )
#ifdef SF2PACK
		|| !stricmp_utf8( ext.c_str(), "sf2pack" )
#endif
		)
	{
		HSOUNDFONT font = cache_open( in_path.c_str() );
		if ( !font )
		{
			shutdown();
			return false;
		}
		_soundFonts.push_back( font );
		BASS_MIDI_FONTEX fex = {font, -1, -1, -1, 0, 0};
		presetList.push_back( fex );
		return true;
	}
	else if ( !stricmp_utf8( ext.c_str(), "sflist" ) )
	{
		size_t fn_offset = 0;
		if (!stricmp_utf8_partial(in_path.c_str(), "file://"))
			fn_offset = 7;
		FILE * fl = _tfopen( pfc::stringcvt::string_os_from_utf8( in_path.c_str() + fn_offset ), _T("r, ccs=UTF-8") );
		if ( fl )
		{
			std::string path, temp;
			pfc::stringcvt::string_utf8_from_os converter;
			TCHAR name[32768];
			TCHAR *nameptr;
			size_t slash = in_path.find_last_of('\\');
			if ( slash != std::string::npos ) path.assign( in_path.begin(), in_path.begin() + slash + 1 );
			while ( !feof( fl ) )
			{
				std::vector<BASS_MIDI_FONTEX> presets;

				if ( !_fgetts( name, 32767, fl ) ) break;
				name[32767] = 0;
				TCHAR * cr = _tcschr( name, _T('\n') );
				if ( cr ) *cr = 0;
				cr = _tcschr( name, _T('\r') );
				if ( cr ) *cr = 0;
				cr = _tcschr( name, '|' );
				if ( cr )
				{
					std::vector<BASS_MIDI_FONTEX> nested_presets;
					std::vector<long> channels;
					bool valid = true;
					bool pushed_back = true;
					TCHAR *endchr;
					nameptr = cr + 1;
					*cr = 0;
					cr = name;
					while ( *cr && valid )
					{
						switch ( *cr++ )
						{
						case 'p':
							{
								// patch override - "p[db#,]dp#=[sb#,]sp#" ex. "p0,5=0,1"
								// may be used once per preset group
								pushed_back = false;
								long dbank = 0;
								long dpreset = _tcstol( cr, &endchr, 10 );
								if ( endchr == cr )
								{
									valid = false;
									break;
								}
								if ( *endchr == ',' )
								{
									dbank = dpreset;
									cr = endchr + 1;
									dpreset = _tcstol( cr, &endchr, 10 );
									if ( endchr == cr )
									{
										valid = false;
										break;
									}
								}
								if ( *endchr != '=' )
								{
									valid = false;
									break;
								}
								cr = endchr + 1;
								long sbank = -1;
								long spreset = _tcstol( cr, &endchr, 10 );
								if ( endchr == cr )
								{
									valid = false;
									break;
								}
								if ( *endchr == ',' )
								{
									sbank = spreset;
									cr = endchr + 1;
									spreset = _tcstol( cr, &endchr, 10 );
									if ( endchr == cr )
									{
										valid = false;
										break;
									}
								}
								if ( *endchr && *endchr != ';' && *endchr != '&' )
								{
									valid = false;
									break;
								}
								cr = endchr;
								BASS_MIDI_FONTEX fex = { 0, (int) spreset, (int) sbank, (int) dpreset, (int) dbank, 0 };
								nested_presets.push_back( fex );
							}
							break;

						case 'c':
							{
								// channel override - implemented using bank LSB, which is disabled from
								// actual use. - format "c#[-#]" ex. "c16" (range is 1-48)
								// may be used multiple times per preset group
								pushed_back = false;
								long channel_start = _tcstol(cr, &endchr, 10);
								long channel_end;
								if ( endchr == cr )
								{
									valid = false;
									break;
								}
								if ( channel_start < 1 || channel_start > 48 )
								{
									valid = false;
									break;
								}
								channel_end = channel_start;
								if ( *endchr == '-' )
								{
									channel_end = _tcstol(cr, &endchr, 10);
									if ( channel_end <= channel_start || channel_end > 48 )
									{
										valid = false;
										break;
									}
								}
								for ( auto it = channels.begin(); it != channels.end(); ++it )
								{
									if ( *it >= channel_start || *it <= channel_end )
									{
										valid = false;
										break;
									}
								}
								if ( *endchr && *endchr != ';' )
								{
									valid = false;
									break;
								}
								cr = endchr;
								for ( long channel = channel_start; channel <= channel_end; ++channel )
									channels.push_back( channel );
							}
							break;

						case '&':
							{
								// separates preset groups per SoundFont bank
								if ( !pushed_back )
								{
									compound_presets( presets, nested_presets, channels );
									nested_presets.clear(); channels.clear();
									pushed_back = true;
								}
							}
							break;

						case ';':
							// separates preset items
							break;

						default:
							// invalid command character
							valid = false;
							break;
						}
					}
					if ( !pushed_back && valid )
						compound_presets( presets, nested_presets, channels );
					if ( !valid )
					{
						presets.clear();
						BASS_MIDI_FONTEX fex = { 0, -1, -1, -1, 0, 0 };
						presets.push_back( fex );
						memset( bank_lsb_override, 0, sizeof(bank_lsb_override) );
					}
				}
				else
				{
					BASS_MIDI_FONTEX fex = { 0, -1, -1, -1, 0, 0 };
					presets.push_back( fex );
					nameptr = name;
				}
				converter.convert( nameptr );
				if ( ( isalpha( nameptr[0] ) && nameptr[1] == _T(':') ) || nameptr[0] == '\\' )
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
				for ( auto it = presets.begin(); it != presets.end(); ++it )
				{
					it->font = font;
					presetList.push_back( *it );
				}
				_soundFonts.push_back( font );
			}
			fclose( fl );
			return true;
		}
	}
	return false;
}

bool BMPlayer::startup()
{
	if ( _stream[0] && _stream[1] && _stream[2] ) return true;

	_stream[0] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ) | ( bEffects ? 0 : BASS_MIDI_NOFX ), (unsigned int) uSampleRate );
	_stream[1] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ) | ( bEffects ? 0 : BASS_MIDI_NOFX ), (unsigned int) uSampleRate );
	_stream[2] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bSincInterpolation ? BASS_MIDI_SINCINTER : 0 ) | ( bEffects ? 0 : BASS_MIDI_NOFX ), (unsigned int) uSampleRate );
	if (!_stream[0] || !_stream[1] || !_stream[2])
	{
		return false;
	}
	memset( bank_lsb_override, 0, sizeof( bank_lsb_override ) );
	std::vector<BASS_MIDI_FONTEX> presetList;
	if (sSoundFontName.length())
	{
		if ( !load_font_item( presetList, sSoundFontName ) )
			return false;
	}

	if ( sFileSoundFontName.length() )
	{
		if ( !load_font_item( presetList, sFileSoundFontName ) )
			return false;
	}

    std::vector< BASS_MIDI_FONTEX > fonts;
	for ( unsigned long i = 0, j = presetList.size(); i < j; ++i )
	{
		fonts.push_back( presetList[ j - i - 1 ] );
	}
	BASS_MIDI_StreamSetFonts( _stream[0], &fonts[0], (unsigned int) fonts.size() | BASS_MIDI_FONT_EX );
	BASS_MIDI_StreamSetFonts( _stream[1], &fonts[0], (unsigned int) fonts.size() | BASS_MIDI_FONT_EX );
	BASS_MIDI_StreamSetFonts( _stream[2], &fonts[0], (unsigned int) fonts.size() | BASS_MIDI_FONT_EX );

	reset_parameters();

	return true;
}

void BMPlayer::reset_parameters()
{
	bank_lsb_overridden = false;
	for ( unsigned int i = 0; i < 48; ++i )
	{
		if ( bank_lsb_override[i] )
			bank_lsb_overridden = true;
		BASS_MIDI_StreamEvent( _stream[i / 16], i % 16, MIDI_EVENT_BANK_LSB, bank_lsb_override[i] );
	}
}
