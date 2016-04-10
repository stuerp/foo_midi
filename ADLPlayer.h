#ifndef __ADLPlayer_h__
#define __ADLPlayer_h__

#include "MIDIPlayer.h"

class MIDIplay;

class ADLPlayer : public MIDIPlayer
{
public:
	// zero variables
	ADLPlayer();

	// close, unload
	virtual ~ADLPlayer();

	// configuration
	void setBank( unsigned );
	void setChipCount( unsigned );
	void set4OpCount( unsigned );
	void setFullPanning( bool );
	void setChorus( bool );

protected:
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

private:
	static void render_internal(void * context, int count, short * out);

	void reset_drum_channels();

	MIDIplay         * midiplay;

	void             * resampler;

	unsigned           uBankNumber;
	unsigned           uChipCount;
	unsigned           u4OpCount;
	bool               bFullPanning;
	bool               bChorus;

	enum
	{
	                   mode_gm = 0,
					   mode_gm2,
	                   mode_gs,
	                   mode_xg
	}
	                   synth_mode;

	uint8_t            gs_part_to_ch[4][16];
	uint8_t            drum_channels[4][16];
};

#endif
