/*
	BASSMIDI simple soundfont packer
	Copyright (c) 2006-2021 Un4seen Developments Ltd.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "bass.h"
#include "bassmidi.h"

// encoder command-lines
#define ENCODERS 6
const char *commands[ENCODERS] = {
	"flac --best --no-padding -", // FLAC lossless
	"wavpack -h -", // WavPack lossless
	"wavpack -hb384 -", // WavPack lossy (high)
	"wavpack -hb256 -", // WavPack lossy (average)
	"wavpack -hb128 -", // WavPack lossy (low)
	"opusenc - -", // Opus
};

void main(int argc, char **argv)
{
	HSOUNDFONT sf2;
	char outfile[300], *ext;
	int arg, encoder = 0, unpack = 0, flags = 0;

	printf("SF2Pack - Soundfont Packer\n"
		"--------------------------\n");

	// process options
	for (arg = 1; arg < argc; arg++) {
		if (argv[arg][0] != '-') break;
		switch (argv[arg][1]) {
			case 'e':
				encoder = atoi(argv[arg] + 2);
				if (encoder >= ENCODERS) {
					printf("Invalid encoder\n");
					return;
				}
				break;
			case 'r':
				flags |= BASS_MIDI_PACK_16BIT;
				break;
			case 'u':
				unpack = 1;
				break;
		}
	}
	if (arg + 1 != argc && arg + 2 != argc) {
		printf("usage: sf2pack [options] <file> [outfile]\n"
			"\t-e<0-5>\tencoder: 0=FLAC lossless (default), 1=WavPack lossless\n"
			"\t\t2=WavPack lossy (high quality), 3=WavPack lossy (average)\n"
			"\t\t4=WavPack lossy (low), 5=Opus lossy\n"
			"\t-r\treduce 24-bit data to 16-bit\n"
			"\t-u\tunpack file\n");
		return;
	}

#ifdef _WIN32
	if (arg + 2 == argc && !stricmp(argv[arg], argv[arg + 1])) {
#else
	if (arg + 2 == argc && !strcmp(argv[arg], argv[arg + 1])) {
#endif
		printf("Input and output files must be different\n");
		return;
	}

	if (unpack) {
		// load plugins to unpack with
		BASS_PluginLoad("bassflac.dll", 0);
		BASS_PluginLoad("basswv.dll", 0);
		BASS_PluginLoad("bassopus.dll", 0);
		// not playing anything, so don't need an update thread
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
		// initialize BASS "no sound" device for decoding
		BASS_Init(0, 44100, 0, 0, NULL);
	}

	// open soundfont
	printf("opening %s\n", argv[arg]);
	sf2 = BASS_MIDI_FontInit(argv[arg], 0);
	if (!sf2) {
		printf("Can't open soundfont (error: %d)\n", BASS_ErrorGetCode());
		return;
	}

	if (unpack) {
		if (arg + 1 < argc) {
			strcpy(outfile, argv[arg + 1]);
		} else {
			strcpy(outfile, argv[arg]);
			strcat(outfile, ".sf2");
		}
		printf("unpacking to %s\n", outfile);
		if (!BASS_MIDI_FontUnpack(sf2, outfile, 0)) {
			printf("Unpacking failed (error: %d)\n", BASS_ErrorGetCode());
			BASS_Free();
			return;
		}
		BASS_Free();
	} else {
		if (arg + 1 < argc) {
			strcpy(outfile, argv[arg + 1]);
		} else {
			strcpy(outfile, argv[arg]);
			ext = strrchr(outfile, '.');
			if (ext && !strpbrk(ext, "\\/")) strcat(ext, "pack");
			else strcat(outfile, ".sf2pack");
		}
		printf("packing to %s (with %s)\n", outfile, commands[encoder]);
		if (!BASS_MIDI_FontPack(sf2, outfile, commands[encoder], flags)) {
			printf("Packing failed (error: %d)\n", BASS_ErrorGetCode());
			return;
		}
	}

	{ // display ratio
		struct stat si, so;
		stat(argv[arg], &si);
		stat(outfile, &so);
		printf("done: %u -> %u (%.1f%%)\n", (DWORD)si.st_size, (DWORD)so.st_size, 100.f * (DWORD)so.st_size / (DWORD)si.st_size);
	}
}
