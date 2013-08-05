#ifndef __SFPlayer_h__
#define __SFPlayer_h__

#include <foobar2000.h>

#include <midi_container.h>

#define FLUIDSYNTH_NOT_A_DLL
#include <fluidsynth.h>

class SFPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	SFPlayer();

	// close, unload
	~SFPlayer();

	// configuration
	void setSoundFont( const char * in );
	void setFileSoundFont( const char * in );

	// setup
	void setSampleRate(unsigned rate);
	void setInterpolationMethod(unsigned method);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

	const char * GetLastError() const;

private:
	void send_event(DWORD b);
	void render(audio_sample * out, unsigned count);

	void shutdown();
	bool startup();

	pfc::string8       _last_error;

	fluid_settings_t * _settings;
	fluid_synth_t    * _synth;
	pfc::string8       sSoundFontName;
	pfc::string8       sFileSoundFontName;

	unsigned           uSamplesRemaining;

	unsigned           uSampleRate;
	unsigned           uLoopMode;
	unsigned           uInterpolationMethod;

	system_exclusive_table mSysexMap;
	std::vector<midi_stream_event> mStream;

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

	void reset_drums();
	BYTE               drum_channels[32];
	BYTE               gs_part_to_ch[16];
	unsigned short     channel_banks[32];
};

#endif
