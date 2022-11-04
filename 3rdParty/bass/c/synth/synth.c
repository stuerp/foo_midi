/*
	BASSMIDI synth
	Copyright (c) 2011-2022 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>
#include "bass.h"
#include "bassmidi.h"

HWND win;
HBRUSH brush[2];

BASS_INFO info;
DWORD input;		// MIDI input device
HSTREAM stream;		// output stream
HSOUNDFONT font;	// soundfont
DWORD preset;		// current preset
BOOL drums;			// drums enabled?
BOOL preload;		// preload samples?
BOOL chans16;		// 16 MIDI channels?
BOOL activity;
DWORD velocity = 110;
DWORD octave = 4;

const DWORD fxtype[5] = { BASS_FX_DX8_REVERB,BASS_FX_DX8_ECHO,BASS_FX_DX8_CHORUS,BASS_FX_DX8_FLANGER,BASS_FX_DX8_DISTORTION };
HFX fx[5];			// effect handles

#define KEYS 20
const WORD keys[KEYS] = {
	'Q','2','W','3','E','R','5','T','6','Y','7','U',
	'I','9','O','0','P',219,187,221
};

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	MessageBox(win, mes, 0, 0);
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))
#define DLGITEM(id) GetDlgItem(win,id)

VOID CALLBACK ActivityTimerProc(HWND h, UINT msg, UINT_PTR id, DWORD time)
{
	KillTimer(h, id);
	activity = 0;
	InvalidateRect(DLGITEM(11), 0, 1);
}

// MIDI input function
void CALLBACK MidiInProc(DWORD handle, double time, const BYTE *buffer, DWORD length, void *user)
{
	if (chans16) // using 16 channels
		BASS_MIDI_StreamEvents(stream, BASS_MIDI_EVENTS_RAW, buffer, length); // send MIDI data to the MIDI stream
	else
		BASS_MIDI_StreamEvents(stream, (BASS_MIDI_EVENTS_RAW + 17) | BASS_MIDI_EVENTS_SYNC, buffer, length); // send MIDI data to channel 17 in the MIDI stream
	activity = 1;
	InvalidateRect(DLGITEM(11), 0, 1);
	SetTimer(win, 1, 100, ActivityTimerProc);
}

// program/preset event sync function
void CALLBACK ProgramEventSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	preset = LOWORD(data);
	MESS(42, CB_SETCURSEL, preset, 0); // update the preset selector
	BASS_MIDI_FontCompact(0); // unload unused samples
}

void UpdatePresetList()
{
	int a;
	MESS(42, CB_RESETCONTENT, 0, 0);
	for (a = 0; a < 128; a++) {
		char text[60];
		const char *name = BASS_MIDI_FontGetPreset(font, a, drums ? 128 : 0); // get preset name
		_snprintf(text, sizeof(text), "%03d: %s", a, name ? name : "");
		MESS(42, CB_ADDSTRING, 0, text);
	}
	MESS(42, CB_SETCURSEL, preset, 0);
}

INT_PTR CALLBACK dialogproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					PostQuitMessage(0);
					break;
				case 10:
					if (HIWORD(w) == CBN_SELCHANGE) {
						BASS_MIDI_InFree(input); // free current input device
						input = MESS(10, CB_GETCURSEL, 0, 0); // get new input device selection
						if (BASS_MIDI_InInit(input, MidiInProc, 0)) // successfully initialized...
							BASS_MIDI_InStart(input); // start it
						else
							Error("Can't initialize MIDI device");
					}
					break;
				case 12:
				case 13:
					chans16 = MESS(13, BM_GETCHECK, 0, 0); // MIDI input channels
					break;
				case 15: // reset
					BASS_MIDI_StreamEvent(stream, 0, MIDI_EVENT_SYSTEM, MIDI_SYSTEM_GS); // send system reset event
					if (drums) BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_DRUMS, drums); // send drum switch event
					BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_PROGRAM, preset); // send program/preset event
					break;
				case 21:
					if (HIWORD(w) == CBN_SELCHANGE) {
						octave = MESS(21, CB_GETCURSEL, 0, 0);
						BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_NOTESOFF, 0);
					}
					break;
				case 22:
					if (HIWORD(w) == EN_CHANGE) {
						int v = GetDlgItemInt(h, 22, NULL, FALSE);
						if (v > 0) {
							if (v > 127) SetDlgItemInt(h, 22, v = 127, FALSE);
							velocity = v;
						}
					}
					break;
				case 30:
					{ // send GS reverb type macro
						int n = MESS(30, CB_GETCURSEL, 0, 0);
						BASS_MIDI_StreamEvent(stream, 0, MIDI_EVENT_REVERB_MACRO, n ? 0x8000 + n - 1 : 0);
					}
					break;
				case 32:
					{ // send GS chorus type macro
						int n = MESS(32, CB_GETCURSEL, 0, 0);
						BASS_MIDI_StreamEvent(stream, 0, MIDI_EVENT_CHORUS_MACRO, n ? 0x8000 + n - 1 : 0);
					}
					break;
				case 35:
				case 36:
				case 37:
				case 38:
				case 39:
					{ // toggle DX8 effects
						int n = LOWORD(w) - 35;
						if (fx[n]) {
							BASS_ChannelRemoveFX(stream, fx[n]);
							fx[n] = 0;
						} else
							fx[n] = BASS_ChannelSetFX(stream, fxtype[n], n);
					}
					break;
				case 40:
					{
						char file[MAX_PATH] = "";
						OPENFILENAME ofn = { 0 };
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = h;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
						ofn.lpstrFilter = "Soundfonts (sf2/sf2pack/sf3/sfz)\0*.sf2;*.sf2pack;*.sf3;*.sfz\0All files\0*.*\0\0";
						ofn.lpstrFile = file;
						if (GetOpenFileName(&ofn)) {
							HSOUNDFONT newfont = BASS_MIDI_FontInit(file, 0);
							if (newfont) {
								BASS_MIDI_FONT sf;
								sf.font = newfont;
								sf.preset = -1; // use all presets
								sf.bank = 0; // use default bank(s)
								BASS_MIDI_StreamSetFonts(0, &sf, 1); // set default soundfont
								BASS_MIDI_StreamSetFonts(stream, &sf, 1); // apply to current stream too
								BASS_MIDI_FontFree(font); // free old soundfont
								font = newfont;
								{
									BASS_MIDI_FONTINFO i;
									BASS_MIDI_FontGetInfo(font, &i);
									MESS(40, WM_SETTEXT, 0, i.name ? i.name : strrchr(file, '\\') + 1);
									if (i.presets == 1) { // only 1 preset, auto-select it...
										DWORD p;
										BASS_MIDI_FontGetPresets(font, &p);
										drums = (HIWORD(p) == 128); // bank 128 = drums
										preset = LOWORD(p);
										MESS(44, BM_SETCHECK, drums, 0);
										BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_DRUMS, drums); // send drum switch event
										BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_PROGRAM, preset); // send program/preset event
										EnableWindow(DLGITEM(42), FALSE);
										EnableWindow(DLGITEM(44), FALSE);
									} else {
										EnableWindow(DLGITEM(42), TRUE);
										EnableWindow(DLGITEM(44), TRUE);
									}
								}
								UpdatePresetList();
								if (preload) BASS_MIDI_FontLoadEx(font, preset, drums ? 128 : 0, 50, BASS_MIDI_FONTLOAD_NOWAIT | BASS_MIDI_FONTLOAD_TIME); // preload 50ms of current preset
							} else
								Error("The file isn't a valid soundfont");
						}
					}
					break;
				case 42:
					if (HIWORD(w) == CBN_SELCHANGE) {
						preset = MESS(42, CB_GETCURSEL, 0, 0);
						BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_PROGRAM, preset); // send program/preset event
						BASS_MIDI_FontCompact(0); // unload unused samples
						if (preload) BASS_MIDI_FontLoadEx(font, preset, drums ? 128 : 0, 50, BASS_MIDI_FONTLOAD_NOWAIT | BASS_MIDI_FONTLOAD_TIME); // preload 50ms of current preset
					}
					break;
				case 44:
					drums = MESS(44, BM_GETCHECK, 0, 0);
					BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_DRUMS, drums); // send drum switch event
					BASS_MIDI_StreamEvents(stream, BASS_MIDI_EVENTS_FLUSH, 0, 0); // process pending events before GetEvent
					preset = BASS_MIDI_StreamGetEvent(stream, 16, MIDI_EVENT_PROGRAM); // preset is reset in drum switch
					UpdatePresetList();
					BASS_MIDI_FontCompact(0); // unload unused samples
					if (preload) BASS_MIDI_FontLoadEx(font, preset, drums ? 128 : 0, 50, BASS_MIDI_FONTLOAD_NOWAIT | BASS_MIDI_FONTLOAD_TIME); // preload 50ms of current preset
					break;
				case 46:
					preload = MESS(46, BM_GETCHECK, 0, 0);
					if (preload) BASS_MIDI_FontLoadEx(font, preset, drums ? 128 : 0, 50, BASS_MIDI_FONTLOAD_NOWAIT | BASS_MIDI_FONTLOAD_TIME); // preload 50ms of current preset
					break;
				case 50:
					BASS_ChannelSetAttribute(stream, BASS_ATTRIB_MIDI_SRC, 0); // 2 point linear interpolation
					break;
				case 51:
					BASS_ChannelSetAttribute(stream, BASS_ATTRIB_MIDI_SRC, 1); // 8 point sinc interpolation
					break;
				case 52:
					BASS_ChannelSetAttribute(stream, BASS_ATTRIB_MIDI_SRC, 2); // 16 point sinc interpolation
					break;
			}
			break;

		case WM_HSCROLL:
			if (l) {
				DWORD p = SendMessage((HWND)l, TBM_GETPOS, 0, 0);
				switch (GetDlgCtrlID((HWND)l)) {
					case 31:
						// reverb level
						BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_REVERB, p);
						break;
					case 33:
						// chorus level
						BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_CHORUS, p);
						break;
				}
			}
			break;

		case WM_TIMER:
			{
				// display loaded sample data
				char text[40];
				DWORD loaded = 0;
				BASS_MIDI_FONTINFO i;
				if (BASS_MIDI_FontGetInfo(font, &i))
					loaded = (i.samload + 1023) / 1024;
				sprintf(text, "Loaded: %u KB", loaded);
				MESS(45, WM_SETTEXT, 0, text);
			}
			break;

		case WM_CTLCOLORSTATIC:
			{
				int id = GetDlgCtrlID((HWND)l);
				if (id == 11) {
					SetBkMode((HDC)w, TRANSPARENT);
					return (BOOL)brush[activity ? 1 : 0];
				}
			}
			return 0;

		case WM_INITDIALOG:
			win = h;
			// initialize default output device
			if (!BASS_Init(-1, 44100, 0, win, NULL)) {
				Error("Can't initialize output device");
				DestroyWindow(win);
				break;
			}
			BASS_GetInfo(&info);
			stream = BASS_MIDI_StreamCreate(17, BASS_MIDI_ASYNC | BASS_SAMPLE_FLOAT, 1); // create the MIDI stream with async processing and 16 MIDI channels for device input + 1 for keyboard input
			BASS_ChannelSetAttribute(stream, BASS_ATTRIB_BUFFER, 0); // no buffering for minimum latency
			BASS_ChannelSetSync(stream, BASS_SYNC_MIDI_EVENT | BASS_SYNC_MIXTIME, MIDI_EVENT_PROGRAM, ProgramEventSync, 0); // catch program/preset changes
			BASS_MIDI_StreamEvent(stream, 0, MIDI_EVENT_SYSTEM, MIDI_SYSTEM_GS); // send GS system reset event
			BASS_ChannelPlay(stream, 0); // start it
			{ // enumerate available input devices
				BASS_MIDI_DEVICEINFO di;
				int dev;
				for (dev = 0; BASS_MIDI_InGetDeviceInfo(dev, &di); dev++)
					MESS(10, CB_ADDSTRING, 0, di.name);
				if (dev) { // got sone, try to initialize one
					int a;
					for (a = 0; a < dev; a++) {
						if (BASS_MIDI_InInit(a, MidiInProc, 0)) { // succeeded, start it
							input = a;
							BASS_MIDI_InStart(input);
							MESS(10, CB_SETCURSEL, input, 0);
							break;
						}
					}
					if (a == dev) Error("Can't initialize MIDI device");
				} else {
					MESS(10, CB_ADDSTRING, 0, "no devices");
					MESS(10, CB_SETCURSEL, 0, 0);
					EnableWindow(DLGITEM(10), FALSE);
					EnableWindow(DLGITEM(12), FALSE);
					EnableWindow(DLGITEM(13), FALSE);
				}
				MESS(12, BM_SETCHECK, BST_CHECKED, 0);
				MESS(50, BM_SETCHECK, BST_CHECKED, 0);
			}
			{ // get default font (28mbgm.sf2/ct8mgm.sf2/ct4mgm.sf2/ct2mgm.sf2 if available)
				BASS_MIDI_FONT sf;
				BASS_MIDI_FONTINFO i;
				if (BASS_MIDI_StreamGetFonts(0, &sf, 1)) {
					font = sf.font;
					BASS_MIDI_FontGetInfo(font, &i);
					MESS(40, WM_SETTEXT, 0, i.name);
				}
			}
			UpdatePresetList();
			{
				HFONT font = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, FIXED_PITCH, "Courier New");
				MESS(20, WM_SETFONT, font, 0);
			}
			{
				int a;
				char text[32];
				for (a = 0; a < 10; a++) {
					sprintf(text, "%d", a * 12);
					MESS(21, CB_ADDSTRING, 0, text);
				}
				MESS(21, CB_SETCURSEL, octave, 0);
				SetDlgItemInt(h, 22, velocity, FALSE);
			}
			MESS(30, CB_ADDSTRING, 0, "Off");
			MESS(30, CB_ADDSTRING, 0, "Room 1");
			MESS(30, CB_ADDSTRING, 0, "Room 2");
			MESS(30, CB_ADDSTRING, 0, "Room 3");
			MESS(30, CB_ADDSTRING, 0, "Hall 1");
			MESS(30, CB_ADDSTRING, 0, "Hall 2");
			MESS(30, CB_ADDSTRING, 0, "Plate");
			MESS(30, CB_SETCURSEL, 5, 0);
			MESS(31, TBM_SETRANGE, FALSE, MAKELONG(0, 127));
			MESS(31, TBM_SETPOS, TRUE, 40);
			MESS(32, CB_ADDSTRING, 0, "Off");
			MESS(32, CB_ADDSTRING, 0, "Chorus 1");
			MESS(32, CB_ADDSTRING, 0, "Chorus 2");
			MESS(32, CB_ADDSTRING, 0, "Chorus 3");
			MESS(32, CB_ADDSTRING, 0, "Chorus 4");
			MESS(32, CB_ADDSTRING, 0, "Feedback Chorus");
			MESS(32, CB_ADDSTRING, 0, "Flanger");
			MESS(32, CB_ADDSTRING, 0, "Short Delay");
			MESS(32, CB_ADDSTRING, 0, "Short Delay (FB)");
			MESS(32, CB_SETCURSEL, 3, 0);
			MESS(33, TBM_SETRANGE, FALSE, MAKELONG(0, 127));
			MESS(55, TBM_SETRANGE, FALSE, MAKELONG(0, 50));
			MESS(55, TBM_SETPOS, TRUE, 1);
			MESS(34, BM_SETCHECK, BST_CHECKED, 0);
			MESS(23, UDM_SETRANGE, 0, MAKELONG(127, 1));
			brush[0] = CreateSolidBrush(0xffffff);
			brush[1] = CreateSolidBrush(0x00ff00);
			SetTimer(h, 0, 1000, 0); // timer to update the loaded data display
			// load optional plugins for packed soundfonts (others may be used too)
			BASS_PluginLoad("bassflac.dll", 0);
			BASS_PluginLoad("basswv.dll", 0);
			BASS_PluginLoad("bassopus.dll", 0);
			return 1;

		case WM_DESTROY:
			// release everything
			BASS_MIDI_InFree(input);
			BASS_Free();
			BASS_PluginFree(0);
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

	win = CreateDialog(hInstance, MAKEINTRESOURCE(1000), 0, dialogproc);
	ShowWindow(win, SW_SHOW);
	{
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			if ((msg.message == WM_KEYDOWN && !(msg.lParam & 0x40000000)) || msg.message == WM_KEYUP) {
				int key;
				for (key = 0; key < KEYS; key++) {
					if (msg.wParam == keys[key]) {
						// send note on/off event
						BASS_MIDI_StreamEvent(stream, 16, MIDI_EVENT_NOTE, MAKEWORD(octave * 12 + key, msg.message == WM_KEYDOWN ? velocity : 0));
						break;
					}
				}
			}
			if (!IsDialogMessage(win, &msg))
				DispatchMessage(&msg);
		}
	}

	return 0;
}
