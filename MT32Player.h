#ifndef __MT32Player_h__
#define __MT32Player_h__

#include "main.h"

#include <mt32emu.h>

#ifndef _AUDIO_CHUNK_H_
typedef float audio_sample;
#endif

class MT32Player
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
	MT32Player( bool gm = false );

	// close, unload
	~MT32Player();

	// configuration
	void setBasePath( const char * in );
	void setAbortCallback( abort_callback * in );

	// setup
	void setSampleRate(unsigned rate);

	bool Load(MIDI_file * mf, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	void send_event(DWORD b);
	void render(audio_sample * out, unsigned count);

	void shutdown();
	bool startup();

	MT32Emu::Synth   * _synth;
	pfc::string8       sBasePath;
	abort_callback   * _abort;

	bool               bGM;

	unsigned           uSamplesRemaining;

	unsigned           uSampleRate;
	unsigned           uLoopMode;

	CSysexMap        * pSysexMap;
	MIDI_EVENT       * pStream;
	UINT               uStreamSize;

	UINT               uStreamPosition;
	DWORD              uTimeCurrent;
	DWORD              uTimeEnd;

	UINT               uStreamLoopStart;
	DWORD              uTimeLoopStart;

	void               reset();

	// callbacks
	static void cb_printDebug( void *userData, const char *fmt, va_list list );
	static MT32Emu::File * cb_openFile( void *userData, const char *filename, MT32Emu::File::OpenMode mode );
};

#endif