//
// Copyright (C) 2015 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#include "opl3class.h"
#include <string.h>

#include "../resampler.h"

const Bit64u lat = (50 * 49716) / 1000;

int opl3class::fm_init(unsigned int rate) {
	OPL3_Reset(&chip, rate);

	memset(samples, 0, sizeof(samples));
	counter = 0;
	resampler = resampler_create();
	if(!resampler) return 0;
	resampler_set_rate(resampler, 49716.0 / (double)rate);

	return 1;
}

void opl3class::fm_writereg(unsigned short reg, unsigned char data) {
	OPL3_WriteRegBuffered(&chip, reg, data);
}

void opl3class::fm_writepan(unsigned short reg, unsigned char data) {
	OPL3_WritePan(&chip, reg, data);
}

void opl3class::fm_generate_one(signed short *buffer) {
	OPL3_Generate(&chip, (Bit16s *)buffer);
	buffer += 2;
	counter++;
}

void opl3class::fm_generate(signed short *buffer, unsigned int length) {
	for(; length--;) {
		sample_t ls, rs;
		unsigned int to_write = resampler_get_min_fill(resampler);
		while(to_write) {
			fm_generate_one(samples);
			resampler_write_pair(resampler, samples[0], samples[1]);
			--to_write;
		}
		resampler_read_pair(resampler, &ls, &rs);
		if((ls + 0x8000) & 0xFFFF0000) ls = (ls >> 31) ^ 0x7FFF;
		if((rs + 0x8000) & 0xFFFF0000) rs = (rs >> 31) ^ 0x7FFF;
		buffer[0] = (short)ls;
		buffer[1] = (short)rs;
		buffer += 2;
	}
}

fm_chip *getchip() {
	opl3class *chip = new opl3class;
	return chip;
}
