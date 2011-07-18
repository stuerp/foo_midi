#include "VSTiPlayer.h"

#include <shared.h>

#include "c:\\programming\\vstsdk2.3\\source\\common\\aeffect.h"
#include "c:\\programming\\vstsdk2.3\\source\\common\\aeffectx.h"

struct VstMidiSysexEvent
{
	long type;        // kVstSysexType
	long byteSize;    // 24
	long deltaFrames;
	long flags;       // none defined yet
	long dumpBytes;   // byte size of sysexDump
	long resvd1;      // zero
	char *sysexDump;
	long resvd2;      // zero
};

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
	pEffect = 0;
	uSampleRate = 1000;
	uNumOutputs = 0;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;

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
	if (pEffect)
	{
		// buffer allocated at start of playback
		if (blState.get_size())
		{
			pEffect->dispatcher(pEffect, effStopProcess, 0, 0, 0, 0);
		}

		pEffect->dispatcher(pEffect, effClose, 0, 0, 0, 0);
	}

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
			pEffect = (*pMain)(&audioMaster);

			if (pEffect)
			{
				pEffect->user = this;

				uNumOutputs = min( pEffect->numOutputs, 2 );

				pEffect->dispatcher(pEffect, effOpen, 0, 0, 0, 0);

				if (pEffect->dispatcher(pEffect, effGetPlugCategory, 0, 0, 0, 0) == kPlugCategSynth &&
					pEffect->dispatcher(pEffect, effCanDo, 0, 0, "sendVstMidiEvent", 0))
				{
					return true;
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

	if (pEffect)
	{
		pEffect->dispatcher(pEffect, effGetVendorString, 0, 0, &temp, 0);
		out = temp;
	}
}

void VSTiPlayer::getProductString(pfc::string_base & out)
{
	char temp[65];
	memset(&temp, 0, sizeof(temp));

	if (pEffect)
	{
		pEffect->dispatcher(pEffect, effGetProductString, 0, 0, &temp, 0);
		out = temp;
	}
}

long VSTiPlayer::getVendorVersion()
{
	if (pEffect) return pEffect->dispatcher(pEffect, effGetVendorVersion, 0, 0, 0, 0);
	else return 0;
}

long VSTiPlayer::getUniqueID()
{
	if (pEffect) return pEffect->uniqueID;
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
		append_be( out, pEffect->uniqueID );
		bool type_chunked = !!( pEffect->flags & effFlagsProgramChunks );
		append_be( out, type_chunked );
		if ( !type_chunked )
		{
			append_be( out, pEffect->numParams );
			for ( unsigned i = 0, j = pEffect->numParams; i < j; ++i )
			{
				float parameter = pEffect->getParameter( pEffect, i );
				append_be( out, parameter );
			}
		}
		else
		{
			void * chunk;
			long size = pEffect->dispatcher(pEffect, effGetChunk, 0, 0, &chunk, 0);
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
		if ( effect_id != pEffect->uniqueID ) return;
		bool type_chunked;
		retrieve_be( type_chunked, inc, size );
		if ( type_chunked != !!( pEffect->flags & effFlagsProgramChunks ) ) return;
		if ( !type_chunked )
		{
			long num_params;
			retrieve_be( num_params, inc, size );
			if ( num_params != pEffect->numParams ) return;
			for ( unsigned i = 0; i < num_params; ++i )
			{
				float parameter;
				retrieve_be( parameter, inc, size );
				pEffect->setParameter( pEffect, i, parameter );
			}
		}
		else
		{
			long chunk_size;
			retrieve_be( chunk_size, inc, size );
			if ( chunk_size > size ) return;
			pEffect->dispatcher( pEffect, effSetChunk, 0, chunk_size, ( void * ) inc, 0 );
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
	if (pEffect && (pEffect->flags & effFlagsHasEditor)) return true;
	return false;
}

void VSTiPlayer::displayEditorModal( HWND hwndParent )
{
	if(pEffect && (pEffect->flags & effFlagsHasEditor))
	{
		MyDLGTEMPLATE t;	
		t.style = WS_POPUPWINDOW|WS_DLGFRAME|DS_MODALFRAME|DS_CENTER;
		DialogBoxIndirectParam (core_api::get_my_instance(), &t, hwndParent, (DLGPROC)EditorProc, (LPARAM)pEffect);
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
		float_list_out = float_list_in + pEffect->numInputs;
		float_null     = (float*) (float_list_out + pEffect->numOutputs);
		float_out      = float_null + BUFFER_SIZE;
		events_list    = (VstEvents*) (float_out + BUFFER_SIZE * pEffect->numOutputs);

		for (i = 0; i < pEffect->numInputs; i++)
		{
			float_list_in[i] = float_null;
		}

		for (i = 0; i < pEffect->numOutputs; i++)
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
		pEffect->dispatcher(pEffect, effSetSampleRate, 0, 0, 0, float(uSampleRate));
		pEffect->dispatcher(pEffect, effSetBlockSize, 0, BUFFER_SIZE, 0, 0);
		pEffect->dispatcher(pEffect, effMainsChanged, 0, 1, 0, 0);
		pEffect->dispatcher(pEffect, effStartProcess, 0, 0, 0, 0);

		buffer_size = sizeof(float*) * (pEffect->numInputs + pEffect->numOutputs); // float lists
		buffer_size += sizeof(float) * BUFFER_SIZE;                                // null input
		buffer_size += sizeof(float) * BUFFER_SIZE * pEffect->numOutputs;          // outputs

		float_list_in = 0;
		resizeState(buffer_size);
	}

	if ( bNeedIdle )
	{
		bNeedIdle = !! pEffect->dispatcher(pEffect, effIdle, 0, 0, 0, 0);
	}

	DWORD done = 0;

	while (done < count)
	{
		DWORD todo = uTimeEnd - uTimeCurrent;
		if (todo > count - done) todo = count - done;
		if (todo > BUFFER_SIZE) todo = BUFFER_SIZE;

		DWORD time_target = todo + uTimeCurrent;
		UINT stream_end = uStreamPosition;

		while (stream_end < mStream.get_count() && mStream[stream_end].m_timestamp < time_target) stream_end++;

		if (stream_end > uStreamPosition)
		{
			unsigned events_size = sizeof(long) * 2 + sizeof(VstEvent*) * (stream_end - uStreamPosition);
			UINT i;

			for (i = uStreamPosition; i < stream_end; i++)
			{
				if (!(mStream[i].m_event & 0x80000000)) events_size += sizeof(VstMidiEvent);
				else events_size += sizeof(VstMidiSysexEvent);
			}

			resizeState(buffer_size + events_size);

			events_list->numEvents = stream_end - uStreamPosition;
			events_list->reserved = 0;

			VstEvent * event = (VstEvent*) (events_list->events + events_list->numEvents);

			for (i = 0; uStreamPosition < stream_end; uStreamPosition++, i++)
			{
				events_list->events[i] = event;

				if (!(mStream[uStreamPosition].m_event & 0x80000000))
				{
					VstMidiEvent * e = (VstMidiEvent*) event;
					memset(e, 0, sizeof(*e));
					e->type = kVstMidiType;
					e->byteSize = sizeof(*e);
					e->deltaFrames = mStream[uStreamPosition].m_timestamp - uTimeCurrent;
					memcpy(&e->midiData, &mStream[uStreamPosition].m_event, 4);
					event = (VstEvent*)(e + 1);
				}
				else
				{
					VstMidiSysexEvent * e = (VstMidiSysexEvent*) event;
					UINT n = mStream[uStreamPosition].m_event & 0xffffff;
					const t_uint8 * data;
					t_size size;
					mSysexMap.get_entry( n, data, size );
					e->type = kVstSysExType;
					e->byteSize = sizeof(*e);
					e->deltaFrames = mStream[uStreamPosition].m_timestamp - uTimeCurrent;
					e->flags = 0;
					e->dumpBytes = size;
					e->resvd1 = 0;
					e->sysexDump = (char *)data;
					e->resvd2 = 0;
					event = (VstEvent*)(e + 1);
				}
			}

			pEffect->dispatcher(pEffect, effProcessEvents, 0, 0, events_list, 0);
		}

		uTimeCurrent = time_target;

		memset(float_out, 0, sizeof(float) * BUFFER_SIZE * pEffect->numOutputs);

		if (pEffect->flags & effFlagsCanReplacing)
			pEffect->processReplacing(pEffect, float_list_in, float_list_out, todo);
		else
			pEffect->process(pEffect, float_list_in, float_list_out, todo);

		for (UINT i = 0; i < todo; i++)
		{
			for (UINT j = 0; j < uNumOutputs; j++)
			{
				out[j] = audio_sample(float_out[i + j * BUFFER_SIZE]);
			}
			out += uNumOutputs;
		}

		done += todo;

		if (uTimeCurrent >= uTimeEnd)
		{
			if ( uStreamPosition < mStream.get_count() )
			{
				unsigned events_size = sizeof(long) * 2 + sizeof(VstEvent*) * (mStream.get_count() - uStreamPosition);
				UINT i;

				for (i = uStreamPosition; i < mStream.get_count(); i++)
				{
					if (!(mStream[i].m_event & 0x80000000)) events_size += sizeof(VstMidiEvent);
					else events_size += sizeof(VstMidiSysexEvent);
				}

				resizeState(buffer_size + events_size);

				events_list->numEvents = mStream.get_count() - uStreamPosition;
				events_list->reserved = 0;

				VstEvent * event = (VstEvent*) (events_list->events + events_list->numEvents);

				for (i = 0; uStreamPosition < mStream.get_count(); uStreamPosition++, i++)
				{
					events_list->events[i] = event;

					if (!(mStream[uStreamPosition].m_event & 0x80000000))
					{
						VstMidiEvent * e = (VstMidiEvent*) event;
						memset(e, 0, sizeof(*e));
						e->type = kVstMidiType;
						e->byteSize = sizeof(*e);
						e->deltaFrames = 0;
						memcpy(&e->midiData, &mStream[uStreamPosition].m_event, 4);
						event = (VstEvent*)(e + 1);
					}
					else
					{
						VstMidiSysexEvent * e = (VstMidiSysexEvent*) event;
						UINT n = mStream[uStreamPosition].m_event & 0xffffff;
						const t_uint8 * data;
						t_size size;
						mSysexMap.get_entry( n, data, size );
						e->type = kVstSysExType;
						e->byteSize = sizeof(*e);
						e->deltaFrames = 0;
						e->flags = 0;
						e->dumpBytes = size;
						e->resvd1 = 0;
						e->sysexDump = (char *)data;
						e->resvd2 = 0;
						event = (VstEvent*)(e + 1);
					}
				}

				pEffect->dispatcher(pEffect, effProcessEvents, 0, 0, events_list, 0);

				if (pEffect->flags & effFlagsCanReplacing) pEffect->processReplacing(pEffect, float_list_in, float_list_out, 1);
				else pEffect->process(pEffect, float_list_in, float_list_out, 1);
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
			pEffect->dispatcher(pEffect, effStopProcess, 0, 0, 0, 0);
			pEffect->dispatcher(pEffect, effClose, 0, 0, 0, 0);

			blState.set_size(0);

			main_func pMain = (main_func) GetProcAddress(hDll, "main");

			pEffect = (*pMain)(&audioMaster);

			if (!pEffect)
			{
				return;
			}

			pEffect->user = this;

			pEffect->dispatcher(pEffect, effOpen, 0, 0, 0, 0);

			if ( blChunk.get_size() )
				setChunk( NULL, 0 );
		}
	}

	if (!blState.get_size())
	{
		Play(0, 0);
	}

	uTimeCurrent = sample;

	pfc::array_t<midi_stream_event> filler;

	UINT stream_start = uStreamPosition;

	for (; uStreamPosition < mStream.get_count() && mStream[uStreamPosition].m_timestamp < uTimeCurrent; uStreamPosition++);

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

		unsigned events_size = sizeof(long) * 2 + sizeof(VstEvent*) * j;

		for (i = 0; i < j; i++)
		{
			if (!(me[i].m_event & 0x80000000)) events_size += sizeof(VstMidiEvent);
			else events_size += sizeof(VstMidiSysexEvent);
		}

		pfc::array_t< t_uint8 > temp;
		temp.set_size( events_size );
		VstEvents * my_events_list = (VstEvents*) temp.get_ptr();

		my_events_list->numEvents = j;
		my_events_list->reserved = 0;

		VstEvent * event = (VstEvent*) (my_events_list->events + my_events_list->numEvents);

		for (i = 0; i < j; i++)
		{
			my_events_list->events[i] = event;

			if (!(me[i].m_event & 0x80000000))
			{
				VstMidiEvent * e = (VstMidiEvent*) event;
				memset(e, 0, sizeof(*e));
				e->type = kVstMidiType;
				e->byteSize = sizeof(*e);
				e->deltaFrames = 0;
				memcpy(&e->midiData, &me[i].m_event, 4);
				event = (VstEvent*)(e + 1);
			}
			else
			{
				VstMidiSysexEvent * e = (VstMidiSysexEvent*) event;
				UINT n = me[i].m_event & 0xffffff;
				const t_uint8 * data;
				t_size size;
				mSysexMap.get_entry( n, data, size );
				e->type = kVstSysExType;
				e->byteSize = sizeof(*e);
				e->deltaFrames = 0;
				e->flags = 0;
				e->dumpBytes = size;
				e->resvd1 = 0;
				e->sysexDump = (char *)data;
				e->resvd2 = 0;
				event = (VstEvent*)(e + 1);
			}
		}

		pEffect->dispatcher(pEffect, effProcessEvents, 0, 0, my_events_list, 0);

		if (pEffect->flags & effFlagsCanReplacing) pEffect->processReplacing(pEffect, float_list_in, float_list_out, 1);
		else pEffect->process(pEffect, float_list_in, float_list_out, 1);
	}
}
