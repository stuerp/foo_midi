#include "VSTiPlayer.h"

#include <stdio.h>

// {2A009D10-6B17-4802-BAD6-A3229CB57879}
static const GUID guid = 
{ 0x2a009d10, 0x6b17, 0x4802, { 0xba, 0xd6, 0xa3, 0x22, 0x9c, 0xb5, 0x78, 0x79 } };

cfg_int cfg_recover_tracks(guid, 0);

int main(int argc, const char * const * argv)
{
	FILE * f = fopen(argv[1], "rb");
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	void * buf = malloc(len);
	fread(buf, 1, len, f);
	fclose(f);
	
	MIDI_file * mf = MIDI_file::Create(buf, len);
	
	if (mf)
	{
		VSTiPlayer vp;
		
		vp.LoadVST("c:\\program files\\steinberg\\vstplugins\\yamaha\\s-yxg50.dll");
		
		vp.setSampleRate(44100);
		
		vp.Load(mf, 0, 0);
		
		f = fopen("output.raw", "wb");
		
		audio_sample * out = (audio_sample *) malloc(sizeof(double) * 44100 * vp.getChannelCount());
		
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		for (unsigned i = 0; i < 30; i++)
		{		
			{
				profiler(synth);
				vp.Play(out, 44100);
			}

			for (unsigned j = 0; j < 88200; j++)
			{
				((float*)out)[j] = out[j];
			}

			fwrite(out, 1, sizeof(float) * 44100 * vp.getChannelCount(), f);
		}
		
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

		fclose(f);
		
		free(out);
		
		mf->Free();
	}
	
	return 0;
}
