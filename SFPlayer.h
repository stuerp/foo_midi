#ifndef __SFPlayer_h__
#define __SFPlayer_h__

#include "MIDIPlayer.h"

#define FLUIDSYNTH_NOT_A_DLL
#include <fluidsynth.h>

class SFPlayer : public MIDIPlayer
{
public:
	// zero variables
	SFPlayer();

	// close, unload
	virtual ~SFPlayer();

	// configuration
	void setSoundFont( const char * in );
	void setFileSoundFont( const char * in );
	void setInterpolationMethod(unsigned method);

	const char * GetLastError() const;

private:
	virtual void send_event(DWORD b);
	virtual void render(audio_sample * out, unsigned count);

	virtual void shutdown();
	virtual bool startup();

	pfc::string8       _last_error;

	fluid_settings_t * _settings;
	fluid_synth_t    * _synth;
	pfc::string8       sSoundFontName;
	pfc::string8       sFileSoundFontName;

	unsigned           uInterpolationMethod;

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
