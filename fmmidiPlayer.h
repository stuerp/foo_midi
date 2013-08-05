#ifndef __fmmidiPlayer_h__
#define __fmmidiPlayer_h__

#include <foobar2000.h>

#include <midi_container.h>

namespace midisynth
{
	namespace opn
	{
		class fm_note_factory;
	}
	class synthesizer;
}

class fmmidiPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	fmmidiPlayer();

	// close, unload
	~fmmidiPlayer();

	// configuration
	void setProgramPath( const char * );

	// setup
	void setSampleRate(unsigned rate);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	void send_event(DWORD b);
	void render(audio_sample * out, unsigned count);

	void shutdown();
	bool startup();

	pfc::string8                 bank_path;

	midisynth::opn::fm_note_factory * factory;

	midisynth::synthesizer     * synthesizers[4];

	unsigned           uSamplesRemaining;

	unsigned           uSampleRate;
	unsigned           uLoopMode;

	system_exclusive_table mSysexMap;
	std::vector<midi_stream_event> mStream;

	UINT               uStreamPosition;
	DWORD              uTimeCurrent;
	DWORD              uTimeEnd;

	UINT               uStreamLoopStart;
	DWORD              uTimeLoopStart;
};

#endif
