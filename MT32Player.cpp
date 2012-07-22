#include "MT32Player.h"

#include "shared.h"

MT32Player::MT32Player( bool gm, bool debug_info )
	: bGM( gm ), bDebug( debug_info )
{
	_synth = NULL;
	uSamplesRemaining = 0;
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
}

void MT32Player::setSampleRate(unsigned rate)
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

bool MT32Player::Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags)
{
	assert(!mStream.get_count());

	midi_file.serialize_as_stream( subsong, mStream, mSysexMap, clean_flags );

	if (mStream.get_count())
	{
		uStreamPosition = 0;
		uTimeCurrent = 0;

		uLoopMode = loop_mode;

		if (uLoopMode & loop_mode_enable)
		{
			uTimeLoopStart = midi_file.get_timestamp_loop_start( subsong, true );
			unsigned uTimeLoopEnd = midi_file.get_timestamp_loop_end( subsong, true );
			uTimeEnd = midi_file.get_timestamp_end( subsong, true );

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
					UINT ev = mStream[ i ].m_event & 0x800000F0;
					if ( ev == 0x90 || ev == 0x80 )
					{
						UINT port = ( mStream[ i ].m_event & 0x7F000000 ) >> 24;
						UINT ch = mStream[ i ].m_event & 0x0F;
						UINT note = ( mStream[ i ].m_event >> 8 ) & 0x7F;
						UINT on = ( ev == 0x90 ) && ( mStream[ i ].m_event & 0xFF0000 );
						UINT bit = 1 << port;
						note_on [ ch * 128 + note ] = ( note_on [ ch * 128 + note ] & ~bit ) | ( bit * on );
					}
				}
				mStream.set_count( i );
				for ( UINT j = 0; j < 128 * 16; j++ )
				{
					if ( note_on[ j ] )
					{
						for ( UINT k = 0; k < 8; k++ )
						{
							if ( note_on[ j ] & ( 1 << k ) )
							{
								mStream.append_single( midi_stream_event( uTimeEnd, ( k << 24 ) + ( j >> 7 ) + ( j & 0x7F ) * 0x100 + 0x90 ) );
							}
						}
					}
				}
			}
		}
		else uTimeEnd = midi_file.get_timestamp_end( subsong, true ) + 1000;

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
	assert(mStream.get_count());

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
			if ((e.m_event & 0x800000F0) == 0x90 && (e.m_event & 0xFF0000)) // note on
			{
				if ((e.m_event & 0x0F) == 9) // hax
				{
					e.m_event = 0;
					continue;
				}
				DWORD m = (e.m_event & 0x7F00FF0F) | 0x80; // note off
				DWORD m2 = e.m_event & 0x7F00FFFF; // also note off
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

void MT32Player::send_event(DWORD b)
{
	if (!(b & 0x80000000))
	{
		_synth->playMsg( b );
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size;
		mSysexMap.get_entry( n, data, size );
		_synth->playSysex( data, size );
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
	MT32Player * thePlayer = ( MT32Player * ) userData;
	if ( thePlayer->bDebug )
	{
		char temp[1024];
		int count = vsnprintf_s( temp, _countof( temp ) - 1, fmt, list );
		if ( count > 0 ) console::formatter() << "MUNT: " << temp;
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
	unsigned char *getData() { return data; }
};

MT32Emu::File * MT32Player::cb_openFile( void *userData, const char *filename )
{
	MT32Player * thePlayer = ( MT32Player * ) userData;
	FBFile * ret = new FBFile;
	if ( ! ret->open( pfc::string8() << thePlayer->sBasePath << filename, *( thePlayer->_abort ) ) )
	{
		delete ret;
		ret = 0;
	}
	return ret;
}
