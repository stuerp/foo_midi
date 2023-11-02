/*
	BASS simple synth
	Copyright (c) 2001-2021 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include "bass.h"

BASS_INFO info;

// display error messages
void Error(const char *text)
{
	printf("Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	ExitProcess(0);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define KEYS 20
const WORD keys[KEYS] = {
	'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U',
	'I', '9', 'O', '0', 'P', 219, 187, 221
};
#define MAXVOL 0.22
#define DECAY (MAXVOL/4000)
float vol[KEYS], pos[KEYS]; // key volume and position/phase

DWORD CALLBACK StreamProc(HSTREAM handle, float *buffer, DWORD length, void *user)
{
	int k, c;
	float omega;
	memset(buffer, 0, length);
	for (k = 0; k < KEYS; k++) {
		if (!vol[k]) continue;
		omega = 2 * M_PI * pow(2.0, (k + 3) / 12.0) * 440.0 / info.freq;
		for (c = 0; c < length / sizeof(float); c += 2) {
			buffer[c] += sin(pos[k]) * vol[k];
			buffer[c + 1] = buffer[c]; // left and right channels are the same
			pos[k] += omega;
			if (vol[k] < MAXVOL) {
				vol[k] -= DECAY;
				if (vol[k] <= 0) { // faded-out
					vol[k] = 0;
					break;
				}
			}
		}
		pos[k] = fmod(pos[k], 2 * M_PI);
	}
	return length;
}

int main(int argc, char **argv)
{
	HSTREAM stream; // the stream
	const char *fxname[9] = { "CHORUS", "COMPRESSOR", "DISTORTION", "ECHO",
		"FLANGER", "GARGLE", "I3DL2REVERB", "PARAMEQ", "REVERB" };
	HFX fx[9] = { 0 }; // effect handles
	INPUT_RECORD keyin;
	DWORD r;

	printf("BASS simple synth\n"
		"-----------------\n");

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		printf("An incorrect version of BASS.DLL was loaded");
		return 0;
	}

	// initialize default output device
	if (!BASS_Init(-1, 44100, 0, 0, NULL))
		Error("Can't initialize device");

	BASS_GetInfo(&info);
	stream = BASS_StreamCreate(info.freq, 2, BASS_SAMPLE_FLOAT, (STREAMPROC*)StreamProc, 0); // create a stream (stereo for effects)
	BASS_ChannelSetAttribute(stream, BASS_ATTRIB_BUFFER, 0); // disable buffering for minimum latency
	BASS_ChannelPlay(stream, FALSE); // start it

	printf("ds version: %d (effects %s)\n", info.dsver, info.dsver < 8 ? "disabled" : "enabled");
	printf("press these keys to play:\n\n"
		"  2 3  5 6 7  9 0  =\n"
		" Q W ER T Y UI O P[ ]\n\n"
		"press spacebar to quit\n\n");
	if (info.dsver >= 8) // DX8 effects available
		printf("press F1-F9 to toggle effects\n\n");

	while (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &keyin, 1, &r)) {
		int key;
		if (keyin.EventType != KEY_EVENT) continue;
		if (keyin.Event.KeyEvent.wVirtualKeyCode == VK_SPACE) break;
		if (keyin.Event.KeyEvent.bKeyDown) {
			if (keyin.Event.KeyEvent.wVirtualKeyCode >= VK_F1
				&& keyin.Event.KeyEvent.wVirtualKeyCode <= VK_F9) {
				int n = keyin.Event.KeyEvent.wVirtualKeyCode - VK_F1;
				if (fx[n]) {
					BASS_ChannelRemoveFX(stream, fx[n]);
					fx[n] = 0;
					printf("effect %s = OFF\t\t\r", fxname[n]);
				} else {
					// set the effect, not bothering with parameters (use defaults)
					if (fx[n] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + n, 0))
						printf("effect %s = ON\t\t\r", fxname[n]);
				}
			}
		}
		for (key = 0; key < KEYS; key++)
			if (keyin.Event.KeyEvent.wVirtualKeyCode == keys[key]) {
				if (keyin.Event.KeyEvent.bKeyDown && vol[key] < MAXVOL) {
					pos[key] = 0;
					vol[key] = MAXVOL + DECAY / 2; // start key (setting "vol" slightly higher than MAXVOL to cover any rounding-down)
				} else if (!keyin.Event.KeyEvent.bKeyDown && vol[key])
					vol[key] -= DECAY; // trigger key fadeout
				break;
			}
	}

	BASS_Free();
	return 0;
}
