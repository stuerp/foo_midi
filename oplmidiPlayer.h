#ifndef __oplmidiPlayer_h__
#define __oplmidiPlayer_h__

#include "MIDIPlayer.h"

namespace midisynth
{
	namespace opl
	{
		class fm_note_factory;
	}
	class synthesizer;
}

class oplmidiPlayer : public MIDIPlayer
{
public:
	// zero variables
	oplmidiPlayer();

	// close, unload
	virtual ~oplmidiPlayer();

	// configuration
	void setBank( unsigned );

private:
	virtual void send_event(DWORD b);
	virtual void render(audio_sample * out, unsigned count);

	virtual void shutdown();
	virtual bool startup();

	unsigned           uBankNumber;

	midisynth::opl::fm_note_factory * factory;

	midisynth::synthesizer     * synthesizers[4];
};

#endif
