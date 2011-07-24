#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include <foobar2000.h>

#include "nu_processing/midi_container.h"

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

#ifdef TIME_INFO
struct VstTimeInfo;
#endif

class VSTiPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
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

	// editor
	bool hasEditor() const;
	void displayEditorModal( HWND hwndParent );

	// setup
	void setSampleRate(unsigned rate);
	unsigned getChannelCount();

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, bool clean_flag);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

	void needIdle();

#ifdef TIME_INFO
	// for audiomaster callback
	VstTimeInfo * getTime();
#endif

private:
	void send_event( DWORD );
	void render(audio_sample *, unsigned);

	HMODULE      hDll;
	AEffect    * pEffect[2];

	unsigned     uSamplesRemaining;

	unsigned     uSampleRate;
	unsigned     uLoopMode;
	unsigned     uNumOutputs;

	system_exclusive_table mSysexMap;
	pfc::array_t<midi_stream_event> mStream;

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

	struct myVstEvent
	{
		struct myVstEvent * next;
		unsigned port;
		union
		{
			VstMidiEvent midiEvent;
			VstMidiSysexEvent sysexEvent;
		} ev;
	} * evChain, * evTail;

	void freeChain();

	bool         bNeedIdle;

	pfc::array_t<t_uint8>    blChunk;

#ifdef TIME_INFO
	VstTimeInfo* time_info;
#endif
};

#endif
