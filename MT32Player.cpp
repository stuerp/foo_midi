#include <foobar2000.h>

#include "MT32Player.h"

MT32Player::MT32Player( bool gm, unsigned gm_set )
	: bGM( gm ), uGMSet( gm_set ), MIDIPlayer()
{
	_synth = NULL;
	controlRom = NULL;
	pcmRom = NULL;
	controlRomFile = NULL;
	pcmRomFile = NULL;
}

MT32Player::~MT32Player()
{
	if (_synth)
	{
		_synth->close();
		delete _synth;
	}
	if (controlRom)
	{
		MT32Emu::ROMImage::freeROMImage( controlRom );
	}
	if (pcmRom)
	{
		MT32Emu::ROMImage::freeROMImage( pcmRom );
	}
	delete controlRomFile;
	delete pcmRomFile;
}

void MT32Player::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		_synth->playMsg( b );
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		_synth->playSysex( data, size );
	}
}

void MT32Player::render(float * out, unsigned long count)
{
#if 1
	pfc::static_assert_t<sizeof(audio_sample) == sizeof(float)>();
	_synth->render_float( out, count );
#else
	MT32Emu::Bit16s temp[512];
	while ( count > 0 )
	{
		unsigned long todo = count;
		if ( todo > 256 ) todo = 256;
		_synth->render( temp, (unsigned) todo );
		audio_math::convert_from_int16( temp, todo * 2, out, 1. );
		out += todo * 2;
		count -= todo;
	}
#endif
}

void MT32Player::setBasePath( const char * in )
{
	sBasePath = in;
	shutdown();
}

void MT32Player::setAbortCallback( abort_callback * in )
{
	_abort = in;
}

void MT32Player::shutdown()
{
	if (_synth)
	{
		_synth->close();
		delete _synth;
	}
	if (controlRom)
	{
		MT32Emu::ROMImage::freeROMImage( controlRom );
	}
	if (pcmRom)
	{
		MT32Emu::ROMImage::freeROMImage( pcmRom );
	}
	delete controlRomFile;
	delete pcmRomFile;
	_synth = 0;
	controlRom = 0;
	pcmRom = 0;
	controlRomFile = 0;
	pcmRomFile = 0;
}

static const char * control_rom_names[] = { "CM32L_CONTROL.ROM", "MT32_CONTROL.ROM" };
static const char * pcm_rom_names[] = { "CM32L_PCM.ROM", "MT32_PCM.ROM" };

bool MT32Player::startup()
{
	if (_synth) return true;

	unsigned rom_set = 0;
	controlRomFile = openFile( control_rom_names[ 0 ] );
	if ( !controlRomFile )
	{
		rom_set = 1;
		controlRomFile = openFile( control_rom_names[ 1 ] );
	}
	if ( !controlRomFile ) return false;
	pcmRomFile = openFile( pcm_rom_names[ rom_set ] );
	if ( !pcmRomFile ) return false;
	controlRom = MT32Emu::ROMImage::makeROMImage( controlRomFile );
	if ( !controlRom ) return false;
	pcmRom = MT32Emu::ROMImage::makeROMImage( pcmRomFile );
	if ( !pcmRom ) return false;
	_synth = new MT32Emu::Synth;
	if ( !_synth->open( *controlRom, *pcmRom, bGM, true ) )
	{
		delete _synth;
		_synth = 0;
		return false;
	}
	reset();
	return true;
}

void MT32Player::reset()
{
	static const uint8_t mt32_reset[10] = {0xF0, MT32Emu::SYSEX_MANUFACTURER_ROLAND, 0x10, MT32Emu::SYSEX_MDL_MT32, MT32Emu::SYSEX_CMD_DT1, 0x7F, 0, 0, 0xF7};

	_synth->playSysex( mt32_reset, sizeof( mt32_reset ) );

	if ( bGM )
	{
#include "mtgm.h"
#include "kq6mtgm.h"
		const unsigned char * start, * end;
		if ( uGMSet == 0 )
		{
			start = mt32_gm_sysex;
			end = start + _countof( mt32_gm_sysex );
		}
		else
		{
			start = kq6_mt32_gm_sysex;
			end = start + _countof( kq6_mt32_gm_sysex );
		}
		while ( start < end )
		{
			const unsigned char * sequence_end = start;
			while ( sequence_end < end && *sequence_end != 0xF7 ) sequence_end++;
			_synth->playSysex( start, sequence_end - start + 1 );
			start = sequence_end + 1;
		}
	}
}

class FBFile : public MT32Emu::File
{
public:
	FBFile()
	{
		data = NULL;
	}
	~FBFile()
	{
		close();
	}

	bool open( const char *filename, abort_callback & p_abort )
	{
		try
		{
			service_ptr_t< file > p_temp;
			filesystem::g_open( p_temp, filename, filesystem::open_mode_read, p_abort );
			t_filesize length64 = p_temp->get_size_ex( p_abort );
			if ( length64 > INT_MAX ) length64 = INT_MAX;
			fileSize = length64;
			data = new unsigned char[ length64 ];
			p_temp->read_object( data, fileSize, p_abort );
			return true;
		}
		catch ( ... )
		{
			return false;
		}
	}

	void close()
	{
		delete [] data;
		data = NULL;
	}

	size_t getSize() { return fileSize; }
	const unsigned char *getData() { return data; }
};

MT32Emu::File * MT32Player::openFile( const char *filename )
{
	FBFile * ret = new FBFile;
	if ( ! ret->open( pfc::string8() << sBasePath << filename, *_abort ) )
	{
		delete ret;
		ret = 0;
	}
	return ret;
}
