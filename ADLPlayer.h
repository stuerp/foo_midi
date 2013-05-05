#ifndef __ADLPlayer_h__
#define __ADLPlayer_h__

#include <foobar2000.h>

#include "nu_processing/midi_container.h"

class MIDIplay;

class ADLPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	ADLPlayer();

	// close, unload
	~ADLPlayer();

	// configuration
	void setBank( unsigned );
	void setChipCount( unsigned );
	void set4OpCount( unsigned );
	void setFullPanning( bool );

	// setup
	void setSampleRate(unsigned rate);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	void send_event(DWORD b);
	void render(audio_sample * out, unsigned count);

	static void render_internal(void * context, int count, short * out);

	void shutdown();
	bool startup();

	void reset_drum_channels();

	MIDIplay         * midiplay;

	void             * resampler;

	unsigned           uBankNumber;
	unsigned           uChipCount;
	unsigned           u4OpCount;
	bool               bFullPanning;

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

	enum
	{
	                   mode_gm = 0,
					   mode_gm2,
	                   mode_gs,
	                   mode_xg
	}
	                   synth_mode;

	BYTE               gs_part_to_ch[4][16];
	BYTE               drum_channels[4][16];
};

#endif
