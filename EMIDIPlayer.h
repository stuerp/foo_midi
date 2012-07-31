#ifndef __EMIDIPlayer_h__
#define __EMIDIPlayer_h__

#include <foobar2000.h>

#include "nu_processing/midi_container.h"

#include "CSMF.hpp"
#include "CMIDIModule.hpp"
#include "COpllDevice.hpp"
#include "CSccDevice.hpp"

class EMIDIPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	EMIDIPlayer();

	// close, unload
	~EMIDIPlayer();

	// setup
	void setSampleRate(unsigned rate);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	void send_event( DWORD );
	void render(audio_sample *, unsigned);

	void reset_drum_channels();
	void set_drum_channel(int channel, int enable);

	dsa::CMIDIModule mModule[8];

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

	enum
	{
	                   mode_gm = 0,
					   mode_gm2,
	                   mode_gs,
	                   mode_xg
	}
	                   synth_mode;

	BYTE               gs_part_to_ch[16];
	BYTE               drum_channels[16];
};

#endif
