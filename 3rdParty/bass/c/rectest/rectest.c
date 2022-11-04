/*
	BASS recording example
	Copyright (c) 2002-2022 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "bass.h"

HWND win;

#define BUFSTEP 1000000	// memory allocation unit

int input;				// current input source
int freq, chans;		// sample format
float volume = 1;		// recording level adjustment
BYTE *recbuf;			// recording buffer
DWORD reclen;			// recording length
HRECORD rchan;			// recording channel
HSTREAM pchan;			// playback channel

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	MessageBox(win, mes, 0, 0);
}

// messaging macros
#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))
#define DLGITEM(id) GetDlgItem(win,id)

// buffer the recorded data
BOOL CALLBACK RecordingCallback(HRECORD handle, const void *buffer, DWORD length, void *user)
{
	// increase buffer size if needed
	if ((reclen % BUFSTEP) + length >= BUFSTEP) {
		void *newbuf = realloc(recbuf, ((reclen + length) / BUFSTEP + 1) * BUFSTEP);
		if (!newbuf) {
			rchan = 0;
			free(recbuf);
			recbuf = NULL;
			Error("Out of memory!");
			MESS(10, WM_SETTEXT, 0, "Record");
			EnableWindow(DLGITEM(17), TRUE);
			return FALSE; // stop recording
		}
		recbuf = (BYTE*)newbuf;
	}
	// buffer the data
	memcpy(recbuf + reclen, buffer, length);
	reclen += length;
	return TRUE; // continue recording
}

void StartRecording()
{
	WAVEFORMATEX *wf;
	if (recbuf) { // free old recording
		BASS_StreamFree(pchan);
		pchan = 0;
		free(recbuf);
		recbuf = NULL;
		EnableWindow(DLGITEM(11), FALSE);
		EnableWindow(DLGITEM(12), FALSE);
	}
	{ // get selected sample format
		int format = MESS(17, CB_GETCURSEL, 0, 0);
		freq = format > 3 ? 22050 : format > 1 ? 44100 : 48000;
		chans = 1 + (format & 1);
	}
	// allocate initial buffer and make space for WAVE header
	recbuf = (BYTE*)malloc(BUFSTEP);
	reclen = 44;
	// fill the WAVE header
	memcpy(recbuf, "RIFF\0\0\0\0WAVEfmt \20\0\0\0", 20);
	memcpy(recbuf + 36, "data\0\0\0\0", 8);
	wf = (WAVEFORMATEX*)(recbuf + 20);
	wf->wFormatTag = 1;
	wf->nChannels = chans;
	wf->wBitsPerSample = 16;
	wf->nSamplesPerSec = freq;
	wf->nBlockAlign = wf->nChannels * wf->wBitsPerSample / 8;
	wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
	// start recording (paused to set VOLDSP first)
	rchan = BASS_RecordStart(freq, chans, BASS_RECORD_PAUSE, RecordingCallback, NULL);
	if (!rchan) {
		Error("Can't start recording");
		free(recbuf);
		recbuf = 0;
		return;
	}
	BASS_ChannelSetAttribute(rchan, BASS_ATTRIB_VOLDSP, volume);
	BASS_ChannelStart(rchan); // resume recording
	MESS(10, WM_SETTEXT, 0, "Stop");
	EnableWindow(DLGITEM(17), FALSE);
}

void StopRecording()
{
	BASS_ChannelStop(rchan);
	rchan = 0;
	// complete the WAVE header
	*(DWORD*)(recbuf + 4) = reclen - 8;
	*(DWORD*)(recbuf + 40) = reclen - 44;
	// enable "save" button
	EnableWindow(DLGITEM(12), TRUE);
	// create a stream from the recording
	if (pchan = BASS_StreamCreateFile(TRUE, recbuf, 0, reclen, 0))
		EnableWindow(DLGITEM(11), TRUE); // enable "play" button
	MESS(10, WM_SETTEXT, 0, "Record");
	EnableWindow(DLGITEM(17), TRUE);
}

void WriteToDisk()
{
	FILE *fp;
	char file[MAX_PATH] = "";
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = win;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFile = file;
	ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
	ofn.lpstrFilter = "WAV files\0*.wav\0All files\0*.*\0\0";
	ofn.lpstrDefExt = "wav";
	if (!GetSaveFileName(&ofn)) return;
	if (!(fp = fopen(file, "wb"))) {
		Error("Can't create the file");
		return;
	}
	fwrite(recbuf, reclen, 1, fp);
	fclose(fp);
}

void UpdateInputInfo()
{
	float level;
	int it = BASS_RecordGetInput(input, &level); // get info on the input
	if (it == -1 || level < 0) { // failed to get level
		it = BASS_RecordGetInput(-1, &level); // try master input instead
		if (it == -1 || level < 0) { // that failed too
			level = 1; // just display 100%
			EnableWindow(DLGITEM(14), FALSE);
		} else
			EnableWindow(DLGITEM(14), TRUE);
	} else
		EnableWindow(DLGITEM(14), TRUE);
	MESS(14, TBM_SETPOS, TRUE, level * 100); // set the level slider
}

BOOL InitDevice(int device)
{
	BASS_RecordFree(); // free current device (and recording channel) if there is one
	// initalize new device
	if (!BASS_RecordInit(device)) {
		Error("Can't initialize recording device");
		return FALSE;
	}
	{ // get list of inputs
		int c;
		const char *name;
		MESS(13, CB_RESETCONTENT, 0, 0);
		input = 0;
		for (c = 0; name = BASS_RecordGetInputName(c); c++) {
			MESS(13, CB_ADDSTRING, 0, name);
			if (!(BASS_RecordGetInput(c, NULL) & BASS_INPUT_OFF)) { // this one is currently "on"
				input = c;
				MESS(13, CB_SETCURSEL, input, 0);
			}
		}
		EnableWindow(DLGITEM(13), c > 0);
		UpdateInputInfo();
	}
	return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_TIMER:
			{ // update the level display
				float level = 0;
				if (rchan || pchan) {
					BASS_ChannelGetLevelEx(rchan ? rchan : pchan, &level, 0.1, BASS_LEVEL_MONO); // get current level
					if (rchan) level *= volume; // apply recording level adjustment
					if (level > 0) {
						level = 1 + 0.5 * log10(level); // convert to dB (40dB range)
						if (level < 0) level = 0;
						if (level > 1) level = 1;
					}
				}
				MESS(21, PBM_SETPOS, level * 100, 0);
			}
			{ // update the recording/playback counter
				char text[30] = "";
				if (rchan) { // recording
					if (rchan != 1 && !BASS_ChannelIsActive(rchan)) { // the recording has stopped, eg. unplugged device
						StopRecording();
						Error("The recording stopped");
						break;
					}
					sprintf(text, "%d", reclen - 44);
				} else if (pchan) {
					if (BASS_ChannelIsActive(pchan)) // playing
						sprintf(text, "%I64d / %I64d", BASS_ChannelGetPosition(pchan, BASS_POS_BYTE), BASS_ChannelGetLength(pchan, BASS_POS_BYTE));
					else
						sprintf(text, "%I64d", BASS_ChannelGetLength(pchan, BASS_POS_BYTE));
				}
				MESS(20, WM_SETTEXT, 0, text);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					EndDialog(h, 0);
					break;

				case 10:
					if (!rchan)
						StartRecording();
					else
						StopRecording();
					break;

				case 11:
					BASS_ChannelPlay(pchan, TRUE); // play the recorded data
					break;

				case 12:
					WriteToDisk();
					break;

				case 13:
					if (HIWORD(w) == CBN_SELCHANGE) { // input selection changed
						int i;
						input = MESS(13, CB_GETCURSEL, 0, 0); // get the selection
						for (i = 0; BASS_RecordSetInput(i, BASS_INPUT_OFF, -1); i++); // 1st disable all inputs, then...
						BASS_RecordSetInput(input, BASS_INPUT_ON, -1); // enable the selected
						UpdateInputInfo();
					}
					break;

				case 16:
					if (HIWORD(w) == CBN_SELCHANGE) { // device selection changed
						int i = MESS(16, CB_GETCURSEL, 0, 0); // get the selection
						if (rchan) rchan = 1; // special handle (real handles always have highest bit set) to prevent timer ending the recording
						// initialize the selected device
						if (InitDevice(i)) {
							if (rchan) { // continue recording on the new device...
								HRECORD newrchan = BASS_RecordStart(freq, chans, BASS_RECORD_PAUSE, RecordingCallback, NULL);
								if (!newrchan) {
									Error("Can't start recording");
									break;
								}
								rchan = newrchan;
								BASS_ChannelSetAttribute(rchan, BASS_ATTRIB_VOLDSP, volume);
								BASS_ChannelStart(rchan);
							}
						}
					}
					break;
			}
			break;

		case WM_HSCROLL:
			if (l) {
				float level = SendMessage((HWND)l, TBM_GETPOS, 0, 0) / 100.f;
				switch (GetDlgCtrlID((HWND)l)) {
					case 14:
						if (!BASS_RecordSetInput(input, 0, level)) // set input source level
							BASS_RecordSetInput(-1, 0, level); // try master level instead
						break;

					case 15:
						BASS_ChannelSetAttribute(rchan, BASS_ATTRIB_VOLDSP, volume = level); // set recording level adjustment
						break;
				}
			}
			break;

		case WM_INITDIALOG:
			win = h;
			MESS(14, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
			MESS(15, TBM_SETRANGE, FALSE, MAKELONG(0, 200));
			MESS(15, TBM_SETPOS, TRUE, 100);
			// initialize default output device
			if (!BASS_Init(-1, 48000, 0, win, NULL))
				Error("Can't initialize output device");
			{ // get list of recording devices
				int c;
				BASS_DEVICEINFO di;
				for (c = 0; BASS_RecordGetDeviceInfo(c, &di); c++) {
					if (di.flags & BASS_DEVICE_LOOPBACK) {
						char name[100];
						_snprintf(name, sizeof(name) - 1, "loopback: %s", di.name);
						MESS(16, CB_ADDSTRING, 0, name);
					} else
						MESS(16, CB_ADDSTRING, 0, di.name);
				}
				MESS(16, CB_SETCURSEL, 0, 0);
				InitDevice(0); // initialize "Default" recording device
			}
			MESS(17, CB_ADDSTRING, 0, "48000 Hz mono 16-bit");
			MESS(17, CB_ADDSTRING, 0, "48000 Hz stereo 16-bit");
			MESS(17, CB_ADDSTRING, 0, "44100 Hz mono 16-bit");
			MESS(17, CB_ADDSTRING, 0, "44100 Hz stereo 16-bit");
			MESS(17, CB_ADDSTRING, 0, "22050 Hz mono 16-bit");
			MESS(17, CB_ADDSTRING, 0, "22050 Hz stereo 16-bit");
			MESS(17, CB_SETCURSEL, 3, 0);
			SetTimer(h, 0, 100, 0); // timer to update the level and position displays
			return 1;

		case WM_DESTROY:
			// release all BASS stuff
			BASS_RecordFree();
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

	{
		INITCOMMONCONTROLSEX cc = { sizeof(cc), ICC_BAR_CLASSES | ICC_PROGRESS_CLASS };
		InitCommonControlsEx(&cc);
	}

	DialogBox(hInstance, MAKEINTRESOURCE(1000), NULL, DialogProc);

	return 0;
}
