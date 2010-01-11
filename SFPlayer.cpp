#include "SFPlayer.h"

SFPlayer::SFPlayer()
{
	_synth = 0;
	_soundFont = -1;
	uSamplesRemaining = 0;
	pStream = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;

	_settings = new_fluid_settings();

	fluid_settings_setnum(_settings, "synth.gain", 1.0);
	fluid_settings_setnum(_settings, "synth.sample-rate", 44100);
}

SFPlayer::~SFPlayer()
{
	if (_soundFont != -1)
		fluid_synth_sfunload(_synth, _soundFont, 1);

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

	if (!_synth) startup();

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

	if (!_synth) startup();

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		fluid_synth_system_reset(_synth);
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
			break;
		case 0xC0:
			fluid_synth_program_change(_synth, chan, param1);
			break;
		case 0xD0:
			fluid_synth_channel_pressure(_synth, chan, param1);
			break;
		case 0xE0:
			fluid_synth_pitch_bend(_synth, chan, (param2 << 7) | param1);
			break;
		case 0xF0:
			if (chan == 15) fluid_synth_system_reset(_synth);
			break;
		}
	}
	else
	{
		// don't know what to do with sysex events yet
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
	if (_soundFont != -1) fluid_synth_sfunload(_synth, _soundFont, 1);
	_soundFont = -1;
	if (_synth) delete_fluid_synth(_synth);
	_synth = 0;
}

void SFPlayer::startup()
{
	_synth = new_fluid_synth(_settings);
	if (sSoundFontName.length()) _soundFont = fluid_synth_sfload(_synth, pfc::stringcvt::string_wide_from_utf8( sSoundFontName ), 1);
}