/*
	BASS plugins example
	Copyright (c) 2005-2022 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <commctrl.h>
#include "bass.h"

HWND win;

DWORD chan;	// channel handle

OPENFILENAME ofn;
char filter[2000];

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	MessageBox(win, mes, 0, 0);
}

// translate a CTYPE value to text
const char *GetCTypeString(DWORD ctype, HPLUGIN plugin)
{
	if (plugin) { // using a plugin
		const BASS_PLUGININFO *pinfo = BASS_PluginGetInfo(plugin); // get plugin info
		int a;
		for (a = 0; a < pinfo->formatc; a++) {
			if (pinfo->formats[a].ctype == ctype) // found a "ctype" match...
				return pinfo->formats[a].name; // return its name
		}
	}
	// check built-in stream formats...
	if (ctype == BASS_CTYPE_STREAM_OGG) return "Ogg Vorbis";
	if (ctype == BASS_CTYPE_STREAM_MP1) return "MPEG layer 1";
	if (ctype == BASS_CTYPE_STREAM_MP2) return "MPEG layer 2";
	if (ctype == BASS_CTYPE_STREAM_MP3) return "MPEG layer 3";
	if (ctype == BASS_CTYPE_STREAM_AIFF) return "Audio IFF";
	if (ctype == BASS_CTYPE_STREAM_WAV_PCM) return "PCM WAVE";
	if (ctype == BASS_CTYPE_STREAM_WAV_FLOAT) return "Floating-point WAVE";
	if (ctype == BASS_CTYPE_STREAM_MF) { // a Media Foundation codec, check the format...
		const WAVEFORMATEX *wf = (const WAVEFORMATEX*)BASS_ChannelGetTags(chan, BASS_TAG_WAVEFORMAT);
		if (wf->wFormatTag == 0x1610) return "Advanced Audio Coding";
		if (wf->wFormatTag >= 0x0160 && wf->wFormatTag <= 0x0163) return "Windows Media Audio";
	}
	if (ctype & BASS_CTYPE_STREAM_WAV) // other WAVE codec, could use acmFormatTagDetails to get its name, but for now...
		return "WAVE";
	return "?";
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

				case 10:
					{
						char file[MAX_PATH] = "";
						ofn.lpstrFile = file;
						ofn.nMaxFile = MAX_PATH;
						if (GetOpenFileName(&ofn)) {
							BASS_StreamFree(chan); // free the old stream
							if (!(chan = BASS_StreamCreateFile(FALSE, file, 0, 0, BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT))) {
								MESS(10, WM_SETTEXT, 0, "Open file...");
								MESS(11, WM_SETTEXT, 0, "");
								MESS(13, WM_SETTEXT, 0, "");
								MESS(12, TBM_SETRANGEMAX, 1, 0);
								Error("Can't play the file");
								break;
							}
							MESS(10, WM_SETTEXT, 0, strrchr(file, '\\') + 1);
							{ // display the file type
								BASS_CHANNELINFO info;
								BASS_ChannelGetInfo(chan, &info);
								sprintf(file, "channel type = %x (%s)", info.ctype, GetCTypeString(info.ctype, info.plugin));
								MESS(11, WM_SETTEXT, 0, file);
							}
							{ // update scroller range
								QWORD len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);
								if (len == -1) len = 0; // unknown length
								MESS(12, TBM_SETRANGEMAX, 1, BASS_ChannelBytes2Seconds(chan, len) * 1000);
							}
							BASS_ChannelPlay(chan, FALSE);
						}
					}
					break;
			}
			break;

		case WM_HSCROLL:
			if (l && LOWORD(w) != SB_THUMBPOSITION && LOWORD(w) != SB_ENDSCROLL) { // set the position
				int pos = SendMessage((HWND)l, TBM_GETPOS, 0, 0);
				BASS_ChannelSetPosition(chan, BASS_ChannelSeconds2Bytes(chan, pos / 1000.0), BASS_POS_BYTE);
			}
			break;

		case WM_TIMER:
			if (chan) {
				char text[64];
				double len = BASS_ChannelBytes2Seconds(chan, BASS_ChannelGetLength(chan, BASS_POS_BYTE));
				double pos = BASS_ChannelBytes2Seconds(chan, BASS_ChannelGetPosition(chan, BASS_POS_BYTE));
				sprintf(text, "%u:%02u / %u:%02u", (int)pos / 60, (int)pos % 60, (int)len / 60, (int)len % 60);
				MESS(13, WM_SETTEXT, 0, text);
				MESS(12, TBM_SETPOS, 1, pos * 1000);
			}
			break;

		case WM_INITDIALOG:
			win = h;
			// initialize default output device
			if (!BASS_Init(-1, 44100, 0, win, NULL)) {
				Error("Can't initialize device");
				EndDialog(h, 0);
				break;
			}
			// initialize file selector
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = h;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			ofn.lpstrFilter = filter;
			memcpy(filter, "BASS built-in formats (*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif)\0*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0", 96);
			{ // look for plugins (alongside BASS library)
				char *fp = filter + 96;
				const char *basspath = BASS_GetConfigPtr(BASS_CONFIG_FILENAME);
				if (basspath) {
					WIN32_FIND_DATA fd;
					HANDLE fh;
					int pathlen = strrchr(basspath, '\\') + 1 - basspath;
					char *pattern = alloca(pathlen + 10);
					sprintf(pattern, "%.*sbass*.dll", pathlen, basspath);
					fh = FindFirstFile(pattern, &fd);
					if (fh != INVALID_HANDLE_VALUE) {
						do {
							HPLUGIN plug = BASS_PluginLoad(fd.cFileName, 0);
							if (plug) { // plugin loaded
								// get plugin info to add to the file selector filter
								const BASS_PLUGININFO *pinfo = BASS_PluginGetInfo(plug);
								int a;
								for (a = 0; a < pinfo->formatc; a++) {
									fp += sprintf(fp, "%s (%s) - %s", pinfo->formats[a].name, pinfo->formats[a].exts, fd.cFileName) + 1; // format description
									fp += sprintf(fp, "%s", pinfo->formats[a].exts) + 1; // extension filter
								}
								// add plugin to the list
								MESS(20, LB_ADDSTRING, 0, fd.cFileName);
							}
						} while (FindNextFile(fh, &fd));
						FindClose(fh);
					}
				}
				if (!MESS(20, LB_GETCOUNT, 0, 0))
					MESS(20, LB_ADDSTRING, 0, "no plugins - visit the BASS webpage to get some");
				// check if Media Foundation is available
				if (!BASS_GetConfig(BASS_CONFIG_MF_DISABLE)) {
					fp += sprintf(fp, "Media Foundation formats (*.aac;*.m4a;*.mp4;*.wma)") + 1;
					fp += sprintf(fp, "*.aac;*.m4a;*.mp4;*.wma") + 1;
				}
				memcpy(fp, "All files\0*.*\0\0", 15);
			}
			MESS(12, TBM_SETLINESIZE, 0, 1000);
			SetTimer(h, 0, 100, 0); // timer to update the position display
			return 1;

		case WM_DESTROY:
			BASS_Free();
			BASS_PluginFree(0); // unload all plugins
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
