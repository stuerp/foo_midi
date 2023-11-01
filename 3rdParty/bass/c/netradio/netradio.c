/*
	BASS internet radio example
	Copyright (c) 2002-2022 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include "bass.h"

// HLS definitions (copied from BASSHLS.H)
#define BASS_SYNC_HLS_SEGMENT	0x10300
#define BASS_TAG_HLS_EXTINF		0x14000

HWND win;
CRITICAL_SECTION lock;
DWORD req;	// request number/counter
HSTREAM chan;	// stream handle

const char *urls[10] = { // preset stream URLs
	"http://stream-dc1.radioparadise.com/rp_192m.ogg", "http://www.radioparadise.com/m3u/mp3-32.m3u",
	"http://somafm.com/secretagent.pls", "http://somafm.com/secretagent32.pls",
	"http://somafm.com/suburbsofgoa.pls", "http://somafm.com/suburbsofgoa32.pls",
	"http://bassdrive.com/bassdrive.m3u", "http://bassdrive.com/bassdrive3.m3u",
	"http://sc6.radiocaroline.net:8040/listen.pls", "http://sc2.radiocaroline.net:8010/listen.pls"
};

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	MessageBox(win, mes, 0, 0);
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

// update stream title from metadata
void DoMeta()
{
	const char *meta = BASS_ChannelGetTags(chan, BASS_TAG_META);
	if (meta) { // got Shoutcast metadata
		const char *p = strstr(meta, "StreamTitle='"); // locate the title
		if (p) {
			const char *p2 = strstr(p, "';"); // locate the end of it
			if (p2) {
				char *t = strdup(p + 13);
				t[p2 - (p + 13)] = 0;
				MESS(30, WM_SETTEXT, 0, t);
				free(t);
			}
		}
	} else {
		meta = BASS_ChannelGetTags(chan, BASS_TAG_OGG);
		if (meta) { // got Icecast/OGG tags
			const char *artist = NULL, *title = NULL, *p = meta;
			for (; *p; p += strlen(p) + 1) {
				if (!strnicmp(p, "artist=", 7)) // found the artist
					artist = p + 7;
				if (!strnicmp(p, "title=", 6)) // found the title
					title = p + 6;
			}
			if (title) {
				if (artist) {
					char text[100];
					_snprintf(text, sizeof(text), "%s - %s", artist, title);
					MESS(30, WM_SETTEXT, 0, text);
				} else
					MESS(30, WM_SETTEXT, 0, title);
			}
		} else {
			meta = BASS_ChannelGetTags(chan, BASS_TAG_HLS_EXTINF);
			if (meta) { // got HLS segment info
				const char *p = strchr(meta, ',');
				if (p) MESS(30, WM_SETTEXT, 0, p + 1);
			}
		}
	}
}

void CALLBACK MetaSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	DoMeta();
}

void CALLBACK StallSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	if (!data) // stalled
		SetTimer(win, 0, 50, 0); // start buffer monitoring
}

void CALLBACK FreeSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	chan = 0;
	MESS(31, WM_SETTEXT, 0, "not playing");
	MESS(30, WM_SETTEXT, 0, "");
	MESS(32, WM_SETTEXT, 0, "");
}

void CALLBACK StatusProc(const void *buffer, DWORD length, void *user)
{
	if (buffer && !length && (DWORD)user == req) // got HTTP/ICY tags, and this is still the current request
		MESS(32, WM_SETTEXT, 0, buffer); // display status
}

DWORD WINAPI OpenURL(void *url)
{
	DWORD c, r;
	EnterCriticalSection(&lock); // make sure only 1 thread at a time can do the following
	r = ++req; // increment the request counter for this request
	LeaveCriticalSection(&lock);
	if (chan) BASS_StreamFree(chan); // close old stream
	MESS(31, WM_SETTEXT, 0, "connecting...");
	MESS(30, WM_SETTEXT, 0, "");
	MESS(32, WM_SETTEXT, 0, "");
	c = BASS_StreamCreateURL(url, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE | BASS_SAMPLE_FLOAT, StatusProc, (void*)r); // open URL
	free(url); // free temp URL buffer
	EnterCriticalSection(&lock);
	if (r != req) { // there is a newer request, discard this stream
		LeaveCriticalSection(&lock);
		if (c) BASS_StreamFree(c);
		return 0;
	}
	chan = c; // this is now the current stream
	LeaveCriticalSection(&lock);
	if (!chan) { // failed to open
		MESS(31, WM_SETTEXT, 0, "not playing");
		Error("Can't play the stream");
	} else {
		// only needed the DOWNLOADPROC to receive HTTP/ICY tags, so disable it now
		void *proc = NULL;
		BASS_ChannelSetAttributeEx(chan, BASS_ATTRIB_DOWNLOADPROC, &proc, sizeof(proc));
		// set syncs for stream title updates
		BASS_ChannelSetSync(chan, BASS_SYNC_META, 0, MetaSync, 0); // Shoutcast
		BASS_ChannelSetSync(chan, BASS_SYNC_OGG_CHANGE, 0, MetaSync, 0); // Icecast/OGG
		BASS_ChannelSetSync(chan, BASS_SYNC_HLS_SEGMENT, 0, MetaSync, 0); // HLS
		// set sync for stalling/buffering
		BASS_ChannelSetSync(chan, BASS_SYNC_STALL, 0, StallSync, 0);
		// set sync for end of stream (when freed due to AUTOFREE)
		BASS_ChannelSetSync(chan, BASS_SYNC_FREE, 0, FreeSync, 0);
		// play it!
		BASS_ChannelPlay(chan, FALSE);
		// start buffer monitoring (and display stream info when done)
		SetTimer(win, 0, 50, 0);
	}
	return 0;
}

INT_PTR CALLBACK DialogProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_TIMER:
			{ // monitor buffering progress
				DWORD active = BASS_ChannelIsActive(chan);
				if (active == BASS_ACTIVE_STALLED) {
					char text[32];
					sprintf(text, "buffering... %d%%", 100 - (int)BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFERING));
					MESS(31, WM_SETTEXT, 0, text);
					break;
				} else {
					KillTimer(win, 0); // finished buffering, stop monitoring
					if (active) {
						MESS(31, WM_SETTEXT, 0, "playing");
						{ // get the stream name and URL
							const char *icy = BASS_ChannelGetTags(chan, BASS_TAG_ICY);
							if (!icy) icy = BASS_ChannelGetTags(chan, BASS_TAG_HTTP); // no ICY tags, try HTTP
							if (icy) {
								for (; *icy; icy += strlen(icy) + 1) {
									if (!strnicmp(icy, "icy-name:", 9))
										MESS(31, WM_SETTEXT, 0, icy + 9);
									if (!strnicmp(icy, "icy-url:", 8))
										MESS(32, WM_SETTEXT, 0, icy + 8);
								}
							}
						}
						// get the stream title
						DoMeta();
					}
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					EndDialog(h, 0);
					break;

				default:
					if ((LOWORD(w) >= 10 && LOWORD(w) < 20) || LOWORD(w) == 21) {
						char *url;
						if (LOWORD(w) == 21) { // custom stream URL
							char temp[500];
							MESS(20, WM_GETTEXT, sizeof(temp), temp);
							url = strdup(temp);
						} else // preset
							url = strdup(urls[LOWORD(w) - 10]);
						if (MESS(41, BM_GETCHECK, 0, 0))
							BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, NULL); // disable proxy
						else {
							char proxy[100];
							GetDlgItemText(win, 40, proxy, sizeof(proxy) - 1);
							proxy[sizeof(proxy) - 1] = 0;
							BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, proxy); // set proxy server
						}
						// open URL in a new thread (so that main thread is free)
						CloseHandle(CreateThread(NULL, 0, OpenURL, url, 0, NULL));
					}
			}
			break;

		case WM_INITDIALOG:
			win = h;
			// initialize default output device
			if (!BASS_Init(-1, 44100, 0, win, NULL)) {
				Error("Can't initialize device");
				EndDialog(win, 0);
				break;
			}
			BASS_PluginLoad("bass_aac", 0); // load BASS_AAC (if present) for AAC support on older Windows
			BASS_PluginLoad("bassflac", 0); // load BASSFLAC (if present) for FLAC support
			BASS_PluginLoad("bassopus", 0); // load BASSOPUS (if present) for OPUS support
			BASS_PluginLoad("basshls", 0); // load BASSHLS (if present) for HLS support
			InitializeCriticalSection(&lock);
			MESS(20, WM_SETTEXT, 0, "http://");
			return 1;

		case WM_DESTROY:
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

	BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing

	DialogBox(hInstance, MAKEINTRESOURCE(1000), NULL, DialogProc);

	return 0;
}
