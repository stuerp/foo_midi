#include "midi_processor.h"

const t_uint8 midi_processor::lds_default_tempo[5] = { 0xFF, 0x51, 0x07, 0xA1, 0x20 };

#define ENABLE_WHEEL
//#define ENABLE_VIB
//#define ENABLE_ARP
//#define ENABLE_TREM

#ifdef ENABLE_WHEEL
#define WHEEL_RANGE_HIGH 12
#define WHEEL_RANGE_LOW 0
#define WHEEL_SCALE(x) ((x) * 512 / WHEEL_RANGE_HIGH)
#define WHEEL_SCALE_LOW(x) (WHEEL_SCALE(x) & 127)
#define WHEEL_SCALE_HIGH(x) (((WHEEL_SCALE(x) >> 7) + 64) & 127)
#endif

#ifdef ENABLE_VIB
// Vibrato (sine) table
static const unsigned char vibtab[] = {
  0, 13, 25, 37, 50, 62, 74, 86, 98, 109, 120, 131, 142, 152, 162,
  171, 180, 189, 197, 205, 212, 219, 225, 231, 236, 240, 244, 247,
  250, 252, 254, 255, 255, 255, 254, 252, 250, 247, 244, 240, 236,
  231, 225, 219, 212, 205, 197, 189, 180, 171, 162, 152, 142, 131,
  120, 109, 98, 86, 74, 62, 50, 37, 25, 13
};
#endif

#ifdef ENABLE_TREM
// Tremolo (sine * sine) table
static const unsigned char tremtab[] = {
  0, 0, 1, 1, 2, 4, 5, 7, 10, 12, 15, 18, 21, 25, 29, 33, 37, 42, 47,
  52, 57, 62, 67, 73, 79, 85, 90, 97, 103, 109, 115, 121, 128, 134,
  140, 146, 152, 158, 165, 170, 176, 182, 188, 193, 198, 203, 208,
  213, 218, 222, 226, 230, 234, 237, 240, 243, 245, 248, 250, 251,
  253, 254, 254, 255, 255, 255, 254, 254, 253, 251, 250, 248, 245,
  243, 240, 237, 234, 230, 226, 222, 218, 213, 208, 203, 198, 193,
  188, 182, 176, 170, 165, 158, 152, 146, 140, 134, 127, 121, 115,
  109, 103, 97, 90, 85, 79, 73, 67, 62, 57, 52, 47, 42, 37, 33, 29,
  25, 21, 18, 15, 12, 10, 7, 5, 4, 2, 1, 1, 0
};
#endif

bool midi_processor::is_lds( file::ptr & p_file, const char * p_extension, abort_callback & p_abort )
{
	try
	{
		t_uint8 mode;
		if ( pfc::stricmp_ascii( p_extension, "LDS" ) ) return false;
		p_file->reopen( p_abort );
		p_file->read_object_t( mode, p_abort );
		if ( mode > 2 ) return false;
		return true;
	}
	catch (exception_io_data_truncation &)
	{
		return false;
	}
}

struct sound_patch
{
	// skip 11 bytes worth of Adlib crap
	t_uint8 keyoff;
#ifdef ENABLE_WHEEL
	t_uint8 portamento;
	t_int8 glide;
#endif
	// skip 1 byte
#ifdef ENABLE_VIB
	t_uint8 vibrato;
	t_uint8 vibrato_delay;
#endif
#ifdef ENABLE_TREM
	t_uint8 modulator_tremolo;
	t_uint8 carrier_tremolo;
	t_uint8 tremolo_delay;
#endif
#ifdef ENABLE_ARP
	t_uint8 arpeggio;
	t_int8 arpeggio_table[12];
#endif
	// skip 4 bytes worth of digital instrument crap
	// skip 3 more bytes worth of Adlib crap that isn't even used
	t_uint8 midi_instrument;
	t_uint8 midi_velocity;
	t_uint8 midi_key;
	t_int8 midi_transpose;
	// skip 2 bytes worth of MIDI dummy fields or whatever
};

struct channel_state {
#ifdef ENABLE_WHEEL
	t_int16 gototune, lasttune;
#endif
	t_uint16 packpos;
	t_int8 finetune;
#ifdef ENABLE_WHEEL
	t_uint8 glideto, portspeed;
#endif
	t_uint8 nextvol, volmod, volcar,
		keycount, packwait;
#ifdef ENABLE_VIB
	t_uint8 vibwait, vibspeed, vibrate, vibcount;
#endif
#ifdef ENABLE_TREM
	t_uint8 trmstay, trmwait, trmspeed, trmrate, trmcount,
		trcwait, trcspeed, trcrate, trccount;
#endif
#ifdef ENABLE_ARP
	t_uint8 arp_count, arp_size, arp_speed, arp_pos;
	t_int8 arp_tab[12];
#endif

	struct {
		t_uint8 chandelay, sound;
		t_uint16 high;
	} chancheat;
};

void playsound( t_uint8 current_instrument[], const pfc::array_t<sound_patch> & patches, t_uint8 last_note[], t_uint8 last_channel[], t_uint8 last_instrument[], t_uint8 last_volume[], t_uint8 last_sent_volume[],
#ifdef ENABLE_WHEEL
	t_int16 last_pitch_wheel[],
#endif
	channel_state * c, t_uint8 allvolume, unsigned current_timestamp, unsigned sound, unsigned chan, unsigned high, midi_track & track )
{
	t_uint8 buffer[ 2 ];
	current_instrument[ chan ] = sound;
	if ( sound >= patches.get_count() ) return;
	const sound_patch & patch = patches[ current_instrument[ chan ] ];
	unsigned channel = ( patch.midi_instrument >= 0x80 ) ? 9 : ( chan == 9 ) ? 10 : chan;
	unsigned saved_last_note = last_note[ chan ];
	unsigned note;
	
	if ( channel != 9 )
	{
		// set fine tune
		high += c->finetune;

		// arpeggio handling
#ifdef ENABLE_ARP
		if(patch.arpeggio)
		{
			short arpcalc = patch.arpeggio_table[0] << 4;

			high += arpcalc;
		}
#endif

		// and MIDI transpose
		high = (int)high + ( patch.midi_transpose << 4 );

		note = high
#ifdef ENABLE_WHEEL
			- c->lasttune
#endif
			;

		// glide handling
#ifdef ENABLE_WHEEL
		if(c->glideto != 0)
		{
			c->gototune = note - ( last_note[ chan ] << 4 ) + c->lasttune;
			c->portspeed = c->glideto;
			c->glideto = c->finetune = 0;
			return;
		}
#endif

		if ( patch.midi_instrument != last_instrument[ chan ] )
		{
			buffer[ 0 ] = patch.midi_instrument;
			track.add_event( midi_event( current_timestamp, midi_event::program_change, channel, buffer, 1 ) );
			last_instrument[ chan ] = patch.midi_instrument;
		}
	}
	else
	{
		note = ( patch.midi_instrument & 0x7F ) << 4;
	}

	unsigned volume = 127;

	if ( c->nextvol )
	{
		volume = ( c->nextvol & 0x3F ) * 127 / 63;
		last_volume[ chan ] = volume;
	}

	if ( allvolume )
	{
		volume = volume * allvolume / 255;
	}

	if ( volume != last_sent_volume[ channel ] )
	{
		buffer[ 0 ] = 7;
		buffer[ 1 ] = volume;
		track.add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ chan ], buffer, 2 ) );
		last_sent_volume[ channel ] = volume;
	}

	if ( saved_last_note != 0xFF )
	{
		buffer[ 0 ] = saved_last_note;
		buffer[ 1 ] = 127;
		track.add_event( midi_event( current_timestamp, midi_event::note_off, last_channel[ chan ], buffer, 2 ) );
		last_note[ chan ] = 0xFF;
#ifdef ENABLE_WHEEL
		if ( channel != 9 )
		{
			note += c->lasttune;
			c->lasttune = 0;
			if ( last_pitch_wheel[ channel ] != 0 )
			{
				buffer[ 0 ] = 0;
				buffer[ 1 ] = 64;
				track.add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
				last_pitch_wheel[ channel ] = 0;
			}
		}
#endif
	}
#ifdef ENABLE_WHEEL
	if ( c->lasttune != last_pitch_wheel[ channel ] )
	{
		buffer[ 0 ] = WHEEL_SCALE_LOW( c->lasttune );
		buffer[ 1 ] = WHEEL_SCALE_HIGH( c->lasttune );
		track.add_event( midi_event( current_timestamp, midi_event::pitch_wheel, channel, buffer, 2 ) );
		last_pitch_wheel[ channel ] = c->lasttune;
	}
	if( !patch.glide || last_note[ chan ] == 0xFF )
#endif
	{
#ifdef ENABLE_WHEEL
		if( !patch.portamento || last_note[ chan ] == 0xFF )
#endif
		{
			buffer[ 0 ] = note >> 4;
			buffer[ 1 ] = patch.midi_velocity;
			track.add_event( midi_event( current_timestamp, midi_event::note_on, channel, buffer, 2 ) );
			last_note[ chan ] = note >> 4;
			last_channel[ chan ] = channel;
#ifdef ENABLE_WHEEL
			c->gototune = c->lasttune;
#endif
		}
#ifdef ENABLE_WHEEL
		else
		{
			c->gototune = note - last_note[ chan ] << 4 + c->lasttune;
			c->portspeed = patch.portamento;
			buffer[ 0 ] = last_note[ chan ] = saved_last_note;
			buffer[ 1 ] = patch.midi_velocity;
			track.add_event( midi_event( current_timestamp, midi_event::note_on, channel, buffer, 2 ) );
		}
#endif
	}
#ifdef ENABLE_WHEEL
	else
	{
		buffer[ 0 ] = note >> 4;
		buffer[ 1 ] = patch.midi_velocity;
		track.add_event( midi_event( current_timestamp, midi_event::note_on, channel, buffer, 2 ) );
		last_note[ chan ] = note >> 4;
		last_channel[ chan ] = channel;
		c->gototune = patch.glide;
		c->portspeed = patch.portamento;
	}
#endif

#ifdef ENABLE_VIB
	if(!patch.vibrato)
	{
		c->vibwait = c->vibspeed = c->vibrate = 0;
	}
	else
	{
		c->vibwait = patch.vibrato_delay;
		// PASCAL:    c->vibspeed = ((i->vibrato >> 4) & 15) + 1;
		c->vibspeed = (patch.vibrato >> 4) + 2;
		c->vibrate = (patch.vibrato & 15) + 1;
	}
#endif

#ifdef ENABLE_TREM
	if(!(c->trmstay & 0xf0))
	{
		c->trmwait = (patch.tremolo_delay & 0xf0) >> 3;
		// PASCAL:    c->trmspeed = (i->mod_trem >> 4) & 15;
		c->trmspeed = patch.modulator_tremolo >> 4;
		c->trmrate = patch.modulator_tremolo & 15;
		c->trmcount = 0;
	}

	if(!(c->trmstay & 0x0f))
	{
		c->trcwait = (patch.tremolo_delay & 15) << 1;
		// PASCAL:    c->trcspeed = (i->car_trem >> 4) & 15;
		c->trcspeed = patch.carrier_tremolo >> 4;
		c->trcrate = patch.carrier_tremolo & 15;
		c->trccount = 0;
	}
#endif

#ifdef ENABLE_ARP
	c->arp_size = patch.arpeggio & 15;
	c->arp_speed = patch.arpeggio >> 4;
	memcpy(c->arp_tab, patch.arpeggio_table, 12);
	c->arp_pos = c->arp_count = 0;
#endif
#ifdef ENABLE_VIB
	c->vibcount = 0;
#endif
#ifdef ENABLE_WHEEL
	c->glideto = 0;
#endif
	c->keycount = patch.keyoff;
	c->nextvol = c->finetune = 0;
}

void midi_processor::process_lds( file::ptr & p_file, midi_container & p_out, abort_callback & p_abort )
{
	struct position_data
	{
		t_uint16 pattern_number;
		t_uint8 transpose;
	};

	t_uint8 mode;
	t_uint16 speed;
	t_uint8 tempo;
	t_uint8 pattern_length;
	t_uint8 channel_delay[ 9 ];
	t_uint8 register_bd;
	t_uint16 patch_count;
	pfc::array_t<sound_patch> patches;
	t_uint16 position_count;
	pfc::array_t<position_data> positions;
	t_size pattern_count;
	pfc::array_t<t_uint16> patterns;

	p_file->read_object_t( mode, p_abort );
	if ( mode > 2 ) throw exception_io_data( "Invalid LDS mode" );
	p_file->read_lendian_t( speed, p_abort );
	p_file->read_object_t( tempo, p_abort );
	p_file->read_object_t( pattern_length, p_abort );
	for ( unsigned i = 0; i < 9; ++i )
		p_file->read_object_t( channel_delay[ i ], p_abort );
	p_file->read_object_t( register_bd, p_abort );

	p_file->read_lendian_t( patch_count, p_abort );
	patches.set_count( patch_count );
	for ( unsigned i = 0; i < patch_count; ++i )
	{
		sound_patch & patch = patches[ i ];
		p_file->skip( 11, p_abort );
		p_file->read_object_t( patch.keyoff, p_abort );
#ifdef ENABLE_WHEEL
		p_file->read_object_t( patch.portamento, p_abort );
		p_file->read_object_t( patch.glide, p_abort );
		p_file->skip( 1, p_abort );
#else
		p_file->skip( 3, p_abort );
#endif
#ifdef ENABLE_VIB
		p_file->read_object_t( patch.vibrato, p_abort );
		p_file->read_object_t( patch.vibrato_delay, p_abort );
#else
		p_file->skip( 2, p_abort );
#endif
#ifdef ENABLE_TREM
		p_file->read_object_t( patch.modulator_tremolo, p_abort );
		p_file->read_object_t( patch.carrier_tremolo, p_abort );
		p_file->read_object_t( patch.tremolo_delay, p_abort );
#else
		p_file->skip( 3, p_abort );
#endif
#ifdef ENABLE_ARP
		p_file->read_object_t( patch.arpeggio, p_abort );
		for ( unsigned j = 0; j < 12; ++j )
			p_file->read_object_t( patch.arpeggio_table[ j ], p_abort );
		p_file->skip( 7, p_abort );
#else
		p_file->skip( 20, p_abort );
#endif
		p_file->read_object_t( patch.midi_instrument, p_abort );
		p_file->read_object_t( patch.midi_velocity, p_abort );
		p_file->read_object_t( patch.midi_key, p_abort );
		p_file->read_object_t( patch.midi_transpose, p_abort );
		p_file->skip( 2, p_abort );

#ifdef ENABLE_WHEEL
		// hax
		if ( patch.midi_instrument >= 0x80 )
		{
			patch.glide = 0;
		}
#endif
	}

	p_file->read_lendian_t( position_count, p_abort );
	positions.set_count( 9 * position_count );
	for ( unsigned i = 0; i < position_count; ++i )
	{
		for ( unsigned j = 0; j < 9; ++j )
		{
			position_data & position = positions[ i * 9 + j ];
			p_file->read_lendian_t( position.pattern_number, p_abort );
			if ( position.pattern_number & 1 ) throw exception_io_data( "Odd LDS pattern number" );
			position.pattern_number >>= 1;
			p_file->read_object_t( position.transpose, p_abort );
		}
	}

	p_file->skip( 2, p_abort );

	pattern_count = ( p_file->get_size_ex( p_abort ) - p_file->get_position( p_abort ) ) / 2;
	patterns.set_count( pattern_count );
	for ( unsigned i = 0; i < pattern_count; ++i )
	{
		p_file->read_lendian_t( patterns[ i ], p_abort );
	}

	t_uint8 jumping, fadeonoff, allvolume, hardfade, tempo_now, pattplay;
	t_uint16 posplay, jumppos;
	t_uint32 mainvolume;
	pfc::array_t<channel_state> channel;
	channel.set_count( 9 );
	pfc::array_t<unsigned> position_timestamps;
	position_timestamps.append_multi( (unsigned)~0, position_count );

	t_uint8 current_instrument[9] = { 0 };

	t_uint8 last_channel[9];
	t_uint8 last_instrument[9];
	t_uint8 last_note[9];
	t_uint8 last_volume[9];
	t_uint8 last_sent_volume[11];
#ifdef ENABLE_WHEEL
	t_int16 last_pitch_wheel[11];
#endif
	t_uint8 ticks_without_notes[11];

	memset( last_channel, 0, sizeof( last_channel ) );
	memset( last_instrument, 0xFF, sizeof( last_instrument ) );
	memset( last_note, 0xFF, sizeof( last_note ) );
	memset( last_volume, 127, sizeof( last_volume ) );
	memset( last_sent_volume, 127, sizeof( last_sent_volume ) );
#ifdef ENABLE_WHEEL
	memset( last_pitch_wheel, 0, sizeof( last_pitch_wheel ) );
#endif
	memset( ticks_without_notes, 0, sizeof( ticks_without_notes ) );

	unsigned current_timestamp = 0;

	t_uint8 buffer[ 2 ];

	p_out.initialize( 1, 35 );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, lds_default_tempo, _countof( lds_default_tempo ) ) );
		for ( unsigned i = 0; i < 11; ++i )
		{
			buffer[ 0 ] = 120;
			buffer[ 1 ] = 0;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 121;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
#ifdef ENABLE_WHEEL
			buffer[ 0 ] = 0x65;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0x64;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0x06;
			buffer[ 1 ] = WHEEL_RANGE_HIGH;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0x26;
			buffer[ 1 ] = WHEEL_RANGE_LOW;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0;
			buffer[ 1 ] = 64;
			track.add_event( midi_event( 0, midi_event::pitch_wheel, i, buffer, 2 ) );
#endif
		}
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	pfc::array_t<midi_track> tracks;
	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		tracks.append_multi( track, 10 );
	}

	tempo_now = 3;
	jumping = 0;
	fadeonoff = 0;
	allvolume = 0;
	hardfade = 0;
	pattplay = 0;
	posplay = 0;
	jumppos = 0;
	mainvolume = 0;
	memset( channel.get_ptr(), 0, sizeof( channel_state ) * 9 );

	const t_uint16 maxsound = 0x3F;
	const t_uint16 maxpos = 0xFF;

	bool playing = true;
	while ( playing )
	{
		t_uint16        chan;
#ifdef ENABLE_VIB
		t_uint16        wibc;
#endif
#if defined(ENABLE_VIB) || defined(ENABLE_ARP)
		t_int16         tune;
#endif
#ifdef ENABLE_ARP
		t_int16         arpreg;
#endif
#ifdef ENABLE_TREM
		t_uint16        tremc;
#endif
		bool            vbreak;
		unsigned        i;
		channel_state * c;

		if(fadeonoff)
		{
			if(fadeonoff <= 128)
			{
				if(allvolume > fadeonoff || allvolume == 0)
				{
					allvolume -= fadeonoff;
				}
				else
				{
					allvolume = 1;
					fadeonoff = 0;
					if(hardfade != 0)
					{
						playing = false;
						hardfade = 0;
						for(i = 0; i < 9; i++)
							channel[i].keycount = 1;
					}
				}
			}
			else if(((allvolume + (0x100 - fadeonoff)) & 0xff) <= mainvolume)
			{
				allvolume += 0x100 - fadeonoff;
			}
			else
			{
				allvolume = mainvolume;
				fadeonoff = 0;
			}
		}

		// handle channel delay
		for(chan = 0; chan < 9; ++chan)
		{
			channel_state * c = &channel[chan];
			if(c->chancheat.chandelay)
			{
				if(!(--c->chancheat.chandelay))
				{
					playsound( current_instrument, patches, last_note, last_channel, last_instrument, last_volume, last_sent_volume,
#ifdef ENABLE_WHEEL
						last_pitch_wheel,
#endif
						c, allvolume, current_timestamp, c->chancheat.sound, chan, c->chancheat.high, tracks[ chan ] );
					ticks_without_notes[ last_channel[ chan ] ] = 0;
				}
			}
		}

		// handle notes
		if(!tempo_now)
		{
			if ( pattplay == 0 && position_timestamps[ posplay ] == ~0 )
			{
				position_timestamps[ posplay ] = current_timestamp;
			}

			vbreak = false;
			for(unsigned chan = 0; chan < 9; chan++)
			{
				channel_state * c = &channel[chan];
				if(!c->packwait)
				{
					unsigned short	patnum = positions[posplay * 9 + chan].pattern_number;
					unsigned char	transpose = positions[posplay * 9 + chan].transpose;

					if ( patnum + c->packpos >= patterns.get_count() ) throw exception_io_data( "Invalid LDS pattern number" );

					unsigned comword = patterns[patnum + c->packpos];
					unsigned comhi = comword >> 8;
					unsigned comlo = comword & 0xff;
					if(comword)
					{
						if(comhi == 0x80)
						{
							c->packwait = comlo;
						}
						else if(comhi >= 0x80)
						{
							switch(comhi) {
							case 0xff:
								{
									unsigned volume = ( comlo & 0x3F ) * 127 / 63;
									last_volume[ chan ] = volume;
									if ( volume != last_sent_volume[ last_channel[ chan ] ] )
									{
										buffer[ 0 ] = 7;
										buffer[ 1 ] = volume;
										tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ chan ], buffer, 2 ) );
										last_sent_volume[ last_channel [ chan ] ] = volume;
									}
								}
								break;
							case 0xfe:
								tempo = comword & 0x3f;
								break;
							case 0xfd:
								c->nextvol = comlo;
								break;
							case 0xfc:
								playing = false;
								// in real player there's also full keyoff here, but we don't need it
								break;
							case 0xfb:
								c->keycount = 1;
								break;
							case 0xfa:
								vbreak = true;
								jumppos = (posplay + 1) & maxpos;
								break;
							case 0xf9:
								vbreak = true;
								jumppos = comlo & maxpos;
								jumping = 1;
								if(jumppos <= posplay)
								{
									p_out.add_track_event( 0, midi_event( position_timestamps[ jumppos ], midi_event::extended, 0, loop_start, _countof( loop_start ) ) );
									p_out.add_track_event( 0, midi_event( current_timestamp + tempo - 1, midi_event::extended, 0, loop_end, _countof( loop_end ) ) );
									playing = false;
								}
								break;
							case 0xf8:
#ifdef ENABLE_WHEEL
								c->lasttune = 0;
#endif
								break;
							case 0xf7:
#ifdef ENABLE_VIB
								c->vibwait = 0;
								// PASCAL: c->vibspeed = ((comlo >> 4) & 15) + 2;
								c->vibspeed = (comlo >> 4) + 2;
								c->vibrate = (comlo & 15) + 1;
#endif
								break;
							case 0xf6:
#ifdef ENABLE_WHEEL
								c->glideto = comlo;
#endif
								break;
							case 0xf5:
								c->finetune = comlo;
								break;
							case 0xf4:
								if(!hardfade) {
									allvolume = mainvolume = comlo;
									fadeonoff = 0;
								}
								break;
							case 0xf3:
								if(!hardfade) fadeonoff = comlo;
								break;
							case 0xf2:
#ifdef ENABLE_TREM
								c->trmstay = comlo;
#endif
								break;
							case 0xf1:
								buffer[ 0 ] = 10;
								buffer[ 1 ] = ( comlo & 0x3F ) * 127 / 63;
								tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ chan ], buffer, 2 ) );
								break;
							case 0xf0:
								buffer[ 0 ] = comlo & 0x7F;
								tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::program_change, last_channel[ chan ], buffer, 1 ) );
								break;
							default:
#ifdef ENABLE_WHEEL
								if(comhi < 0xa0)
									c->glideto = comhi & 0x1f;
#endif
								break;
							}
						}
						else
						{
							unsigned char	sound;
							unsigned short	high;
							signed char	transp = transpose << 1;
							transp >>= 1;

							if(transpose & 128) {
								sound = (comlo + transp) & maxsound;
								high = comhi << 4;
							} else {
								sound = comlo & maxsound;
								high = (comhi + transp) << 4;
							}

							/*
							PASCAL:
							sound = comlo & maxsound;
							high = (comhi + (((transpose + 0x24) & 0xff) - 0x24)) << 4;
							*/

							if( !channel_delay[ chan ] )
							{
								playsound( current_instrument, patches, last_note, last_channel, last_instrument, last_volume, last_sent_volume,
#ifdef ENABLE_WHEEL
									last_pitch_wheel,
#endif
									c, allvolume, current_timestamp, sound, chan, high, tracks[ chan ] );
								ticks_without_notes[ last_channel[ chan ] ] = 0;
							}
							else
							{
								c->chancheat.chandelay = channel_delay[chan];
								c->chancheat.sound = sound;
								c->chancheat.high = high;
							}
						}
					}
					c->packpos++;
				}
				else
				{
					c->packwait--;
				}
			}

			tempo_now = tempo;
			/*
			The continue table is updated here, but this is only used in the
			original player, which can be paused in the middle of a song and then
			unpaused. Since AdPlug does all this for us automatically, we don't
			have a continue table here. The continue table update code is noted
			here for reference only.

			if(!pattplay) {
			conttab[speed & maxcont].position = posplay & 0xff;
			conttab[speed & maxcont].tempo = tempo;
			}
			*/
			pattplay++;
			if(vbreak)
			{
				pattplay = 0;
				for(i = 0; i < 9; i++) channel[i].packpos = channel[i].packwait = 0;
				posplay = jumppos;
				if ( posplay >= position_count ) throw exception_io_data( "Invalid LDS position jump" );
			}
			else if(pattplay >= pattern_length)
			{
				pattplay = 0;
				for(i = 0; i < 9; i++) channel[i].packpos = channel[i].packwait = 0;
				posplay = (posplay + 1) & maxpos;
				if ( posplay >= position_count ) playing = false; //throw exception_io_data( "LDS reached the end without a loop or end command" );
			}
		}
		else
		{
			tempo_now--;
		}

		// make effects
		for(chan = 0; chan < 9; ++chan)
		{
			c = &channel[chan];
			if(c->keycount > 0)
			{
				if( c->keycount == 1 && last_note[ chan ] != 0xFF )
				{
					buffer[ 0 ] = last_note[ chan ];
					buffer[ 1 ] = 127;
					tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::note_off, last_channel[ chan ], buffer, 2 ) );
					last_note[ chan ] = 0xFF;
#ifdef ENABLE_WHEEL
					if ( 0 != last_pitch_wheel[ last_channel[ chan ] ] )
					{
						buffer[ 0 ] = 0;
						buffer[ 1 ] = 64;
						tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
						last_pitch_wheel[ last_channel[ chan ] ] = 0;
						c->lasttune = 0;
						c->gototune = 0;
					}
#endif
				}
				c->keycount--;
			}

#ifdef ENABLE_ARP
			// arpeggio
			if(c->arp_size == 0)
			{
				arpreg = 0;
			}
			else
			{
				arpreg = c->arp_tab[c->arp_pos] << 4;
				if(arpreg == -0x800)
				{
					if(c->arp_pos > 0) c->arp_tab[0] = c->arp_tab[c->arp_pos - 1];
					c->arp_size = 1; c->arp_pos = 0;
					arpreg = c->arp_tab[0] << 4;
				}

				if(c->arp_count == c->arp_speed) {
					c->arp_pos++;
					if(c->arp_pos >= c->arp_size) c->arp_pos = 0;
					c->arp_count = 0;
				}
				else
				{
					c->arp_count++;
				}
			}
#endif

#ifdef ENABLE_WHEEL
			// glide & portamento
			if(c->lasttune != c->gototune)
			{
				if(c->lasttune > c->gototune)
				{
					if(c->lasttune - c->gototune < c->portspeed)
					{
						c->lasttune = c->gototune;
					}
					else
					{
						c->lasttune -= c->portspeed;
					}
				}
				else
				{
					if(c->gototune - c->lasttune < c->portspeed)
					{
						c->lasttune = c->gototune;
					}
					else
					{
						c->lasttune += c->portspeed;
					}
				}

#ifdef ENABLE_ARP
				arpreg +=
#else
				t_int16 arpreg =
#endif
					c->lasttune;

				if ( arpreg != last_pitch_wheel[ last_channel[ chan ] ] )
				{
					buffer[ 0 ] = WHEEL_SCALE_LOW( arpreg );
					buffer[ 1 ] = WHEEL_SCALE_HIGH( arpreg );
					tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
					last_pitch_wheel[ last_channel[ chan ] ] = arpreg;
				}
			} else
			{
#ifdef ENABLE_VIB
				// vibrato
				if(!c->vibwait)
				{
					if(c->vibrate)
					{
						wibc = vibtab[c->vibcount & 0x3f] * c->vibrate;

						if((c->vibcount & 0x40) == 0)
							tune = c->lasttune + (wibc >> 8);
						else
							tune = c->lasttune - (wibc >> 8);

#ifdef ENABLE_ARP
						tune += arpreg;
#endif

						if ( tune != last_pitch_wheel[ last_channel[ chan ] ] )
						{
							buffer[ 0 ] = WHEEL_SCALE_LOW( tune );
							buffer[ 1 ] = WHEEL_SCALE_HIGH( tune );
							tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
							last_pitch_wheel[ last_channel[ chan ] ] = tune;
						}

						c->vibcount += c->vibspeed;
					}
#ifdef ENABLE_ARP
					else if(c->arp_size != 0)
					{	// no vibrato, just arpeggio
						tune = c->lasttune + arpreg;

						if ( tune != last_pitch_wheel[ last_channel[ chan ] ] )
						{
							buffer[ 0 ] = WHEEL_SCALE_LOW( tune );
							buffer[ 1 ] = WHEEL_SCALE_HIGH( tune );
							tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
							last_pitch_wheel[ last_channel[ chan ] ] = tune;
						}
					}
#endif
				}
#ifdef ENABLE_ARP
				else
#endif
#endif
#ifdef ENABLE_ARP
				{	// no vibrato, just arpeggio
#ifdef ENABLE_VIB
					c->vibwait--;
#endif

					if(c->arp_size != 0)
					{
						tune = c->lasttune + arpreg;

						if ( tune != last_pitch_wheel[ last_channel[ chan ] ] )
						{
							buffer[ 0 ] = WHEEL_SCALE_LOW( tune );
							buffer[ 1 ] = WHEEL_SCALE_HIGH( tune );
							tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
							last_pitch_wheel[ last_channel[ chan ] ] = tune;
						}
					}
				}
#endif
			}
#endif

#ifdef ENABLE_TREM
			unsigned volume = last_volume[ chan ];

			// tremolo (modulator)
			if(!c->trmwait)
			{
				if(c->trmrate)
				{
					tremc = tremtab[c->trmcount & 0x7f] * c->trmrate;
					if((tremc >> 7) <= volume)
						volume = volume - (tremc >> 7);
					else
						volume = 0;

					c->trmcount += c->trmspeed;
				}
			}
			else
			{
				c->trmwait--;
			}

			// tremolo (carrier)
			if(!c->trcwait)
			{
				if(c->trcrate)
				{
					tremc = tremtab[c->trccount & 0x7f] * c->trcrate;
					if((tremc >> 7) <= volume)
						volume = volume - (tremc >> 8);
					else
						volume = 0;
				}
			}
			else
			{
				c->trcwait--;
			}

			if ( allvolume )
			{
				volume = volume * allvolume / 255;
			}

			if ( volume != last_sent_volume[ last_channel[ chan ] ] )
			{
				buffer[ 0 ] = 7;
				buffer[ 1 ] = volume;
				tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ chan ], buffer, 2 ) );
				last_sent_volume[ last_channel[ chan ] ] = volume;
			}
#endif

		}

		++current_timestamp;
	}

	--current_timestamp;

	for ( unsigned i = 0; i < 9; ++i )
	{
		midi_track & track = tracks[ i ];
		unsigned count = track.get_count();
		if ( count > 1 )
		{
			if ( last_note[ i ] != 0xFF )
			{
				buffer[ 0 ] = last_note[ i ];
				buffer[ 1 ] = 127;
				track.add_event( midi_event( current_timestamp + channel[ i ].keycount, midi_event::note_off, last_channel[ i ], buffer, 2 ) );
#ifdef ENABLE_WHEEL
				if ( last_pitch_wheel[ last_channel[ i ] ] != 0 )
				{
					buffer[ 0 ] = 0;
					buffer[ 1 ] = 0x40;
					track.add_event( midi_event( current_timestamp + channel[ i ].keycount, midi_event::pitch_wheel, last_channel[ i ], buffer, 2 ) );
				}
#endif
			}
			p_out.add_track( track );
		}
	}
}
