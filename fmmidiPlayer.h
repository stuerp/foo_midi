#ifndef __fmmidiPlayer_h__
#define __fmmidiPlayer_h__

#include "MIDIPlayer.h"

namespace midisynth
{
	namespace opn
	{
		class fm_note_factory;
	}
	class synthesizer;
}

class fmmidiPlayer : public MIDIPlayer
{
public:
	// zero variables
	fmmidiPlayer();

	// close, unload
	virtual ~fmmidiPlayer();

	// configuration
	void setProgramPath( const char * );

private:
	virtual void send_event(DWORD b);
	virtual void render(audio_sample * out, unsigned count);

	virtual void shutdown();
	virtual bool startup();

	pfc::string8                 bank_path;

	midisynth::opn::fm_note_factory * factory;

	midisynth::synthesizer     * synthesizers[4];
};

#endif
