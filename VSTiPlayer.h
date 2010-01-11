#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include "main.h"

struct AEffect;
struct VstEvents;

#ifndef _AUDIO_CHUNK_H_
typedef double audio_sample;
#endif

class VSTiPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1,
		loop_mode_xmi    = 1 << 2,
		loop_mode_marker = 1 << 3
	};

	// zero variables
	VSTiPlayer();

	// close, unload
	~VSTiPlayer();

	// load, open, verify type
	bool LoadVST(const char * path);

	// must be loaded for the following to work
	void getVendorString(pfc::string_base & out);
	void getProductString(pfc::string_base & out);
	long getVendorVersion();
	long getUniqueID();

	// configuration
	void getChunk(pfc::array_t<t_uint8> & out);
	void setChunk( const void * in, unsigned size );

	// setup
	void setSampleRate(unsigned rate);
	unsigned getChannelCount();

	bool Load(MIDI_file * mf, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	HMODULE      hDll;
	AEffect    * pEffect;

	unsigned     uSampleRate;
	unsigned     uLoopMode;

	CSysexMap  * pSysexMap;
	MIDI_EVENT * pStream;
	UINT         uStreamSize;

	UINT         uStreamPosition;
	DWORD        uTimeCurrent;
	DWORD        uTimeEnd;

	UINT         uStreamLoopStart;
	DWORD        uTimeLoopStart;

	void resizeState(unsigned size);

	pfc::array_t<t_uint8>    blState;
	unsigned     buffer_size;

	float     ** float_list_in;
	float     ** float_list_out;
	float      * float_null;
	float      * float_out;

	VstEvents  * events_list;
};

#endif