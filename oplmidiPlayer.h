#ifndef __oplmidiPlayer_h__
#define __oplmidiPlayer_h__

#include <foobar2000.h>

#include <midi_container.h>

namespace midisynth
{
	namespace opl
	{
		class fm_note_factory;
	}
	class synthesizer;
}

class oplmidiPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	oplmidiPlayer();

	// close, unload
	~oplmidiPlayer();

	// configuration
	void setBank( unsigned );

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

	unsigned           uBankNumber;

	midisynth::opl::fm_note_factory * factory;

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
