#ifndef __oplmidiPlayer_h__
#define __oplmidiPlayer_h__

#include "MIDIPlayer.h"

namespace midisynth {
	namespace opl {
		class fm_note_factory;
	}
	class synthesizer;
} // namespace midisynth

class oplmidiPlayer : public MIDIPlayer {
	public:
	// zero variables
	oplmidiPlayer();

	// close, unload
	virtual ~oplmidiPlayer();

	// configuration
	void setBank(unsigned);

	private:
	virtual void send_event(uint32_t b);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	unsigned uBankNumber;

	midisynth::opl::fm_note_factory* factory;

	midisynth::synthesizer* synthesizers[4];
};

#endif
