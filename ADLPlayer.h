#ifndef __ADLPlayer_h__
#define __ADLPlayer_h__

#include "MIDIPlayer.h"

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

protected:
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

private:
	static void render_internal(void * context, int count, short * out);

	void reset_drum_channels();

	struct ADL_MIDIPlayer * midiplay[3];

	unsigned           uBankNumber;
	unsigned           uChipCount;
	unsigned           u4OpCount;
	bool               bFullPanning;
};

#endif
