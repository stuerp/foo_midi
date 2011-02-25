#ifndef __BMPlayer_h__
#define __BMPlayer_h__

#include <foobar2000.h>

#include "nu_processing/midi_container.h"

#include "../../../bass/c/bassmidi.h"

class BMPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	BMPlayer();

	// close, unload
	~BMPlayer();

	// configuration
	void setSoundFont( const char * in );
	void setFileSoundFont( const char * in );

	// setup
	void setSampleRate(unsigned rate);

	bool Load(const midi_container & midi_file, unsigned loop_mode, bool clean_flag);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	void send_event(DWORD b);
	void render(audio_sample * out, unsigned count);

	void shutdown();
	bool startup();

	void reset_drum_channels();

	pfc::array_t<HSOUNDFONT> _soundFonts;
	pfc::string8       sSoundFontName;
	pfc::string8       sFileSoundFontName;

	HSTREAM            _stream;

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

	BYTE               gs_part_to_ch[16];
	BYTE               drum_channels[16];
};

#endif
