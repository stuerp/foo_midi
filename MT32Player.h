#ifndef __MT32Player_h__
#define __MT32Player_h__

#include <foobar2000.h>

#include "nu_processing/midi_container.h"

#include <mt32emu.h>

class MT32Player
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	MT32Player( bool gm = false, unsigned gm_set = 0 );

	// close, unload
	~MT32Player();

	// configuration
	void setBasePath( const char * in );
	void setAbortCallback( abort_callback * in );

	// setup
	void setSampleRate(unsigned rate);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
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

	MT32Emu::File    * controlRomFile, * pcmRomFile;
	const MT32Emu::ROMImage * controlRom, * pcmRom;

	bool               bGM;
	unsigned           uGMSet;

	unsigned           uSamplesRemaining;

	unsigned           uSampleRate;
	unsigned           uLoopMode;

	system_exclusive_table mSysexMap;
	pfc::array_t<midi_stream_event> mStream;

	UINT               uStreamPosition;
	DWORD              uTimeCurrent;
	DWORD              uTimeEnd;

	UINT               uStreamLoopStart;
	DWORD              uTimeLoopStart;

	void               reset();

	MT32Emu::File * openFile( const char *filename );
};

#endif
