/*
	BASS full-duplex exmple
	Copyright (c) 2002-2021 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>
#include "bass.h"

HWND win;

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))
#define DLGITEM(id) GetDlgItem(win,id)

const DWORD fxtype[4] = { BASS_FX_DX8_REVERB, BASS_FX_DX8_GARGLE, BASS_FX_DX8_FLANGER, BASS_FX_DX8_CHORUS };

BASS_INFO info;
HRECORD rchan;		// recording channel
HSTREAM chan;		// playback stream
HFX fx[4];			// FX handles
int input;			// current input source
float volume = 0.5;	// volume level

#define DEFAULTRATE 44100
#define TARGETBUFS	2	// managed buffer level target (higher = more safety margin + higher latency)

BOOL prebuf;		// prebuffering
DWORD initrate;		// initial output rate
DWORD rate;			// current output rate
DWORD ratedelay;	// rate change delay
DWORD buftarget;	// target buffer amount
float buflevel;		// current/average buffer amount

void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	MessageBox(win, mes, 0, 0);
}

// STREAMPROC that pulls data from the recording
DWORD CALLBACK StreamProc(HSTREAM handle, void *buffer, DWORD length, void *user)
{
	DWORD got = BASS_ChannelGetData(rchan, NULL, BASS_DATA_AVAILABLE); // get recording buffer level
	buflevel = (buflevel * 49 + got) / 50.0;
	if (!got) prebuf = TRUE; // prebuffer again if buffer is empty
	if (prebuf) { // prebuffering
		buflevel = got;
		if (got < (buftarget ? buftarget : length)) return 0; // haven't got enough yet
		prebuf = FALSE;
		ratedelay = 10;
	}
	{
		DWORD r = rate;
		if (buftarget) {
#if 1 // target buffer amount = minimum
			if (got < buftarget) { // buffer level is low
				r = initrate * 0.999; // slow down slightly
				ratedelay = 10 + (buftarget - got) / 16; // prevent speeding-up again too soon
			} else if (!ratedelay) {
				if (buflevel >= buftarget * 1.1) // buffer level is high
					r = initrate * 1.001; // speed up slightly
				else // buffer level is in range
					r = initrate; // normal speed
			} else
				ratedelay--;
#else // target buffer amount = average
			if (buflevel < buftarget) // buffer level is low
				r = initrate * 0.999; // slow down slightly
			else if (buflevel >= buftarget * 1.1) // buffer level is high
				r = initrate * 1.001; // speed up slightly
			else // buffer level is in range
				r = initrate; // normal speed
#endif
		} else
			r = initrate;
		if (r != rate) BASS_ChannelSetAttribute(chan, BASS_ATTRIB_FREQ, rate = r);
	}
	return BASS_ChannelGetData(rchan, buffer, length); // get the data
}

BOOL InitDevice(int device)
{
	if (chan) BASS_StreamFree(chan); // free output stream
	BASS_RecordFree(); // free current device (and recording channel) if there is one
	rchan = 0;
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
	}
	{
		BASS_RECORDINFO rinfo;
		BASS_RecordGetInfo(&rinfo);
		rate = initrate = rinfo.freq ? rinfo.freq : DEFAULTRATE; // use the native recording rate (if available)
	}
	if (!chan) {
		// start output immediately to avoid needing a burst of data at start of stream
		BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, 1);
		// initialize default output device
		if (!BASS_Init(-1, rate, 0, win, NULL)) {
			Error("Can't initialize output device");
			BASS_RecordFree();
			return FALSE;
		}
		BASS_GetInfo(&info);
		if (info.dsver < 8) {
			// no DX8, so disable effect buttons
			EnableWindow(DLGITEM(20), FALSE);
			EnableWindow(DLGITEM(21), FALSE);
			EnableWindow(DLGITEM(22), FALSE);
			EnableWindow(DLGITEM(23), FALSE);
		}
	}

	chan = BASS_StreamCreate(rate, 2, BASS_SAMPLE_FLOAT, StreamProc, 0); // create output stream
	BASS_ChannelSetAttribute(chan, BASS_ATTRIB_BUFFER, 0); // disable playback buffering
	BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume); // set volume level
	{ // set effects
		int a;
		for (a = 0; a < 4; a++) {
			if (MESS(20 + a, BM_GETCHECK, 0, 0))
				fx[a] = BASS_ChannelSetFX(chan, fxtype[a], a);
			else
				fx[a] = 0;
		}
	}

	// don't need/want a big recording buffer
	BASS_SetConfig(BASS_CONFIG_REC_BUFFER, (TARGETBUFS + 3) * info.minbuf);

	// record without a RECORDPROC so output stream can pull data from it
	rchan = BASS_RecordStart(rate, 2, BASS_SAMPLE_FLOAT, NULL, 0);
	if (!rchan) {
		Error("Can't start recording");
		BASS_RecordFree();
		BASS_StreamFree(chan);
		return FALSE;
	}
	if (MESS(15, BM_GETCHECK, 0, 0))
		buftarget = BASS_ChannelSeconds2Bytes(rchan, TARGETBUFS * info.minbuf / 1000.0); // target buffer level
	else
		buftarget = 0;
	prebuf = TRUE; // start prebuffering
	BASS_ChannelPlay(chan, FALSE); // start the output

	return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_TIMER:
			if (rchan) { // display current buffer level
				char buf[30];
				sprintf(buf, "%dms", (int)(BASS_ChannelBytes2Seconds(rchan, buflevel) * 1000));
				MESS(18, WM_SETTEXT, 0, buf);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					EndDialog(h, 0);
					break;

				case 13:
					if (HIWORD(w) == CBN_SELCHANGE) { // input selection changed
						int i;
						input = MESS(13, CB_GETCURSEL, 0, 0); // get the selection
						for (i = 0; BASS_RecordSetInput(i, BASS_INPUT_OFF, -1); i++); // 1st disable all inputs, then...
						BASS_RecordSetInput(input, BASS_INPUT_ON, -1); // enable the selected
					}
					break;

				case 16:
					if (HIWORD(w) == CBN_SELCHANGE) { // device selection changed
						int i = MESS(16, CB_GETCURSEL, 0, 0); // get the selection
						// initialize the selected device
						InitDevice(i);
					}
					break;

				case 15:
					if (MESS(15, BM_GETCHECK, 0, 0))
						buftarget = BASS_ChannelSeconds2Bytes(rchan, TARGETBUFS * info.minbuf / 1000.0); // target buffer level
					else
						buftarget = 0;
					break;

				case 20: // toggle chorus
				case 21: // toggle gargle
				case 22: // toggle reverb
				case 23: // toggle flanger
					{
						int n = LOWORD(w) - 20;
						if (fx[n]) {
							BASS_ChannelRemoveFX(chan, fx[n]);
							fx[n] = 0;
						} else
							fx[n] = BASS_ChannelSetFX(chan, fxtype[n], n);
					}
					break;
			}
			break;

		case WM_HSCROLL:
			if (l) {
				int pos = SendMessage((HWND)l, TBM_GETPOS, 0, 0);
				volume = pos / 100.f;
				BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume);
			}
			break;

		case WM_INITDIALOG:
			win = h;
			MessageBox(win, "Setting the input to the output loopback (or 'Stereo Mix' or similar on the same soundcard) "
				"with the level set high is likely to result in nasty feedback.", "Feedback warning", MB_ICONWARNING);
			{ // get list of recording devices
				int c, def;
				BASS_DEVICEINFO di;
				for (c = 0; BASS_RecordGetDeviceInfo(c, &di); c++) {
					MESS(16, CB_ADDSTRING, 0, di.name);
					if (di.flags & BASS_DEVICE_DEFAULT) { // got the default device
						MESS(16, CB_SETCURSEL, c, 0);
						def = c;
					}
				}
				InitDevice(def); // initialize default recording device
			}
			MESS(14, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
			MESS(14, TBM_SETPOS, TRUE, volume * 100);
			SetTimer(h, 0, 500, 0); // timer to update the buffer display
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
		INITCOMMONCONTROLSEX cc = { sizeof(cc), ICC_BAR_CLASSES };
		InitCommonControlsEx(&cc);
	}

	DialogBox(hInstance, MAKEINTRESOURCE(1000), NULL, DialogProc);

	return 0;
}
