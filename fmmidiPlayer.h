#ifndef __fmmidiPlayer_h__
#define __fmmidiPlayer_h__

#include "MIDIPlayer.h"

namespace midisynth {
	namespace opn {
		class fm_note_factory;
	}
	class synthesizer;
} // namespace midisynth

class fmmidiPlayer : public MIDIPlayer {
	public:
	// zero variables
	fmmidiPlayer();

	// close, unload
	virtual ~fmmidiPlayer();

	// configuration
	void setProgramPath(const char*);

	private:
	virtual void send_event(uint32_t b);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	std::string bank_path;

	midisynth::opn::fm_note_factory* factory;

	midisynth::synthesizer* synthesizers[4];
};

#endif
