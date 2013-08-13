#include "SFPlayer.h"

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

SFPlayer::SFPlayer() : MIDIPlayer()
{
	_synth = 0;
	uInterpolationMethod = FLUID_INTERP_DEFAULT;

	reset_drums();

	synth_mode = mode_gm;

	_settings = new_fluid_settings();

	fluid_settings_setnum(_settings, "synth.gain", 1.0);
	fluid_settings_setnum(_settings, "synth.sample-rate", 44100);
	fluid_settings_setint(_settings, "synth.midi-channels", 32);
}

SFPlayer::~SFPlayer()
{
	if (_synth) delete_fluid_synth(_synth);
	if (_settings) delete_fluid_settings(_settings);
}

void SFPlayer::setInterpolationMethod(unsigned method)
{
	uInterpolationMethod = method;
	if ( _synth ) fluid_synth_set_interp_method( _synth, -1, method );
}

void SFPlayer::send_event(DWORD b)
{
	if (!(b & 0x80000000))
	{
		int param2 = (b >> 16) & 0xFF;
		int param1 = (b >> 8) & 0xFF;
		int cmd = b & 0xF0;
		int chan = b & 0x0F;
		int port = (b >> 24) & 0x7F;

		if ( port & 1 ) chan += 16;

		switch (cmd)
		{
		case 0x80:
			fluid_synth_noteoff(_synth, chan, param1);
			break;
		case 0x90:
			fluid_synth_noteon(_synth, chan, param1, param2);
			break;
		case 0xA0:
			break;
		case 0xB0:
			fluid_synth_cc(_synth, chan, param1, param2);
			if ( param1 == 0 || param1 == 0x20 )
			{
				unsigned bank = channel_banks[ chan ];
				if ( param1 == 0x20 ) bank = ( bank & 0x3F80 ) | ( param2 & 0x7F );
				else bank = ( bank & 0x007F ) | ( ( param2 & 0x7F ) << 7 );
				channel_banks[ chan ] = bank;
				if ( synth_mode == mode_xg )
				{
					if ( bank == 16256 ) drum_channels [chan] = 1;
					else drum_channels [chan] = 0;
				}
				else if ( synth_mode == mode_gm2 )
				{
					if ( bank == 15360 )
						drum_channels [chan] = 1;
					else if ( bank == 15488 )
						drum_channels [chan] = 0;
				}
			}
			break;
		case 0xC0:
			if ( drum_channels [chan] ) fluid_synth_bank_select(_synth, chan, 16256 /*DRUM_INST_BANK*/);
			fluid_synth_program_change(_synth, chan, param1);
			break;
		case 0xD0:
			fluid_synth_channel_pressure(_synth, chan, param1);
			break;
		case 0xE0:
			fluid_synth_pitch_bend(_synth, chan, (param2 << 7) | param1);
			break;
		}
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size;
		mSysexMap.get_entry( n, data, size );
		if ( ( size == _countof( sysex_gm_reset ) && !memcmp( data, sysex_gm_reset, _countof( sysex_gm_reset ) ) ) ||
			( size == _countof( sysex_gm2_reset ) && !memcmp( data, sysex_gm2_reset, _countof( sysex_gm2_reset ) ) ) ||
			is_gs_reset( data, size ) ||
			( size == _countof( sysex_xg_reset ) && !memcmp( data, sysex_xg_reset, _countof( sysex_xg_reset ) ) ) )
		{
			fluid_synth_system_reset( _synth );
			reset_drums();
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
				gs_part_to_ch [ data [6] & 15 ] = data [8];
			}
			else if ( data [7] == 0x15 )
			{
				// GS part to rhythm allocation
				unsigned int drum_channel = gs_part_to_ch [ data [6] & 15 ];
				if ( drum_channel < 16 )
				{
					drum_channels [ drum_channel ] = data [8];
					drum_channels [ 16 + drum_channel ] = data [8];
				}
			}
		}
	}
}

void SFPlayer::render(audio_sample * out, unsigned count)
{
	fluid_synth_write_float(_synth, count, out, 0, 2, out, 1, 2);
}

void SFPlayer::setSoundFont( const char * in )
{
	sSoundFontName = in;
	shutdown();
}

void SFPlayer::setFileSoundFont( const char * in )
{
	sFileSoundFontName = in;
	shutdown();
}

void SFPlayer::shutdown()
{
	if (_synth) delete_fluid_synth(_synth);
	_synth = 0;
}

bool SFPlayer::startup()
{
	if ( _synth ) return true;

	fluid_settings_setnum(_settings, "synth.sample-rate", uSampleRate);

	_synth = new_fluid_synth(_settings);
	if (!_synth)
	{
		_last_error = "Out of memory";
		return false;
	}
	fluid_synth_set_interp_method( _synth, -1, uInterpolationMethod );
	reset_drums();
	synth_mode = mode_gm;
	if (sSoundFontName.length())
	{
		pfc::string_extension ext(sSoundFontName);
		if ( !pfc::stricmp_ascii( ext, "sf2" ) )
		{
			if ( FLUID_FAILED == fluid_synth_sfload(_synth, pfc::stringcvt::string_wide_from_utf8( sSoundFontName ), 1) )
			{
				shutdown();
				_last_error = "Failed to load SoundFont bank: ";
				_last_error += sSoundFontName;
				return false;
			}
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
					if ( FLUID_FAILED == fluid_synth_sfload( _synth, cr, 1 ) )
					{
						fclose( fl );
						shutdown();
						_last_error = "Failed to load SoundFont bank: ";
						_last_error += pfc::stringcvt::string_utf8_from_os( cr );
						return false;
					}
				}
				fclose( fl );
			}
			else
			{
				_last_error = "Failed to open SoundFont list: ";
				_last_error += sSoundFontName;
				return false;
			}
		}
	}

	if ( sFileSoundFontName.length() )
	{
		if ( FLUID_FAILED == fluid_synth_sfload(_synth, pfc::stringcvt::string_wide_from_utf8( sFileSoundFontName ), 1) )
		{
			shutdown();
			_last_error = "Failed to load SoundFont bank: ";
			_last_error += sFileSoundFontName;
			return false;
		}
	}

	_last_error.reset();

	return true;
}

void SFPlayer::reset_drums()
{
	static const BYTE part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

	memset( drum_channels, 0, sizeof( drum_channels ) );
	drum_channels [9] = 1;
	drum_channels [25] = 1;

	memcpy( gs_part_to_ch, part_to_ch, sizeof( gs_part_to_ch ) );

	memset( channel_banks, 0, sizeof( channel_banks ) );

	if ( _synth )
	{
		for ( unsigned i = 0; i < 32; ++i )
		{
			if ( drum_channels [i] )
			{
				fluid_synth_bank_select( _synth, i, 16256 /*DRUM_INST_BANK*/);
			}
		}
	}
}

const char * SFPlayer::GetLastError() const
{
	if ( _last_error.length() ) return _last_error;
	return NULL;
}
