#include <foobar2000.h>

// Remove Windows versions and use std namespace versions
#undef min
#undef max

#include "ADLPlayer.h"

#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdarg.h>
#include <vector> // vector
#include <cmath>  // exp, log, ceil
#include <limits>

#include <assert.h>

#include <algorithm>

#include "dbopl.h"

#include "adldata.h"

static const unsigned short Operators[23*2] =
    {0x000,0x003,0x001,0x004,0x002,0x005, // operators  0, 3,  1, 4,  2, 5
     0x008,0x00B,0x009,0x00C,0x00A,0x00D, // operators  6, 9,  7,10,  8,11
     0x010,0x013,0x011,0x014,0x012,0x015, // operators 12,15, 13,16, 14,17
     0x100,0x103,0x101,0x104,0x102,0x105, // operators 18,21, 19,22, 20,23
     0x108,0x10B,0x109,0x10C,0x10A,0x10D, // operators 24,27, 25,28, 26,29
     0x110,0x113,0x111,0x114,0x112,0x115, // operators 30,33, 31,34, 32,35
     0x010,0x013,   // operators 12,15
     0x014,0xFFF,   // operator 16
     0x012,0xFFF,   // operator 14
     0x015,0xFFF,   // operator 17
     0x011,0xFFF }; // operator 13

static const unsigned short Channels[23] =
    {0x000,0x001,0x002, 0x003,0x004,0x005, 0x006,0x007,0x008, // 0..8
     0x100,0x101,0x102, 0x103,0x104,0x105, 0x106,0x107,0x108, // 9..17 (secondary set)
     0x006,0x007,0x008,0xFFF,0xFFF }; // <- hw percussions, 0xFFF = no support for pitch/pan

/*
    In OPL3 mode:
         0    1    2    6    7    8     9   10   11    16   17   18
       op0  op1  op2 op12 op13 op14  op18 op19 op20  op30 op31 op32
       op3  op4  op5 op15 op16 op17  op21 op22 op23  op33 op34 op35
         3    4    5                   13   14   15
       op6  op7  op8                 op24 op25 op26
       op9 op10 op11                 op27 op28 op29
    Ports:
        +0   +1   +2  +10  +11  +12  +100 +101 +102  +110 +111 +112
        +3   +4   +5  +13  +14  +15  +103 +104 +105  +113 +114 +115
        +8   +9   +A                 +108 +109 +10A
        +B   +C   +D                 +10B +10C +10D

    Percussion:
      bassdrum = op(0): 0xBD bit 0x10, operators 12 (0x10) and 15 (0x13) / channels 6, 6b
      snare    = op(3): 0xBD bit 0x08, operators 16 (0x14)               / channels 7b
      tomtom   = op(4): 0xBD bit 0x04, operators 14 (0x12)               / channels 8
      cym      = op(5): 0xBD bit 0x02, operators 17 (0x17)               / channels 8b
      hihat    = op(2): 0xBD bit 0x01, operators 13 (0x11)               / channels 7


    In OPTi mode ("extended FM" in 82C924, 82C925, 82C931 chips):
         0   1   2    3    4    5    6    7     8    9   10   11   12   13   14   15   16   17
       op0 op4 op6 op10 op12 op16 op18 op22  op24 op28 op30 op34 op36 op38 op40 op42 op44 op46
       op1 op5 op7 op11 op13 op17 op19 op23  op25 op29 op31 op35 op37 op39 op41 op43 op45 op47
       op2     op8      op14      op20       op26      op32
       op3     op9      op15      op21       op27      op33    for a total of 6 quad + 12 dual
    Ports: ???
      

*/


struct OPL3
{
    unsigned PcmRate;
    unsigned NumChannels;
    unsigned NumFourOps;
    unsigned NumCards;
    bool HighTremoloMode;
    bool HighVibratoMode;
    bool AdlPercussionMode;
	bool FullPan;

	OPL3() : PcmRate( 44100 ), NumFourOps( 7 ), NumCards( 2 ),
		HighTremoloMode( false ), HighVibratoMode( false ),
		AdlPercussionMode( false ), FullPan( false )
	{
	}

    std::vector<DBOPL::Handler> cards;
private:
    std::vector<unsigned short> ins; // index to adl[], cached, needed by Touch()
	std::vector<unsigned int> vol;   // note volume, cached, needed by Pan() in FullPan mode
	std::vector<unsigned char> pan;  // note panning, cached, needed by Touch() in FullPan mode
    std::vector<unsigned char> pit;  // value poked to B0, cached, needed by NoteOff)(
    std::vector<unsigned char> regBD;
public:
    std::vector<char> four_op_category; // 1 = quad-master, 2 = quad-slave, 0 = regular
                                        // 3 = percussion BassDrum
                                        // 4 = percussion Snare
                                        // 5 = percussion Tom
                                        // 6 = percussion Crash cymbal
                                        // 7 = percussion Hihat
                                        // 8 = percussion slave

    void Poke(unsigned card, unsigned index, unsigned value, bool both = true)
    {
        cards[card].WriteReg(index, value);
		if (FullPan && both) cards[card + NumCards].WriteReg(index, value);
    }
    void NoteOff_Real(unsigned c)
    {
        unsigned card = c/23, cc = c%23;
        if(cc >= 18)
        {
            regBD[card] &= ~(0x10 >> (cc-18));
            Poke(card, 0xBD, regBD[card], false);
            return;
        }
        Poke(card, 0xB0 + Channels[cc], pit[c] & 0xDF, false);
    }
	void NoteOff(unsigned c)
	{
		NoteOff_Real(c);
		if ( FullPan ) NoteOff_Real( c + 23 * NumCards );
	}
    void NoteOn_Real(unsigned c, double hertz) // Hertz range: 0..131071
    {
        unsigned card = c/23, cc = c%23;
        unsigned x = 0x2000;
        while(hertz >= 1023.5) { hertz /= 2.0; x += 0x400; } // Calculate octave
        x += (int)(hertz + 0.5);
        unsigned chn = Channels[cc];
        if(cc >= 18)
        {
            regBD[card] |= (0x10 >> (cc-18));
            Poke(card, 0x0BD, regBD[card], false);
            x &= ~0x2000;
            //x |= 0x800; // for test
        }
        if(chn != 0xFFF)
        {
            Poke(card, 0xA0 + chn, x & 0xFF, false);
            Poke(card, 0xB0 + chn, pit[c] = x >> 8, false);
        }
    }
    void NoteOn(unsigned c, double hertz, double hertz_phased) // Hertz range: 0..131071
	{
		NoteOn_Real( c, hertz );
		if ( FullPan ) NoteOn_Real( c + 23 * NumCards, hertz_phased );
	}
    void Touch_Real(unsigned c, unsigned volume, bool both = true)
    {
        if(volume > 63) volume = 63;
        unsigned card = c/23, cc = c%23;
        unsigned i = ins[c %= (NumCards * 23)], o1 = Operators[cc*2], o2 = Operators[cc*2+1];
        unsigned x = adl[i].modulator_40, y = adl[i].carrier_40;
		bool do_modulator;
		bool do_carrier;
		unsigned mode = 1;
		if ( four_op_category[c] == 0 || four_op_category[c] == 3 ) mode = adl[i].feedconn & 1;
		else if ( four_op_category[c] == 1 || four_op_category[c] == 2 )
		{
			unsigned i0, i1;
			if ( four_op_category[c] == 1 )
			{
				i0 = i;
				i1 = ins[c + 3];
				mode = 2;
			}
			else
			{
				i0 = ins[c - 3];
				i1 = i;
				mode = 6;
			}
			mode += (adl[i0].feedconn & 1) + (adl[i1].feedconn & 1) * 2;
		}
		switch (mode)
		{
		case 0: // 2op FM
			do_modulator = false;
			do_carrier = true;
			break;

		case 1: // 2op AM
			do_modulator = true;
			do_carrier = true;
			break;

		case 2: // 4op FM-FM ops 1&2
			do_modulator = false;
			do_carrier = false;
			break;

		case 3: // 4op AM-FM ops 1&2
			do_modulator = true;
			do_carrier = false;
			break;

		case 4: // 4op FM-AM ops 1&2
			do_modulator = false;
			do_carrier = true;
			break;

		case 5: // 4op AM-AM ops 1&2
			do_modulator = true;
			do_carrier = false;
			break;

		case 6: // 4op FM-FM ops 3&4
			do_modulator = false;
			do_carrier = true;
			break;

		case 7: // 4op AM-FM ops 3&4
			do_modulator = false;
			do_carrier = true;
			break;

		case 8: // 4op FM-AM ops 3&4
			do_modulator = false;
			do_carrier = true;
			break;

		case 9: // 4op AM-AM ops 3&4
			do_modulator = true;
			do_carrier = true;
			break;
		}
        Poke(card, 0x40+o1, do_modulator ? (x|63) - volume + volume*(x&63)/63 : x, both);
        if(o2 != 0xFFF)
        Poke(card, 0x40+o2, do_carrier ? (y|63) - volume + volume*(y&63)/63 : y, both);
        // Correct formula (ST3, AdPlug):
        //   63-((63-(instrvol))/63)*chanvol
        // Reduces to (tested identical):
        //   63 - chanvol + chanvol*instrvol/63
        // Also (slower, floats):
        //   63 + chanvol * (instrvol / 63.0 - 1)
    }
    void Touch(unsigned c, unsigned volume) // Volume maxes at 127*127*127
    {
        // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
		if ( FullPan )
		{
			vol[ c ] = volume;
			unsigned p = pan[ c ];
			unsigned volume_left = volume * (128 - p) / 64;
			unsigned volume_right = volume * p / 64;
			Touch_Real(c, volume_left>8725  ? std::log((float)volume_left)*11.541561 + (0.5 - 104.22845) : 0, false);
			Touch_Real(c + NumCards * 23, volume_right>8725  ? std::log((float)volume_right)*11.541561 + (0.5 - 104.22845) : 0, false);
		}
		else
		{
			Touch_Real(c, volume>8725  ? std::log((float)volume)*11.541561 + (0.5 - 104.22845) : 0);
		}
        // The incorrect formula below: SOLVE(V=127^3 * (2^(A/63)-1), A)
        //Touch_Real(c, volume>11210 ? 91.61112 * std::log(4.8819E-7*volume + 1.0)+0.5 : 0);
    }
    void Patch(unsigned c, unsigned i)
    {
        unsigned card = c/23, cc = c%23;
        static const unsigned char data[4] = {0x20,0x60,0x80,0xE0};
        ins[c] = i;
        unsigned o1 = Operators[cc*2+0], o2 = Operators[cc*2+1];
        unsigned x = adl[i].modulator_E862, y = adl[i].carrier_E862;
        for(unsigned a=0; a<4; ++a)
        {
            Poke(card, data[a]+o1, x&0xFF); x>>=8;
            if(o2 != 0xFFF)
            Poke(card, data[a]+o2, y&0xFF); y>>=8;
        }
		if ( FullPan )
		{
			Poke(card, 0xC0 + Channels[cc], adl[ins[c]].feedconn | 0x10, false);
			Poke(card + NumCards, 0xC0 + Channels[cc], adl[ins[c]].feedconn | 0x20, false);
		}
    }
    void Pan(unsigned c, unsigned value)
    {
        unsigned card = c/23, cc = c%23;
        if(Channels[cc] != 0xFFF)
		{
			if ( FullPan )
			{
				pan[c] = value;
				Touch(c, vol[c]);
			}
			else
			{
				unsigned realvalue = 0x00;
                if(value  < 64+32) realvalue |= 0x10;
                if(value >= 64-32) realvalue |= 0x20;
				Poke(card, 0xC0 + Channels[cc], adl[ins[c]].feedconn | realvalue);
			}
		}
    }
    void Silence() // Silence all OPL channels.
    {
        for(unsigned c=0; c<NumChannels; ++c) { NoteOff(c); Touch_Real(c,0); }
    }
    void Reset()
    {
		unsigned RealNumCards = NumCards * (FullPan ? 2 : 1);
        cards.resize(RealNumCards);
        NumChannels = NumCards * 23;
        ins.resize(NumChannels,     189);
		if (FullPan)
		{
			vol.resize(NumChannels, 0);
			pan.resize(NumChannels, 0x40);
		}
        pit.resize(NumChannels * (FullPan ? 2 : 1),       0);
        regBD.resize(RealNumCards);
        four_op_category.resize(NumChannels);
        for(unsigned p=0, a=0; a<NumCards; ++a)
        {
            for(unsigned b=0; b<18; ++b) four_op_category[p++] = 0;
            for(unsigned b=0; b< 5; ++b) four_op_category[p++] = 8;
        }

        static const short data[] =
        { 0x004,96, 0x004,128,        // Pulse timer
          0x105, 0, 0x105,1, 0x105,0, // Pulse OPL3 enable
          0x001,32, 0x105,1           // Enable wave, OPL3 extensions
        };
        unsigned fours = NumFourOps;
        for(unsigned card=0; card<NumCards; ++card)
        {
            cards[card].Init(PcmRate);
			if (FullPan) cards[card+NumCards].Init(PcmRate);
            for(unsigned a=0; a< 18; ++a) Poke(card, 0xB0+Channels[a], 0x00);
            for(unsigned a=0; a< sizeof(data)/sizeof(*data); a+=2)
                Poke(card, data[a], data[a+1]);
            Poke(card, 0x0BD, regBD[card] = (HighTremoloMode*0x80
                                           + HighVibratoMode*0x40
                                           + AdlPercussionMode*0x20));
            unsigned fours_this_card = std::min(fours, 6u);
            Poke(card, 0x104, (1 << fours_this_card) - 1);
            //fprintf(stderr, "Card %u: %u four-ops.\n", card, fours_this_card);
            fours -= fours_this_card;
        }

        // Mark all channels that are reserved for four-operator function
        if(AdlPercussionMode)
            for(unsigned a=0; a<NumCards; ++a)
            {
                for(unsigned b=0; b<5; ++b) four_op_category[a*23 + 18 + b] = b+3;
                for(unsigned b=0; b<3; ++b) four_op_category[a*23 + 6  + b] = 8;
            }

        unsigned nextfour = 0;
        for(unsigned a=0; a<NumFourOps; ++a)
        {
            four_op_category[nextfour  ] = 1;
            four_op_category[nextfour+3] = 2;
            switch(a % 6)
            {
                case 0: case 1: nextfour += 1; break;
                case 2:         nextfour += 9-2; break;
                case 3: case 4: nextfour += 1; break;
                case 5:         nextfour += 23-9-2; break;
            }
        }

        /**/
        /*
        In two-op mode, channels 0..8 go as follows:
                      Op1[port]  Op2[port]
          Channel 0:  00  00     03  03
          Channel 1:  01  01     04  04
          Channel 2:  02  02     05  05
          Channel 3:  06  08     09  0B
          Channel 4:  07  09     10  0C
          Channel 5:  08  0A     11  0D
          Channel 6:  12  10     15  13
          Channel 7:  13  11     16  14
          Channel 8:  14  12     17  15
        In four-op mode, channels 0..8 go as follows:
                      Op1[port]  Op2[port]  Op3[port]  Op4[port]
          Channel 0:  00  00     03  03     06  08     09  0B
          Channel 1:  01  01     04  04     07  09     10  0C
          Channel 2:  02  02     05  05     08  0A     11  0D
          Channel 3:  CHANNEL 0 SLAVE
          Channel 4:  CHANNEL 1 SLAVE
          Channel 5:  CHANNEL 2 SLAVE
          Channel 6:  12  10     15  13
          Channel 7:  13  11     16  14
          Channel 8:  14  12     17  15
         Same goes principally for channels 9-17 respectively.
        */

        Silence();
    }
};

static const char PercussionMap[256] =
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"//GM
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // 3 = bass drum
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // 4 = snare
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // 5 = tom
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // 6 = cymbal
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // 7 = hihat
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"//GP0
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"//GP16
//2 3 4 5 6 7 8 940 1 2 3 4 5 6 7
"\0\0\0\3\3\7\4\7\4\5\7\5\7\5\7\5"//GP32
//8 950 1 2 3 4 5 6 7 8 960 1 2 3
"\5\6\5\6\6\0\5\6\0\6\0\6\5\5\5\5"//GP48
//4 5 6 7 8 970 1 2 3 4 5 6 7 8 9
"\5\0\0\0\0\0\7\0\0\0\0\0\0\0\0\0"//GP64
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

class MIDIplay
{
	double arpeggio_cache;
	unsigned arpeggio_counter;
	bool Chorus;

public:
	MIDIplay() : arpeggio_cache(0.0), arpeggio_counter(0), Chorus(true) { }

private:
    // Persistent settings for each MIDI channel
    struct MIDIchannel
    {
        unsigned short portamento;
        unsigned char bank_lsb, bank_msb;
        unsigned char patch;
        unsigned char volume, expression;
        unsigned char panning, vibrato, sustain;
        double bend, bendsense;
        double vibpos, vibspeed, vibdepth;
        long   vibdelay;
        unsigned char lastlrpn,lastmrpn; bool nrpn;
		bool drum_mode;
        struct NoteInfo
        {
            // Current pressure
            unsigned char  vol;
            // Tone selected on noteon:
            signed short tone;
            // Patch selected on noteon; index to banks[AdlBank][]
            unsigned char midiins;
            // Index to physical adlib data structure, adlins[]
            unsigned short insmeta;
            // List of adlib channels it is currently occupying.
            std::map<unsigned short/*adlchn*/,
                     unsigned short/*ins, inde to adl[]*/
                    > phys;
        };
        typedef std::map<unsigned char,NoteInfo> activenotemap_t;
        typedef activenotemap_t::iterator activenoteiterator;
        activenotemap_t activenotes;

        MIDIchannel()
            : portamento(0),
              bank_lsb(0), bank_msb(0), patch(0),
              volume(100),expression(100),
              panning(0x40), vibrato(0), sustain(0),
              bend(0.0), bendsense(2 / 8192.0),
              vibpos(0), vibspeed(2*3.141592653*5.0),
              vibdepth(0.5/127), vibdelay(0),
              lastlrpn(127),lastmrpn(127),nrpn(false),
			  drum_mode(false),
              activenotes() { }
    };
    std::vector<MIDIchannel> Ch;

    // Additional information about AdLib channels
    struct AdlChannel
    {
        // For collisions
        struct Location
        {
            unsigned short MidCh;
            unsigned char  note;
            bool operator==(const Location&b) const
                { return MidCh==b.MidCh && note==b.note; }
            bool operator< (const Location&b) const
                { return MidCh<b.MidCh || (MidCh==b.MidCh&& note<b.note); }
        };
        struct LocationData
        {
            bool sustained;
            unsigned short ins;  // a copy of that in phys[]
            long kon_time_until_neglible;
            long vibdelay;
        };
        typedef std::map<Location, LocationData> users_t;
        users_t users;

        // If the channel is keyoff'd
        long koff_time_until_neglible;
        // For channel allocation:
        AdlChannel(): users(), koff_time_until_neglible(0) { }

        void AddAge(long ms)
        {
            if(users.empty())
                koff_time_until_neglible =
                    std::max(koff_time_until_neglible-ms, -0x1FFFFFFFl);
            else
            {
                koff_time_until_neglible = 0;
                for(users_t::iterator i = users.begin(); i != users.end(); ++i)
                {
                    i->second.kon_time_until_neglible =
                    std::max(i->second.kon_time_until_neglible-ms, -0x1FFFFFFFl);
                    i->second.vibdelay += ms;
                }
            }
        }
    };
    std::vector<AdlChannel> ch;

public:
    OPL3 opl;

    unsigned AdlBank;

	void init() {
		Ch.resize( 64 );
        ch.resize(opl.NumChannels);
	}

	void setDrumMode( unsigned ch, bool mode ) {
		Ch[ ch ].drum_mode = mode;
	}

	void setChorus( bool _Chorus ) {
		Chorus = _Chorus;
	}

    /* Periodic tick handler.
     *   Input: s           = seconds since last call
     *   Input: granularity = don't expect intervals smaller than this, in seconds
     *   Output: desired number of seconds until next call
     */
    void Tick(double s)
    {
        for(unsigned c = 0; c < opl.NumChannels; ++c)
            ch[c].AddAge(s * 1000);

        UpdateVibrato(s);
        UpdateArpeggio(s);
    }

private:
    enum { Upd_Patch  = 0x1,
           Upd_Pan    = 0x2,
           Upd_Volume = 0x4,
           Upd_Pitch  = 0x8,
           Upd_All    = Upd_Pan + Upd_Volume + Upd_Pitch,
           Upd_Off    = 0x20 };

    void NoteUpdate
        (unsigned MidCh,
         MIDIchannel::activenoteiterator i,
         unsigned props_mask,
         int select_adlchn = -1)
    {
        MIDIchannel::NoteInfo& info = i->second;
        const int tone    = info.tone;
        const int vol     = info.vol;
        const int midiins = info.midiins;
        const int insmeta = info.insmeta;

        AdlChannel::Location my_loc;
        my_loc.MidCh = MidCh;
        my_loc.note  = i->first;

        for(std::map<unsigned short,unsigned short>::iterator
            jnext = info.phys.begin();
            jnext != info.phys.end();
           )
        {
            std::map<unsigned short,unsigned short>::iterator j(jnext++);
            int c   = j->first;
            int ins = j->second;
            if(select_adlchn >= 0 && c != select_adlchn) continue;

			// Both patches must be applied before volume and panning changes,
			// due to how the volume/operator setting logic works with four
			// operator voices now
            if(props_mask & Upd_Patch)
            {
                opl.Patch(c, ins);
                AdlChannel::LocationData& d = ch[c].users[my_loc];
                d.sustained = false; // inserts if necessary
                d.vibdelay  = 0;
                d.kon_time_until_neglible = adlins[insmeta].ms_sound_kon;
                d.ins       = ins;
            }
        }

        for(std::map<unsigned short,unsigned short>::iterator
            jnext = info.phys.begin();
            jnext != info.phys.end();
           )
        {
            std::map<unsigned short,unsigned short>::iterator j(jnext++);
            int c   = j->first;
            int ins = j->second;
            if(select_adlchn >= 0 && c != select_adlchn) continue;

            if(props_mask & Upd_Off) // note off
            {
                if(Ch[MidCh].sustain == 0)
                {
                    AdlChannel::users_t::iterator k = ch[c].users.find(my_loc);
                    if(k != ch[c].users.end())
                        ch[c].users.erase(k);

                    if(ch[c].users.empty())
                    {
                        opl.NoteOff(c);
                        ch[c].koff_time_until_neglible =
                            adlins[insmeta].ms_sound_koff;
                    }
                }
                else
                {
                    // Sustain: Forget about the note, but don't key it off.
                    //          Also will avoid overwriting it very soon.
                    AdlChannel::LocationData& d = ch[c].users[my_loc];
                    d.sustained = true; // note: not erased!
                }
                info.phys.erase(j);
                continue;
            }
            if(props_mask & Upd_Pan)
            {
				opl.Pan(c, Ch[MidCh].panning);
            }
            if(props_mask & Upd_Volume)
            {
                int volume = vol * Ch[MidCh].volume * Ch[MidCh].expression;
                /* If the channel has arpeggio, the effective volume of
                 * *this* instrument is actually lower due to timesharing.
                 * To compensate, add extra volume that corresponds to the
                 * time this note is *not* heard.
                 * Empirical tests however show that a full equal-proportion
                 * increment sounds wrong. Therefore, using the square root.
                 */
                //volume = (int)(volume * std::sqrt( (double) ch[c].users.size() ));
                opl.Touch(c, volume);
            }
            if(props_mask & Upd_Pitch)
            {
                AdlChannel::LocationData& d = ch[c].users[my_loc];
                // Don't bend a sustained note
                if(!d.sustained)
                {
					const bool double_ins = (adlins[insmeta].flags & adlinsdata::Flag_Pseudo4op) != 0;
					const bool second_ins = double_ins && ins == adlins[insmeta].adlno2;
					double bend = Ch[MidCh].bend + adl[ins].finetune;
					double phase = 0.0;
					const double phase_offset = Chorus ? 0.125 : 0.0;
					if (second_ins)
						phase += (adl[ins].p4o_second_offset * (1.0 / 64.0));
					double hertz, hertz_phased;
					if(Ch[MidCh].vibrato && d.vibdelay >= Ch[MidCh].vibdelay)
						bend += Ch[MidCh].vibrato * Ch[MidCh].vibdepth * std::sin(Ch[MidCh].vibpos);
					hertz = 172.00093 * std::exp(0.057762265 * (tone + bend + phase));
					hertz_phased = 172.00093 * std::exp(0.057762265 * (tone + bend + phase + phase_offset));
					if ( hertz == std::numeric_limits<double>::infinity() )
					{
						continue; // This should never happen, but better safe than sorry. Also blocked off to ease setting a breakpoint.
					}
                    opl.NoteOn(c, hertz, hertz_phased);
                }
            }
        }
        if(info.phys.empty())
            Ch[MidCh].activenotes.erase(i);
    }

public:
    void HandleEvent(const unsigned char * event, unsigned int event_size, unsigned int port)
    {
        if ( event_size < 1 ) return;
        unsigned char byte = event[ 0 ];
        if(byte == 0xF7 || byte == 0xF0) // Ignore SysEx
        {
            return;
        }
        if(byte == 0xFF)
        {
            return;
        }
        // Any normal event (80..EF)
        /*UI.PrintLn("@%X Track %u: %02X %02X",
            CurrentPosition.track[tk].ptr-1, (unsigned)tk, byte,
            TrackData[tk][CurrentPosition.track[tk].ptr]);*/
        unsigned MidCh = (byte & 0x0F) + (port & 3) * 16, EvType = byte >> 4;

        switch(EvType)
        {
            case 0x8: // Note off
            case 0x9: // Note on
            {
                if ( event_size < 3 ) return;
                int note = event[ 1 ];
                int  vol = event[ 2 ];
                //if(MidCh != 9) note -= 12; // HACK
                NoteOff(MidCh, note);
                // On Note on, Keyoff the note first, just in case keyoff
                // was omitted; this fixes Dance of sugar-plum fairy
                // by Microsoft. Now that we've done a Keyoff,
                // check if we still need to do a Keyon.
                // vol=0 and event 8x are both Keyoff-only.
                if(vol == 0 || EvType == 0x8) break;

                unsigned midiins = Ch[MidCh].patch;
				if(Ch[MidCh].drum_mode) midiins = 128 + note; // Percussion instrument

                /*
                if(MidCh%16 == 9 || (midiins != 32 && midiins != 46 && midiins != 48 && midiins != 50))
                    break; // HACK
                if(midiins == 46) vol = (vol*7)/10;          // HACK
                if(midiins == 48 || midiins == 50) vol /= 4; // HACK
                */
                //if(midiins == 56) vol = vol*6/10; // HACK

                int meta = banks[AdlBank][midiins];
                int tone = note;
                if(adlins[meta].tone)
                {
                    if(adlins[meta].tone < 20)
                        tone += adlins[meta].tone;
                    else if(adlins[meta].tone < 128)
                        tone = adlins[meta].tone;
                    else
                        tone -= adlins[meta].tone-128;
                }
                int i[2] = { adlins[meta].adlno1, adlins[meta].adlno2 };
                bool pseudo_fourop = !!(adlins[meta].flags & adlinsdata::Flag_Pseudo4op);

                if(opl.AdlPercussionMode && PercussionMap[midiins & 0xFF]) i[1] = i[0];

                // Allocate AdLib channel (the physical sound channel for the note)
                int adlchannel[2] = { -1, -1 };
                for(unsigned ccount = 0; ccount < 2; ++ccount)
                {
                    if(ccount == 1)
                    {
                        if(i[0] == i[1]) break; // No secondary channel
                        if(adlchannel[0] == -1) break; // No secondary if primary failed
                    }

                    int c = -1;
                    long bs = -0x7FFFFFFFl;
                    for(int a = 0; a < (int)opl.NumChannels; ++a)
                    {
                        if(ccount == 1 && a == adlchannel[0]) continue;
                        // ^ Don't use the same channel for primary&secondary

                        if(i[0] == i[1] || pseudo_fourop)
                        {
                            // Only use regular channels
                            int expected_mode = 0;
                            if(opl.AdlPercussionMode)
                                expected_mode = PercussionMap[midiins & 0xFF];
                            if(opl.four_op_category[a] != expected_mode)
                                continue;
                        }
                        else
                        {
                            if(ccount == 0)
                            {
                                // Only use four-op master channels
                                if(opl.four_op_category[a] != 1)
                                    continue;
                            }
                            else
                            {
                                // The secondary must be played on a specific channel.
                                if(a != adlchannel[0] + 3)
                                    continue;
                            }
                        }

                        long s = CalculateAdlChannelGoodness(a, i[ccount], MidCh);
                        if(s > bs) { bs=s; c = a; } // Best candidate wins
                    }

                    if(c < 0)
                    {
                        //UI.PrintLn("ignored unplaceable note");
                        continue; // Could not play this note. Ignore it.
                    }
                    PrepareAdlChannelForNewNote(c, i[ccount]);
                    adlchannel[ccount] = c;
                }
                if(adlchannel[0] < 0 && adlchannel[1] < 0)
                {
                    // The note could not be played, at all.
                    break;
                }
                //UI.PrintLn("i1=%d:%d, i2=%d:%d", i[0],adlchannel[0], i[1],adlchannel[1]);

                // Allocate active note for MIDI channel
                std::pair<MIDIchannel::activenoteiterator,bool>
                    ir = Ch[MidCh].activenotes.insert(
                        std::make_pair(note, MIDIchannel::NoteInfo()));
                ir.first->second.vol     = vol;
                ir.first->second.tone    = tone;
                ir.first->second.midiins = midiins;
                ir.first->second.insmeta = meta;
                for(unsigned ccount=0; ccount<2; ++ccount)
                {
                    int c = adlchannel[ccount];
                    if(c < 0) continue;
                    ir.first->second.phys[ adlchannel[ccount] ] = i[ccount];
                }
                NoteUpdate(MidCh, ir.first, Upd_All | Upd_Patch);
                break;
            }
            case 0xA: // Note touch
            {
                if (event_size < 3) return;
                int note = event[ 1 ];
                int  vol = event[ 2 ];
                MIDIchannel::activenoteiterator
                    i = Ch[MidCh].activenotes.find(note);
                if(i == Ch[MidCh].activenotes.end())
                {
                    // Ignore touch if note is not active
                    break;
                }
                i->second.vol = vol;
                NoteUpdate(MidCh, i, Upd_Volume);
                break;
            }
            case 0xB: // Controller change
            {
                if (event_size < 3) return;
                int ctrlno = event[ 1 ];
                int  value = event[ 2 ];
                switch(ctrlno)
                {
                    case 1: // Adjust vibrato
                        //UI.PrintLn("%u:vibrato %d", MidCh,value);
                        Ch[MidCh].vibrato = value; break;
                    case 0: // Set bank msb (GM bank)
                        Ch[MidCh].bank_msb = value;
                        break;
                    case 32: // Set bank lsb (XG bank)
                        Ch[MidCh].bank_lsb = value;
                        break;
                    case 5: // Set portamento msb
                        Ch[MidCh].portamento = (Ch[MidCh].portamento & 0x7F) | (value<<7);
                        UpdatePortamento(MidCh);
                        break;
                    case 37: // Set portamento lsb
                        Ch[MidCh].portamento = (Ch[MidCh].portamento & 0x3F80) | (value);
                        UpdatePortamento(MidCh);
                        break;
                    case 65: // Enable/disable portamento
                        // value >= 64 ? enabled : disabled
                        //UpdatePortamento(MidCh);
                        break;
                    case 7: // Change volume
                        Ch[MidCh].volume = value;
                        NoteUpdate_All(MidCh, Upd_Volume);
                        break;
                    case 64: // Enable/disable sustain
                        Ch[MidCh].sustain = value;
                        if(!value) KillSustainingNotes( MidCh );
                        break;
                    case 11: // Change expression (another volume factor)
                        Ch[MidCh].expression = value;
                        NoteUpdate_All(MidCh, Upd_Volume);
                        break;
                    case 10: // Change panning
                        Ch[MidCh].panning = value;
                        NoteUpdate_All(MidCh, Upd_Pan);
                        break;
                    case 121: // Reset all controllers
                        Ch[MidCh].bend       = 0;
                        Ch[MidCh].volume     = 100;
                        Ch[MidCh].expression = 100;
                        Ch[MidCh].sustain    = 0;
                        Ch[MidCh].vibrato    = 0;
                        Ch[MidCh].vibspeed   = 2*3.141592653*5.0;
                        Ch[MidCh].vibdepth   = 0.5/127;
                        Ch[MidCh].vibdelay   = 0;
                        Ch[MidCh].panning    = 0x30;
                        Ch[MidCh].portamento = 0;
                        UpdatePortamento(MidCh);
                        NoteUpdate_All(MidCh, Upd_Pan+Upd_Volume+Upd_Pitch);
                        // Kill all sustained notes
                        KillSustainingNotes(MidCh);
                        break;
                    case 123: // All notes off
                        NoteUpdate_All(MidCh, Upd_Off);
                        break;
                    case 91: break; // Reverb effect depth. We don't do per-channel reverb.
                    case 92: break; // Tremolo effect depth. We don't do...
                    case 93: break; // Chorus effect depth. We don't do.
                    case 94: break; // Celeste effect depth. We don't do.
                    case 95: break; // Phaser effect depth. We don't do.
                    case 98: Ch[MidCh].lastlrpn=value; Ch[MidCh].nrpn=true; break;
                    case 99: Ch[MidCh].lastmrpn=value; Ch[MidCh].nrpn=true; break;
                    case 100:Ch[MidCh].lastlrpn=value; Ch[MidCh].nrpn=false; break;
                    case 101:Ch[MidCh].lastmrpn=value; Ch[MidCh].nrpn=false; break;
                    case 113: break; // Related to pitch-bender, used by missimp.mid in Duke3D
                    case  6: SetRPN(MidCh, value, true); break;
                    case 38: SetRPN(MidCh, value, false); break;
                    default:
						break;
                }
                break;
            }
            case 0xC: // Patch change
                if (event_size < 2) return;
                Ch[MidCh].patch = event[ 1 ];
                break;
            case 0xD: // Channel after-touch
            {
                if (event_size < 2) return;
                // TODO: Verify, is this correct action?
                int  vol = event[ 1 ];
                for(MIDIchannel::activenoteiterator
                    i = Ch[MidCh].activenotes.begin();
                    i != Ch[MidCh].activenotes.end();
                    ++i)
                {
                    // Set this pressure to all active notes on the channel
                    i->second.vol = vol;
                }
                NoteUpdate_All(MidCh, Upd_Volume);
                break;
            }
            case 0xE: // Wheel/pitch bend
            {
                if (event_size < 3) return;
                int a = event[ 1 ];
                int b = event[ 2 ];
                Ch[MidCh].bend = (a + b*128 - 8192) * Ch[MidCh].bendsense;
                NoteUpdate_All(MidCh, Upd_Pitch);
                break;
            }
        }
    }

private:
    // Determine how good a candidate this adlchannel
    // would be for playing a note from this instrument.
    long CalculateAdlChannelGoodness
        (unsigned c, unsigned ins, unsigned MidCh) const
    {
        long s = -ch[c].koff_time_until_neglible;

        // Same midi-instrument = some stability
        //if(c == MidCh) s += 4;
        for(AdlChannel::users_t::const_iterator
            j = ch[c].users.begin();
            j != ch[c].users.end();
            ++j)
        {
            s -= 4000;
            if(!j->second.sustained)
                s -= j->second.kon_time_until_neglible;
            else
                s -= j->second.kon_time_until_neglible / 2;

            MIDIchannel::activenotemap_t::const_iterator
                k = Ch[j->first.MidCh].activenotes.find(j->first.note);
            if(k != Ch[j->first.MidCh].activenotes.end())
            {
                // Same instrument = good
                if(j->second.ins == ins)
                {
                    s += 300;
                    // Arpeggio candidate = even better
                    if(j->second.vibdelay < 70
                    || j->second.kon_time_until_neglible > 20000)
                        s += 0;
                }
                // Percussion is inferior to melody
                s += 50 * (k->second.midiins / 128);

                /*
                if(k->second.midiins >= 25
                && k->second.midiins < 40
                && j->second.ins != ins)
                {
                    s -= 14000; // HACK: Don't clobber the bass or the guitar
                }
                */
            }

            // If there is another channel to which this note
            // can be evacuated to in the case of congestion,
            // increase the score slightly.
            unsigned n_evacuation_stations = 0;
            for(unsigned c2 = 0; c2 < opl.NumChannels; ++c2)
            {
                if(c2 == c) continue;
                if(opl.four_op_category[c2]
                != opl.four_op_category[c]) continue;
                for(AdlChannel::users_t::const_iterator
                    m = ch[c2].users.begin();
                    m != ch[c2].users.end();
                    ++m)
                {
                    if(m->second.sustained)       continue;
                    if(m->second.vibdelay >= 200) continue;
                    if(m->second.ins != j->second.ins) continue;
                    n_evacuation_stations += 1;
                }
            }
            s += n_evacuation_stations * 4;
        }
        return s;
    }

    // A new note will be played on this channel using this instrument.
    // Kill existing notes on this channel (or don't, if we do arpeggio)
    void PrepareAdlChannelForNewNote(int c, int ins)
    {
        if(ch[c].users.empty()) return; // Nothing to do
        bool doing_arpeggio = false;
        for(AdlChannel::users_t::iterator
            jnext = ch[c].users.begin();
            jnext != ch[c].users.end();
            )
        {
            AdlChannel::users_t::iterator j(jnext++);
            if(!j->second.sustained)
            {
                // Collision: Kill old note,
                // UNLESS we're going to do arpeggio

                MIDIchannel::activenoteiterator i
                ( Ch[j->first.MidCh].activenotes.find( j->first.note ) );

                // Check if we can do arpeggio.
                if((j->second.vibdelay < 70
                 || j->second.kon_time_until_neglible > 20000)
                && j->second.ins == ins)
                {
                    // Do arpeggio together with this note.
                    doing_arpeggio = true;
                    continue;
                }

                KillOrEvacuate(c,j,i);
                // ^ will also erase j from ch[c].users.
            }
        }

        // Kill all sustained notes on this channel
        // Don't keep them for arpeggio, because arpeggio requires
        // an intact "activenotes" record. This is a design flaw.
        KillSustainingNotes(-1, c);

        // Keyoff the channel so that it can be retriggered,
        // unless the new note will be introduced as just an arpeggio.
        if(ch[c].users.empty())
            opl.NoteOff(c);
    }

    void KillOrEvacuate(
        unsigned from_channel,
        AdlChannel::users_t::iterator j,
        MIDIchannel::activenoteiterator i)
    {
        // Before killing the note, check if it can be
        // evacuated to another channel as an arpeggio
        // instrument. This helps if e.g. all channels
        // are full of strings and we want to do percussion.
        // FIXME: This does not care about four-op entanglements.
        for(unsigned c = 0; c < opl.NumChannels; ++c)
        {
            if(c == from_channel) continue;
            if(opl.four_op_category[c]
            != opl.four_op_category[from_channel]
              ) continue;
            for(AdlChannel::users_t::iterator
                m = ch[c].users.begin();
                m != ch[c].users.end();
                ++m)
            {
                if(m->second.vibdelay >= 200
                && m->second.kon_time_until_neglible < 10000) continue;
                if(m->second.ins != j->second.ins) continue;

                // the note can be moved here!

                i->second.phys.erase(from_channel);
                i->second.phys[c] = j->second.ins;
                ch[c].users.insert( *j );
                ch[from_channel].users.erase( j );
                return;
            }
        }

        /*UI.PrintLn(
            "collision @%u: [%ld] <- ins[%3u]",
            c,
            //ch[c].midiins<128?'M':'P', ch[c].midiins&127,
            ch[c].age, //adlins[ch[c].insmeta].ms_sound_kon,
            ins
            );*/

        // Kill it
        NoteUpdate(j->first.MidCh,
                   i,
                   Upd_Off,
                   from_channel);
    }

    void KillSustainingNotes(int MidCh = -1, int this_adlchn = -1)
    {
        unsigned first=0, last=opl.NumChannels;
        if(this_adlchn >= 0) { first=this_adlchn; last=first+1; }
        for(unsigned c = first; c < last; ++c)
        {
            if(ch[c].users.empty()) continue; // Nothing to do
            for(AdlChannel::users_t::iterator
                jnext = ch[c].users.begin();
                jnext != ch[c].users.end();
                )
            {
                AdlChannel::users_t::iterator j(jnext++);
                if((MidCh < 0 || j->first.MidCh == MidCh)
                && j->second.sustained)
                {
                    int midiins = '?';
                    ch[c].users.erase(j);
                }
            }
            // Keyoff the channel, if there are no users left.
            if(ch[c].users.empty())
                opl.NoteOff(c);
        }
    }

    void SetRPN(unsigned MidCh, unsigned value, bool MSB)
    {
        bool nrpn = Ch[MidCh].nrpn;
        unsigned addr = Ch[MidCh].lastmrpn*0x100 + Ch[MidCh].lastlrpn;
        switch(addr + nrpn*0x10000 + MSB*0x20000)
        {
            case 0x0000 + 0*0x10000 + 1*0x20000: // Pitch-bender sensitivity
                Ch[MidCh].bendsense = value/8192.0;
                break;
            case 0x0108 + 1*0x10000 + 1*0x20000: // Vibrato speed
                if(value == 64)
                    Ch[MidCh].vibspeed = 1.0;
                else if(value < 100)
                    Ch[MidCh].vibspeed = 1.0/(1.6e-2*(value?value:1));
                else
                    Ch[MidCh].vibspeed = 1.0/(0.051153846*value-3.4965385);
                Ch[MidCh].vibspeed *= 2*3.141592653*5.0;
                break;
            case 0x0109 + 1*0x10000 + 1*0x20000: // Vibrato depth
                Ch[MidCh].vibdepth = (((signed)value-64)*0.15)*0.01;
                break;
            case 0x010A + 1*0x10000 + 1*0x20000: // Vibrato delay in millisecons
                Ch[MidCh].vibdelay =
                    value ? long(0.2092 * std::exp(0.0795 * value)) : 0.0;
                break;
            default:
				break;
        }
    }

    void UpdatePortamento(unsigned MidCh)
    {
        // mt = 2^(portamento/2048) * (1.0 / 5000.0)
        /*
        double mt = std::exp(0.00033845077 * Ch[MidCh].portamento);
        NoteUpdate_All(MidCh, Upd_Pitch);
        */
    }

    void NoteUpdate_All(unsigned MidCh, unsigned props_mask)
    {
        for(MIDIchannel::activenoteiterator
            i = Ch[MidCh].activenotes.begin();
            i != Ch[MidCh].activenotes.end();
            )
        {
            MIDIchannel::activenoteiterator j(i++);
            NoteUpdate(MidCh, j, props_mask);
        }
    }

    void NoteOff(unsigned MidCh, int note)
    {
        MIDIchannel::activenoteiterator
            i = Ch[MidCh].activenotes.find(note);
        if(i != Ch[MidCh].activenotes.end())
        {
            NoteUpdate(MidCh, i, Upd_Off);
        }
    }

    void UpdateVibrato(double amount)
    {
        for(unsigned a=0, b=Ch.size(); a<b; ++a)
            if(Ch[a].vibrato && !Ch[a].activenotes.empty())
            {
                NoteUpdate_All(a, Upd_Pitch);
                Ch[a].vibpos += amount * Ch[a].vibspeed;
            }
            else
                Ch[a].vibpos = 0.0;
    }

    void UpdateArpeggio(double amount)
    {
        // If there is an adlib channel that has multiple notes
        // simulated on the same channel, arpeggio them.
        const double desired_arpeggio_rate = 40; // Hz (upper limit)
      #if 0
        static unsigned cache=0;
        amount=amount; // Ignore amount. Assume we get a constant rate.
        cache += MaxSamplesAtTime * desired_arpeggio_rate;
        if(cache < PCM_RATE) return;
        cache %= PCM_RATE;
      #else
        arpeggio_cache += amount * desired_arpeggio_rate;
        if(arpeggio_cache < 1.0) return;
        arpeggio_cache -= 1.0;
      #endif
        ++arpeggio_counter;

        for(unsigned c = 0; c < opl.NumChannels; ++c)
        {
        retry_arpeggio:;
            size_t n_users = ch[c].users.size();
            /*if(true)
            {
                UI.GotoXY(64,c+1); UI.Color(2);
                std::fprintf(stderr, "%7ld/%7ld,%3u\r",
                    ch[c].keyoff,
                    (unsigned) n_users);
                UI.x = 0;
            }*/
            if(n_users > 1)
            {
                AdlChannel::users_t::const_iterator i = ch[c].users.begin();
                size_t rate_reduction = 3;
                if(n_users >= 3) rate_reduction = 2;
                if(n_users >= 4) rate_reduction = 1;
                std::advance(i, (arpeggio_counter / rate_reduction) % n_users);
                if(i->second.sustained == false)
                {
                    if(i->second.kon_time_until_neglible <= 0l)
                    {
                        NoteUpdate(
                            i->first.MidCh,
                            Ch[ i->first.MidCh ].activenotes.find( i->first.note ),
                            Upd_Off,
                            c);
                        goto retry_arpeggio;
                    }
                    NoteUpdate(
                        i->first.MidCh,
                        Ch[ i->first.MidCh ].activenotes.find( i->first.note ),
                        Upd_Pitch | Upd_Volume | Upd_Pan,
                        c);
                }
            }
        }
    }
};

static const uint8_t sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t sysex_gm2_reset[]= { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool is_gs_reset(const unsigned char * data, unsigned long size)
{
	if ( size != _countof( sysex_gs_reset ) ) return false;

	if ( memcmp( data, sysex_gs_reset, 5 ) != 0 ) return false;
	if ( memcmp( data + 7, sysex_gs_reset + 7, 2 ) != 0 ) return false;
	if ( ( ( data[ 5 ] + data[ 6 ] + 1 ) & 127 ) != data[ 9 ] ) return false;
	if ( data[ 10 ] != sysex_gs_reset[ 10 ] ) return false;

	return true;
}

ADLPlayer::ADLPlayer(): MIDIPlayer()
{
	midiplay = 0;
	resampler = 0;
	bChorus = true;
}

ADLPlayer::~ADLPlayer()
{
	shutdown();
}

void ADLPlayer::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned port = (b >> 24) & 3;
		unsigned channel = b & 0x0F;
		unsigned command = b & 0xF0;
		unsigned event_length = (command >= 0xF8 && command <= 0xFF) ? 1 : (( command == 0xC0 || command == 0xD0 ) ? 2 : 3);
		midiplay->HandleEvent( event, event_length, port );
		if ( command == 0xB0 && event[ 1 ] == 0 )
		{
			if ( synth_mode == mode_xg )
			{
				if ( event[ 2 ] == 127 ) drum_channels[ port ][ channel ] = 1;
				else drum_channels[ port ][ channel ] = 0;
			}
			else if ( synth_mode == mode_gm2 )
			{
				if ( event[ 2 ] == 120 ) drum_channels[ port ][ channel ] = 1;
				else if ( event[ 2 ] == 121 ) drum_channels[ port ][ channel ] = 0;
			}
		}
		else if ( command == 0xC0 )
		{
			unsigned drum_channel = drum_channels[ port ][ channel ];
			midiplay->setDrumMode( channel + (port * 16), !!drum_channel );
		}
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		port &= 3;
		midiplay->HandleEvent( data, size, port );
		if ( ( size == _countof( sysex_gm_reset ) && !memcmp( data, sysex_gm_reset, _countof( sysex_gm_reset ) ) ) ||
			( size == _countof( sysex_gm2_reset ) && !memcmp( data, sysex_gm2_reset, _countof( sysex_gm2_reset ) ) ) ||
			is_gs_reset( data, size ) ||
			( size == _countof( sysex_xg_reset ) && !memcmp( data, sysex_xg_reset, _countof( sysex_xg_reset ) ) ) )
		{
			reset_drum_channels();
			synth_mode = ( size == _countof( sysex_xg_reset ) ) ? mode_xg :
			             ( size == _countof( sysex_gs_reset ) ) ? mode_gs :
			             ( data [4] == 0x01 )                   ? mode_gm :
			                                                      mode_gm2;
		}
		else if ( synth_mode == mode_gs && size == 11 &&
			data [0] == 0xF0 && data [1] == 0x41 && data [3] == 0x42 &&
			data [4] == 0x12 && data [5] == 0x40 && (data [6] & 0xF0) == 0x10 &&
			data [10] == 0xF7)
		{
			if (data [7] == 2)
			{
				// GS MIDI channel to part assign
				gs_part_to_ch [ port ][ data [6] & 15 ] = data [8];
			}
			else if ( data [7] == 0x15 )
			{
				// GS part to rhythm allocation
				unsigned int drum_channel = gs_part_to_ch [ port ][ data [6] & 15 ];
				if ( drum_channel < 16 )
				{
					drum_channel += 16 * port;
					drum_channels [ port ][ drum_channel ] = data [8];
				}
			}
		}
	}
}

#include "resampler.h"

void ADLPlayer::render( float * out, unsigned long count )
{
	short intermediate_buffer[ 512 ];
	int resampled_buffer[ 512 ];

	while ( count )
	{
		unsigned todo = count;
		if ( todo > 256 ) todo = 256;

		if ( resampler )
		{
			unsigned samples_written = 0;

			while ( todo )
			{
				int to_write = resampler_get_min_fill( resampler );
				if ( to_write )
				{
					render_internal( this, to_write, intermediate_buffer );
					for ( unsigned i = 0; i < to_write; i++ )
					{
						resampler_write_pair( resampler, intermediate_buffer[ i * 2 ], intermediate_buffer[ i * 2 + 1 ] );
					}
				}
				/* if ( !lanczos_resampler_ready( resampler ) ) break; */ /* We assume that by filling the input buffer completely every pass, there will always be samples ready. */
				int ls, rs;
				resampler_read_pair( resampler, &ls, &rs );
				resampled_buffer[ samples_written++ ] = ls;
				resampled_buffer[ samples_written++ ] = rs;
				--todo;
			}

			audio_math::convert_from_int32( resampled_buffer, samples_written, out, 1 << 16 );

			todo = samples_written / 2;
		}
		else
		{
			render_internal( this, todo, intermediate_buffer );

			audio_math::convert_from_int16( intermediate_buffer, todo * 2, out, 2.0 );
		}

		out += todo * 2;
		count -= todo;
	}
}

void ADLPlayer::render_internal(void * context, int count, short * out)
{
	ADLPlayer * pthis = (ADLPlayer *) context;

	Bit32s accum_buffer[ 512 ];
	Bit32s buffer[ 512 ];

	while ( count )
	{
		unsigned todo = count;
		if ( todo > 256 ) todo = 256;

		memset( accum_buffer, 0, todo * sizeof( Bit32s ) * 2 );

		unsigned num_cards = pthis->midiplay->opl.NumCards * (pthis->midiplay->opl.FullPan ? 2 : 1);
		for ( unsigned i = 0; i < num_cards; i++ )
		{
			pthis->midiplay->opl.cards[ i ].Generate( buffer, todo );
			for ( unsigned j = 0; j < todo * 2; j++ )
			{
				accum_buffer[ j ] += buffer[ j ];
			}
		}

		for ( unsigned long i = 0; i < todo * 2; i++ )
		{
			Bit32s sample = accum_buffer[ i ];
			if ( (Bit32u)( sample + 0x8000 ) >= 0x10000 ) sample = ( sample >> 31 ) ^ 0x7FFF;
			out[ i ] = ( short ) sample;
		}

		pthis->midiplay->Tick( (double)todo / (double)pthis->uSampleRate );

		out += todo * 2;
		count -= todo;
	}
}

void ADLPlayer::setBank( unsigned bank )
{
	uBankNumber = bank;
}

void ADLPlayer::setChipCount( unsigned count )
{
	uChipCount = count;
}

void ADLPlayer::set4OpCount( unsigned count )
{
	u4OpCount = count;
}

void ADLPlayer::setFullPanning( bool enable )
{
	bFullPanning = enable;
}

void ADLPlayer::setChorus( bool enable )
{
	bChorus = enable;
}

void ADLPlayer::shutdown()
{
	if ( resampler ) resampler_destroy( resampler );
	resampler = NULL;
	delete midiplay;
	midiplay = NULL;
}

bool ADLPlayer::startup()
{
	if ( midiplay ) return true;

	midiplay = new MIDIplay;
	midiplay->AdlBank = uBankNumber;
	midiplay->opl.NumCards = uChipCount;
	midiplay->opl.NumFourOps = u4OpCount;
	midiplay->opl.FullPan = bFullPanning;
	midiplay->opl.PcmRate = 49716;
	midiplay->opl.Reset();
	midiplay->setChorus(bChorus);
	midiplay->init();

	if ( uSampleRate != 49716 )
	{
		resampler = resampler_create();
		if ( !resampler )
			return false;
		resampler_set_rate( resampler, 49716.0 / (double)uSampleRate );
	}

	reset_drum_channels();

	synth_mode = mode_gm;

	return true;
}

void ADLPlayer::reset_drum_channels()
{
	static const BYTE part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

	memset( drum_channels, 0, sizeof( drum_channels ) );
	for (unsigned i = 0; i < 4; i++)
	{
		drum_channels[ i ][ 9 ] = 1;

		memcpy( gs_part_to_ch[ i ], part_to_ch, sizeof( gs_part_to_ch[ i ] ) );
	}

	if ( midiplay )
	{
		for ( unsigned i = 0; i < 64; ++i )
		{
			midiplay->setDrumMode( i, !!drum_channels[ i / 16 ][ i & 15 ] );
		}
	}
}
