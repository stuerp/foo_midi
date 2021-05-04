#ifndef __SFPlayer_h__
#define __SFPlayer_h__

#include "MIDIPlayer.h"

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
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	std::string       _last_error;

	fluid_settings_t * _settings;
	fluid_sfloader_t * _loader;
	fluid_synth_t    * _synth;
	std::string        sSoundFontName;
	std::string        sFileSoundFontName;

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
	uint8_t            drum_channels[48];
	uint8_t            gs_part_to_ch[48];
	unsigned short     channel_banks[48];
};

#endif
