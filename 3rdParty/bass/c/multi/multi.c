/*
	BASS multiple output example
	Copyright (c) 2001-2021 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "bass.h"

HWND win;
OPENFILENAME ofn;

DWORD outdev[2] = { 1, 0 };	// output devices
DWORD chan[2];		// channel handles

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	MessageBox(win, mes, 0, 0);
}

// Cloning DSP function
void CALLBACK CloneDSP(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	BASS_StreamPutData((HSTREAM)user, buffer, length); // user = clone
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

INT_PTR CALLBACK DialogProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					EndDialog(h, 0);
					break;

				case 10: // open a file to play on device #1
				case 11: // open a file to play on device #2
					{
						int devn = LOWORD(w) - 10;
						char file[MAX_PATH] = "";
						ofn.lpstrFile = file;
						if (GetOpenFileName(&ofn)) {
							BASS_ChannelFree(chan[devn]); // free the old channel
							BASS_SetDevice(outdev[devn]); // set the device to create new channel on
							if (!(chan[devn] = BASS_StreamCreateFile(FALSE, file, 0, 0, BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT))
								&& !(chan[devn] = BASS_MusicLoad(FALSE, file, 0, 0, BASS_MUSIC_RAMPS | BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT, 1))) {
								MESS(10 + devn, WM_SETTEXT, 0, "Open file...");
								Error("Can't play the file");
								break;
							}
							MESS(10 + devn, WM_SETTEXT, 0, strrchr(file, '\\') + 1);
							BASS_ChannelPlay(chan[devn], FALSE);
						}
					}
					break;

				case 12: // device #1
				case 13: // device #2
					if (HIWORD(w) == CBN_SELCHANGE) { // device selection changed
						int sel = MESS(LOWORD(w), CB_GETCURSEL, 0, 0); // get the selection
						int devn = LOWORD(w) - 12;
						if (outdev[devn] == sel) break;
						if (!BASS_Init(sel, 44100, 0, win, NULL)) { // initialize new device
							Error("Can't initialize device");
							MESS(LOWORD(w), CB_SETCURSEL, outdev[devn], 0);
							break;
						}
						if (chan[devn]) BASS_ChannelSetDevice(chan[devn], sel); // move channel to new device
						BASS_SetDevice(outdev[devn]); // set context to old device
						BASS_Free(); // free it
						outdev[devn] = sel;
					}
					break;

				case 15: // clone on device #1
				case 16: // clone on device #2
					{
						int devn = LOWORD(w) - 15;
						BASS_CHANNELINFO chaninfo;
						if (!BASS_ChannelGetInfo(chan[devn ^ 1], &chaninfo)) {
							Error("Nothing to clone");
							break;
						}
						BASS_ChannelFree(chan[devn]); // free the old channel
						BASS_SetDevice(outdev[devn]); // set the device to create clone on
						if (!(chan[devn] = BASS_StreamCreate(chaninfo.freq, chaninfo.chans, chaninfo.flags, STREAMPROC_PUSH, 0))) { // create a "push" stream
							MESS(10 + devn, WM_SETTEXT, 0, "Open file...");
							Error("Can't create clone");
						} else {
							BASS_INFO info;
							BASS_GetInfo(&info); // get latency info
							BASS_ChannelLock(chan[devn ^ 1], TRUE); // lock source stream to synchonise buffer contents
							BASS_ChannelSetDSP(chan[devn ^ 1], CloneDSP, (void*)chan[devn], 0); // set DSP to feed data to clone
							{ // copy buffered data to clone
								DWORD d = BASS_ChannelSeconds2Bytes(chan[devn], info.latency / 1000.0); // playback delay
								DWORD c = BASS_ChannelGetData(chan[devn ^ 1], 0, BASS_DATA_AVAILABLE);
								BYTE *buf = (BYTE*)malloc(c);
								c = BASS_ChannelGetData(chan[devn ^ 1], buf, c);
								if (c > d) BASS_StreamPutData(chan[devn], buf + d, c - d);
								free(buf);
							}
							BASS_ChannelLock(chan[devn ^ 1], FALSE); // unlock source stream
							BASS_ChannelPlay(chan[devn], FALSE); // play clone
							MESS(10 + devn, WM_SETTEXT, 0, "clone");
						}
					}
					break;

				case 30: // swap channel devices
					{
						{ // swap handles
							HSTREAM temp = chan[0];
							chan[0] = chan[1];
							chan[1] = temp;
						}
						{ // swap text
							char temp1[MAX_PATH], temp2[MAX_PATH];
							MESS(10, WM_GETTEXT, MAX_PATH, temp1);
							MESS(11, WM_GETTEXT, MAX_PATH, temp2);
							MESS(10, WM_SETTEXT, 0, temp2);
							MESS(11, WM_SETTEXT, 0, temp1);
						}
						// update the channel devices
						BASS_ChannelSetDevice(chan[0], outdev[0]);
						BASS_ChannelSetDevice(chan[1], outdev[1]);
					}
					break;
			}
			break;

		case WM_INITDIALOG:
			win = h;
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = h;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			ofn.lpstrFilter = "Playable files\0*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx;*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0All files\0*.*\0\0";
			{ // get list of output devices
				int c;
				BASS_DEVICEINFO di;
				for (c = 0; BASS_GetDeviceInfo(c, &di); c++) {
					MESS(12, CB_ADDSTRING, 0, di.name);
					if (c == outdev[0]) MESS(12, CB_SETCURSEL, c, 0);
					MESS(13, CB_ADDSTRING, 0, di.name);
					if (c == outdev[1]) MESS(13, CB_SETCURSEL, c, 0);
				}
			}
			// initialize the output devices
			if (!BASS_Init(outdev[0], 44100, 0, win, NULL) || !BASS_Init(outdev[1], 44100, 0, win, NULL)) {
				Error("Can't initialize device");
				EndDialog(h, 0);
			}
			return 1;

		case WM_DESTROY:
			// release both devices
			BASS_SetDevice(outdev[0]);
			BASS_Free();
			BASS_SetDevice(outdev[1]);
			BASS_Free();
			break;
	}

	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		MessageBox(0, "An incorrect version of BASS.DLL was loaded", 0, MB_ICONERROR);
		return 0;
	}

	DialogBox(hInstance, MAKEINTRESOURCE(1000), NULL, DialogProc);

	return 0;
}

