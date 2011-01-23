#include "SFPlayer.h"

SFPlayer::SFPlayer()
{
	_synth = 0;
	uSamplesRemaining = 0;
	pStream = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;
	uInterpolationMethod = FLUID_INTERP_DEFAULT;

	reset_drums();

	_settings = new_fluid_settings();

	fluid_settings_setnum(_settings, "synth.gain", 1.0);
	fluid_settings_setnum(_settings, "synth.sample-rate", 44100);
}

SFPlayer::~SFPlayer()
{
	if (_synth) delete_fluid_synth(_synth);
	if (_settings) delete_fluid_settings(_settings);

	if (pStream)
	{
		free(pStream);
	}
}

void SFPlayer::setSampleRate(unsigned rate)
{
	if (pStream)
	{
		for (UINT i = 0; i < uStreamSize; i++)
		{
			pStream[i].tm = MulDiv(pStream[i].tm, rate, uSampleRate);
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

	fluid_settings_setnum(_settings, "synth.sample-rate", uSampleRate);

	shutdown();
}

void SFPlayer::setInterpolationMethod(unsigned method)
{
	uInterpolationMethod = method;
	if ( _synth ) fluid_synth_set_interp_method( _synth, -1, method );
}

bool SFPlayer::Load(MIDI_file * mf, unsigned loop_mode, unsigned clean_flags)
{
	assert(!pStream);

	pSysexMap = mf->smap;
	pStream = do_table(mf, 1, &uStreamSize, clean_flags);

	if (pStream && uStreamSize)
	{
		uStreamPosition = 0;
		uTimeCurrent = 0;

		uLoopMode = loop_mode;

		if (uLoopMode & loop_mode_enable)
		{
			uTimeEnd = mf->len;
			uStreamLoopStart = ~0;

			if (uLoopMode & loop_mode_xmi)
			{
				UINT i;

				for (i = 0; i < uStreamSize; i++)
				{
					MIDI_EVENT & e = pStream[i];
					switch (e.ev & 0xFF00FFF0)
					{
					case 0x74B0:
						if (uStreamLoopStart == ~0)
						{
							//uStreamLoopStart = i;
							UINT j;
							uTimeLoopStart = pStream[i].tm;
							for (j = i - 1; j != ~0; --j)
							{
								if (pStream[j].tm < uTimeLoopStart) break;
							}
							uStreamLoopStart = j + 1;
							uLoopMode |= loop_mode_force;
						}
						break;

					case 0x75B0:
						uTimeEnd = e.tm;
						uLoopMode |= loop_mode_force;
						break;
					}
				}
			}

			if (uLoopMode & loop_mode_marker &&
				mf->mmap->pos)
			{
				CMarkerMap * mmap = mf->mmap->Translate(mf);
				if (mmap)
				{
					UINT i, j;
					for (i = 0; i < mmap->pos; i++)
					{
						SYSEX_ENTRY & e = mmap->events[i];
						if (e.len == 9 && !memcmp(mmap->data + e.ofs, "loopStart", 9))
						{
							if (uStreamLoopStart == ~0)
							{
								uTimeLoopStart = e.pos;
								for (j = 0; j < uStreamSize; j++)
								{
									if (pStream[j].tm >= uTimeLoopStart) break;
								}
								uStreamLoopStart = j;
								uLoopMode |= loop_mode_force;
							}
						}
						else if (e.len == 7 && !memcmp(mmap->data + e.ofs, "loopEnd", 7))
						{
							uTimeEnd = e.pos;
							uLoopMode |= loop_mode_force;
						}
					}

					delete mmap;
				}
			}

			if (!(uLoopMode & loop_mode_force)) uTimeEnd += 1000;
			else
			{
				UINT i;
				unsigned char note_on[128 * 16];
				memset( note_on, 0, sizeof( note_on ) );
				for (i = 0; i < uStreamSize; i++)
				{
					UINT ev = pStream[i].ev & 0xFF0000F0;
					if ( ev == 0x90 || ev == 0x80 )
					{
						UINT ch = pStream[i].ev & 0x0F;
						UINT note = ( pStream[i].ev >> 8 ) & 0x7F;
						UINT on = ( ev == 0x90 ) && ( pStream[i].ev & 0xFF0000 );
						note_on [ ch * 128 + note ] = on;
					}
					if (pStream[i].tm > uTimeEnd) break;
				}
				UINT note_off_count = 0;
				for ( UINT j = 0; j < 128 * 16; j++ )
				{
					if ( note_on[ j ] ) note_off_count++;
				}
				if ( note_off_count > uStreamSize - i )
				{
					pStream = ( MIDI_EVENT * ) realloc( pStream, ( i + note_off_count ) * sizeof( MIDI_EVENT ) );
				}
				for ( UINT j = 0; j < 128 * 16; j++ )
				{
					if ( note_on[ j ] )
					{
						pStream[i].tm = uTimeEnd;
						pStream[i].ev = 0x80 + ( j >> 8 ) + ( j & 0x7F ) * 0x100;
						i++;
					}
				}
				uStreamSize = i;
			}
		}
		else uTimeEnd = mf->len + 1000;

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

unsigned SFPlayer::Play(audio_sample * out, unsigned count)
{
	assert(pStream);

	if (!_synth && !startup()) return 0;

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

		while (stream_end < uStreamSize && pStream[stream_end].tm < time_target) stream_end++;

		if (stream_end > uStreamPosition)
		{
			for (; uStreamPosition < stream_end; uStreamPosition++)
			{
				MIDI_EVENT * me = pStream + uStreamPosition;
				
				DWORD samples_todo = me->tm - uTimeCurrent;
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
						uTimeCurrent = me->tm;
						return done;
					}
				}

				send_event( me->ev );

				uTimeCurrent = me->tm;
			}
		}

		if ( done < count )
		{
			DWORD samples_todo;
			if ( uStreamPosition < uStreamSize ) samples_todo = pStream[uStreamPosition].tm;
			else samples_todo = uTimeEnd;
			samples_todo -= uTimeCurrent;
			if ( samples_todo > count - done ) samples_todo = count - done;
			render( out + done * 2, samples_todo );
			done += samples_todo;
		}

		uTimeCurrent = time_target;

		if (uTimeCurrent >= uTimeEnd)
		{
			if ( uStreamPosition < uStreamSize )
			{
				for (; uStreamPosition < uStreamSize; uStreamPosition++)
				{
					MIDI_EVENT * me = pStream + uStreamPosition;
					send_event( me->ev );
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

void SFPlayer::Seek(unsigned sample)
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

	if (!_synth && !startup()) return;

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		reset_drums();

		fluid_synth_system_reset(_synth);

		fluid_synth_set_interp_method( _synth, -1, uInterpolationMethod );
	}

	uTimeCurrent = sample;

	pfc::array_t<MIDI_EVENT> filler;

	UINT stream_start = uStreamPosition;

	for (; uStreamPosition < uStreamSize && pStream[uStreamPosition].tm < uTimeCurrent; uStreamPosition++);

	uSamplesRemaining = pStream[uStreamPosition].tm - uTimeCurrent;

	if (uStreamPosition > stream_start)
	{
		filler.set_size( uStreamPosition - stream_start );
		memcpy(filler.get_ptr(), &pStream[stream_start], sizeof(MIDI_EVENT) * (uStreamPosition - stream_start));

		UINT i, j;
		MIDI_EVENT * me = filler.get_ptr();

		for (i = 0, stream_start = uStreamPosition - stream_start; i < stream_start; i++)
		{
			MIDI_EVENT & e = me[i];
			if ((e.ev & 0xFF0000F0) == 0x90) // note on
			{
				if ((e.ev & 0x0F) == 9) // hax
				{
					e.ev = 0;
					continue;
				}
				DWORD m = (e.ev & 0xFF0F) | 0x80; // note off
				for (j = i + 1; j < stream_start; j++)
				{
					MIDI_EVENT & e2 = me[j];
					if ((e2.ev & 0xFF00FFFF) == m)
					{
						// kill 'em
						e.ev = 0;
						e2.ev = 0;
						break;
					}
				}
			}
		}

		for (i = 0, j = 0; i < stream_start; i++)
		{
			if (me[i].ev)
			{
				if (i != j) me[j] = me[i];
				j++;
			}
		}

		if (!j) return;

		for (i = 0; i < j; i++)
		{
			send_event( me[i].ev );
		}
	}
}

void SFPlayer::send_event(DWORD b)
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
				if ( bank == 16256 || bank == 15360 )
					drum_channels [chan] = 1;
				else if ( bank == 15488 )
					drum_channels [chan] = 0;
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
		case 0xF0:
			if (chan == 15)
			{
				reset_drums();
				fluid_synth_system_reset(_synth);
				fluid_synth_set_interp_method( _synth, -1, uInterpolationMethod );
			}
			break;
		}
	}
	else
	{
		UINT n = b & 0xffffff;
		SYSEX_ENTRY & sysex = pSysexMap->events[n];
		BYTE * sysex_data = pSysexMap->data + sysex.ofs;
		if ( sysex.len == 11 &&
			sysex_data [0] == 0xF0 && sysex_data [1] == 0x41 && sysex_data [3] == 0x42 &&
			sysex_data [4] == 0x12 && sysex_data [5] == 0x40 && (sysex_data [6] & 0xF0) == 0x10 &&
			sysex_data [10] == 0xF7)
		{
			if (sysex_data [7] == 2)
			{
				// GS MIDI channel to part assign
				gs_part_to_ch [ sysex_data [6] & 15 ] = sysex_data [8];
			}
			else if ( sysex_data [7] == 0x15 )
			{
				// GS part to rhythm allocation
				unsigned int drum_channel = gs_part_to_ch [ sysex_data [6] & 15 ];
				if ( drum_channel < 16 )
				{
					drum_channels [ drum_channel ] = sysex_data [8];
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

void SFPlayer::shutdown()
{
	if (_synth) delete_fluid_synth(_synth);
	_synth = 0;
}

bool SFPlayer::startup()
{
	reset_drums();
	_synth = new_fluid_synth(_settings);
	if (!_synth)
	{
		_last_error = "Out of memory";
		return false;
	}
	fluid_synth_set_interp_method( _synth, -1, uInterpolationMethod );
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
	_last_error.reset();

	return true;
}

void SFPlayer::reset_drums()
{
	static const BYTE part_to_ch[16] = {9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15};

	memset( drum_channels, 0, sizeof( drum_channels ) );
	drum_channels [9] = 1;

	memcpy( gs_part_to_ch, part_to_ch, sizeof( gs_part_to_ch ) );

	memset( channel_banks, 0, sizeof( channel_banks ) );
}

const char * SFPlayer::GetLastError() const
{
	if ( _last_error.length() ) return _last_error;
	return NULL;
}
