#ifndef __OPNPlayer_h__
#define __OPNPlayer_h__

#include "MIDIPlayer.h"

class OPNPlayer : public MIDIPlayer {
	public:
	// zero variables
	OPNPlayer();

	// close, unload
	virtual ~OPNPlayer();

	enum {
		/*! Mame YM2612 */
		OPNMIDI_EMU_MAME = 0,
		/*! Nuked OPN2 */
		OPNMIDI_EMU_NUKED,
		/*! GENS */
		OPNMIDI_EMU_GENS,
	};

	// configuration
	void setCore(unsigned);
	void setBank(unsigned);
	void setChipCount(unsigned);
	void setFullPanning(bool);

	protected:
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t* event, uint32_t size, size_t port);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	private:
	static void render_internal(void* context, int count, short* out);

	void reset_drum_channels();

	struct OPN2_MIDIPlayer* midiplay[3];

	unsigned uEmuCore;
	unsigned uBankNumber;
	unsigned uChipCount;
	unsigned u4OpCount;
	bool bFullPanning;
};

#endif
