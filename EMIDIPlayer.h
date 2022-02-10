#ifndef __EMIDIPlayer_h__
#define __EMIDIPlayer_h__

#include "MIDIPlayer.h"

#include "CMIDIModule.hpp"
#include "COpllDevice.hpp"
#include "CSMF.hpp"
#include "CSccDevice.hpp"

class EMIDIPlayer : public MIDIPlayer {
	public:
	// zero variables
	EMIDIPlayer();

	// close, unload
	virtual ~EMIDIPlayer();

	private:
	virtual void send_event(uint32_t);
	virtual void send_sysex(const uint8_t* event, uint32_t size, size_t port);
	virtual void render(float*, unsigned long);

	virtual void shutdown();
	virtual bool startup();

	void reset_drum_channels();
	void set_drum_channel(int channel, int enable);

	bool bInitialized;

	dsa::CMIDIModule mModule[8];

	enum {
		mode_gm = 0,
		mode_gm2,
		mode_gs,
		mode_xg
	} synth_mode;

	BYTE gs_part_to_ch[16];
	BYTE drum_channels[16];
};

#endif
