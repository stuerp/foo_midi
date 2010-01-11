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

#ifdef TIME_INFO
class VSTiPlayer;
struct host_effect
{
	VSTiPlayer * host;
	AEffect * effect;
	host_effect( VSTiPlayer * h, AEffect * e ) : host(h), effect(e) {}
};

critical_section effect_list_lock;
pfc::chain_list_simple_t<host_effect> effect_list;
#endif

static long __cdecl audioMaster(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
#ifdef TIME_INFO
	VSTiPlayer * host = 0;
	{
		insync(effect_list_lock);
		for ( pfc::chain_list_simple_t<host_effect>::t_iter i = effect_list.first(); i; i = effect_list.next( i ) )
		{
			if ( effect_list.get_item( i ).effect == effect )
			{
				host = effect_list.get_item( i ).host;
				break;
			}
		}
	}
#endif

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
	}

	return 0;
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
	pStream = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;

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

#ifdef TIME_INFO
		{
			insync( effect_list_lock );
			for ( pfc::chain_list_simple_t<host_effect>::t_iter i = effect_list.first(); i; i = effect_list.next( i ) )
			{
				if ( effect_list.get_item( i ).host == this )
				{
					effect_list.remove( i );
					break;
				}
			}
		}
#endif
	}

	if (hDll)
	{
		FreeLibrary(hDll);
	}

	if (pStream)
	{
		free(pStream);
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

#ifdef TIME_INFO
			{
				insync(effect_list_lock);
				effect_list.insert_last( host_effect( this, pEffect ) );
			}
#endif

			if (pEffect)
			{
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

void VSTiPlayer::getChunk( pfc::array_t<t_uint8> & out )
{
	if (pEffect)
	{
		void * chunk;
		long size = pEffect->dispatcher(pEffect, effGetChunk, 0, 0, &chunk, 0);
		out.set_size( size );
		memcpy( out.get_ptr(), chunk, size );
	}
}

void VSTiPlayer::setChunk( const void * in, unsigned size )
{
	if ( pEffect )
	{
		pEffect->dispatcher( pEffect, effSetChunk, 0, size, ( void * ) in, 0 );
	}
}

void VSTiPlayer::setSampleRate(unsigned rate)
{
	assert(!blState.get_size());

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
#ifdef TIME_INFO
	time_info->sampleRate = rate;
#endif
}

unsigned VSTiPlayer::getChannelCount()
{
	if (pEffect) return pEffect->numOutputs;
	else return 0;
}

bool VSTiPlayer::Load(MIDI_file * mf, unsigned loop_mode, unsigned clean_flags)
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
							for (j = i - 1; j != ~0; --j)
							{
								if (pStream[j].tm < uTimeLoopStart) break;
							}
							uStreamLoopStart = j + 1;
							uTimeLoopStart = pStream[i].tm;
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
				for (i = 0; i < uStreamSize; i++)
				{
					if (pStream[i].tm >= uTimeEnd) break;
				}
				for (; i < uStreamSize; i++)
				{
					pStream[i].tm = uTimeEnd - 1;
				}
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
	assert(pStream);

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

	pEffect->dispatcher(pEffect, effIdle, 0, 0, 0, 0);

	DWORD done = 0;

	while (done < count)
	{
		DWORD todo = uTimeEnd - uTimeCurrent;
		if (todo > count - done) todo = count - done;
		if (todo > BUFFER_SIZE) todo = BUFFER_SIZE;

		DWORD time_target = todo + uTimeCurrent;
		UINT stream_end = uStreamPosition;

		while (stream_end < uStreamSize && pStream[stream_end].tm < time_target) stream_end++;

		if (stream_end > uStreamPosition)
		{
			unsigned events_size = sizeof(long) * 2 + sizeof(VstEvent*) * (stream_end - uStreamPosition);
			UINT i;

			for (i = uStreamPosition; i < stream_end; i++)
			{
				if (!(pStream[i].ev & 0xFF000000)) events_size += sizeof(VstMidiEvent);
				else events_size += sizeof(VstMidiSysexEvent);
			}

			resizeState(buffer_size + events_size);

			events_list->numEvents = stream_end - uStreamPosition;
			events_list->reserved = 0;

			VstEvent * event = (VstEvent*) (events_list->events + events_list->numEvents);

			for (i = 0; uStreamPosition < stream_end; uStreamPosition++, i++)
			{
				events_list->events[i] = event;

				if (!(pStream[uStreamPosition].ev & 0xFF000000))
				{
					VstMidiEvent * e = (VstMidiEvent*) event;
					memset(e, 0, sizeof(*e));
					e->type = kVstMidiType;
					e->byteSize = sizeof(*e);
					e->deltaFrames = pStream[uStreamPosition].tm - uTimeCurrent;
					memcpy(&e->midiData, &pStream[uStreamPosition].ev, 4);
					event = (VstEvent*)(e + 1);
				}
				else
				{
					VstMidiSysexEvent * e = (VstMidiSysexEvent*) event;
					UINT n = pStream[uStreamPosition].ev & 0xffffff;
					SYSEX_ENTRY & sysex = pSysexMap->events[n];
					e->type = kVstSysExType;
					e->byteSize = sizeof(*e);
					e->deltaFrames = pStream[uStreamPosition].tm - uTimeCurrent;
					e->flags = 0;
					e->dumpBytes = sysex.len;
					e->resvd1 = 0;
					e->sysexDump = (char *)(pSysexMap->data + sysex.ofs);
					e->resvd2 = 0;
					event = (VstEvent*)(e + 1);
				}
			}

			pEffect->dispatcher(pEffect, effProcessEvents, 0, 0, events_list, 0);
		}

		uTimeCurrent = time_target;

		if (pEffect->flags & effFlagsCanReplacing)
		{
			pEffect->processReplacing(pEffect, float_list_in, float_list_out, todo);
		}
		else
		{
			memset(float_out, 0, sizeof(float) * BUFFER_SIZE * pEffect->numOutputs);
			pEffect->process(pEffect, float_list_in, float_list_out, todo);
		}

		for (UINT i = 0, nch = pEffect->numOutputs; i < todo; i++)
		{
			for (UINT j = 0; j < nch; j++)
			{
				out[j] = audio_sample(float_out[i + j * BUFFER_SIZE]);
			}
			out += nch;
		}

		done += todo;

		if (uTimeCurrent >= uTimeEnd)
		{
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
#ifdef TIME_INFO
			insync( effect_list_lock );
#endif

			// total shutdown
			pEffect->dispatcher(pEffect, effStopProcess, 0, 0, 0, 0);
			pEffect->dispatcher(pEffect, effClose, 0, 0, 0, 0);

#ifdef TIME_INFO
			pfc::chain_list_simple_t<host_effect>::t_iter item = 0;

			for ( pfc::chain_list_simple_t<host_effect>::t_iter i = effect_list.first(); i; i = effect_list.next( i ) )
			{
				if ( effect_list.get_item( i ).effect == pEffect )
				{
					item = i;
					effect_list.get_item( i ).host = 0;
					break;
				}
			}
#endif

			blState.set_size(0);

			main_func pMain = (main_func) GetProcAddress(hDll, "main");

			pEffect = (*pMain)(&audioMaster);

			if (!pEffect)
			{
#ifdef TIME_INFO
				if ( item ) effect_list.remove( item );
#endif
				return;
			}

#ifdef TIME_INFO
			if ( item )
			{
				effect_list.get_item( item ).host = this;
				effect_list.get_item( item ).effect = pEffect;
			}
			else
			{
				effect_list.insert_last( host_effect( this, pEffect ) );
			}
#endif
		}
	}

	if (!blState.get_size())
	{
		Play(0, 0);
	}

	uTimeCurrent = sample;

	pfc::array_t<MIDI_EVENT> filler;

	UINT stream_start = uStreamPosition;

	for (; uStreamPosition < uStreamSize && pStream[uStreamPosition].tm < uTimeCurrent; uStreamPosition++);

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

		unsigned events_size = sizeof(long) * 2 + sizeof(VstEvent*) * j;

		for (i = 0; i < j; i++)
		{
			if (!(me[i].ev & 0xFF000000)) events_size += sizeof(VstMidiEvent);
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

			if (!(me[i].ev & 0xFF000000))
			{
				VstMidiEvent * e = (VstMidiEvent*) event;
				memset(e, 0, sizeof(*e));
				e->type = kVstMidiType;
				e->byteSize = sizeof(*e);
				e->deltaFrames = 0;
				memcpy(&e->midiData, &me[i].ev, 4);
				event = (VstEvent*)(e + 1);
			}
			else
			{
				VstMidiSysexEvent * e = (VstMidiSysexEvent*) event;
				UINT n = me[i].ev & 0xffffff;
				SYSEX_ENTRY & sysex = pSysexMap->events[n];
				e->type = kVstSysExType;
				e->byteSize = sizeof(*e);
				e->deltaFrames = 0;
				e->flags = 0;
				e->dumpBytes = sysex.len;
				e->resvd1 = 0;
				e->sysexDump = (char *)(pSysexMap->data + sysex.ofs);
				e->resvd2 = 0;
				event = (VstEvent*)(e + 1);
			}
		}

		pEffect->dispatcher(pEffect, effProcessEvents, 0, 0, my_events_list, 0);

		if (pEffect->flags & effFlagsCanReplacing) pEffect->processReplacing(pEffect, float_list_in, float_list_out, 1);
		else pEffect->process(pEffect, float_list_in, float_list_out, 1);
	}
}