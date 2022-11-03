/*-
 * Copyright (c) 2007, 2008 Edward Tomasz Napierała <trasz@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * ALTHOUGH THIS SOFTWARE IS MADE OF WIN AND SCIENCE, IT IS PROVIDED BY THE
 * AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * \file
 *
 * Standard MIDI File format loader.
 *
 */

/* Reference: http://www.borg.com/~jglatt/tech/midifile.htm */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include "smf.h"
#include "smf_private.h"

/**
 * Returns pointer to the next SMF chunk in smf->buffer, based on length of the previous one.
 * Returns NULL in case of error.
 */
static struct chunk_header_struct *
next_chunk(smf_t *smf)
{
	struct chunk_header_struct *chunk;
	void *next_chunk_ptr;

	assert(smf->file_buffer != NULL);
	assert(smf->file_buffer_length > 0);
	assert(smf->next_chunk_offset >= 0);

	if (smf->next_chunk_offset + sizeof(struct chunk_header_struct) >= smf->file_buffer_length) {
		g_critical("SMF warning: no more chunks left.");
		return (NULL);
	}

	next_chunk_ptr = (unsigned char *)smf->file_buffer + smf->next_chunk_offset;

	chunk = (struct chunk_header_struct *)next_chunk_ptr;

	if (!isalpha(chunk->id[0]) || !isalpha(chunk->id[1]) || !isalpha(chunk->id[2]) || !isalpha(chunk->id[3])) {
		g_critical("SMF error: chunk signature contains at least one non-alphanumeric byte.");
		return (NULL);
	}

	/*
	 * XXX: On SPARC, after compiling with "-fast" option there will be SIGBUS here.
	 * Please compile with -xmemalign=8i".
	 */
	smf->next_chunk_offset += sizeof(struct chunk_header_struct) + g_ntohl(chunk->length);

	if (smf->next_chunk_offset > smf->file_buffer_length) {
		g_critical("SMF warning: malformed chunk; truncated file?");
		smf->next_chunk_offset = smf->file_buffer_length;
	}

	return (chunk);
}

/**
 * Returns 1, iff signature of the "chunk" is the same as string passed as "signature".
 */
static int
chunk_signature_matches(const struct chunk_header_struct *chunk, const char *signature)
{
	if (!memcmp(chunk->id, signature, 4))
		return (1);

	return (0);
}

/**
 * Verifies if MThd header looks OK.  Returns 0 iff it does.
 */
static int
parse_mthd_header(smf_t *smf)
{
	int len;
	struct chunk_header_struct *mthd, *tmp_mthd;

	/* Make sure compiler didn't do anything stupid. */
	assert(sizeof(struct chunk_header_struct) == 8);

	/*
	 * We could just do "mthd = smf->file_buffer;" here, but this way we wouldn't
	 * get useful error messages.
	 */
	if (smf->file_buffer_length < 6) {
		g_critical("SMF error: file is too short, it cannot be a MIDI file.");

		return (-1);
	}

	tmp_mthd = smf->file_buffer;

	if (!chunk_signature_matches(tmp_mthd, "MThd")) {
		g_critical("SMF error: MThd signature not found, is that a MIDI file?");
		
		return (-2);
	}

	/* Ok, now use next_chunk(). */
	mthd = next_chunk(smf);
	if (mthd == NULL)
		return (-3);

	assert(mthd == tmp_mthd);

	len = g_ntohl(mthd->length);
	if (len != 6) {
		g_critical("SMF error: MThd chunk length %d, must be 6.", len);

		return (-4);
	}

	return (0);
}

/**
 * Parses MThd chunk, filling "smf" structure with values extracted from it.  Returns 0 iff everything went OK.
 */
static int
parse_mthd_chunk(smf_t *smf)
{
	signed char first_byte_of_division, second_byte_of_division;

	struct mthd_chunk_struct *mthd;

	assert(sizeof(struct mthd_chunk_struct) == 14);

	if (parse_mthd_header(smf))
		return (1);

	mthd = (struct mthd_chunk_struct *)smf->file_buffer;

	smf->format = g_ntohs(mthd->format);
	if (smf->format < 0 || smf->format > 2) {
		g_critical("SMF error: bad MThd format field value: %d, valid values are 0-2, inclusive.", smf->format);
		return (-1);
	}

	if (smf->format == 2) {
		g_critical("SMF file uses format #2, no support for that yet.");
		return (-2);
	}

	smf->expected_number_of_tracks = g_ntohs(mthd->number_of_tracks);
	if (smf->expected_number_of_tracks <= 0) {
		g_critical("SMF error: bad number of tracks: %d, must be greater than zero.", smf->expected_number_of_tracks);
		return (-3);
	}

	/* XXX: endianess? */
	first_byte_of_division = *((signed char *)&(mthd->division));
	second_byte_of_division = *((signed char *)&(mthd->division) + 1);

	if (first_byte_of_division >= 0) {
		smf->ppqn = g_ntohs(mthd->division);
		smf->frames_per_second = 0;
		smf->resolution = 0;
	} else {
		smf->ppqn = 0;
		smf->frames_per_second = - first_byte_of_division;
		smf->resolution = second_byte_of_division;
	}

	if (smf->ppqn == 0) {
		g_critical("SMF file uses FPS timing instead of PPQN, no support for that yet.");
		return (-4);
	}
	
	return (0);
}

/**
 * Interprets Variable Length Quantity pointed at by "buf" and puts its value into "value" and number
 * of bytes consumed into "len", making sure it does not read past "buf" + "buffer_length".
 * Explanation of Variable Length Quantities is here: http://www.borg.com/~jglatt/tech/midifile/vari.htm
 * Returns 0 iff everything went OK, different value in case of error.
 */
static int
extract_vlq(const unsigned char *buf, const int buffer_length, int *value, int *len)
{
	int val = 0;
	const unsigned char *c = buf;

	assert(buffer_length > 0);

	for (;;) {
		if (c >= buf + buffer_length) {
			g_critical("End of buffer in extract_vlq().");
			return (-1);
		}

		val = (val << 7) + (*c & 0x7F);

		if (*c & 0x80)
			c++;
		else
			break;
	};

	*value = val;
	*len = c - buf + 1;

	if (*len > 4) {
		g_critical("SMF error: Variable Length Quantities longer than four bytes are not supported yet.");
		return (-2);
	}

	return (0);
}

/**
 * Returns 1 if the given byte is a valid status byte, 0 otherwise.
 */
int
is_status_byte(const unsigned char status)
{
	return (status & 0x80);
}

static int
is_sysex_byte(const unsigned char status)
{
	if (status == 0xF0)
		return (1);

	return (0);
}

static int
is_escape_byte(const unsigned char status)
{
	if (status == 0xF7)
		return (1);

	return (0);
}

/**
 * Just like expected_message_length(), but only for System Exclusive messages.
 * Note that value returned by this thing here is the length of SysEx "on the wire",
 * not the number of bytes that this sysex takes in the file - in SMF format sysex
 * contains VLQ telling how many bytes it takes, "on the wire" format does not have
 * this.
 */
static int
expected_sysex_length(const unsigned char status, const unsigned char *second_byte, const int buffer_length, int *consumed_bytes)
{
	int sysex_length, len;

	assert(status == 0xF0);

	if (buffer_length < 3) {
		g_critical("SMF error: end of buffer in expected_sysex_length().");
		return (-1);
	}

	if (extract_vlq(second_byte, buffer_length, &sysex_length, &len))
		return (-1);

	if (consumed_bytes != NULL)
		*consumed_bytes = len;

	/* +1, because the length does not include status byte. */
	return (sysex_length + 1);
}

/**
 * Returns expected length of the midi message (including the status byte), in bytes, for the given status byte.
 * The "second_byte" points to the expected second byte of the MIDI message.  "buffer_length" is the buffer
 * length limit, counting from "second_byte".  Returns value < 0 iff there was an error.
 */
static int
expected_message_length(unsigned char status, const unsigned char *second_byte, const int buffer_length)
{
	/* Make sure this really is a valid status byte. */
	assert(is_status_byte(status));

	/* We cannot use this routine for sysexes. */
	assert(!is_sysex_byte(status));

	/* We cannot use this routine for escaped events. */
	assert(!is_escape_byte(status));

	/* Buffer length may be zero, for e.g. realtime messages. */
	assert(buffer_length >= 0);

	/* Is this a metamessage? */
	if (status == 0xFF) {
		if (buffer_length < 2) {
			g_critical("SMF error: end of buffer in expected_message_length().");
			return (-1);
		}

		/*
		 * Format of this kind of messages is like this: 0xFF 0xwhatever 0xlength and then "length" bytes.
		 * Second byte points to this:                        ^^^^^^^^^^
		 */
		return (*(second_byte + 1) + 3);
	}

	if ((status & 0xF0) == 0xF0) {
		switch (status) {
			case 0xF2: /* Song Position Pointer. */
				return (3);

			case 0xF1: /* MTC Quarter Frame. */
			case 0xF3: /* Song Select. */
				return (2);

			case 0xF6: /* Tune Request. */
			case 0xF8: /* MIDI Clock. */
			case 0xF9: /* Tick. */
			case 0xFA: /* MIDI Start. */
			case 0xFB: /* MIDI Continue. */
			case 0xFC: /* MIDI Stop. */
			case 0xFE: /* Active Sense. */
				return (1);

			default:
				g_critical("SMF error: unknown 0xFx-type status byte '0x%x'.", status);
				return (-2);
		}
	}

	/* Filter out the channel. */
	status &= 0xF0;

	switch (status) {
		case 0x80: /* Note Off. */
		case 0x90: /* Note On. */
		case 0xA0: /* AfterTouch. */
		case 0xB0: /* Control Change. */
		case 0xE0: /* Pitch Wheel. */
			return (3);	

		case 0xC0: /* Program Change. */
		case 0xD0: /* Channel Pressure. */
			return (2);

		default:
			g_critical("SMF error: unknown status byte '0x%x'.", status);
			return (-3);
	}
}

static int
extract_sysex_event(const unsigned char *buf, const int buffer_length, smf_event_t *event, int *len, int last_status, int *has_unterminated_sysex)
{
	int status, message_length, vlq_length;
	const unsigned char *c = buf;

	status = *buf;

	assert(is_sysex_byte(status));

	c++;

	message_length = expected_sysex_length(status, c, buffer_length - 1, &vlq_length);

	if (message_length < 0)
		return (-3);

	c += vlq_length;

	if (vlq_length + message_length >= buffer_length) {
		g_critical("End of buffer in extract_sysex_event().");
		return (-5);
	}

	event->midi_buffer_length = message_length;
	event->midi_buffer = malloc(event->midi_buffer_length);
	if (event->midi_buffer == NULL) {
		g_critical("Cannot allocate memory in extract_sysex_event(): %s", strerror(errno));
		return (-4);
	}

	event->midi_buffer[0] = status;
	memcpy(event->midi_buffer + 1, c, message_length - 1);

	*has_unterminated_sysex = event->midi_buffer[event->midi_buffer_length - 1] != 0xF7;

	*len = vlq_length + message_length;

	return (0);
}

static int
extract_escaped_event(const unsigned char *buf, const int buffer_length, smf_event_t *event, int *len, int last_status, int *has_unterminated_sysex)
{
	int status, message_length, vlq_length;
	const unsigned char *c = buf;

	status = *buf;

	assert(is_escape_byte(status));

	c++;

	if (extract_vlq(c, buffer_length - 1, &message_length, &vlq_length) != 0)
		return (-3);

	if (message_length < 1) {
		g_critical("0-length escaped event in extract_escaped_event().");
		return (-4);
	}

	c += vlq_length;

	if (1 + vlq_length + message_length > buffer_length) {
		g_critical("End of buffer in extract_escaped_event().");
		return (-5);
	}

	/* If *has_unterminated_sysex is non-zero, we want to add the F7 status byte to the start of the message
	   so that it can be identified as such. */
	event->midi_buffer_length = *has_unterminated_sysex ? message_length + 1 : message_length;
	event->midi_buffer = malloc(event->midi_buffer_length);
	if (event->midi_buffer == NULL) {
		g_critical("Cannot allocate memory in extract_escaped_event(): %s", strerror(errno));
		return (-4);
	}
	if (*has_unterminated_sysex) {
		event->midi_buffer[0] = 0xF7;
		memcpy(event->midi_buffer + 1, c, message_length);
		*has_unterminated_sysex = c[message_length - 1] != 0xF7;
	} else {
		memcpy(event->midi_buffer, c, message_length);
		if (smf_event_is_system_realtime(event) || smf_event_is_system_common(event)) {
			g_warning("Escaped event is not System Realtime nor System Common.");
		}
	}

	if (!smf_event_is_valid(event)) {
		g_critical("Escaped event is invalid.");
		return (-1);
	}

	*len = 1 + vlq_length + message_length;

	return (0);
}


/**
 * Puts MIDI data extracted from from "buf" into "event" and number of consumed bytes into "len".
 * In case valid status is not found, it uses "last_status" (so called "running status").
 * Returns 0 iff everything went OK, value < 0 in case of error.
 */
static int
extract_midi_event(const unsigned char *buf, const int buffer_length, smf_event_t *event, int *len, int last_status, int *has_unterminated_sysex)
{
	int status, message_length;
	const unsigned char *c = buf;

	assert(buffer_length > 0);

	/* Is the first byte the status byte? */
	if (is_status_byte(*c)) {
		status = *c;
		c++;

	} else {
		/* No, we use running status then. */
		status = last_status;
	}

	if (!is_status_byte(status)) {
		g_critical("SMF error: bad status byte (MSB is zero).");
		return (-1);
	}

	if (is_sysex_byte(status))
		return (extract_sysex_event(buf, buffer_length, event, len, last_status, has_unterminated_sysex));

	if (is_escape_byte(status))
		return (extract_escaped_event(buf, buffer_length, event, len, last_status, has_unterminated_sysex));

	/* At this point, "c" points to first byte following the status byte. */
	message_length = expected_message_length(status, c, buffer_length - (c - buf));

	if (message_length < 0)
		return (-3);

	if (message_length - 1 > buffer_length - (c - buf)) {
		g_critical("End of buffer in extract_midi_event().");
		return (-5);
	}

	event->midi_buffer_length = message_length;
	event->midi_buffer = malloc(event->midi_buffer_length);
	if (event->midi_buffer == NULL) {
		g_critical("Cannot allocate memory in extract_midi_event(): %s", strerror(errno));
		return (-4);
	}

	event->midi_buffer[0] = status;
	memcpy(event->midi_buffer + 1, c, message_length - 1);

	*len = c + message_length - 1 - buf;

	return (0);
}

/**
 * Locates, basing on track->next_event_offset, the next event data in track->buffer,
 * interprets it, allocates smf_event_t and fills it properly.  Returns smf_event_t
 * or NULL, if there was an error.  Allocating event means adding it to the track;
 * see smf_event_new().
 */
static smf_event_t *
parse_next_event(smf_track_t *track)
{
	int time = 0, len, buffer_length;
	unsigned char *c, *start;

	smf_event_t *event = smf_event_new();
	if (event == NULL)
		goto error;

	c = start = (unsigned char *)track->file_buffer + track->next_event_offset;

	assert(track->file_buffer != NULL);
	assert(track->file_buffer_length > 0);
	assert(track->next_event_offset > 0);

	buffer_length = track->file_buffer_length - track->next_event_offset;
	assert(buffer_length > 0);

	/* First, extract time offset from previous event. */
	if (extract_vlq(c, buffer_length, &time, &len))
		goto error;

	c += len;
	buffer_length -= len;

	if (buffer_length <= 0)
		goto error;

	/* Now, extract the actual event. */
	if (extract_midi_event(c, buffer_length, event, &len, track->last_status, &track->has_unterminated_sysex))
		goto error;

	c += len;
	buffer_length -= len;
	track->last_status = event->midi_buffer[0];
	track->next_event_offset += c - start;

	smf_track_add_event_delta_pulses(track, event, time);

	return (event);

error:
	if (event != NULL)
		smf_event_delete(event);

	return (NULL);
}

/**
 * Takes "len" characters starting in "buf", making sure it does not access past the length of the buffer,
 * and makes ordinary, zero-terminated string from it.  May return NULL if there was any problem.
 */ 
static char *
make_string(const unsigned char *buf, const int buffer_length, int len)
{
	char *str;

	assert(buffer_length > 0);
	assert(len > 0);

	if (len > buffer_length) {
		g_critical("End of buffer in make_string().");

		len = buffer_length;
	}

	str = malloc(len + 1);
	if (str == NULL) {
		g_critical("Cannot allocate memory in make_string().");
		return (NULL);
	}

	memcpy(str, buf, len);
	str[len] = '\0';

	return (str);
}

/**
 * \return 1, if passed a metaevent containing text, that is, Text, Copyright,
 * Sequence/Track Name, Instrument, Lyric, Marker, Cue Point, Program Name,
 * or Device Name; 0 otherwise.
 */
int
smf_event_is_textual(const smf_event_t *event)
{
	if (!smf_event_is_metadata(event))
		return (0);

	if (event->midi_buffer_length < 4)
		return (0);

	if (event->midi_buffer[3] < 1 && event->midi_buffer[3] > 9)
		return (0);

	return (1);
}

/**
 * Extracts text from "textual metaevents", such as Text or Lyric.
 *
 * \return Zero-terminated string extracted from "text events" or NULL, if there was any problem.
 */
char *
smf_event_extract_text(const smf_event_t *event)
{
	int string_length = -1, length_length = -1;

	if (!smf_event_is_textual(event))
		return (NULL);

	if (event->midi_buffer_length < 3) {
		g_critical("smf_event_extract_text: truncated MIDI message.");
		return (NULL);
	}

	extract_vlq((void *)&(event->midi_buffer[2]), event->midi_buffer_length - 2, &string_length, &length_length);

	if (string_length <= 0) {
		g_critical("smf_event_extract_text: truncated MIDI message.");
		return (NULL);
	}

	return (make_string((void *)(&event->midi_buffer[2] + length_length), event->midi_buffer_length - 2 - length_length, string_length));
}

/**
 * Verify if the next chunk really is MTrk chunk, and if so, initialize some track variables and return 0.
 * Return different value otherwise.
 */
static int
parse_mtrk_header(smf_track_t *track)
{
	struct chunk_header_struct *mtrk;

	/* Make sure compiler didn't do anything stupid. */
	assert(sizeof(struct chunk_header_struct) == 8);
	assert(track->smf != NULL);

	mtrk = next_chunk(track->smf);

	if (mtrk == NULL)
		return (-1);

	if (!chunk_signature_matches(mtrk, "MTrk")) {
		g_warning("SMF warning: Expected MTrk signature, got %c%c%c%c instead; ignoring this chunk.",
				mtrk->id[0], mtrk->id[1], mtrk->id[2], mtrk->id[3]);
		
		return (-2);
	}

	track->file_buffer = mtrk;
	track->file_buffer_length = sizeof(struct chunk_header_struct) + g_ntohl(mtrk->length);
	track->next_event_offset = sizeof(struct chunk_header_struct);

	return (0);
}

/**
 * Return 1 if event is end-of-the-track, 0 otherwise.
 */
static int
event_is_end_of_track(const smf_event_t *event)
{
	if (event->midi_buffer[0] == 0xFF && event->midi_buffer[1] == 0x2F)
		return (1);

	return (0);
}

/**
 * \return Nonzero, if event is as long as it should be, from the MIDI specification point of view.
 * Does not work for SysExes - it doesn't recognize internal structure of SysEx.
 */
int
smf_event_length_is_valid(const smf_event_t *event)
{
	assert(event);
	assert(event->midi_buffer);

	if (event->midi_buffer_length < 1)
		return (0);

	/* We cannot use expected_message_length on sysexes or sysex continuations. */
	if (smf_event_is_sysex(event) || smf_event_is_sysex_continuation(event))
		return (1);

	if (event->midi_buffer_length != expected_message_length(event->midi_buffer[0],
		&(event->midi_buffer[1]), event->midi_buffer_length - 1)) {

		return (0);
	}

	return (1);
}

/**
 * \return Nonzero, if MIDI data in the event is valid, 0 otherwise.  For example,
 * it checks if event length is correct.
 */
/* XXX: this routine requires some more work to detect more errors. */
int
smf_event_is_valid(const smf_event_t *event)
{
	assert(event);
	assert(event->midi_buffer);
	assert(event->midi_buffer_length >= 1);

	if (!is_status_byte(event->midi_buffer[0])) {
		g_critical("First byte of MIDI message (%02X) is not a valid status byte.", event->midi_buffer[0] & 0xFF);

		return (0);
	}

	if (!smf_event_length_is_valid(event))
		return (0);

	return (1);
}

/**
 * Parse events and put it on the track.
 */
static int
parse_mtrk_chunk(smf_track_t *track)
{
	smf_event_t *event;

	if (parse_mtrk_header(track))
		return (-1);

	for (;;) {
		event = parse_next_event(track);

		/* Couldn't parse an event? */
		if (event == NULL) {
			g_critical("Unable to parse MIDI event; truncating track.");
			if (smf_track_add_eot_delta_pulses(track, 0) != 0) {
				g_critical("smf_track_add_eot_delta_pulses failed.");
				return (-2);
			}
			break;
		}

		assert(smf_event_is_valid(event));

		if (event_is_end_of_track(event))
			break;
	}

	track->file_buffer = NULL;
	track->file_buffer_length = 0;
	track->next_event_offset = -1;

	return (0);
}

/**
 * Allocate buffer of proper size and read file contents into it.  Close file afterwards.
 */
static int
load_file_into_buffer(void **file_buffer, int *file_buffer_length, const char *file_name)
{
	FILE *stream = fopen(file_name, "rb");

	if (stream == NULL) {
		g_critical("Cannot open input file: %s", strerror(errno));

		return (-1);
	}

	if (fseek(stream, 0, SEEK_END)) {
		g_critical("fseek(3) failed: %s", strerror(errno));

		return (-2);
	}

	*file_buffer_length = ftell(stream);
	if (*file_buffer_length == -1) {
		g_critical("ftell(3) failed: %s", strerror(errno));

		return (-3);
	}

	if (fseek(stream, 0, SEEK_SET)) {
		g_critical("fseek(3) failed: %s", strerror(errno));

		return (-4);
	}

	*file_buffer = malloc(*file_buffer_length);
	if (*file_buffer == NULL) {
		g_critical("malloc(3) failed: %s", strerror(errno));

		return (-5);
	}

	if (fread(*file_buffer, 1, *file_buffer_length, stream) != *file_buffer_length) {
		g_critical("fread(3) failed: %s", strerror(errno));

		return (-6);
	}
	
	if (fclose(stream)) {
		g_critical("fclose(3) failed: %s", strerror(errno));

		return (-7);
	}

	return (0);
}

/**
  * Creates new SMF and fills it with data loaded from the given buffer.
 * \return SMF or NULL, if loading failed.
  */
smf_t *
smf_load_from_memory(const void *buffer, const int buffer_length)
{
	int i;

	smf_t *smf = smf_new();

	smf->file_buffer = (void *)buffer;
	smf->file_buffer_length = buffer_length;
	smf->next_chunk_offset = 0;

	if (parse_mthd_chunk(smf))
		return (NULL);

	for (i = 1; i <= smf->expected_number_of_tracks; i++) {
		smf_track_t *track = smf_track_new();
		if (track == NULL)
			return (NULL);

		smf_add_track(smf, track);

		/* Skip unparseable chunks. */
		if (parse_mtrk_chunk(track)) {
			g_warning("SMF warning: Cannot load track.");
			smf_track_delete(track);
		}

		track->file_buffer = NULL;
		track->file_buffer_length = 0;
		track->next_event_offset = -1;
	}

	if (smf->expected_number_of_tracks != smf->number_of_tracks) {
		g_warning("SMF warning: MThd header declared %d tracks, but only %d found; continuing anyway.",
				smf->expected_number_of_tracks, smf->number_of_tracks);

		smf->expected_number_of_tracks = smf->number_of_tracks;
	}

	smf->file_buffer = NULL;
	smf->file_buffer_length = 0;
	smf->next_chunk_offset = -1;

	return (smf);
}

/**
 * Loads SMF file.
 *
 * \param file_name Path to the file.
 * \return SMF or NULL, if loading failed.
 */
smf_t *
smf_load(const char *file_name)
{
	int file_buffer_length;
	void *file_buffer;
	smf_t *smf;

	if (load_file_into_buffer(&file_buffer, &file_buffer_length, file_name))
		return (NULL);

	smf = smf_load_from_memory(file_buffer, file_buffer_length);

	memset(file_buffer, 0, file_buffer_length);
	free(file_buffer);

	if (smf == NULL)
		return (NULL);

	smf_rewind(smf);

	return (smf);
}

