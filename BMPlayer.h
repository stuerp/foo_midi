#ifndef __BMPlayer_h__
#define __BMPlayer_h__

#include "MIDIPlayer.h"

#include "../../../bass/c/bassmidi.h"

class BMPlayer : public MIDIPlayer
{
public:
	// zero variables
	BMPlayer();

	// close, unload
	virtual ~BMPlayer();

	// configuration
	void setSoundFont( const char * in );
	void setFileSoundFont( const char * in );
	void setSincInterpolation(bool enable = true);

private:
	virtual void send_event(DWORD b);
	virtual void render(audio_sample * out, unsigned count);

	virtual void shutdown();
	virtual bool startup();

	void reset_drum_channels();

	pfc::array_t<HSOUNDFONT> _soundFonts;
	pfc::string8       sSoundFontName;
	pfc::string8       sFileSoundFontName;

	HSTREAM            _stream;

	bool               bSincInterpolation;

	enum
	{
	                   mode_gm = 0,
					   mode_gm2,
	                   mode_gs,
	                   mode_xg
	}
	                   synth_mode;

	BYTE               gs_part_to_ch[3][16];
	BYTE               drum_channels[48];
};

#endif
