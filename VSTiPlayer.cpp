#include "VSTiPlayer.h"

#include <shared.h>

#define BUFFER_SIZE 4096

class VSTiPlayer;

static long __cdecl audioMaster(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
	VSTiPlayer * host = 0;
	if ( effect ) host = (VSTiPlayer *) effect->user;

	switch (opcode)
	{
#ifdef TIME_INFO
	case audioMasterGetTime:
		if (host) return (long) host->getTime();
		break;
#endif

	case audioMasterVersion:
		return 2300;

	case audioMasterCurrentId:
		if (effect) return effect->uniqueID;
		break;

	case audioMasterGetVendorString:
		strncpy((char *)ptr, "Chris Moeller", 64);
		//strncpy((char *)ptr, "YAMAHA", 64);
		break;

	case audioMasterGetProductString:
		strncpy((char *)ptr, "VSTiPlayer", 64);
		//strncpy((char *)ptr, "SOL/SQ01", 64);
		break;

	case audioMasterGetVendorVersion:
		return 1337; // uhuhuhuhu
		//return 0;

	case audioMasterGetLanguage:
		return kVstLangEnglish;

	case audioMasterWillReplaceOrAccumulate:
		return 1;

	case audioMasterNeedIdle:
		if ( host ) host->needIdle();
		return 1;
	}

	return 0;
}

void VSTiPlayer::needIdle()
{
	bNeedIdle = true;
}

#ifdef TIME_INFO
VstTimeInfo * VSTiPlayer::getTime()
{
	return time_info;
}
#endif

VSTiPlayer::VSTiPlayer()
{
	hDll = 0;
	pEffect[0] = 0;
	pEffect[1] = 0;
	uSamplesRemaining = 0;
	uSampleRate = 1000;
	uNumOutputs = 0;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;
	evChain = 0;
	evTail = 0;

	bNeedIdle = false;

#ifdef TIME_INFO
	time_info = new VstTimeInfo;

	time_info->samplePos = 0.0;
	time_info->sampleRate = uSampleRate;
	time_info->nanoSeconds = 0.0;
	time_info->ppqPos = 0.0;
	time_info->tempo = 120.0;
	time_info->barStartPos = 0.0;
	time_info->cycleStartPos = 0.0;
	time_info->cycleEndPos = 0.0;
	time_info->timeSigNumerator = 4;
	time_info->timeSigDenominator = 4;
	time_info->smpteOffset = 0;
	time_info->smpteFrameRate = 1;
	time_info->samplesToNextClock = 0;
	time_info->flags = 0;
#endif
}

VSTiPlayer::~VSTiPlayer()
{
	if (pEffect[1])
	{
		// buffer allocated at start of playback
		if (blState.get_size())
		{
			pEffect[1]->dispatcher(pEffect[1], effStopProcess, 0, 0, 0, 0);
		}

		pEffect[1]->dispatcher(pEffect[1], effClose, 0, 0, 0, 0);
	}

	if (pEffect[0])
	{
		if (blState.get_size())
		{
			pEffect[0]->dispatcher(pEffect[0], effStopProcess, 0, 0, 0, 0);
		}

		pEffect[0]->dispatcher(pEffect[0], effClose, 0, 0, 0, 0);
	}

	freeChain();

	if (hDll)
	{
		FreeLibrary(hDll);
	}

#ifdef TIME_INFO
	delete time_info;
#endif
}

typedef AEffect * (*main_func)(audioMasterCallback audioMaster);

bool VSTiPlayer::LoadVST(const char * path)
{
	hDll = uLoadLibrary(path);

	if (hDll)
	{
		main_func pMain = (main_func) GetProcAddress(hDll, "main");

		if (pMain)
		{
			pEffect[0] = (*pMain)(&audioMaster);

			if (pEffect[0])
			{
				pEffect[0]->user = this;

				uNumOutputs = min( pEffect[0]->numOutputs, 2 );

				pEffect[0]->dispatcher(pEffect[0], effOpen, 0, 0, 0, 0);

				if (pEffect[0]->dispatcher(pEffect[0], effGetPlugCategory, 0, 0, 0, 0) == kPlugCategSynth &&
					pEffect[0]->dispatcher(pEffect[0], effCanDo, 0, 0, "sendVstMidiEvent", 0))
				{
					pEffect[1] = (*pMain)(&audioMaster);
					if (pEffect[1])
					{
						pEffect[1]->user = this;
						pEffect[1]->dispatcher(pEffect[1], effOpen, 0, 0, 0, 0);
						return true;
					}
				}
			}
		}
	}

	return false;
}

void VSTiPlayer::getVendorString(pfc::string_base & out)
{
	char temp[65];
	memset(&temp, 0, sizeof(temp));

	if (pEffect[0])
	{
		pEffect[0]->dispatcher(pEffect[0], effGetVendorString, 0, 0, &temp, 0);
		out = temp;
	}
}

void VSTiPlayer::getProductString(pfc::string_base & out)
{
	char temp[65];
	memset(&temp, 0, sizeof(temp));

	if (pEffect[0])
	{
		pEffect[0]->dispatcher(pEffect[0], effGetProductString, 0, 0, &temp, 0);
		out = temp;
	}
}

long VSTiPlayer::getVendorVersion()
{
	if (pEffect) return pEffect[0]->dispatcher(pEffect[0], effGetVendorVersion, 0, 0, 0, 0);
	else return 0;
}

long VSTiPlayer::getUniqueID()
{
	if (pEffect) return pEffect[0]->uniqueID;
	else return 0;
}

template<typename T>
static void append_be( pfc::array_t<t_uint8> & out, const T & value )
{
	union
	{
		T original;
		t_uint8 raw[sizeof(T)];
	} carriage;
	carriage.original = value;
	for ( unsigned i = 0; i < sizeof(T); ++i )
	{
		out.append_single( carriage.raw[ sizeof(T) - 1 - i ] );
	}
}

template<typename T>
static void retrieve_be( T & out, const t_uint8 * & in, unsigned & size )
{
	if ( size < sizeof(T) ) return;

	size -= sizeof(T);

	union
	{
		T original;
		t_uint8 raw[sizeof(T)];
	} carriage;
	for ( unsigned i = 0; i < sizeof(T); ++i )
	{
		carriage.raw[ sizeof(T) - 1 - i ] = *in++;
	}

	out = carriage.original;
}

void VSTiPlayer::getChunk( pfc::array_t<t_uint8> & out )
{
	out.set_count( 0 );
	if (pEffect)
	{
		append_be( out, pEffect[0]->uniqueID );
		bool type_chunked = !!( pEffect[0]->flags & effFlagsProgramChunks );
		append_be( out, type_chunked );
		if ( !type_chunked )
		{
			append_be( out, pEffect[0]->numParams );
			for ( unsigned i = 0, j = pEffect[0]->numParams; i < j; ++i )
			{
				float parameter = pEffect[0]->getParameter( pEffect[0], i );
				append_be( out, parameter );
			}
		}
		else
		{
			void * chunk;
			long size = pEffect[0]->dispatcher(pEffect[0], effGetChunk, 0, 0, &chunk, 0);
			append_be( out, size );
			out.append_fromptr( ( const t_uint8 * )chunk, size );
		}
	}
}

void VSTiPlayer::setChunk( const void * in, unsigned size )
{
	if ( pEffect && size )
	{
		if ( in != NULL )
		{
			blChunk.set_count( size );
			memcpy( blChunk.get_ptr(), in, size );
		}
		in = ( const void * )( blChunk.get_ptr() );
		size = blChunk.get_count();

		const t_uint8 * inc = ( const t_uint8 * ) in;
		long effect_id;
		retrieve_be( effect_id, inc, size );
		if ( effect_id != pEffect[0]->uniqueID ) return;
		bool type_chunked;
		retrieve_be( type_chunked, inc, size );
		if ( type_chunked != !!( pEffect[0]->flags & effFlagsProgramChunks ) ) return;
		if ( !type_chunked )
		{
			long num_params;
			retrieve_be( num_params, inc, size );
			if ( num_params != pEffect[0]->numParams ) return;
			for ( unsigned i = 0; i < num_params; ++i )
			{
				float parameter;
				retrieve_be( parameter, inc, size );
				pEffect[0]->setParameter( pEffect[0], i, parameter );
				pEffect[1]->setParameter( pEffect[1], i, parameter );
			}
		}
		else
		{
			long chunk_size;
			retrieve_be( chunk_size, inc, size );
			if ( chunk_size > size ) return;
			pEffect[0]->dispatcher( pEffect[0], effSetChunk, 0, chunk_size, ( void * ) inc, 0 );
			pEffect[1]->dispatcher( pEffect[1], effSetChunk, 0, chunk_size, ( void * ) inc, 0 );
		}
	}
}

struct ERect
{
	short top;
	short left;
	short bottom;
	short right;
};

struct MyDLGTEMPLATE: DLGTEMPLATE
{
	WORD ext[3];
	MyDLGTEMPLATE ()
	{ memset (this, 0, sizeof(*this)); };
};

INT_PTR CALLBACK EditorProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	AEffect* effect;
	switch(msg)
	{
	case WM_INITDIALOG :
		{
			SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
			effect = (AEffect*)lParam;
			SetWindowText (hwnd, L"VST Editor");
			SetTimer (hwnd, 1, 20, 0);
			if(effect)
			{
				effect->dispatcher (effect, effEditOpen, 0, 0, hwnd, 0);
				ERect* eRect = 0;
				effect->dispatcher (effect, effEditGetRect, 0, 0, &eRect, 0);
				if(eRect)
				{
					int width = eRect->right - eRect->left;
					int height = eRect->bottom - eRect->top;
					if(width < 50)
						width = 50;
					if(height < 50)
						height = 50;
					RECT wRect;
					SetRect (&wRect, 0, 0, width, height);
					AdjustWindowRectEx (&wRect, GetWindowLong (hwnd, GWL_STYLE), FALSE, GetWindowLong (hwnd, GWL_EXSTYLE));
					width = wRect.right - wRect.left;
					height = wRect.bottom - wRect.top;
					SetWindowPos (hwnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
				}
			}
		}
		break;
	case WM_TIMER :
		effect = (AEffect*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
		if(effect)
			effect->dispatcher (effect, effEditIdle, 0, 0, 0, 0);
		break;
	case WM_CLOSE :
		{
			effect = (AEffect*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
			KillTimer (hwnd, 1);
			if(effect)
			{
				effect->dispatcher (effect, effEditClose, 0, 0, 0, 0);
			}

			EndDialog (hwnd, IDOK);
		}
		break;
	}

	return 0;
}

bool VSTiPlayer::hasEditor() const
{
	if (pEffect[0] && (pEffect[0]->flags & effFlagsHasEditor)) return true;
	return false;
}

void VSTiPlayer::displayEditorModal( HWND hwndParent )
{
	if(pEffect[0] && (pEffect[0]->flags & effFlagsHasEditor))
	{
		MyDLGTEMPLATE t;	
		t.style = WS_POPUPWINDOW|WS_DLGFRAME|DS_MODALFRAME|DS_CENTER;
		DialogBoxIndirectParam (core_api::get_my_instance(), &t, hwndParent, (DLGPROC)EditorProc, (LPARAM)(pEffect[0]));
	}
}

void VSTiPlayer::setSampleRate(unsigned rate)
{
	assert(!blState.get_size());

	if (mStream.get_count())
	{
		for (UINT i = 0; i < mStream.get_count(); i++)
		{
			mStream[ i ].m_timestamp = MulDiv( mStream[ i ].m_timestamp, rate, uSampleRate );
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
#ifdef TIME_INFO
	time_info->sampleRate = rate;
#endif
}

unsigned VSTiPlayer::getChannelCount()
{
	return uNumOutputs;
}

bool VSTiPlayer::Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, bool clean_flag)
{
	assert(!mStream.get_count());

	midi_file.serialize_as_stream( subsong, mStream, mSysexMap, clean_flag );

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

void VSTiPlayer::resizeState(unsigned size)
{
	blState.grow_size( size );

	if ( ( float ** ) blState.get_ptr() != float_list_in )
	{
		long i;

		float_list_in  = (float**) blState.get_ptr();
		float_list_out = float_list_in + pEffect[0]->numInputs;
		float_null     = (float*) (float_list_out + pEffect[0]->numOutputs * 2);
		float_out      = float_null + BUFFER_SIZE;

		for (i = 0; i < pEffect[0]->numInputs; i++)
		{
			float_list_in[i] = float_null;
		}

		for (i = 0; i < pEffect[0]->numOutputs * 2; i++)
		{
			float_list_out[i] = float_out + BUFFER_SIZE * i;
		}

		memset(float_null, 0, sizeof(float) * BUFFER_SIZE);
	}
}

unsigned VSTiPlayer::Play(audio_sample * out, unsigned count)
{
	assert(mStream.get_count());

	if (!blState.get_size())
	{
		pEffect[0]->dispatcher(pEffect[0], effSetSampleRate, 0, 0, 0, float(uSampleRate));
		pEffect[0]->dispatcher(pEffect[0], effSetBlockSize, 0, BUFFER_SIZE, 0, 0);
		pEffect[0]->dispatcher(pEffect[0], effMainsChanged, 0, 1, 0, 0);
		pEffect[0]->dispatcher(pEffect[0], effStartProcess, 0, 0, 0, 0);

		pEffect[1]->dispatcher(pEffect[1], effSetSampleRate, 0, 0, 0, float(uSampleRate));
		pEffect[1]->dispatcher(pEffect[1], effSetBlockSize, 0, BUFFER_SIZE, 0, 0);
		pEffect[1]->dispatcher(pEffect[1], effMainsChanged, 0, 1, 0, 0);
		pEffect[1]->dispatcher(pEffect[1], effStartProcess, 0, 0, 0, 0);

		buffer_size = sizeof(float*) * (pEffect[0]->numInputs + pEffect[0]->numOutputs * 2); // float lists
		buffer_size += sizeof(float) * BUFFER_SIZE;                                // null input
		buffer_size += sizeof(float) * BUFFER_SIZE * pEffect[0]->numOutputs * 2;          // outputs

		float_list_in = 0;
		resizeState(buffer_size);
	}

	if ( bNeedIdle )
	{
		bNeedIdle = pEffect[0]->dispatcher(pEffect[0], effIdle, 0, 0, 0, 0) || pEffect[1]->dispatcher(pEffect[1], effIdle, 0, 0, 0, 0);
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

void VSTiPlayer::Seek(unsigned sample)
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

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		if (blState.get_size())
		{
			// total shutdown
			pEffect[ 0 ]->dispatcher(pEffect[ 0 ], effStopProcess, 0, 0, 0, 0);
			pEffect[ 0 ]->dispatcher(pEffect[ 0 ], effClose, 0, 0, 0, 0);

			pEffect[ 1 ]->dispatcher(pEffect[ 1 ], effStopProcess, 0, 0, 0, 0);
			pEffect[ 1 ]->dispatcher(pEffect[ 1 ], effClose, 0, 0, 0, 0);

			blState.set_size(0);

			freeChain();

			main_func pMain = (main_func) GetProcAddress(hDll, "main");

			pEffect[ 0 ] = (*pMain)(&audioMaster);
			pEffect[ 1 ] = (*pMain)(&audioMaster);

			if (!pEffect[ 0 ] || !pEffect[ 1 ])
			{
				return;
			}

			pEffect[ 0 ]->user = this;
			pEffect[ 1 ]->user = this;

			pEffect[ 0 ]->dispatcher(pEffect[ 0 ], effOpen, 0, 0, 0, 0);
			pEffect[ 1 ]->dispatcher(pEffect[ 1 ], effOpen, 0, 0, 0, 0);

			if ( blChunk.get_size() )
				setChunk( NULL, 0 );
		}
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

		for ( i = 0; i < j; i++ )
		{
			send_event( me[i].m_event );
		}
	}
}

void VSTiPlayer::freeChain()
{
	myVstEvent * ev = evChain;
	while ( ev )
	{
		myVstEvent * next = ev->next;
		free( ev );
		ev = next;
	}
	evChain = NULL;
	evTail = NULL;
}

void VSTiPlayer::send_event( DWORD b )
{
	myVstEvent * ev = ( myVstEvent * ) calloc( sizeof( myVstEvent ), 1 );
	if ( evTail ) evTail->next = ev;
	evTail = ev;
	if ( !evChain ) evChain = ev;
	if (!(b & 0x80000000))
	{
		ev->port = (b & 0x7F000000) ? 1 : 0;
		ev->ev.midiEvent.type = kVstMidiType;
		ev->ev.midiEvent.byteSize = sizeof(ev->ev.midiEvent);
		memcpy(&ev->ev.midiEvent.midiData, &b, 3);
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size;
		mSysexMap.get_entry( n, data, size );
		ev->ev.sysexEvent.type = kVstSysExType;
		ev->ev.sysexEvent.byteSize = sizeof(ev->ev.sysexEvent);
		ev->ev.sysexEvent.dumpBytes = size;
		ev->ev.sysexEvent.sysexDump = (char *)data;

		evTail = ( myVstEvent * ) calloc( sizeof( myVstEvent ), 1 );
		ev->next = evTail;
		evTail->port = 1;
		evTail->ev.sysexEvent = ev->ev.sysexEvent;
	}
}

void VSTiPlayer::render( audio_sample * out, unsigned count )
{
	VstEvents * events[2] = {0};

	if ( evChain )
	{
		unsigned event_count[2] = {0};
		myVstEvent * ev = evChain;
		while ( ev )
		{
			event_count[ ev->port ]++;
			ev = ev->next;
		}

		if ( event_count[ 0 ] )
		{
			events[ 0 ] = ( VstEvents * ) malloc( sizeof(long) + sizeof(long) + sizeof(VstEvent*) * event_count[ 0 ] );

			events[ 0 ]->numEvents = event_count[ 0 ];
			events[ 0 ]->reserved = 0;

			ev = evChain;

			for ( unsigned i = 0; ev; )
			{
				if ( !ev->port ) events[ 0 ]->events[ i++ ] = (VstEvent*) &ev->ev;
				ev = ev->next;
			}

			pEffect[ 0 ]->dispatcher(pEffect[ 0 ], effProcessEvents, 0, 0, events[ 0 ], 0);
		}

		if ( event_count[ 1 ] )
		{
			events[ 1 ] = ( VstEvents * ) malloc( sizeof(long) + sizeof(long) + sizeof(VstEvent*) * event_count[ 1 ] );

			events[ 1 ]->numEvents = event_count[ 1 ];
			events[ 1 ]->reserved = 0;

			ev = evChain;

			for ( unsigned i = 0; ev; )
			{
				if ( ev->port ) events[ 1 ]->events[ i++ ] = (VstEvent*) &ev->ev;
				ev = ev->next;
			}

			pEffect[ 1 ]->dispatcher(pEffect[ 1 ], effProcessEvents, 0, 0, events[ 1 ], 0);
		}
	}

	while ( count )
	{
		unsigned count_to_do = min( count, BUFFER_SIZE );
		unsigned num_outputs = pEffect[ 0 ]->numOutputs;

		pEffect[ 0 ]->processReplacing( pEffect[ 0 ], float_list_in, float_list_out, count_to_do );
		pEffect[ 1 ]->processReplacing( pEffect[ 1 ], float_list_in, float_list_out + num_outputs, count_to_do );

		if ( uNumOutputs == 2 )
		{
			for (unsigned i = 0; i < count_to_do; i++)
			{
				audio_sample sample = ( float_out[ i ] + float_out[ i + BUFFER_SIZE * num_outputs ] );
				out[0] = sample;
				sample = ( float_out[ i + BUFFER_SIZE ] + float_out[ i + BUFFER_SIZE + BUFFER_SIZE * num_outputs ] );
				out[1] = sample;
				out += 2;
			}
		}
		else
		{
			for (unsigned i = 0; i < count_to_do; i++)
			{
				audio_sample sample = ( float_out[ i ] + float_out[ i + BUFFER_SIZE * num_outputs ] );
				out[0] = sample;
				out++;
			}
		}

		count -= count_to_do;
	}

	if ( events[ 0 ] ) free( events[ 0 ] );
	if ( events[ 1 ] ) free( events[ 1 ] );

	freeChain();
}
