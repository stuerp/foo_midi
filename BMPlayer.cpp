#include <foobar2000.h>

#include "BMPlayer.h"

#include <sflist.h>

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
	sflist_presets * presetlist;
    Cached_SoundFont() : handle( 0 ), presetlist( 0 ) { }
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
		if ( it->second.handle )
			BASS_MIDI_FontFree( it->second.handle );
		if ( it->second.presetlist )
			sflist_free( it->second.presetlist );
    }
}

static HSOUNDFONT cache_open_font( const char * path )
{
    HSOUNDFONT font = NULL;
    
    insync( Cache_Lock );

    Cached_SoundFont & entry = Cache_List[ path ];
    
    if ( !entry.handle )
    {
		if ((!stricmp_utf8_partial(path, "file://") || !strstr(path, "://")))
		{
			size_t path_offset = !stricmp_utf8_partial(path, "file://") ? 7 : 0;
			font = BASS_MIDI_FontInit(pfc::stringcvt::string_wide_from_utf8(path + path_offset), 0);
		}
		else
		{
			return 0;
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

static sflist_presets * sflist_open_file(const char * path)
{
	sflist_presets * presetlist;
	FILE * f;
	char * sflist_file = 0;
	char * separator;
	size_t length;
	char error[sflist_max_error];
	pfc::string8 base_path;
	pfc::string8 our_path = "";
	size_t path_offset = (!stricmp_utf8_partial(path, "file://")) ? 7 : 0;

	base_path = path + path_offset;
	base_path.truncate( base_path.scan_filename() );

	if (!path_offset)
		our_path = "file://";
	our_path += path;

	try
	{
		abort_callback_dummy m_abort;
		file::ptr f;
		filesystem::g_open_read(f, our_path, m_abort);

		length = f->get_size_ex(m_abort);

		sflist_file = (char *)malloc(length + 1);
		if (!sflist_file)
		{
			return 0;
		}

		f->read_object(sflist_file, length, m_abort);
	}
	catch (...)
	{
		if (sflist_file) free(sflist_file);
		return 0;
	}

	sflist_file[length] = '\0';

	presetlist = sflist_load(sflist_file, strlen(sflist_file), base_path, error);

	free(sflist_file);

	return presetlist;
}

static sflist_presets * cache_open_list(const char * path)
{
	sflist_presets * presetlist = NULL;

	insync(Cache_Lock);

	Cached_SoundFont & entry = Cache_List[path];

	if (!entry.presetlist)
	{
		if ((!stricmp_utf8_partial(path, "file://") || !strstr(path, "://")))
		{
			presetlist = sflist_open_file(path);
		}
		else
		{
			return 0;
		}

		if (presetlist)
		{
			entry.presetlist = presetlist;
			entry.ref_count = 1;
		}
		else
		{
			Cache_List.erase(path);
		}
	}
	else
	{
		presetlist = entry.presetlist;
		++entry.ref_count;
	}

	return presetlist;
}

static void cache_close_font( HSOUNDFONT handle )
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

static void cache_close_list( sflist_presets * presetlist )
{
	insync(Cache_Lock);

	for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
	{
		if ( it->second.presetlist == presetlist )
		{
			if (--it->second.ref_count == 0)
				time( &it->second.time_released );
			break;
		}
	}
}

struct has_font
{
	int has;
	has_font() : has(0) { }
};

static void cache_get_stats( uint64_t & total_sample_size, uint64_t & samples_loaded_size )
{
	std::map<HSOUNDFONT, has_font> uniqueList;

	insync( Cache_Lock );

	total_sample_size = 0;
	samples_loaded_size = 0;

	for ( auto it = Cache_List.begin(); it != Cache_List.end(); ++it )
	{
		BASS_MIDI_FONTINFO info;
		if ( it->second.handle )
		{
			has_font & h = uniqueList[it->second.handle];
			if (!h.has)
			{
				h.has = 1;
				if (BASS_MIDI_FontGetInfo(it->second.handle, &info))
				{
					total_sample_size += info.samsize;
					samples_loaded_size += info.samload;
				}
			}
		}
		else if (it->second.presetlist)
		{
			sflist_presets * presetlist = it->second.presetlist;
			for (unsigned int i = 0, j = presetlist->count; i < j; ++i)
			{
				HSOUNDFONT hfont = presetlist->presets[i].font;
				has_font & h = uniqueList[hfont];
				if (!h.has)
				{
					h.has = 1;
					if (BASS_MIDI_FontGetInfo(hfont, &info))
					{
						total_sample_size += info.samsize;
						samples_loaded_size += info.samload;
					}
				}
			}
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
						if (it->second.handle)
							BASS_MIDI_FontFree( it->second.handle );
						if (it->second.presetlist)
							sflist_free(it->second.presetlist);
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
				BASS_SetConfigPtr( BASS_CONFIG_MIDI_DEFFONT, (const void *) NULL );
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
	iInterpolation = 0;
	bEffects = true;
	_presetList[0] = 0;
	_presetList[1] = 0;

	if ( !g_initializer.initialize() ) throw std::runtime_error( "Unable to initialize BASS" );
}

BMPlayer::~BMPlayer()
{
	shutdown();
}

void BMPlayer::setInterpolation(int level)
{
	iInterpolation = level;

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
		cache_close_font( _soundFonts[i] );
    }
	_soundFonts.resize( 0 );
	if (_presetList[0])
	{
		cache_close_list(_presetList[0]);
		_presetList[0] = 0;
	}
	if (_presetList[1])
	{
		cache_close_list(_presetList[1]);
		_presetList[1] = 0;
	}
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
		|| !stricmp_utf8( ext.c_str(), "sfogg" )
#endif
		)
	{
		HSOUNDFONT font = cache_open_font( in_path.c_str() );
		if ( !font )
		{
			shutdown();
			sLastError = "Unable to load SoundFont: ";
			sLastError += in_path.c_str();
			return false;
		}
		_soundFonts.push_back( font );
		BASS_MIDI_FONTEX fex = {font, -1, -1, -1, 0, 0};
		presetList.push_back( fex );
		return true;
	}
	else if ( !stricmp_utf8( ext.c_str(), "sflist" ) || !stricmp_utf8( ext.c_str(), "json" ) )
	{
		sflist_presets ** __presetList = &_presetList[0];
		if (*__presetList)
			__presetList = &_presetList[1];

		*__presetList = cache_open_list(in_path.c_str());

		if (!*__presetList)
			return false;

		BASS_MIDI_FONTEX * fontex = (*__presetList)->presets;

		for (unsigned int i = 0, j = (*__presetList)->count; i < j; ++i)
		{
			presetList.push_back(fontex[i]);
		}

		return true;
	}
	return false;
}

bool BMPlayer::startup()
{
	if ( _stream[0] && _stream[1] && _stream[2] ) return true;

	_stream[0] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bEffects ? 0 : BASS_MIDI_NOFX ), (unsigned int) uSampleRate );
	_stream[1] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bEffects ? 0 : BASS_MIDI_NOFX ), (unsigned int) uSampleRate );
	_stream[2] = BASS_MIDI_StreamCreate( 16, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | ( bEffects ? 0 : BASS_MIDI_NOFX ), (unsigned int) uSampleRate );
	if (!_stream[0] || !_stream[1] || !_stream[2])
	{
		return false;
	}
	BASS_ChannelSetAttribute(_stream[0], BASS_ATTRIB_MIDI_SRC, (float)iInterpolation);
	BASS_ChannelSetAttribute(_stream[1], BASS_ATTRIB_MIDI_SRC, (float)iInterpolation);
	BASS_ChannelSetAttribute(_stream[2], BASS_ATTRIB_MIDI_SRC, (float)iInterpolation);
	memset( bank_lsb_override, 0, sizeof( bank_lsb_override ) );
	std::vector<BASS_MIDI_FONTEX> presetList;
	if (sFileSoundFontName.length())
	{
		if (!load_font_item(presetList, sFileSoundFontName))
			return false;
	}

	if (sSoundFontName.length())
	{
		if ( !load_font_item( presetList, sSoundFontName ) )
			return false;
	}

	BASS_MIDI_StreamSetFonts( _stream[0], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX );
	BASS_MIDI_StreamSetFonts( _stream[1], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX );
	BASS_MIDI_StreamSetFonts( _stream[2], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX );

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

bool BMPlayer::get_last_error(std::string & p_out)
{
	if (sLastError.length())
	{
		p_out = sLastError;
		return true;
	}
	return false;
}
