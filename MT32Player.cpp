#include "MT32Player.h"

#include "shared.h"

#include "../helpers/file_cached.h"

MT32Player::MT32Player( bool gm )
	: bGM( gm )
{
	_synth = NULL;
	uSamplesRemaining = 0;
	pStream = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;
}

MT32Player::~MT32Player()
{
	if (_synth)
	{
		_synth->close();
		delete _synth;
	}

	if (pStream)
	{
		free(pStream);
	}
}

void MT32Player::setSampleRate(unsigned rate)
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

	shutdown();
}

bool MT32Player::Load(MIDI_file * mf, unsigned loop_mode, unsigned clean_flags)
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

unsigned MT32Player::Play(audio_sample * out, unsigned count)
{
	assert(pStream);

	if ( !_synth )
	{
		if ( !startup() ) return 0;
		if ( bGM ) reset();
	}

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

void MT32Player::Seek(unsigned sample)
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

	if (!_synth)
	{
		if ( !startup() ) return;
		if ( bGM ) reset();
	}

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		reset();
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

void MT32Player::send_event(DWORD b)
{
	if (!(b & 0xFF000000))
	{
		_synth->playMsg( b );
	}
	else
	{
		UINT n = b & 0xffffff;
		SYSEX_ENTRY & sysex = pSysexMap->events[n];
		BYTE * sysex_data = pSysexMap->data + sysex.ofs;
		_synth->playSysex( sysex_data, sysex.len );
	}
}

void MT32Player::render(audio_sample * out, unsigned count)
{
	MT32Emu::Bit16s temp[512];
	while ( count > 0 )
	{
		unsigned todo = count;
		if ( todo > 256 ) todo = 256;
		_synth->render( temp, todo );
		audio_math::convert_from_int16( temp, todo * 2, out, 1. );
		out += todo * 2;
		count -= todo;
	}
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
	_synth = 0;
}

bool MT32Player::startup()
{
	_synth = new MT32Emu::Synth;
	MT32Emu::SynthProperties props = {0};
	props.sampleRate = uSampleRate;
	props.useReverb = true;
	props.useDefaultReverb = true;
	props.userData = this;
	props.printDebug = cb_printDebug;
	props.openFile = cb_openFile;
	if ( !_synth->open( props ) )
	{
		delete _synth;
		_synth = 0;
		return false;
	}
	return true;
}

void MT32Player::reset()
{
	static const BYTE mt32_reset[10] = {0xF0, MT32Emu::SYSEX_MANUFACTURER_ROLAND, 0x10, MT32Emu::SYSEX_MDL_MT32, MT32Emu::SYSEX_CMD_DT1, 0x7F, 0, 0, 0xF7};

	_synth->playSysex( mt32_reset, sizeof( mt32_reset ) );

	if ( bGM )
	{
#include "mtgm.h"
		const unsigned char * start, * end;
		start = mt32_gm_sysex;
		end = start + _countof( mt32_gm_sysex );
		while ( start < end )
		{
			const unsigned char * sequence_end = start;
			while ( sequence_end < end && *sequence_end != 0xF7 ) sequence_end++;
			_synth->playSysex( start, sequence_end - start + 1 );
			start = sequence_end + 1;
		}
	}
}

void MT32Player::cb_printDebug( void *userData, const char *fmt, va_list list )
{
	char temp[1024];
	int count = vsnprintf_s( temp, _countof( temp ) - 1, fmt, list );
	if ( count > 0 ) console::formatter() << "MUNT: " << temp;
}

class FBFile : public MT32Emu::File
{
	service_ptr_t<file>   m_file;
	abort_callback      & m_abort;

public:
	FBFile( abort_callback & p_abort ) : m_abort( p_abort ) { }
	~FBFile() { }

	bool open( const char *filename, OpenMode mode )
	{
		if ( mode != OpenMode_read ) return false;
		try
		{
			service_ptr_t< file > p_temp;
			filesystem::g_open( p_temp, filename, filesystem::open_mode_read, m_abort );
			file_cached::g_create( m_file, p_temp, m_abort, 4096 );
			return true;
		}
		catch ( ... )
		{
			return false;
		}
	}

	void close()
	{
		try
		{
			m_file.release();
		}
		catch ( ... )
		{
		}
	}

	size_t read( void *in, size_t size )
	{
		try
		{
			return m_file->read( in, size, m_abort );
		}
		catch ( ... )
		{
			return 0;
		}
	}

	bool readLine( char *in, size_t size )
	{
		return false;
	}

	bool readBit8u( MT32Emu::Bit8u *in )
	{
		if ( read( in, 1 ) != 1 ) return false;
		else return true;
	}

	size_t write( const void *out, size_t size )
	{
		return 0;
	}

	bool writeBit8u( MT32Emu::Bit8u out )
	{
		return false;
	}

	bool isEOF()
	{
		try
		{
			return m_file->is_eof( m_abort );
		}
		catch ( ... )
		{
			return true;
		}
	}
};

MT32Emu::File * MT32Player::cb_openFile( void *userData, const char *filename, MT32Emu::File::OpenMode mode )
{
	MT32Player * thePlayer = ( MT32Player * ) userData;
	FBFile * ret = new FBFile( *( thePlayer->_abort ) );
	if ( ! ret->open( pfc::string8() << thePlayer->sBasePath << filename, mode ) )
	{
		delete ret;
		ret = 0;
	}
	return ret;
}