
## Format.md

### Sequence Files

#### Header v2 [RCP]

Pos	Len	Description

0000   20       "RCM-PC98V2.0(C)COME ON MUSIC" 0D 0A 00 00
0020   40       Song title
0060  150       Comment (12 lines with 1Ch bytes each)
01B0   10       Unknown
01C0   01       Ticks per quarter (low byte)
01C1   01       Tempo in BPM
01C2   01       Beat Numerator
01C3   01       Beat Denominator
01C4   01       Key Signature (see sequence command F5)
01C5   01       "PlayBIAS" (global transposition)
01C6   10       CM6 file name
01D6   10       GSD file name
01E6   01       Track Count (Can be 0 (RCP v0), 12h (RCP v1) or 24h (RCP v2))
01E7   01       Ticks per quarter (high byte)
01E8   1E       Unknown (Includes TONENAME.TB file path)
0206 0x20 * 0x10    Rhythm Definitions
0406 0x30 * 0x08    User SysEx
0586            Track Data

#### Header v3 [G36]

000	20	"COME ON MUSIC RECOMPOSER RCP3.0" 00
020	80	song title
0A0	168	comment (12 lines with 1Eh bytes each)
208	02	Track Count
		should be 12h or 24h
20A	02	Ticks per Quarter
20C	02	Tempo in BPM
20E	01	Beat Numerator
20F	01	Beat Denominator
210	01	Key Signature (see sequence command F5)
211	01	"PlayBIAS" (global transposition)
212	06	dummy ??
218	10	??
228	70	??
298	10	GSD (port A) file name
2A8	10	GSD (port B) file name
2B8	10	CM6 file name
2C8	50	??
318	80h*10h	Rhythm Definitions
B18	30h*8	User SysEx
C98	??	Track Data

#### Rhythm Definitions (0x10)

0Eh bytes - name
1 byte - key
1 byte - gate time??

#### User SysEx (0x30)

18h bytes: Name
18h bytes: SysEx data without leading F0, padded with F7 bytes

#### Track Data

2Ch/2Eh bytes - Track Header
?? bytes - Sequence Data

##### Track Header [RCP]

2 bytes - track length (includes this value)
1 byte - track ID (1-based)
1 byte - rhythm mode (0x00 - off, 0x80 - on, others undefined/fall back to off?)
1 byte - MIDI channel (0xFF = null device, 0x00..0x0F = port A ch 0..15, 0x10..0x1F = port B ch 0..15)
1 byte - "key" (transposition in semitones, 7-bit signed)
	0 = no transposition, 0x0C = +1 octave, 0x74 = -1 octave, added together with global transposition
	0x80..0xFF = rhythm track (no per-track transposition, ignores global transposition as well)
1 byte - tick offset (signed 8-bit, moves all events by X ticks)
1 byte - Mute (0x00 == off, 0x01 == on, others undefined/fall back to off?)
24h bytes - track name
-> 2Ch bytes

##### Track Header [G36]

4 bytes - track length (includes this value)
1 byte - track ID (1-based)
1 byte - rhythm mode (0x80 - on, 0x00 - off)
1 byte - MIDI channel (0xFF = null device, 0x00..0x0F = port A ch 0..15, 0x10..0x1F = port B ch 0..15)
1 byte - "key" (transposition)
1 byte - tick offset (signed 8-bit, moves all events by X ticks)
1 byte - Mute (0x00 == off, 0x01 == on, others undefined/fall back to off?)
24h bytes - track name
-> 2Eh bytes

#### Sequence Data [RCP]

Series of events, 4 bytes each, terminated by Track End command (FEh):

1 byte - note height (00h..7Fh) / command ID (80h..FFh)
1 byte - delay until next command [p0]
1 byte - note duration / param 1 [p1]
1 byte - note velocity / param 2 [p2]

#### Sequence Data [G36]

Series of events, 6 bytes each, terminated by Track End command (FEh):

1 byte  - note height (00h..7Fh) / command ID (80h..FFh)
1 byte  - note velocity/param 2 [p2]
2 bytes - delay until next command [p0]
2 bytes - note duration/param 1 [p1]

### Commands

90..97 - send User SysEx 1..8
        The SysEx data is preprocessed the same way as data of command 98.
        P1/P2 contain data bytes that may be used as parameters for the SysEx stream. (see command 98)
98 [RCP] - Send Channel Exclusive (p1/p2 = data, continued by F7 events with p1/p2 being used)
98 [G98] - Send Channel Exclusive (p1/p2 = data bytes with only the low byte used, continued by F7 events with all parameters used)
        The P1/P2 parameters of the 98 command are data parameters that are to be used by the actual data stream later.

    Special command bytes present in the stream:
        80 - Put data value (p1 from 98 command)
        81 - Put data value (p2 from 98 command)
        82 - Put current MIDI channel ID (channel only, without port)
        83 - Initialize Roland checksum calculation
        84 - Put Roland checksum (based on current state)

C0 - DX7 Function
	-> results in: F0 43 (10+cc) 08 p1 p2 F7 [cc = current MIDI channel]
C1 - DX Parameter
	-> results in: F0 43 (10+cc) 00 p1 p2 F7
C2 - DX RERF
	-> results in: F0 43 (10+cc) 04 p1 p2 F7
C3 - TX Function
	-> results in: F0 43 (10+cc) 11 p1 p2 F7
C5 - FB-01 P Parameter
	-> results in: F0 43 (10+cc) 15 p1 p2 F7
C6 - FB-01 S System
	-> results in: F0 43 75 cc 10 p1 p2 F7
C7 - TX81Z V VCED
	-> results in: F0 43 (10+cc) 12 p1 p2 F7
C8 - TX81Z A ACED
	-> results in: F0 43 (10+cc) 13 p1 p2 F7
C9 - TX81Z P PCED
	-> results in: F0 43 (10+cc) 10 p1 p2 F7
CA - TX81Z S System
	-> results in: F0 43 (10+cc) 10 7B p1 p2 F7
CB - TX81Z E EFFECT
	-> results in: F0 43 (10+cc) 10 7C p1 p2 F7
CC - DX7-2 R Remote SW
	-> results in: F0 43 (10+cc) 1B p1 p2 F7
CD - DX7-2 A ACED
	-> results in: F0 43 (10+cc) 18 p1 p2 F7
CE - DX7-2 P PCED
	-> results in: F0 43 (10+cc) 19 p1 p2 F7
CF - TX802 P PCED
	-> results in: F0 43 (10+cc) 1A p1 p2 F7
D0 - YAMAHA Base Address (p1 = high "yaH", p2 = mid "yaM")
D1 - YAMAHA Device Data (p1 = Device ID "yDev", p2 = Model ID "yMod")
D2 - YAMAHA Address / Parameter (p1 = address low "yaL", p2 = parameter "yPar")
	-> results in: F0 43 yDev yMod yaH yaM yaL yPar F7
D3 - YAMAHA XG Address / Parameter (p1 = address low "yaL", p2 = parameter "yPar")
	-> results in: F0 43 10 4C yaH yaM yaL yPar F7
DC - MKS-7
	-> results in: F0 41 32 cc p1 p2 F7 [cc = current MIDI channel]
DD - Roland Base Address (p1 = high "raH", p2 = mid "raM")
DE - Roland Parameter (p1 = address low "raL", p2 = parameter "rPar")
	-> results in: F0 41 rDev rMod 12 raH raM raL rPar [checksum] F7
DF - Roland Device (p1 = Device ID "rDev", p2 = Model ID "rMod")
E2 - Bank MSB p2 / Instrument p1
E5 - "Key Scan" p1
E6 - MIDI Channel p1
	00h = mute
	01h..10h = port A, channel 0..15
	11h..20h = port B, channel 0..15
	-> same as "MIDI channel" from header, except 1-based
E7 - Tempo Modifier
    P1 = multiplicator, 20h = 50%, 40h = 100%, 80h = 200%
    P2 = 0 - just set tempo, 1..FF - interpolate tempo over p2 ticks
EA - Channel Aftertouch p1
EB - Control Change p1, data p2
EC - Instrument p1
ED - Note Aftertouch p1/p2
EE - Pitch Bend (14-bit MIDI format, p1 = low 7 bits, p2 = high 7 bits)
F5* - Key Signature Change (p0 = key signature)
	Bits 0-2 (07h) - number of flats/sharps
	Bit   3  (08h) - sharps (0) / flats (1)
	Bit   4  (10h) - major key (0) / minor key (1)
F6* [RCP] - Comment (p1/p2 = text characters, continued by F7 events with p1/p2 being used)
F6* [G36] - Comment (all 5 parameter bytes = text characters, continued by F7 events with all parameters used)
F7* - continuation of previous command
F8* - Loop End (p0 == loop count, 0 = infinite)
F9* - Loop Start (p0 is always set to 1)
	Note: Loops can be nested.
FC* - repeat previous measure
	p0 - measure to repeat, 0 = beginning, 1 = after first FD command, etc.
	[RCP] p2p1 = file offset of the measure, relative to the beginning of the track header.
	      Note: It is possible that one FC command jumps to another one.
	            e.g. Measure 21: FC -> measure 12: FC -> measure 8
	            After finishing measure 8, processing continues right after the FC command in measure 21.
	[G36] p1 - number of first command in the measure
	      For some reason, a value of 0030h refers to the first command.
	      So the formula for a RCP-style offset is:
	          offset = 002Eh (header size) + (p1 - 0030h) * 6
FD* - Measure End
FE* - Track End

* - delay time p0 ignored (seems to apply to all commands F0..FF)

### Control Files

#### CM6 Format

0000	10	"COME ON MUSIC" 00 00 00
0010	0C	"R CM-64   " 03 00
0010	0C	"R MT-32   " 00 00
...
0040	40	description?
0080	17	SysEx Data: MT-32 System Area (offset 10 00 00)
0097	09	MT-32 channel volume settings?
00A0	90	SysEx Data: MT-32 Patch Temporary Area (offset 03 00 00) [9 channels with 10h bytes each]
0130	154	SysEx Data: MT-32 Rhythm Setup Temporary Area (offset 03 01 10) [85 drums with 4 bytes each]
0284	7B0	SysEx Data: MT-32 Timbre Temporary Area (offset 04 00 00) [8 channels with F6h bytes each]
0A34	400	SysEx Data: MT-32 Patch Memory (offset 05 00 00) [8 channels with 40h bytes each]
0E34	4000	SysEx Data: MT-32 Timbre Memory (offset 08 00 00) [128 instruments with 100h bytes each]
4E34	7E	SysEx Data: CM-32P Patch Temporary Area (offset 50 00 00) [6 channels with 15h bytes each]
4EB2	980	SysEx Data: CM-32P Patch Memory (offset 51 00 00) [128 instruments with 13h bytes each]
5832	11	SysEx Data: CM-32P System Area (offset 52 00 00)
5843	06	CM-32P channel volume settings?
-> 5849h bytes

#### GSD Format

0000	0E	"COME ON MUSIC" 00
000E	12	"GS CONTROL 1.0" 00 00 00 00
0020	07	SysEx: System Params (offset 40 00 00)
 +00	04	Master Tune
 +04	01	Master Volume
 +05	01	Master Key-Shift
 +06	01	Master Pan
0027	07	SysEx: Reverb Params (offset 40 01 30)
002E	08	SysEx: Chorus Params (offset 40 01 38)
0036	7A0	Patch Parameters [16 channels with 7Ah bytes each]
		The data is sorted by MIDI channel. (*not* SC-55 part ID, those begin with the drum channel)
 +00	02	Bank MSB + program number (offset 40 1n 00)
 +02	01	Rx. Channel (offset 40 1n 02)
 +03	01	Rx. Pitch Bend
 +04	01	Rx. Ch. Pressure
 +05	01	Rx. Program Change
 +06	01	Rx. Control Change
 +07	01	Rx. Poly Pressure
 +08	01	Rx. Note Message
 +09	01	Rx. RPN
 +0A	01	Rx. NRPN
 +0B	01	Rx. Modulation
 +0C	01	Rx. Volume
 +0D	01	Rx. Panpot
 +0E	10	Rx. Expression
 +0F	01	Rx. Hold 1 (Sustain)
 +10	01	Rx. Portamento
 +11	01	Rx. Sostenuto
 +12	01	Rx. Soft Pedal
 +13	01	Mono/Poly Mode
 +14	01	Assign Mode
 +15	01	Rhythm Part Mode
 +16	01	Pitch Key Shift
 +17	02	Pitch Offset Fine
 +19	01	Part Level
 +1A	01	Velocity Sense Depth
 +1B	01	Velocity Sense Offset
 +1C	01	Part Panpot
 +1D	01	Key Range Low
 +1E	01	Key Range High
 +1F	01	CC1 Controller Number
 +20	01	CC2 Controller Number
 +21	01	Chorus Send Depth
 +22	01	Reverb Send Depth (offset 40 1n 22)
 +23	08	Tone Modify 1-8 (offset 40 1n 30)
 +2B	0C	Scale Tuning C to B (offset 40 1n 40)
 +37	42	Destination Controllers [6 controllers, 0Bh bytes each]
			0 Modulation (offset 40 2n 00)
			1 Bend (offset 40 2n 10)
			2 Ch. Pressure (offset 40 2n 20)
			3 Poly Pressure (offset 40 2n 30)
			4 CC1 (offset 40 2n 40)
			5 CC2 (offset 40 2n 50)
 +79	01	Voice Reserve
07D6	298	Drum Setup Parameters [2 drum sets, 14Ch bytes each] (offset 49 n0 00)
		[82 drum notes (notes 27..108), 4 bytes each, use of the 83th entry is unknown]
 +00	01	Level
 +01	01	Panpot
 +02	01	Reverb Send Level
 +03	01	Chrous Send Level
0A6E	02	Master Fine Tuning (Little Endian word, 2000 = no detune, 0000 = 1 semitone down, 3FFF = 1 semitone up)
0A70	01	Master Coarse Tuning (40h = default, 3Fh = 1 semitone down, 41h = 1 semitone up)
-> A71h bytes
