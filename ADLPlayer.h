#ifndef __ADLPlayer_h__
#define __ADLPlayer_h__

#include "MIDIPlayer.h"

class ADLPlayer : public MIDIPlayer {
	public:
	// zero variables
	ADLPlayer();

	// close, unload
	virtual ~ADLPlayer();

	enum {
		ADLMIDI_EMU_NUKED = 0,
		ADLMIDI_EMU_NUKED_174,
		ADLMIDI_EMU_DOSBOX
	};

	// configuration
	void setCore(unsigned);
	void setBank(unsigned);
	void setChipCount(unsigned);
	void set4OpCount(unsigned);
	void setFullPanning(bool);

	protected:
	virtual void send_event(uint32_t b);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	private:
	static void render_internal(void* context, int count, short* out);

	void reset_drum_channels();

	struct ADL_MIDIPlayer* midiplay[3];

	unsigned uEmuCore;
	unsigned uBankNumber;
	unsigned uChipCount;
	unsigned u4OpCount;
	bool bFullPanning;
};

#endif
