#define MYVERSION "2.1.5"

// #define DXISUPPORT
// #define FLUIDSYNTHSUPPORT
#define SF2PACK

/*
	change log

2018-08-29 10:10 UTC - kode54
- Re-enabled the VSTi enumeration message pump, as otherwise, the whole
  process deadlocks, except when it's running under Wine.
  - This was supposed to fix a bug where dialog creation could be interrupted
    by a switch to other preferences panes while VSTi enumeration is still
	happening, causing the MIDI prefpane to override the other. Apparently,
	that's yet another bug that doesn't occur when running the process under
	Wine.
- Version is now 2.1.5

2018-08-28 11:18 UTC - kode54
- Change optimization settings and profile optimize once again
- Updated Munt
- Updated libADLMIDI
- Changed ADLMIDI player to embed full WOPL banks for two of the presets
- Disabled VSTi player running a message pump during plugin enumeration
- Removed effects from fmmidi, they were too slow
- Version is now 2.1.4

2018-08-01 23:57 UTC - kode54
- Fixed VSTi selection on opening preferences after removing oplmidi
- Combined two cases on reset to defaults
- Version is now 2.1.3

2018-08-01 01:51 UTC - kode54
- Updated libADLMIDI
- Fixed preferences dialog description for soft panning mode (no longer requires double chips)
- Halved volume for Nuclear Option mode, since this version of NukedOPL is louder
- Updated BASS to version 2.4.13.8, BASSFLAC to 2.4.4, and BASSOPUS to 2.4.1.10
- Version is now 2.1.2

2018-07-31 00:04 UTC - kode54
- Updated libADLMIDI
- Version is now 2.1.1

2018-07-30 04:21 UTC - kode54
- Replaced bundled copy of ADLMIDI with libADLMIDI
- Adjusted Nuclear Option drivers to use libADLMIDI Nuked OPL, including panning
- Version is now 2.1

2018-05-21 01:36 UTC - kode54
- Updated Nuked OPL3 to version 1.8 and ported over my changes
- Version is now 2.0.24

2018-04-24 21:48 UTC - kode54
- Fixed MUNT enabling Super / GM behavior for strictly MT-32 files
- Version is now 2.0.23

2018-02-09 10:19 UTC - kode54
- Fixed MUNT path activating the Apply button
- Version is now 2.0.22

2017-12-31 04:30 UTC - kode54
- Updated input API for player version 1.4
- Version is now 2.0.21

2017-11-19 22:28 UTC - kode54
- Fix SysEx adding a random extra byte when it shouldn't, which
  broke synthesizers that observed the length, like Windows
  drivers, or VST instruments in this case.
- Version is now 2.0.20

2017-10-05 19:35 UTC - kode54
- Added support for running SysEx status in Standard MIDI files
- Version is now 2.0.19

2017-08-29 00:56 UTC - kode54
- Added support for SC-VA version 1.0.7
- Version is now 2.0.18

2017-08-28 03:52 UTC - kode54
- Updated DMXOPL to version 2.0b
- Version is now 2.0.17

2017-08-17 03:58 UTC - kode54
- Updated DMXOPL to version 1.10 Final
- Version is now 2.0.16

2017-07-18 02:03 UTC - kode54
- Updated DMXOPL to version 1.9.1
- Version is now 2.0.15

2017-06-25 23:50 UTC - kode54
- Updated BASS, BASSMIDI, and BASSFLAC
- Version is now 2.0.14

2017-06-25 23:41 UTC - kode54
- Fixed EMIDI loops, forgot to pull a midi_processing change long implemented in
  Cog, DeaDBeeF - Sorry, Duke Nukem 3D fans!

2017-06-24 00:03 UTC - kode54
- Updated DMXOPL to version 1.8.2
- Updated Nuclear Option Doom flavor synth
- Version is now 2.0.13

2017-06-14 01:56 UTC - kode54
- Implemented DMXOPL FM bank in both Nuclear Option and ADLmidi
- Implemented support in ADLmidi for proper Doom pseudo 4op voice pitch offset
- Version is now 2.0.12

2017-03-14 00:47 UTC - kode54
- Added configuration validation for Munt driver
- Fixed VSTHost to correctly check canDo features
- Version is now 2.0.11

2017-03-13 01:49 UTC - kode54
- Fixed support for some VSTi synthesizers, as I was incorrectly testing for
  sendVstMidiEvent instead of receiveVstMidiEvent
- Version is now 2.0.10

2017-02-24 02:34 UTC - kode54
- Updated BASSMIDI and BASSFLAC libraries
- Added Apogee MIDI driver to Nuclear Option set
- Added dumb cleanup to Secret Sauce startup
- Version is now 2.0.9

2017-02-04 19:28 UTC - kode54
- Updated BASS libraries
- Version is now 2.0.8

2017-02-04 04:51 UTC - kode54
- Add link to about string
- Version is now 2.0.7

2017-01-27 22:20 UTC - kode54
- Fixed Secret Sauce XG reset
- Secret Sauce now prevents GS flavor setting if an override is in place
- Version is now 2.0.6

2017-01-26 01:46 UTC - kode54
- Fixed Secret Sauce GS reset
- Version is now 2.0.5

2016-12-21 01:03 UTC - kode54
- Fixed Secret Sauce seeking
- Version is now 2.0.4

2016-12-01 03:58 UTC - kode54
- Improved Secret Sauce reset with even heavier resetting
- Added Secret Sauce reverb/chorus disable
- Added Nuclear Option soft panning control
- Oops! Was reading Secret Sauce flavor from configuration rather than preset
- Version is now 2.0.3

2016-11-29 07:11 UTC - kode54
- Unleash the Secret Sauce!
- Version is now 2.0.2
	
2016-11-25 23:49 UTC - kode54
- Fix new sflist parser:
  - Handling Unicode byte order markers
  - Handling patch mapping range checks from correct JSON variable
  - Handling some miscellaneous warnings
- Version is now 2.0.1

2016-11-21 04:00 UTC - kode54
- Nuclear Option!
- New name!
- Version is now 2.0

2016-11-16 22:50 UTC - kode54
- Reworded one of the settings
- Version is now 1.256

2016-06-19 22:07 UTC - kode54
- Rewrote SFList parser, and added support for the new JSON format
- Version is now 1.255

2016-04-10 06:19 UTC - kode54
- Added an advanced preferences option to disable the chorus in adlmidi
- Version is now 1.254

2016-04-10 04:19 UTC - kode54
- Fixed trimming some type 2 MIDI files, due to trimming non-existent tempo maps
- Version is now 1.253

2016-04-10 02:43 UTC - kode54
- Fixed VSTi crashes with the latest Sound Canvas VA (v1.0.2.0+), but break
  compatibility with older versions
- Fix VSTi propagating saved settings to playback instances
- Version is now 1.252

2016-04-03 05:42 UTC - kode54
- Now correctly handles invalid files that have no playable tracks
- Version is now 1.251

2016-04-03 05:09 UTC - kode54
- Added an option to start playback on the first note, enabled by default
- Rearranged the preferences dialog a bit
- Version is now 1.250

2016-03-24 01:49 UTC - kode54
- Let's forget all about the secret sauce.
- Updated VSTHost to work with Sound Canvas VA
- Version is now 1.249

2015-10-04 08:26 UTC - kode54
- Rebuilt with MSVC 2010
- Widened the AdLib baank selector control
- Version is now 1.247

2015-09-23 00:56 UTC - kode54
- Hopefully fix MT-32 SysEx message handling
- Version is now 1.246

2015-09-21 01:41 UTC - kode54
- Fix accidental dependency that broke Windows XP compatibility
- Version is now 1.245

2015-09-14 03:00 UTC - kode54
- Fix MUNT by updating to the latest version and correcting how its API is used
- Updated BASSMIDI to version 2.4.9.15
- Version is now 1.244

2015-07-16 00:21 UTC - kode54
- Updated BASSMIDI to version 2.4.9.12
- Relaxed XMI IFF validation
- Fixed RPG Maker loop handling, since it appears to only indicate loop start
- Version is now 1.243

2015-04-05 05:54 UTC - kode54
- Updated BASSMIDI to version 2.4.9.4
- Version is now 1.242

2015-03-17 06:17 UTC - kode54
- Fixed SFZ loading
- Added SoundFont load error reporting
- Adjusted VSTi player so process termination can't get in a recursive call loop
- Version is now 1.241

2015-03-17 05:57 UTC - kode54
- Changed BASSMIDI SoundFont loader to not use FontInitUser for local .sfz files
- Version is now 1.240

2015-02-22 21:58 UTC - kode54
- Fixed loading song specific .sflist files
- Version is now 1.239

2015-02-22 07:01 UTC - kode54
- Split up BASSMIDI into three streams, and apply port 0 SysEx messages to all ports
- Version is now 1.238

2015-02-22 01:57 UTC - kode54
- Fixed lockups on MIDI files with loops that start on the end of the file
- Fixed bank/drum/bank LSB handling
- Version is now 1.237

- Updated BASS to version 2.4.11
- Updated BASSMIDI to version 2.4.9
- Updated BASSFLAC to version 2.4.2
- Version is now 1.235

2014-11-30 01:56 UTC - kode54
- Fixed handling sequencer specific commands in adlmidi, Emu de MIDI, and BASSMIDI drivers
- Fixed midi_processing to support sequencer specific commands on import, playback, and
  export
- Version is now 1.234

2014-11-21 04:47 UTC - kode54
- Fixed SysEx relative path resolving to absolute path
- Version is now 1.233

2014-10-21 00:56 UTC - kode54
- Added some extra validity checks to LDS format parsing
- Version is now 1.232

2014-10-12 23:52 UTC - kode54
- Fixed BASSMIDI player to correctly release unused SoundFonts after 10 seconds, which
  it wasn't due to misuse of the standard difftime function
- Version is now 1.231

2014-07-08 08:16 UTC - kode54
- Updated BASSMIDI, now with support for SFZ instruments
- Fixed initialization, in case there are multiple copies of BASS resident
- Version is now 1.230

2013-12-15 05:04 UTC - kode54
- Removed CC 111 from EMIDI track exclusion check
- Added RPG Maker loop support, which is CC 111 with values of 0 for start and 1 for end,
  so long as no other values are used with that control change throughout the same file
- Implemented an option to disable BASSMIDI reverb and chorus processing
- Version is now 1.229

2013-11-03 07:20 UTC - kode54
- Fixed finite looping when loop commands are found
- Version is now 1.228

2013-11-03 07:07 UTC - kode54
- Fixed MIDI loop end handling

2013-10-31 23:42 UTC - kode54
- Odd Windows versions don't like forward slashes in local paths, now directory-specific
  SoundFonts will preserve the last path separator
- Version is now 1.227

2013-10-31 00:12 UTC - kode54
- Rewrote file- and directory-specific SoundFont handling
- Fixed SoundFont player actually using file- and directory-specific banks
- SoundFont player is now capable of loading file- and directory-specific sflist files
- Version is now 1.226

2013-10-30 19:17 UTC - kode54
- Re-added custom file accessor to SoundFont loading
- Version is now 1.225

2013-10-27 13:23 UTC - kode54
- Added an extra range check to the Standard MIDI track parser
- Fixed System Exclusive message parsing in the Standard MIDI track parser
- Version is now 1.224

2013-10-25 22:09 UTC - kode54
- Foolishly thought I could code sign BASS and company. Reverted to original versions.
- Version is now 1.223

2013-10-25 20:48 UTC - kode54
- Added safety checks to the MIDI processing loaders
- Version is now 1.222

2013-10-21 00:22 UTC - kode54
- BASSMIDI sflist preset command system now supports more than one preset (p) command
  per command group (once again, groups separated by &)
- BASSMIDI sflist channel command now supports ranges of multiple channels, optionally
  specified using a hyphen and a higher channel number: c1-4
- Version is now 1.221

2013-10-19 02:05 UTC - kode54
- Added support for extended SFList preset overrides. To use, precede any line of an
  .sflist file with a vertical bar "|" and fill in a list of commands, separated into
  groups by ampersand "&" characters. Each group may contain one preset "p" command
  and any number of non-overlapping channel "c" commands. The preset command is of
  the format "p[#,]#=[#,]#", where the four parameters, left to right, are destination
  bank, destination program, source bank, source program. The either of the bank
  numbers is optional, and the default will map all banks, or map to the default bank.
  The channel command is of the format "c#", where the one parameter is a channel
  number in the range of 1-48. The channel command is implemented by routing the paired
  programs, or all programs in the bank if none is specified, to the bank LSB of the
  channels' numbers. Thus, playback through the BASSMIDI driver ignores bank LSB
  commands in the actual files. If anyone can suggest a better way to do this, or ask
  Ian Luck to further extend the BASS_MIDI_FONTEX structure with a channel bitfield,
  be my guest.
- Version is now 1.220

2013-10-16 16:54 UTC - kode54
- Fixed MIDIPlayer length enforcement for non-looping files
- Version is now 1.216

2013-10-16 10:37 UTC - kode54
- Fixed SoundFont handle cache
- Version is now 1.215

2013-10-16 03:12 UTC - kode54
- Disabled use of std::thread in BASSMIDI Player to spare wine users the fallout of
  MSVC 2012+ std::thread not working there yet
- Version is now 1.214

2013-10-16 03:01 UTC - kode54
- Major overhaul to use more STL and C++11 features in the MIDI players, to hopefully
  ease synchronizing the code between this component and other Unixish platforms

2013-09-02 08:45 UTC - kode54
- Added back error checking that was missing after switching to the alternative
  midi_processing library
- Version is now 1.213

2013-08-23 01:10 UTC - kode54
- Adjusted reverb send level calculation
- Version is now 1.212

2013-08-23 01:03 UTC - kode54
- Optimized reverb and corrected wet/dry mixing

2013-08-23 00:48 UTC - kode54
- Fixed MIDI preset storage for oplmidi

2013-08-23 00:35 UTC - kode54
- Doubled oplmidi volume
- Added reverb to fmmidi and oplmidi

2013-08-13 01:08 UTC - kode54
- Migrated all common MIDI player code to a single base class
- Version is now 1.211

2013-08-12 23:28 UTC - kode54
- Moved midisynth into its own external library
- Changed oplmidi interface to borrow instruments from adlmidi
- Fixed several bounds checking exceptions which could occur in
  midi_processing

2013-08-05 01:57 UTC - kode54
- Replaced customized MIDI processing routines with in-memory STL
  based midi_processing library, improving loading speed greatly
- Version is now 1.210

2013-05-09 01:22 UTC - kode54
- Fixed midisynth coarse and fine tuning
- Version is now 1.209

2013-05-08 07:59 UTC - kode54
- Updated BASSMIDI to version 2.4.8
- Updated SoundFont support to use foobar2000 file system functions
- Version is now 1.208

2013-05-08 06:54 UTC - kode54
- Container normalizes port numbers correctly now
- Version is now 1.207

2013-05-08 02:39 UTC - kode54
- Implemented VSTHost hacks necessary to get Universal Sound Module
  working acceptably
- Version is now 1.206

2013-05-08 01:40 UTC - kode54
- Fixed a stupid VSTPlayer memory leak introduced with the new MIDI
  preset system

2013-05-08 01:27 UTC - kode54
- Reverted VSTHost process to use standard input and output,
  but reassign them to a null file for the duration of execution

2013-05-08 00:06 UTC - kode54
- Fixed external SysEx dump support for absolute paths
- Fixed external SysEx dump support for multiple items
- Version is now 1.205

2013-05-05 17:56 UTC - kode54
- Changed optimization settings
- Version is now 1.204

2013-05-05 03:52 UTC - kode54
- Minor optimization and fix to the Lanczos resampler
- Version is now 1.203

2013-05-03 02:23 UTC - kode54
- Replaced blargg's Fir_Resampler with a different Lanczos sinc resampler

2013-05-02 01:28 UTC - kode54
- Normalized MIDI port number meta commands per channel which they
  are applied to
- Version is now 1.202

2013-05-01 01:35 UTC - kode54
- Added King's Quest 6 GM set for MT-32 GM playback

2013-04-28 05:48 UTC - kode54
- Fixed configuration Apply button remaining active if plugin is
  set to oplmidi
- Verson is now 1.201

2013-04-28 03:31 UTC - kode54
- Rebased MUNT to version 1.2.0

2013-04-28 01:04 UTC - kode54
- Implemented support for applying external SysEx dumps to files
- Version is now 1.200

2013-04-24 16:09 UTC - kode54
- Added .KAR to extensions list for real this time
- Version is now 1.199

2013-03-07 17:27 UTC - kode54
- Fixed adlmidi negative vibrato depth
- Version is now 1.198

2013-03-07 16:27 UTC - kode54
- Split fmmidi into separate note factory implementation
- Implemented OPL-like synthesizer based on fmmidi

2013-02-26 03:57 UTC - kode54
- Implemented database metadata storage system
- Implemented per-file synthesizer preset configuration
- Version is now 1.197

2013-02-26 00:57 UTC - kode54
- Fixed adlmidi to initialize RPN to 0x3fff

2013-01-30 19:14 UTC - kode54
- Implemented support for MIDI meta number 9 for port name
- Implemented support for four ports in adlmidi player
- Fixed the Bisqwit bank, and all the banks were renumbered
- Version is now 1.196

2013-01-20 01:06 UTC - kode54
- Added more instrument banks to adlmidi
- Version is now 1.195

2013-01-17 02:57 UTC - kode54
- Regenerated the adldata file from latest adlmidi
- Fixed fmmidi bank LSB control
- Rearranged the preferences page
- Added automatic sorting of the adlmidi bank list
- Disabled adlmidi configuration when adlmidi isn't selected
- Version is now 1.194

2013-01-15 06:34 UTC - kode54
- Implemented support for fmmidi by yuno
- Version is now 1.193

2013-01-10 03:02 UTC - kode54
- Fixed adldata fixed drum notes for OP3 bank files
- Fixed adldata note length calculation for four operator voices
- Fixed adlmidi volume control to only apply to carrier voices
- Version is now 1.192

2013-01-08 03:02 UTC - kode54
- Added precalculated attack rates table to dbopl for 49716Hz, speeding
  up initialization
- Fixed notes with base notes or fine tune offsets so low that they wrap
  into the negative by changing one note variable to a signed type, and
  also fixed the instrument table generator for that case
- Version is now 1.191

2013-01-08 01:48 UTC - kode54
- Disabled adlmidi resampler if sample rate matches OPL3 native rate
- Version is now 1.190

2013-01-07 21:32 UTC - kode54
- Fixed gen_adldata to handle relative note number values, and also adjust
  DMX note numbers so they're never relative
- Version is now 1.189

2013-01-07 04:30 UTC - kode54
- Implement adlmidi resampler so emulated chips run at their
  native clock rate
- Version is now 1.188

2013-01-06 03:17 UTC - kode54
- Implemented adlmidi full panning mode
- Version is now 1.187

2013-01-06 01:12 UTC - kode54
- Reduced 4OP voice count to 4 per chip
- Fixed DMX instruments, now dual voice instruments are pseudo-
  four-operator, with each voice playing out of a separate
  speaker and the second voice slightly phased

2012-12-31 21:09 UTC - kode54
- Included new OPL3 banks and changed the default bank
- Version is now 1.186

2012-12-31 00:33 UTC - kode54
- Fixed adlmidi arpeggio handling
- Version is now 1.185

2012-12-30 17:41 UTC - kode54
- Automatically switch to BASSMIDI when a file SoundFont is detected
- Version is now 1.184

2012-12-30 17:34 UTC - kode54
- Fixed VSTi configuration for real this time

2012-12-30 17:04 UTC - kode54
- Removed 4OP configuration variable and set 4OP limit to 2 voices per chip
- Version is now 1.183

2012-12-30 00:09 UTC - kode54
- Fixed VSTi configuration
- Version is now 1.182

2012-12-29 16:18 UTC - kode54
- Implemented Adlib/OPL3 synthesizer driver based on Joel Yliluoma's adlmidi
- Version is now 1.181

2012-12-16 01:00 UTC - kode54
- Fixed sample rate configuration disable state for Emu de MIDI
- Version is now 1.180

2012-12-15 18:27 UTC - kode54
- Fixed file termination when seeking
- Version is now 1.179

2012-12-14 03:38 UTC - kode54
- Renamed MUNT output setting and added a warning
- Fixed enabling and disabling of BASSMIDI cache status
- Version is now 1.178

2012-12-14 01:28 UTC - kode54
- Added BASSMIDI sample cache statistics reporting to configuration dialog
- Version is now 1.177

2012-12-xx xx:xx UTC - kode54
- Updated BASSMIDI to version 2.4.7.10.
- Version is now 1.176

2012-11-26 11:24 UTC - kode54
- Added .KAR to the supported file name extensions list
- Version is now 1.175

2012-09-28 18:30 UTC - kode54
- Added support for automatically loading SoundFont banks named after the
  directory containing the MIDI file
- Version is now 1.174

2012-09-03 00:21 UTC - kode54
- Added storage for multiple VSTi configuration blocks
- Version is now 1.173

2012-09-02 22:47 UTC - kode54
- Fixed VSTi loading when the search path has a trailing slash
- Added exception handling around VSTi platform check
- Version is now 1.172

2012-09-02 01:58 UTC - kode54
- Implemented finite looping and fading support
- Version is now 1.171

2012-08-31 20:01 UTC - kode54
- Hotfix to include updated VSTHost.exe
- Now supports 64-bit VST instruments
- Version is now 1.170

2012-08-29 03:23 UTC - kode54
- Implemented support for 48 channel MIDI files
- Version is now 1.169

2012-08-22 19:44 UTC - kode54
- Updated BASSMIDI to version 2.4.7.9
- Hopefully fixed BASSMIDI player bank select control
- Version is now 1.168

2012-08-04 00:01 UTC - kode54
- Changed VSTi player to pass pipe names to the VST host and let it open
  them itself
- Version is now 1.167

2012-08-03 03:02 UTC - kode54
- Fixed VSTi driver for Edirol Hyper Canvas, stupid thing floods stdout
- Version is now 1.166

2012-08-01 01:14 UTC - kode54
- Added code page detection to MIDI metadata retrieval
- Version is now 1.165

2012-08-01 00:16 UTC - kode54
- Changed BASS plug-in loader to search using a wildcard match so
  I can add more plug-ins without changing the source code
- Version is now 1.164

2012-07-31 18:00 UTC - kode54
- Enabled sf2pack support with FLAC and WavPack plug-ins
- Version is now 1.163

2012-07-31 16:17 UTC - kode54
- Restructured MIDI port detection and routing
- Updated BASSMIDI to version 2.4.7.5
- Version is now 1.162

2012-07-28 10:39 UTC - kode54
- Restored and enabled tone portamento support in the LDS loader
- Version is now 1.161

2012-07-27 08:35 UTC - kode54
- Updated BASSMIDI to version 2.4.7.4
- Version is now 1.160

2012-07-27 08:24 UTC - kode54
- Made GS reset message detection more lenient
- Version is now 1.159

2012-07-27 03:36 UTC - kode54
- Added direct floating point output to MUNT
- Added "Super" mode to MUNT for non-MT-32 files
- Version is now 1.158

2012-07-22 15:48 UTC - kode54
- Updated MUNT to git revision munt_0_1_3-484-g6807408
- Version is now 1.157

2012-07-14 02:15 UTC - kode54
- Changed MIDI delta processing to signed 32 bit format and added guards and
  workarounds for negative results
- Version is now 1.156

2012-03-26 18:47 UTC - kode54
- Updated BASSMIDI to version 2.4.6.11
- Implemented support for BASSMIDI sinc interpolation
- Version is now 1.155

2012-02-19 19:59 UTC - kode54
- Added abort check to decoder
- Version is now 1.154

2012-02-09 14:28 UTC - kode54
- BASSMIDI driver now correctly frees SoundFont banks when all playback stops
- Version is now 1.153

2012-01-28 22:57 UTC - kode54
- Updated BASSMIDI to version 2.4.6.10
- Version is now 1.152

2012-01-25 10:39 UTC - kode54
- Added sanity checking to loop scanner so zero-length loops are ignored
- Version is now 1.151

2012-01-08 16:13 UTC - kode54
- Implemented MIDI instrument and bank change filters
- Version is now 1.150

2012-01-08 01:30 UTC - kode54
- Improved the VSTi host bridge and its communication system

2012-01-06 09:23 UTC - kode54
- Fixed automatic paired SoundFont loading for FluidSynth
- Disabled FluidSynth support
- Version is now 1.149

2012-01-06 09:18 UTC - kode54
- Rewrote VSTi player to use an external bridge process
- Version is now 1.148

2012-01-05 20:16 UTC - kode54
- Removed default VST search path and added a warning if no path is configured
- Version is now 1.147

2011-11-27 02:45 UTC - kode54
- Implemented support for GMF format files
- Version is now 1.146

2011-09-17 03:00 UTC - kode54
- Made XMI parser more tolerant of broken files
- Version is now 1.145

2011-09-11 09:41 UTC - kode54
- Implemented drum channel control for Emu de MIDI
- Version is now 1.144

2011-09-11 07:38 UTC - kode54
- Replaced Emu de MIDI main player core with my own stream parser
- Version is now 1.143

2011-09-10 03:19 UTC - kode54
- Made VSTi search path configurable
- Version is now 1.142

2011-08-09 23:13 UTC - kode54
- Removed faulty format detection logic from the MDS/MIDS format handler
- Version is now 1.141

2011-07-24 06:57 UTC - kode54
- Limited MIDI port numbers to 0 and 1
- Version is now 1.140

2011-07-24 04:51 UTC - kode54
- Restructured VSTi player and added dual port support
- Version is now 1.139

2011-07-11 19:58 UTC - kode54
- Implemented GM2 reset handling in FluidSynth and BASSMIDI drivers
- Fixed FluidSynth and BASSMIDI GS drum channel control again for files
  where tracks may end up mapped to the second port
- Version is now 1.138

2011-07-10 23:19 UTC - kode54
- Added VSTi plug-in configuration
- Version is now 1.137

2011-07-05 04:03 UTC - kode54
- Limited FluidSynth and BASSMIDI GS drum channel control to 16 channels
- Updated BASSMIDI to version 2.4.6.2
- Version is now 1.136

2011-07-04 22:45 UTC - kode54
- Changed FluidSynth and BASSMIDI drivers to require GM, GS, or XG mode for
  relevant drum channel controls to work
- Version is now 1.135

2011-07-03 03:42 UTC - kode54
- Fixed S-YXG50 VSTi configuration preset for 'funny' versions with junk after
  the preset name
- Version is now 1.134

2011-03-31 09:31 UTC - kode54
- Updated BASSMIDI to version 2.4.6
- Version is now 1.133

2011-03-21 17:55 UTC - kode54
- Updated BASSMIDI to version 2.4.5.14
- Version is now 1.132

2011-03-06 00:53 UTC - kode54
- Fixed the consequences of odd files with lots of weird instrument name meta
  events losing entire tracks in playback
- Version is now 1.131

2011-03-05 07:02 UTC - kode54
- Fixed multi-port MIDI file support
- Version is now 1.130

2011-03-03 13:44 UTC - kode54
- Implemented support for multi-port MIDI files, properly supported by the
  FluidSynth and BASSMIDI players
- Version is now 1.129

2011-02-29 02:00 UTC - kode54
- Updated BASSMIDI to version 2.4.5.13
- Amended EMIDI track exclusion to include GS tracks
- Version is now 1.128

2011-02-27 16:19 UTC - kode54
- Enabled Standard MIDI File type 2 support
- Fixed type 2 tempo handling
- Version is now 1.127

2011-02-25 23:28 UTC - kode54
- Changed BASSMIDI SoundFont reference counting system
- Updated BASSMIDI to version 2.4.5.12
- Fixed SoundFont list handler loading the last item twice
- Version is now 1.126

2011-02-25 19:27 UTC - kode54
- Fixed XMI tempo
- Version is now 1.125

2011-02-25 16:11 UTC - kode54
- Implemented MIDI type 2 subsong support

2011-02-25 01:44 UTC - kode54
- Fixed drum channel handling with new BASSMIDI
- Version is now 1.124

2011-02-22 06:38 UTC - kode54
- Fixed tempo map handling multiple tempos on the same timestamp

2011-02-22 05:41 UTC - kode54
- Implemented delayed SoundFont closing in BASSMIDI player

2011-02-22 05:00 UTC - kode54
- Updated to newer version of BASSMIDI which simplifies sending MIDI data

2011-02-20 09:22 UTC - kode54
- Implemented support for per-file SoundFont banks
- Version is now 1.123

2011-02-20 08:50 UTC - kode54
- Fixed MUS note off handling

2011-02-20 04:50 UTC - kode54
- Implemented BASSMIDI support

2011-02-14 13:13 UTC - kode54
- Completed LDS file processor
- Version is now 1.122

2011-02-13 01:29 UTC - kode54
- Fixed EMIDI track exclusion
- Version is now 1.121

2011-02-13 00:33 UTC - kode54
- Changed MIDI event class to store up to 16 bytes of parameter data statically
- Version is now 1.120

2011-02-12 00:09 UTC - kode54
- Standard MIDI file processor now ignores MIDI start and stop status codes
- Version is now 1.119

2011-02-11 23:56 UTC - kode54
- Changed standard MIDI file last event handling to work around Meta/SysEx events

2011-02-11 23:27 UTC - kode54
- Changed a few stricmps and stricmp_utf8s to pfc::stricmp_ascii, and added a safety check to
  VSTi finding function

2011-02-11 22:53 UTC - kode54
- Added MDS to file name extensions list

2011-02-11 22:42 UTC - kode54
- Completed MIDS file processor

2011-02-11 20:26 UTC - kode54
- Changed tempo map timestamp converter to default to a tempo of 500000

2011-02-11 20:05 UTC - kode54
- Completed MUS file processor

2011-02-11 17:14 UTC - kode54
- Fixed VSTi, FluidSynth, and MUNT players to correctly handle note on with velocity of zero
  when seeking

2011-02-11 17:05 UTC - kode54
- Removed incomplete track recovery option

2011-02-11 16:55 UTC - kode54
- Completed XMI file processor

2011-02-11 13:43 UTC - kode54
- Fixed loop end truncation in VSTi, FluidSynth, and MUNT players adding note off commands
  for the wrong channel numbers

2011-02-11 13:22 UTC - kode54
- Fixed loop end truncation in VSTi, FluidSynth, and MUNT players

2011-02-11 13:03 UTC - kode54
- Completed HMI file processor

2011-02-11 12:43 UTC - kode54
- Increased FluidSynth missing drum instrument search to include every preset below the
  missing preset

2011-02-11 11:40 UTC - kode54
- Completed HMP file processor

2011-02-10 12:39 UTC - kode54
- Completed RIFF MIDI file processor

2011-02-10 10:07 UTC - kode54
- Completed Standard MIDI file processor

2011-02-09 23:38 UTC - kode54
- Started rewrite of MIDI file processing routines

2011-02-09 08:22 UTC - kode54
- Fixed FluidSynth initializing channel 10 bank to 128 instead of DRUM_INST_BANK by default
- Version is now 1.118

2011-02-07 14:51 UTC - kode54
- Reverted reverb and chorus send level scale back to 20%
- Version is now 1.117

2011-02-05 06:40 UTC - kode54
- Reverted default reverb send level
- Version is now 1.116

2011-01-24 10:40 UTC - kode54
- Updated MUNT with MMX/3DNow!/SSE acceleration from the DOSBox Development forum
- Version is now 1.115

2011-01-23 09:04 UTC - kode54
- Modified SoundFont loader to translate banks to MSB
- Increased reverb and chorus level scale from 20% to 100%
- Enabled default reverb send level of 40
- Version is now 1.114

2011-01-23 02:55 UTC - kode54
- Implemented FluidSynth interpolation method configuration

2010-11-25 00:59 UTC - kode54
- Fixed a typo in the SoundFont loader dialog description string
- Fixed the SoundFont player to so it fails if the specified sflist file fails to open
- Fixed the FluidSynth SoundFont loader so it won't attempt to fclose a NULL file handle
- Version is now 1.113

2010-11-21 23:51 UTC - kode54
- FluidSynth player will now correctly fail instead of crashing if any SoundFonts fail to load
- Changed mostly cosmetic error with MT32Player using SFPlayer control flags
- Version is now 1.112

2010-11-16 03:46 UTC - kode54
- Fixed a potential crash in all readers involving deltas returning negative values
- Version is now 1.111

2010-09-10 19:39 UTC - kode54
- Made MUNT data file path configurable
- Made MUNT debug info configurable, disabled by default
- Version is now 1.110

2010-09-06 21:13 UTC - kode54
- Added some range checks to various MIDI parsing code, should hopefully prevent some crashes
  on invalid files
- Version is now 1.109

2010-08-10 20:39 UTC - kode54
- Changed open function to report unsupported file type when file loading fails, to
  allow fallback to other inputs
- Version is now 1.108

2010-08-08 22:04 UTC - kode54
- Added error checking to some memory allocation calls in MIDI loader
- Version is now 1.107

2010-07-21 07:26 UTC - kode54
- Implemented support for SoundFont list files in the FluidSynth player
- Version is now 1.106

2010-07-20 21:54 UTC - kode54
- Plugged a memory leak in FluidSynth with a dirty workaround
- Version is now 1.105

2010-07-19 20:37 UTC - kode54
- Updated FluidSynth to revision 328
- Version is now 1.104

2010-07-18 22:55 UTC - kode54
- Implemented error checking in FluidSynth dynamic sample loader
- Version is now 1.103

2010-07-18 07:46 UTC - kode54
- Fixed FluidSynth XG and GM2 drum channel allocation
- Version is now 1.102

2010-07-17 23:14 UTC - kode54
- Implemented dynamic sample loading in FluidSynth

2010-06-30 09:45 UTC - kode54
- Fixed FluidSynth pitch bend range RPN

2010-06-22 01:14 UTC - kode54
- Implemented GM support for MT-32 player

2010-05-13 12:48 UTC - kode54
- Implemented MT-32 player based on MUNT, triggered by MT-32 SysEx autodetection

2010-05-01 06:55 UTC - kode54
- Removed full path from the SoundFont selection box display, and made the file selection
  dialog open where the last file was selected.
- Version is now 1.101

2010-04-13 14:57 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 1.100

2010-02-18 20:14 UTC - kode54
- Fixed Emu de MIDI handling of initial track deltas
- Version is now 1.99

2010-02-16 21:17 UTC - kode54
- Fixed preferences apply for multiple settings
- Version is now 1.98

2010-02-10 09:49 UTC - kode54
- Made some parts of the MIDI processor used by VSTi and FluidSynth safer
- Version is now 1.97

2010-01-13 00:33 UTC - kode54
- Updated context menu code
- Version is now 1.96

2010-01-11 14:30 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.95

2009-09-01 03:13 UTC - kode54
- FluidSynth player now supports basic GS/XG drum channel control
- Version is now 1.94

2009-08-30 01:37 UTC - kode54
- Fixed loop end event handling for both VSTi and FluidSynth players
- Version is now 1.93

2009-08-23 02:20 UTC - kode54
- Fixed control change loop start position for VSTi and FluidSynth players

2009-08-22 23:28 UTC - kode54
- Now static linking to FluidSynth
- Version is now 1.92

2009-08-22 01:08 UTC - kode54
- Fixed crash bug with delay load checking by calling LoadLibraryEx instead

2009-08-22 00:49 UTC - kode54
- Fixed crash bug in FluidSynth player when seeking before starting playback
- Version is now 1.91

2009-08-21 08:02 UTC - kode54
- Added support for FluidSynth
- Version is now 1.9

2009-08-21 00:57 UTC - kode54
- Fixed memory leak in General MIDI converter context menu handler
- Version is now 1.83

2009-08-16 03:16 UTC - kode54
- Solved rounding error in MIDI do_table and Marker/SysEx map translation
- Version is now 1.82

2009-06-14 20:15 UTC - kode54
- Changed VSTi player loop end handler to simply truncate, rather than mash all end events together
- Version is now 1.81

2009-03-18 20:22 UTC - kode54
- Added General MIDI converter context menu item.
- Version is now 1.8

2006-12-24 05:15 UTC - kode54
- Fixed plug-in droplist change handler for VSTi plug-ins so the last item in the list won't enable
  the GS/XG to GM2 checkbox.

2006-08-21 09:22 UTC - kode54
- Added call to effMainsChanged before effStartProcess for some VST instruments which require
  it. (Steinberg Hypersonic 2 crashes without this.)
- Fixed a bug in VSTiPlayer Play(), where processReplace would be called even if it didn't exist.
- Added GM initialization preset for Steinberg Hypersonic 2, requires foo_unpack.
- Version is now 1.7

2005-11-14 00:00 UTC - kode54
- Fixed bug in VSTiPlayer seeking ( bleh, C-style cast flubbed changes to mem_block class )

2005-10-08 22:10 UTC - kode54
- Ported to 0.9 beta 7 + input API
- DXi support disabled because it is a bloated mess and should die
- Version is now 1.6

2005-06-06 08:02 UTC - kode54
- VSTi seeking now resets the synthesizer when seeking backwards, and drops complete note on/off pairs
- Version is now 1.55

2005-05-29 04:21 UTC - kode54
- Completed VSTi support (complete with default polyphony override for S-YXG50)
- Version is now 1.5

2005-03-12 11:02 UTC - kode54
- Added basic GS/XG to GM2 remapping for Hyper Canvas
- Version is now 1.2.3

2004-09-29 19:11 UTC - kode54
- Bug fix in first rmi_data parsing loop, forgot to enum next item uhuhuhu
- Version is now 1.2.2

2004-09-25 04:48 UTC - kode54
- Renamed internal synth to "Emu de MIDI" hopefully to prevent confusion with the EMIDI setting
- Implemented Data Increment and Decrement controller changes in CMIDIModule.cpp
- Changed default pitch bend range in CMIDIModule.cpp from 12 semitones to 2
- Version is now 1.2.1

2004-09-23 10:45 UTC - kode54
- Added internal Emu de MIDI synthesizer, for great chip style justice
- Commented out external input wrapper
- Version is now 1.2

2004-09-19 08:50 UTC - kode54
- Implemented external input wrapper with looping, for foo_emidi
- Version is now 1.1.1

2004-09-19 04:27 UTC - kode54
- Whoops, forgot that all RIFF chunks are word aligned, fixed LIST INFO parser
- Version is now 1.1

2004-09-19 03:08 UTC - kode54
- Changed grow_buf truncate from a #define to an inline function wrapper
- Implemented RIFF LIST INFO chunk parser
- Expanded metadata gathering

2004-03-31 17:53 UTC - kode54
- Fixed seeking past the end of the file
- Version is now 1.0.1

*/

#include <foobar2000.h>
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpersLean.h"
#include "../ATLHelpers/misc.h"

#include <midi_processing/midi_processor.h>

#include <map>

#include <shlobj.h>
#include <shlwapi.h>

#include "VSTiPlayer.h"
#ifdef FLUIDSYNTHSUPPORT
#include "SFPlayer.h"
#endif
#include "BMPlayer.h"
#include "MT32Player.h"
#include "EMIDIPlayer.h"

#include "ADLPlayer.h"
#include "../../../libADLMIDI/include/adlmidi.h"

#include "fmmidiPlayer.h"

#include "MSPlayer.h"

#include "SCPlayer.h"

#ifdef DXISUPPORT
#include "DXiProxy.h"
#include "PlugInInventory.h"
#pragma comment( lib, "strmiids.lib" )
#endif

#include "resource.h"

#define XMIDI_CONTROLLER_FOR_LOOP			0x74	// For Loop
#define XMIDI_CONTROLLER_NEXT_BREAK			0x75	// Next/Break

#define EMIDI_CONTROLLER_TRACK_DESIGNATION	110		// Track Designation
#define EMIDI_CONTROLLER_TRACK_EXCLUSION	111		// Track Exclusion
#define EMIDI_CONTROLLER_LOOP_BEGIN			XMIDI_CONTROLLER_FOR_LOOP
#define EMIDI_CONTROLLER_LOOP_END			XMIDI_CONTROLLER_NEXT_BREAK

// {4D11DD87-7A27-4ECC-BCB8-F018020BA2D5}
static const GUID guid_cfg_rpgmloopz = 
{ 0x4d11dd87, 0x7a27, 0x4ecc, { 0xbc, 0xb8, 0xf0, 0x18, 0x2, 0xb, 0xa2, 0xd5 } };
// {0F580D09-D57B-450C-84A2-D60E34BD64F5}
static const GUID guid_cfg_xmiloopz = 
{ 0xf580d09, 0xd57b, 0x450c, { 0x84, 0xa2, 0xd6, 0xe, 0x34, 0xbd, 0x64, 0xf5 } };
// {2E0DBDC2-7436-4B70-91FC-FD98378732B2}
static const GUID guid_cfg_ff7loopz = 
{ 0x2e0dbdc2, 0x7436, 0x4b70, { 0x91, 0xfc, 0xfd, 0x98, 0x37, 0x87, 0x32, 0xb2 } };
// {C090F9C7-47F9-4f6f-847A-27CD7596C9D4}
static const GUID guid_cfg_emidi_exclusion = 
{ 0xc090f9c7, 0x47f9, 0x4f6f, { 0x84, 0x7a, 0x27, 0xcd, 0x75, 0x96, 0xc9, 0xd4 } };
// {6D30C919-B053-43AA-9F1B-1D401882805E}
static const GUID guid_cfg_filter_instruments = 
{ 0x6d30c919, 0xb053, 0x43aa, { 0x9f, 0x1b, 0x1d, 0x40, 0x18, 0x82, 0x80, 0x5e } };
// {3145963C-7322-4B48-99FF-75EAC5F4DACC}
static const GUID guid_cfg_filter_banks = 
{ 0x3145963c, 0x7322, 0x4b48, { 0x99, 0xff, 0x75, 0xea, 0xc5, 0xf4, 0xda, 0xcc } };
// {FE5B24D8-C8A5-4b49-A163-972649217185}
/*static const GUID guid_cfg_recover_tracks = 
{ 0xfe5b24d8, 0xc8a5, 0x4b49, { 0xa1, 0x63, 0x97, 0x26, 0x49, 0x21, 0x71, 0x85 } };*/
// {DA3A7D23-BCEB-40f9-B594-2A9428A1E533}
static const GUID guid_cfg_loop_type = 
{ 0xda3a7d23, 0xbceb, 0x40f9, { 0xb5, 0x94, 0x2a, 0x94, 0x28, 0xa1, 0xe5, 0x33 } };
// {AE5BA73B-B0D4-4261-BFF2-11A1C44E57EA}
static const GUID guid_cfg_srate = 
{ 0xae5ba73b, 0xb0d4, 0x4261, { 0xbf, 0xf2, 0x11, 0xa1, 0xc4, 0x4e, 0x57, 0xea } };
// {1253BAC2-9193-420c-A919-9A1CF8706E2C}
static const GUID guid_cfg_plugin = 
{ 0x1253bac2, 0x9193, 0x420c, { 0xa9, 0x19, 0x9a, 0x1c, 0xf8, 0x70, 0x6e, 0x2c } };
// {F9DDD2C0-D8FD-442F-9E49-D901B51D6D38}
static const GUID guid_cfg_resampling = 
{ 0xf9ddd2c0, 0xd8fd, 0x442f, { 0x9e, 0x49, 0xd9, 0x1, 0xb5, 0x1d, 0x6d, 0x38 } };
// {408AA155-4C42-42b5-8C3E-D10C35DD5EF1}
static const GUID guid_cfg_history_rate = 
{ 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
// {F3EE2258-65D3-4219-B932-BF52119F2484}
/*static const GUID guid_cfg_gm2 = 
{ 0xf3ee2258, 0x65d3, 0x4219, { 0xb9, 0x32, 0xbf, 0x52, 0x11, 0x9f, 0x24, 0x84 } };*/
// {1A6EA7E5-718A-485a-B167-CFDF3B406145}
static const GUID guid_cfg_vst_path = 
{ 0x1a6ea7e5, 0x718a, 0x485a, { 0xb1, 0x67, 0xcf, 0xdf, 0x3b, 0x40, 0x61, 0x45 } };
// {696D12DD-AF32-43d9-8DF6-BDD11E818329}
static const GUID guid_cfg_soundfont_path = 
{ 0x696d12dd, 0xaf32, 0x43d9, { 0x8d, 0xf6, 0xbd, 0xd1, 0x1e, 0x81, 0x83, 0x29 } };
// {D7E0EC5E-872F-41E3-9B5B-D202D8B942A7}
static const GUID guid_cfg_munt_base_path = 
{ 0xd7e0ec5e, 0x872f, 0x41e3, { 0x9b, 0x5b, 0xd2, 0x2, 0xd8, 0xb9, 0x42, 0xa7 } };
#ifdef FLUIDSYNTHSUPPORT
// {A395C6FD-492A-401B-8BDB-9DF53E2EF7CF}
static const GUID guid_cfg_fluid_interp_method = 
{ 0xa395c6fd, 0x492a, 0x401b, { 0x8b, 0xdb, 0x9d, 0xf5, 0x3e, 0x2e, 0xf7, 0xcf } };
#endif
// {44E7C715-D256-44C4-8FB6-B720FA9B31FC}
static const GUID guid_cfg_vst_config = 
{ 0x44e7c715, 0xd256, 0x44c4, { 0x8f, 0xb6, 0xb7, 0x20, 0xfa, 0x9b, 0x31, 0xfc } };
#ifdef DXISUPPORT
// {D5C87282-A9E6-40F3-9382-9568E6541A46}
static const GUID guid_cfg_dxi_plugin = 
{ 0xd5c87282, 0xa9e6, 0x40f3, { 0x93, 0x82, 0x95, 0x68, 0xe6, 0x54, 0x1a, 0x46 } };
#endif
// {66524470-7EC7-445E-A6FD-C0FBAE74E5FC}
static const GUID guid_cfg_midi_parent = 
{ 0x66524470, 0x7ec7, 0x445e, { 0xa6, 0xfd, 0xc0, 0xfb, 0xae, 0x74, 0xe5, 0xfc } };
// {BB4C61A1-03C4-4B62-B04D-2C86CEDE005D}
static const GUID guid_cfg_vsti_search_path = 
{ 0xbb4c61a1, 0x3c4, 0x4b62, { 0xb0, 0x4d, 0x2c, 0x86, 0xce, 0xde, 0x0, 0x5d } };
// {851583F7-98B4-44C7-9DF4-4C7F859D13BA}
static const GUID guid_cfg_midi_timing_parent = 
{ 0x851583f7, 0x98b4, 0x44c7, { 0x9d, 0xf4, 0x4c, 0x7f, 0x85, 0x9d, 0x13, 0xba } };
// {D8492AD0-3B70-4768-8D07-97F5508C08E8}
static const GUID guid_cfg_midi_loop_count = 
{ 0xd8492ad0, 0x3b70, 0x4768, { 0x8d, 0x7, 0x97, 0xf5, 0x50, 0x8c, 0x8, 0xe8 } };
// {1CC76581-6FC8-445E-9E3D-020043D98B65}
static const GUID guid_cfg_midi_fade_time = 
{ 0x1cc76581, 0x6fc8, 0x445e, { 0x9e, 0x3d, 0x2, 0x0, 0x43, 0xd9, 0x8b, 0x65 } };
// {A62A00A7-0DBF-4475-BECA-EDBF5D064A80}
static const GUID guid_cfg_adl_bank = 
{ 0xa62a00a7, 0xdbf, 0x4475, { 0xbe, 0xca, 0xed, 0xbf, 0x5d, 0x6, 0x4a, 0x80 } };
static const GUID guid_cfg_adl_chips = 
{ 0x974365ed, 0xd4f9, 0x4daa, { 0xb4, 0x89, 0xad, 0x7a, 0xd2, 0x91, 0xfa, 0x94 } };
// {AD6821B4-493F-4BB3-B7BB-E0A67C5D5907}
static const GUID guid_cfg_adl_panning = 
{ 0xad6821b4, 0x493f, 0x4bb3, { 0xb7, 0xbb, 0xe0, 0xa6, 0x7c, 0x5d, 0x59, 0x7 } };
// {C5FB4053-75BF-4C0D-A1B1-7173863288A6}
/*static const GUID guid_cfg_adl_4op = 
{ 0xc5fb4053, 0x75bf, 0x4c0d, { 0xa1, 0xb1, 0x71, 0x73, 0x86, 0x32, 0x88, 0xa6 } };*/
// {07257AC7-9901-4A5F-9D8B-C5B5F1B8CF5B}
static const GUID guid_cfg_munt_gm = 
{ 0x7257ac7, 0x9901, 0x4a5f, { 0x9d, 0x8b, 0xc5, 0xb5, 0xf1, 0xb8, 0xcf, 0x5b } };
// {62BF901B-9C51-45FE-BE8A-14FB56205E5E}
static const GUID guid_cfg_bassmidi_effects = 
{ 0x62bf901b, 0x9c51, 0x45fe, { 0xbe, 0x8a, 0x14, 0xfb, 0x56, 0x20, 0x5e, 0x5e } };
// {F90C8ABF-68B5-474A-8D9C-FFD9CA80202F}
static const GUID guid_cfg_skip_to_first_note =
{ 0xf90c8abf, 0x68b5, 0x474a,{ 0x8d, 0x9c, 0xff, 0xd9, 0xca, 0x80, 0x20, 0x2f } };
// {F56FA8C3-38A1-49E0-AD8B-D65166314719}
static const GUID guid_cfg_adl_chorus = 
{ 0xf56fa8c3, 0x38a1, 0x49e0, { 0xad, 0x8b, 0xd6, 0x51, 0x66, 0x31, 0x47, 0x19 } };
// {7423A720-EB39-4D7D-9B85-524BC779B58B}
static const GUID guid_cfg_ms_synth =
{ 0x7423a720, 0xeb39, 0x4d7d,{ 0x9b, 0x85, 0x52, 0x4b, 0xc7, 0x79, 0xb5, 0x8b } };
// {A91D31F4-22AE-4C5C-A621-F6B6011F5DDC}
static const GUID guid_cfg_ms_bank =
{ 0xa91d31f4, 0x22ae, 0x4c5c,{ 0xa6, 0x21, 0xf6, 0xb6, 0x1, 0x1f, 0x5d, 0xdc } };
// {ABEE932A-2DAF-4A07-A64E-9B5A88DA0070}
static const GUID guid_cfg_sc_flavor =
{ 0xabee932a, 0x2daf, 0x4a07,{ 0xa6, 0x4e, 0x9b, 0x5a, 0x88, 0xda, 0x0, 0x70 } };
// {1BF1799D-7691-4075-98AE-43AE82D8C9CF}
static const GUID guid_cfg_sc_path =
{ 0x1bf1799d, 0x7691, 0x4075,{ 0x98, 0xae, 0x43, 0xae, 0x82, 0xd8, 0xc9, 0xcf } };
// {849C5C09-520A-4D62-A6D1-E8B432664948}
static const GUID guid_cfg_ms_panning =
{ 0x849c5c09, 0x520a, 0x4d62,{ 0xa6, 0xd1, 0xe8, 0xb4, 0x32, 0x66, 0x49, 0x48 } };
// {091C12A1-D42B-4F4E-8058-8B7F4C4DF3A1}
static const GUID guid_cfg_sc_reverb =
{ 0x91c12a1, 0xd42b, 0x4f4e,{ 0x80, 0x58, 0x8b, 0x7f, 0x4c, 0x4d, 0xf3, 0xa1 } };


class cfg_map : public cfg_var, public std::map<uint32_t, std::vector<uint8_t>> {
public:
	cfg_map(const GUID& guid) : cfg_var(guid), std::map<t_uint32, std::vector<uint8_t>>() {}

	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
		stream_writer_formatter<> out(*p_stream, p_abort);
		out.write_int( size() );
		for ( auto it = begin(); it != end(); ++it )
		{
			out.write_int( it->first );
			const t_uint32 size = pfc::downcast_guarded<t_uint32>(it->second.size());
			out << size;
			for(t_uint32 walk = 0; walk < size; ++walk) out << it->second[walk];
		}
	}

	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		stream_reader_formatter<> in(*p_stream,p_abort);

		clear();

		t_size count;
		in.read_int( count );

		for ( t_size i = 0; i < count; i++ )
		{
			t_uint32 p_key;
			std::vector<uint8_t> p_value;

			in.read_int( p_key );

			{
				t_uint32 size; in >> size; p_value.resize(size);
				for(t_uint32 walk = 0; walk < size; ++walk) in >> p_value[walk];
			}

			operator[] ( p_key ) = p_value;
		}
	}
};

enum
{
	default_cfg_rpgmloopz = 1,
	default_cfg_xmiloopz = 1,
	default_cfg_ff7loopz = 1,
	default_cfg_emidi_exclusion = 1,
	default_cfg_filter_instruments = 0,
	default_cfg_filter_banks = 0,
	//default_cfg_recover_tracks = 0,
	default_cfg_loop_type = 0,
	default_cfg_srate = 44100,
	default_cfg_plugin = 6,
	default_cfg_resampling = 1,
	default_cfg_adl_bank = 72,
	default_cfg_adl_chips = 10,
	default_cfg_adl_panning = 1,
	//default_cfg_adl_4op = 14,
	default_cfg_munt_gm = 0,
	default_cfg_ms_synth = 0,
	default_cfg_ms_bank = 2,
	default_cfg_sc_flavor = SCPlayer::sc_default,
	default_cfg_ms_panning = 0,
	default_cfg_sc_reverb = 1,
#ifdef FLUIDSYNTHSUPPORT
	default_cfg_fluid_interp_method = FLUID_INTERP_DEFAULT
#endif
};

#ifdef DXISUPPORT
static const GUID default_cfg_dxi_plugin = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
#endif

cfg_int cfg_rpgmloopz(guid_cfg_rpgmloopz, default_cfg_rpgmloopz), cfg_xmiloopz(guid_cfg_xmiloopz, default_cfg_xmiloopz), cfg_ff7loopz(guid_cfg_ff7loopz, default_cfg_ff7loopz),
		cfg_emidi_exclusion(guid_cfg_emidi_exclusion, default_cfg_emidi_exclusion), /*cfg_hack_xg_drums("yam", 0),*/
		cfg_filter_instruments(guid_cfg_filter_instruments, default_cfg_filter_instruments),
		cfg_filter_banks(guid_cfg_filter_banks, default_cfg_filter_banks),
		/*cfg_recover_tracks(guid_cfg_recover_tracks, default_cfg_recover_tracks),*/ cfg_loop_type(guid_cfg_loop_type, default_cfg_loop_type),
		/*cfg_nosysex("sux", 0),*/ /*cfg_gm2(guid_cfg_gm2, 0),*/
		cfg_srate(guid_cfg_srate, default_cfg_srate), cfg_plugin(guid_cfg_plugin, default_cfg_plugin),
		cfg_resampling(guid_cfg_resampling, default_cfg_resampling),
		cfg_adl_bank(guid_cfg_adl_bank, default_cfg_adl_bank),
		cfg_adl_chips(guid_cfg_adl_chips, default_cfg_adl_chips),
		cfg_adl_panning(guid_cfg_adl_panning, default_cfg_adl_panning),
		cfg_munt_gm(guid_cfg_munt_gm, default_cfg_munt_gm),
		/*cfg_adl_4op(guid_cfg_adl_4op, default_cfg_adl_4op),*/
		cfg_ms_synth(guid_cfg_ms_synth, default_cfg_ms_synth),
		cfg_ms_bank(guid_cfg_ms_bank, default_cfg_ms_bank),
		cfg_sc_flavor(guid_cfg_sc_flavor, default_cfg_sc_flavor),
		cfg_ms_panning(guid_cfg_ms_panning, default_cfg_ms_panning),
		cfg_sc_reverb(guid_cfg_sc_reverb, default_cfg_sc_reverb)
#ifdef FLUIDSYNTHSUPPORT
		,cfg_fluid_interp_method(guid_cfg_fluid_interp_method, default_cfg_fluid_interp_method)
#endif
		;

#ifdef DXISUPPORT
cfg_guid cfg_dxi_plugin(guid_cfg_dxi_plugin, default_cfg_dxi_plugin);
#endif

cfg_string cfg_vst_path(guid_cfg_vst_path, "");

cfg_map cfg_vst_config(guid_cfg_vst_config);

cfg_string cfg_soundfont_path(guid_cfg_soundfont_path, "");

cfg_string cfg_munt_base_path(guid_cfg_munt_base_path, "");

advconfig_branch_factory cfg_midi_parent("MIDI Player", guid_cfg_midi_parent, advconfig_branch::guid_branch_playback, 0);

advconfig_string_factory cfg_vsti_search_path("VSTi search path", guid_cfg_vsti_search_path, guid_cfg_midi_parent, 0, "");

advconfig_string_factory_MT cfg_sc_path("Secret Sauce path", guid_cfg_sc_path, guid_cfg_midi_parent, 0, "");

advconfig_branch_factory cfg_midi_timing_parent("Playback timing when loops present", guid_cfg_midi_timing_parent, guid_cfg_midi_parent, 1.0);

advconfig_integer_factory cfg_midi_loop_count("Loop count", guid_cfg_midi_loop_count, guid_cfg_midi_timing_parent, 0, 2, 1, 10);
advconfig_integer_factory cfg_midi_fade_time("Fade time (ms)", guid_cfg_midi_fade_time, guid_cfg_midi_timing_parent, 1, 5000, 10, 30000);

advconfig_checkbox_factory cfg_bassmidi_effects("BASSMIDI - Enable reverb and chorus processing", guid_cfg_bassmidi_effects, guid_cfg_midi_parent, 0, true);

advconfig_checkbox_factory cfg_skip_to_first_note("Skip to first note", guid_cfg_skip_to_first_note, guid_cfg_midi_parent, 0, false);

static const char * munt_bank_names[] =
{
	"Roland",
	"Sierra / King's Quest 6",
};

typedef struct ms_preset
{
	unsigned int synth;
	unsigned int bank;
	pfc::string8 name;
};

static pfc::array_t<ms_preset> g_ms_presets;

static struct init_ms_presets
{
	init_ms_presets()
	{
		MSPlayer::enum_synthesizers(enum_callback);
	}

	static void enum_callback(unsigned int synth, unsigned int bank, const char * name)
	{
		ms_preset temp = { synth, bank, name };
		g_ms_presets.append_single(temp);
	}
} g_init_ms_presets;

const char * ms_get_preset_name(unsigned int synth, unsigned int bank)
{
	for (size_t i = 0; i < g_ms_presets.get_count(); ++i)
	{
		const ms_preset & preset = g_ms_presets[i];
		if (preset.synth == synth && preset.bank == bank)
			return preset.name;
	}
	return "Unknown Preset";
}

void ms_get_preset(const char * name, unsigned int & synth, unsigned int & bank)
{
	for (size_t i = 0; i < g_ms_presets.get_count(); ++i)
	{
		const ms_preset & preset = g_ms_presets[i];
		if (strcmp(preset.name, name) == 0)
		{
			synth = preset.synth;
			bank = preset.bank;
			return;
		}
	}
	synth = default_cfg_ms_synth;
	bank = default_cfg_ms_bank;
}

struct midi_preset
{
	enum { version = 6 };

	// version 0
	unsigned int plugin;

	// v0 - plug-in == 1 - VSTi
	pfc::string8 vst_path;
	std::vector<uint8_t> vst_config;

	// v0 - plug-in == 2/4 - SoundFont synthesizer
	pfc::string8 soundfont_path;

#ifdef DXISUPPORT
	// v0 - plug-in == 5 - DXi
	GUID dxi_plugin;
#endif

	// v0 - plug-in == 6 - adlmidi
	// v0 - plug-in == 8 - oplmidi
	unsigned int adl_bank;

	// v0 - plug-in == 6 - adlmidi
	unsigned int adl_chips;
	bool adl_panning;
	// v3 - chorus
	bool adl_chorus;

	// v1 - plug-in == 3 - MUNT
	unsigned int munt_gm_set;

	// v2 - plug-in == 2/4 - SoundFont synthesizer
	bool effects;

	// v4 - plug-in == 9 - Nuclear Option
	unsigned int ms_synth;
	unsigned int ms_bank;
	// v6 - panning
	bool ms_panning;

	// v5 - plug-in == 10 - Secret Sauce
	unsigned int sc_flavor;
	// v6 - reverb
	bool sc_reverb;

	midi_preset()
	{
		plugin = cfg_plugin;
		vst_path = cfg_vst_path;
		VSTiPlayer * vstPlayer = NULL;
		try
		{
			vstPlayer = new VSTiPlayer;
			if (vstPlayer->LoadVST(vst_path))
			{
				vst_config = cfg_vst_config[ vstPlayer->getUniqueID() ];
			}
		}
		catch (...)
		{
			if ( plugin == 1 ) plugin = 0;
		}
		delete vstPlayer;
		soundfont_path = cfg_soundfont_path;
		effects = cfg_bassmidi_effects;
#ifdef DXISUPPORT
		dxi_plugin = cfg_dxi_plugin;
#endif
		adl_bank = cfg_adl_bank;
		adl_chips = cfg_adl_chips;
		adl_panning = !!cfg_adl_panning;
		munt_gm_set = cfg_munt_gm;
		ms_synth = cfg_ms_synth;
		ms_bank = cfg_ms_bank;
		sc_flavor = cfg_sc_flavor;
		ms_panning = cfg_ms_panning;
		sc_reverb = cfg_sc_reverb;
	}

	void serialize(pfc::string8 & p_out)
	{
		const char * const * banknames = adl_getBankNames();

		p_out.reset();

		p_out += pfc::format_int( version );
		p_out += "|";

		p_out += pfc::format_int( plugin );

		if ( plugin == 1 )
		{
			p_out += "|";

			p_out += vst_path;
			p_out += "|";

			for ( unsigned i = 0; i < vst_config.size(); i++ )
			{
				p_out += pfc::format_hex( vst_config[ i ], 2 );
			}
		}
		else if ( plugin == 2 || plugin == 4 )
		{
			p_out += "|";

			p_out += soundfont_path;

			p_out += "|";

			p_out += pfc::format_int( effects );
		}
#ifdef DXISUPPORT
		else if ( plugin == 5 )
		{
			p_out += "|";

			p_out += pfc::format_hex( dxi_plugin.Data1, 8 );
			p_out += "-";
			p_out += pfc::format_hex( dxi_plugin.Data2, 4 );
			p_out += "-";
			p_out += pfc::format_hex( dxi_plugin.Data3, 4 );
			p_out += "-";
			p_out += pfc::format_hex( dxi_plugin.Data4[0], 2 );
			p_out += pfc::format_hex( dxi_plugin.Data4[1], 2 );
			p_out += "-";
			p_out += pfc::format_hex( dxi_plugin.Data4[2], 2 );
			p_out += pfc::format_hex( dxi_plugin.Data4[3], 2 );
			p_out += pfc::format_hex( dxi_plugin.Data4[4], 2 );
			p_out += pfc::format_hex( dxi_plugin.Data4[5], 2 );
			p_out += pfc::format_hex( dxi_plugin.Data4[6], 2 );
			p_out += pfc::format_hex( dxi_plugin.Data4[7], 2 );
		}
#endif
		else if ( plugin == 6 )
		{
			p_out += "|";

			p_out += banknames[ adl_bank ];
			p_out += "|";

			p_out += pfc::format_int( adl_chips );
			p_out += "|";

			p_out += pfc::format_int( adl_panning );
			p_out += "|";

			p_out += pfc::format_int( adl_chorus );
		}
		else if ( plugin == 3 )
		{
			p_out += "|";

			p_out += munt_bank_names[ munt_gm_set ];
		}
		else if ( plugin == 9 )
		{
			p_out += "|";
			p_out += ms_get_preset_name( ms_synth, ms_bank );
			p_out += "|";
			p_out += pfc::format_int( ms_panning );
		}
		else if ( plugin == 10 )
		{
			p_out += "|";
			p_out += pfc::format_int( sc_flavor );
			p_out += "|";
			p_out += pfc::format_int( sc_reverb );
		}
	}

	void unserialize( const char * p_in )
	{
		const char * bar_pos = strchr( p_in, '|' );
		if ( !bar_pos ) return;

		unsigned in_version = pfc::atodec<unsigned>( p_in, bar_pos - p_in );
		if ( in_version > version ) return;

		p_in = bar_pos + 1;

		bar_pos = strchr( p_in, '|' );
		if ( !bar_pos ) bar_pos = p_in + strlen( p_in );

		unsigned in_plugin = pfc::atodec<unsigned>( p_in, bar_pos - p_in );
		pfc::string8 in_vst_path;
		std::vector<uint8_t> in_vst_config;
		pfc::string8 in_soundfont_path;
		bool in_effects;
		GUID in_dxi_plugin;
		unsigned in_adl_bank, in_adl_chips;
		bool in_adl_panning;
		bool in_adl_chorus;
		unsigned in_munt_gm_set;
		unsigned in_ms_synth, in_ms_bank;
		unsigned in_sc_flavor;
		bool in_ms_panning;
		bool in_sc_reverb;

		if ( *bar_pos )
		{
			p_in = bar_pos + 1;
			bar_pos = strchr( p_in, '|' );
			if ( !bar_pos ) bar_pos = p_in + strlen( p_in );

			if ( in_plugin == 1 )
			{
				in_vst_path.set_string( p_in, bar_pos - p_in );

				p_in = bar_pos + (*bar_pos == '|');
				bar_pos = strchr( p_in, '|' );
				if ( !bar_pos ) bar_pos = p_in + strlen( p_in );

				while ( *p_in )
				{
					in_vst_config.push_back( pfc::atohex<unsigned char>( p_in, 2 ) );
					p_in += 2;
				}
			}
			else if ( in_plugin == 2 || in_plugin == 4 )
			{
				in_soundfont_path.set_string( p_in, bar_pos - p_in );

				p_in = bar_pos + (*bar_pos == '|');
				bar_pos = strchr( p_in, '|' );
				if ( !bar_pos ) bar_pos = p_in + strlen( p_in );

				if ( bar_pos > p_in )
				{
					in_effects = pfc::atodec<bool>( p_in, 1 );
				}
				else
				{
					in_effects = cfg_bassmidi_effects;
				}
			}
#ifdef DXISUPPORT
			else if ( in_plugin == 5 )
			{
				if ( bar_pos - p_in < 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 ) return;
				in_dxi_plugin.Data1 = pfc::atohex<t_uint32>( p_in, 8 );
				in_dxi_plugin.Data2 = pfc::atohex<t_uint16>( p_in + 8 + 1, 4 );
				in_dxi_plugin.Data3 = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1, 4 );
				in_dxi_plugin.Data4[0] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1, 2 );
				in_dxi_plugin.Data4[1] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2, 2 );
				in_dxi_plugin.Data4[2] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1, 2 );
				in_dxi_plugin.Data4[3] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2, 2 );
				in_dxi_plugin.Data4[4] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2, 2 );
				in_dxi_plugin.Data4[5] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2, 2 );
				in_dxi_plugin.Data4[6] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2, 2 );
				in_dxi_plugin.Data4[7] = pfc::atohex<t_uint16>( p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2 + 2, 2 );				
			}
#endif
			else if ( in_plugin == 6 )
			{
				const char * const * banknames = adl_getBankNames();
				unsigned j = adl_getBanksCount();
				unsigned i;
				for ( i = 0; i < j; i++ )
				{
					size_t len = strlen( banknames[ i ] );
					if ( len == bar_pos - p_in && !strncmp( p_in, banknames[ i ], len ) )
					{
						in_adl_bank = i;
						break;
					}
				}
				if ( i == j ) return;

				p_in = bar_pos + (*bar_pos == '|');
				bar_pos = strchr( p_in, '|' );
				if ( !bar_pos ) bar_pos = p_in + strlen( p_in );

				if ( !*p_in ) return;
				in_adl_chips = pfc::atodec<unsigned>( p_in, bar_pos - p_in );

				p_in = bar_pos + (*bar_pos == '|');
				bar_pos = strchr( p_in, '|' );
				if ( !bar_pos ) bar_pos = p_in + strlen( p_in );

				if ( !*p_in ) return;
				in_adl_panning = !!pfc::atodec<unsigned>( p_in, bar_pos - p_in );
			}
		}
		else if ( in_plugin == 3 )
		{
			unsigned i, j;
			for ( i = 0, j = _countof( munt_bank_names ); i < j; i++ )
			{
				size_t len = strlen( munt_bank_names[ i ] );
				if ( len == bar_pos - p_in && !strncmp( p_in, munt_bank_names[ i ], len ) )
				{
					in_munt_gm_set = i;
					break;
				}
			}
			if ( i == j ) return;
		}
		else if ( in_plugin == 9 )
		{
			pfc::string8 temp;
			temp.set_string(p_in, bar_pos - p_in);
			ms_get_preset(temp, in_ms_synth, in_ms_bank);
			if ( version >= 6 )
			{
				p_in = bar_pos + (*bar_pos == '|');
				bar_pos = strchr(p_in, '|');
				if (!bar_pos) bar_pos = p_in + strlen(p_in);

				if (!*p_in) return;
				in_ms_panning = !!pfc::atodec<unsigned>(p_in, bar_pos - p_in);
			}
			else
			{
				in_ms_panning = true;
			}
		}
		else if ( in_plugin == 10 )
		{
			in_sc_flavor = pfc::atodec<unsigned>( p_in, bar_pos - p_in );
			if ( in_sc_flavor > SCPlayer::sc_xg )
				in_sc_flavor = SCPlayer::sc_default;
			if (version >= 6)
			{
				p_in = bar_pos + (*bar_pos == '|');
				bar_pos = strchr(p_in, '|');
				if (!bar_pos) bar_pos = p_in + strlen(p_in);

				if (!*p_in) return;
				in_sc_reverb = !!pfc::atodec<unsigned>(p_in, bar_pos - p_in);
			}
			else
			{
				in_sc_reverb = true;
			}
		}

		plugin = in_plugin;
		vst_path = in_vst_path;
		vst_config = in_vst_config;
		soundfont_path = in_soundfont_path;
		effects = in_effects;
#ifdef DXISUPPORT
		dxi_plugin = in_dxi_plugin;
#endif
		adl_bank = in_adl_bank;
		adl_chips = in_adl_chips;
		adl_panning = in_adl_panning;
		adl_chorus = in_adl_chorus;
		munt_gm_set = in_munt_gm_set;
		ms_synth = in_ms_synth;
		ms_bank = in_ms_bank;
		sc_flavor = in_sc_flavor;
		ms_panning = in_ms_panning;
		sc_reverb = in_sc_reverb;
	}
};

static void relative_path_create( const char * p_file, const char * p_base_path, pfc::string_base & p_out )
{
	t_size p_file_fn = pfc::scan_filename( p_file );
	t_size p_base_path_fn = pfc::scan_filename( p_base_path );

	if ( p_file_fn == p_base_path_fn && !strncmp( p_file, p_base_path, p_file_fn ) )
	{
		p_out = p_file + p_file_fn;
	}
	else if ( p_file_fn > p_base_path_fn && !strncmp( p_file, p_base_path, p_base_path_fn ) && pfc::is_path_separator( p_file[ p_base_path_fn - 1 ] ) )
	{
		p_out = p_file + p_base_path_fn;
	}
	else if ( p_base_path_fn > p_file_fn && !strncmp( p_file, p_base_path, p_file_fn ) && pfc::is_path_separator( p_base_path[ p_file_fn - 1 ] ) )
	{
		p_out.reset();
		t_size p_base_path_search = p_file_fn;
		while ( p_base_path_search < p_base_path_fn )
		{
			if ( pfc::is_path_separator( p_base_path[ p_base_path_search++ ] ) )
			{
				p_out += "..\\";
			}
		}
		p_out += p_file + p_file_fn;
	}
	else
	{
		p_out = p_file;
	}
}

static void relative_path_parse( const char * p_relative, const char * p_base_path, pfc::string_base & p_out )
{
	if ( strstr( p_relative, "://" ) )
	{
		p_out = p_relative;
	}
	else
	{
		pfc::string8 temp = p_base_path;
		temp.truncate( temp.scan_filename() );
		temp += p_relative;
		filesystem::g_get_canonical_path( temp, p_out );
	}
}

struct midi_syx_dumps
{
	pfc::array_t<pfc::string8> dumps;

	midi_syx_dumps() { }
	midi_syx_dumps( const midi_syx_dumps & p_in ) : dumps( p_in.dumps ) { }

	void serialize( const char * p_midi_path, pfc::string8 & p_out )
	{
		pfc::string8_fast p_relative;

		p_out.reset();

		for ( unsigned i = 0; i < dumps.get_count(); i++ )
		{
			relative_path_create( dumps[ i ], p_midi_path, p_relative );

			if ( i ) p_out += "\n";
			p_out += p_relative;
		}
	}

	void unserialize( const char * p_in, const char * p_midi_path )
	{
		pfc::string8_fast p_relative, p_absolute;

		const char * end = p_in + strlen( p_in );

		while ( p_in < end )
		{
			const char * lf_pos = strchr( p_in, '\n' );
			if ( !lf_pos ) lf_pos = end;

			p_relative.set_string( p_in, lf_pos - p_in );
			relative_path_parse( p_relative, p_midi_path, p_absolute );

			dumps.append_single( p_absolute );

			p_in = lf_pos;
			while ( *p_in == '\n' ) ++p_in;
		}
	}

	void merge_into_file( midi_container & p_midi_file, abort_callback & p_abort )
	{
		std::vector<uint8_t> file_data;

		for ( unsigned i = 0; i < dumps.get_count(); i++ )
		{
			file::ptr p_file;
			midi_container p_dump;

			try
			{
				filesystem::g_open( p_file, dumps[ i ], filesystem::open_mode_read, p_abort );

				t_filesize size = p_file->get_size_ex( p_abort );

				file_data.resize( size );
				p_file->read_object( &file_data[0], size, p_abort );

				if ( ! midi_processor::process_syx_file( file_data, p_dump ) )
					break;

				p_midi_file.merge_tracks( p_dump );
			}
			catch ( std::exception & e )
			{
				pfc::string8 path;
				filesystem::g_get_canonical_path( dumps[ i ], path );
				throw exception_io_data( pfc::string_formatter() << "Error processing dump " << path << ": " << e.what() );
			}
		}
	}
};

static const char * exts[]=
{
	"MID",
	"MIDI",
	"KAR",
	"RMI",
	"MIDS", "MDS",
//	"CMF",
//	"GMF",
	"HMI",
	"HMP",
	"MUS",
	"XMI",
	"LDS",
};

static const char * exts_syx[]=
{
	"SYX",
	"DMP"
};

static bool g_test_extension(const char * ext)
{
	int n;
	for(n=0;n<_countof(exts);n++)
	{
		if (!stricmp(ext,exts[n])) return true;
	}
	return false;
}

static bool g_test_extension_syx(const char * ext)
{
	int n;
	for(n=0;n<_countof(exts_syx);n++)
	{
		if (!stricmp(ext,exts_syx[n])) return true;
	}
	return false;
}

// {4209C12E-C2F4-40CA-B2BC-FB61C32687D0}
static const GUID guid_midi_index = 
{ 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };

static const char field_hash[] = "midi_hash";
static const char field_format[] = "midi_format";
static const char field_tracks[] = "midi_tracks";
static const char field_channels[] = "midi_channels";
static const char field_ticks[] = "midi_ticks";
static const char field_type[] = "midi_type";
static const char field_loop_start[] = "midi_loop_start";
static const char field_loop_end[] = "midi_loop_end";
static const char field_loop_start_ms[] = "midi_loop_start_ms";
static const char field_loop_end_ms[] = "midi_loop_end_ms";
static const char field_preset[] = "midi_preset";
static const char field_syx[] = "midi_sysex_dumps";

class metadb_index_client_midi : public metadb_index_client
{
	virtual metadb_index_hash transform(const file_info & info, const playable_location & location)
	{
		const metadb_index_hash hash_null = 0;

		if ( !g_test_extension( pfc::string_extension( location.get_path() ) ) ) return hash_null;

		hasher_md5_state hasher_state;
		static_api_ptr_t<hasher_md5> hasher;

		t_uint32 subsong = location.get_subsong();

		hasher->initialize( hasher_state );

		hasher->process( hasher_state, &subsong, sizeof(subsong) );
		
		const char * str = info.info_get( field_hash );
		if ( str ) hasher->process_string( hasher_state, str );
		else hasher->process_string( hasher_state, location.get_path() );

#define HASH_STRING(s) \
		str = info.info_get( s ); \
		if ( str ) hasher->process_string( hasher_state, str );

		HASH_STRING( field_format );
		HASH_STRING( field_tracks );
		HASH_STRING( field_channels );
		HASH_STRING( field_ticks );
		HASH_STRING( field_type );
		HASH_STRING( field_loop_start );
		HASH_STRING( field_loop_end );
		HASH_STRING( field_loop_start_ms );
		HASH_STRING( field_loop_end_ms );

		return from_md5( hasher->get_result( hasher_state ) );
	}
};

class initquit_midi : public initquit
{
public:
	void on_init()
	{
		static_api_ptr_t<metadb_index_manager>()->add( new service_impl_t<metadb_index_client_midi>, guid_midi_index, 4 * 7 * 24 * 60 * 60 * 10000000 );
	}

	void on_quit() { }
};

static critical_section sync;
static volatile int g_running = 0;
static volatile int g_srate;

class input_midi : public input_stubs
{
#ifdef DXISUPPORT
	DXiProxy * dxiProxy;
#endif

	bool is_emidi;

	MIDIPlayer * midiPlayer;

	midi_container midi_file;

	unsigned srate;
	unsigned plugin;
	unsigned resampling;

	bool b_rpgmloopz;
	bool b_xmiloopz;
	bool b_ff7loopz;
	unsigned loop_type;
	unsigned clean_flags;
	//bool b_gm2;

	unsigned length_ms;
	unsigned length_samples;
	unsigned length_ticks;
	unsigned samples_done;

	unsigned loop_begin, loop_begin_ms;
	unsigned loop_end, loop_end_ms;

	unsigned loop_count, fade_ms;

	unsigned samples_played;
	unsigned samples_fade_begin;
	unsigned samples_fade_end;

	bool eof;
	bool dont_loop;

	bool first_block;

	unsigned original_track_count;

	bool is_syx;

	pfc::string8 m_path;

	t_filestats m_stats;

	metadb_index_hash m_index_hash;
	hasher_md5_result m_file_hash;

#if audio_sample_size != 32
	pfc::array_t<float> sample_buffer;
#endif

	/* crap for external input */
	/*
	input * external_decoder;
	reader * mem_reader;

	unsigned sample_loop_start;
	unsigned sample_loop_end;
	*/

public:
	input_midi() : srate(cfg_srate), resampling(cfg_resampling),
		loop_type(cfg_loop_type), b_rpgmloopz(!!cfg_rpgmloopz), b_xmiloopz(!!cfg_xmiloopz), b_ff7loopz(!!cfg_ff7loopz) //, b_gm2(!!cfg_gm2)
	{
#ifdef DXISUPPORT
		dxiProxy = NULL;
#endif

		is_emidi = false;

		midiPlayer = NULL;

		length_samples = 0;
		length_ticks = 0;

		/*
		external_decoder = 0;
		mem_reader = 0;
		*/

		clean_flags = (cfg_emidi_exclusion ? midi_container::clean_flag_emidi : 0) |
			(cfg_filter_instruments ? midi_container::clean_flag_instruments : 0) |
			(cfg_filter_banks ? midi_container::clean_flag_banks : 0);

		loop_count = cfg_midi_loop_count.get();
		fade_ms = cfg_midi_fade_time.get();
	}

	~input_midi()
	{
		/*if (external_decoder) external_decoder->service_release();
		if (mem_reader) mem_reader->reader_release();*/
		delete midiPlayer;
		if (is_emidi)
		{
			insync(sync);
			g_running--;
		}
#ifdef DXISUPPORT
		if (dxiProxy) delete dxiProxy;
#endif
	}

private:
	double get_length( unsigned p_subsong )
	{
		length_ms = midi_file.get_timestamp_end( p_subsong, true );
		double length = length_ms * .001 + 1.;
		length_ticks = midi_file.get_timestamp_end( p_subsong ); //theSequence->m_tempoMap.Sample2Tick(len, 1000);
		length_samples = (unsigned)(((__int64)length_ms * (__int64)srate) / 1000) + srate;
		loop_begin = midi_file.get_timestamp_loop_start( p_subsong );
		loop_end = midi_file.get_timestamp_loop_end( p_subsong );
		loop_begin_ms = midi_file.get_timestamp_loop_start( p_subsong, true );
		loop_end_ms = midi_file.get_timestamp_loop_end( p_subsong, true );
		if ( loop_begin != ~0 || loop_end != ~0 || loop_type > 1 )
		{
			if ( loop_begin_ms == ~0 ) loop_begin_ms = 0;
			if ( loop_end_ms == ~0 ) loop_end_ms = length_ms;
			length = (double)( loop_begin_ms + ( loop_end_ms - loop_begin_ms ) * loop_count + fade_ms ) * 0.001;
		}
		return length;
	}

	void set_loop()
	{
#ifdef DXISUPPORT
		if (plugin == 5 && dxiProxy)
		{
			dxiProxy->setLoop( loop_begin != ~0 ? loop_begin : 0, loop_end != ~0 ? loop_end : length_ticks );
		}
		/*else
		{
			sample_loop_start = theSequence->m_tempoMap.Tick2Sample(loop_begin != -1 ? loop_begin : 0, srate);
			sample_loop_end = theSequence->m_tempoMap.Tick2Sample((loop_end != -1 ? loop_end : length_ticks) + 1, srate);
		}*/
		else
#endif
		dont_loop = false;
	}

public:
	void open( service_ptr_t<file> p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_file.is_empty() )
		{
			filesystem::g_open( p_file, p_path, filesystem::open_mode_read, p_abort );
		}

		m_path = p_path;
		is_syx = g_test_extension_syx( pfc::string_extension( p_path ) );

		m_stats = p_file->get_stats( p_abort );
		if ( ! m_stats.m_size || m_stats.m_size > ( 1 << 30 ) ) throw exception_io_unsupported_format();

		std::vector<uint8_t> file_data;

		file_data.resize( m_stats.m_size );
		p_file->read_object( &file_data[0], m_stats.m_size, p_abort );

		if ( is_syx )
		{
			if ( ! midi_processor::process_syx_file( file_data, midi_file ) )
				throw exception_io_data( "Invalid SysEx dump" );
			return;
		}

		if ( ! midi_processor::process_file( file_data, pfc::string_extension( p_path ), midi_file ) )
			throw exception_io_data( "Invalid MIDI file" );

		original_track_count = midi_file.get_track_count();

		if ( ! original_track_count )
			throw exception_io_data( "Invalid MIDI file" );

		midi_file.scan_for_loops( b_xmiloopz, b_ff7loopz, b_rpgmloopz );

		file_data.resize( 0 );

		midi_file.serialize_as_standard_midi_file( file_data );

		hasher_md5_state hasher_state;
		static_api_ptr_t<hasher_md5> hasher;

		hasher->initialize( hasher_state );
		hasher->process( hasher_state, &file_data[0], file_data.size() );

		m_file_hash = hasher->get_result( hasher_state );

		if ( cfg_skip_to_first_note )
			midi_file.trim_start();
	}

	unsigned get_subsong_count()
	{
		return is_syx ? 1 : midi_file.get_subsong_count();
	}

	t_uint32 get_subsong( unsigned p_index )
	{
		return is_syx ? 0 : midi_file.get_subsong( p_index );
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		if ( is_syx ) return;

		midi_meta_data meta_data;
		midi_file.get_meta_data( p_subsong, meta_data );

		midi_meta_data_item item;
		bool remap_display_name = !meta_data.get_item( "title", item );

		for ( t_size i = 0; i < meta_data.get_count(); ++i )
		{
			const midi_meta_data_item & item = meta_data[ i ];
			if ( pfc::stricmp_ascii( item.m_name.c_str(), "type" ) )
			{
				std::string name = item.m_name;
				if ( remap_display_name && !pfc::stricmp_ascii( name.c_str(), "display_name" ) ) name = "title";
				p_info.meta_add( name.c_str(), item.m_value.c_str() );
			}
		}

		p_info.info_set_int(field_format, midi_file.get_format());
		p_info.info_set_int(field_tracks, midi_file.get_format() == 2 ? 1 : midi_file.get_track_count());
		p_info.info_set_int(field_channels, midi_file.get_channel_count(p_subsong));
		p_info.info_set_int(field_ticks, midi_file.get_timestamp_end(p_subsong));
		if (meta_data.get_item("type", item)) p_info.info_set(field_type, item.m_value.c_str());

		unsigned loop_begin = midi_file.get_timestamp_loop_start( p_subsong );
		unsigned loop_end = midi_file.get_timestamp_loop_end( p_subsong );
		unsigned loop_begin_ms = midi_file.get_timestamp_loop_start( p_subsong, true );
		unsigned loop_end_ms = midi_file.get_timestamp_loop_end( p_subsong, true );

		if (loop_begin != ~0) p_info.info_set_int(field_loop_start, loop_begin );
		if (loop_end != ~0) p_info.info_set_int(field_loop_end, loop_end );
		if (loop_begin_ms != ~0) p_info.info_set_int(field_loop_start_ms, loop_begin_ms );
		if (loop_end_ms != ~0) p_info.info_set_int(field_loop_end_ms, loop_end_ms );
		//p_info.info_set_int("samplerate", srate);
		unsigned length_ms = midi_file.get_timestamp_end( p_subsong, true );
		double length = double( length_ms ) * 0.001 + 1.0;
		if ( loop_begin != ~0 || loop_end != ~0 || loop_type > 1 )
		{
			if ( loop_begin_ms == ~0 ) loop_begin_ms = 0;
			if ( loop_end_ms == ~0 ) loop_end_ms = length_ms;
			length = (double)( loop_begin_ms + ( loop_end_ms - loop_begin_ms ) * loop_count + fade_ms ) * 0.001;
		}
		p_info.info_set_int("channels", 2);
		p_info.info_set( "encoding", "synthesized" );
		p_info.set_length( length );

		pfc::string8 hash_string;

		for ( unsigned i = 0; i < 16; i++ ) hash_string += pfc::format_uint( (t_uint8)m_file_hash.m_data[i], 2, 16 );

		p_info.info_set( field_hash, hash_string );

		service_ptr_t<metadb_index_client> index_client = new service_impl_t<metadb_index_client_midi>;
		m_index_hash = index_client->transform( p_info, playable_location_impl( m_path, p_subsong ) );

		pfc::array_t<t_uint8> tag;
		static_api_ptr_t<metadb_index_manager>()->get_user_data_t( guid_midi_index, m_index_hash, tag );

		if ( tag.get_count() )
		{
			file::ptr tag_file;
			filesystem::g_open_tempmem( tag_file, p_abort );
			tag_file->write_object( tag.get_ptr(), tag.get_count(), p_abort );

			p_info.meta_remove_all();

			tag_processor::read_trailing( tag_file, p_info, p_abort );
			p_info.info_set( "tagtype", "apev2 db" );

			const char * midi_preset = p_info.meta_get( field_preset, 0 );
			if ( midi_preset )
			{
				p_info.info_set( field_preset, midi_preset );
				p_info.meta_remove_field( field_preset );
			}

			const char * midi_syx = p_info.meta_get( field_syx, 0 );
			if ( midi_syx )
			{
				p_info.info_set( field_syx, midi_syx );
				p_info.meta_remove_field( field_syx );
			}
		}
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	static bool test_soundfont_extension( const char * base_path, pfc::string_base & path, abort_callback & p_abort )
	{
		static const char * extensions[] = {
			"json",
			"sflist",
#ifdef SF2PACK
			"sf2pack",
#endif
			"sf2"
		};
		path = base_path;
		size_t length = path.length();
		for (int i = 0; i < _countof(extensions); ++i)
		{
			path.truncate( length );
			path += ".";
			path += extensions[ i ];
			if ( filesystem::g_exists( path, p_abort ) )
				return true;
		}
		return false;
	}

	void decode_initialize( unsigned p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		if ( is_syx )
		{
			throw exception_io_data( "You cannot play SysEx dumps" );
		}

		midi_preset thePreset;

		midi_syx_dumps theDumps;

		{
			file_info_impl p_info;

			midi_meta_data meta_data;
			midi_file.get_meta_data( p_subsong, meta_data );

			midi_meta_data_item item;

			p_info.info_set_int(field_format, midi_file.get_format());
			p_info.info_set_int(field_tracks, midi_file.get_format() == 2 ? 1 : midi_file.get_track_count());
			p_info.info_set_int(field_channels, midi_file.get_channel_count(p_subsong));
			p_info.info_set_int(field_ticks, midi_file.get_timestamp_end(p_subsong));
			if (meta_data.get_item("type", item)) p_info.info_set(field_type, item.m_value.c_str());

			unsigned loop_begin = midi_file.get_timestamp_loop_start( p_subsong );
			unsigned loop_end = midi_file.get_timestamp_loop_end( p_subsong );
			unsigned loop_begin_ms = midi_file.get_timestamp_loop_start( p_subsong, true );
			unsigned loop_end_ms = midi_file.get_timestamp_loop_end( p_subsong, true );

			if (loop_begin != ~0) p_info.info_set_int(field_loop_start, loop_begin );
			if (loop_end != ~0) p_info.info_set_int(field_loop_end, loop_end );
			if (loop_begin_ms != ~0) p_info.info_set_int(field_loop_start_ms, loop_begin_ms );
			if (loop_end_ms != ~0) p_info.info_set_int(field_loop_end_ms, loop_end_ms );

			pfc::string8 hash_string;

			for ( unsigned i = 0; i < 16; i++ ) hash_string += pfc::format_uint( (t_uint8)m_file_hash.m_data[i], 2, 16 );

			p_info.info_set( field_hash, hash_string );

			service_ptr_t<metadb_index_client> index_client = new service_impl_t<metadb_index_client_midi>;
			m_index_hash = index_client->transform( p_info, playable_location_impl( m_path, p_subsong ) );

			pfc::array_t<t_uint8> tag;
			static_api_ptr_t<metadb_index_manager>()->get_user_data_t( guid_midi_index, m_index_hash, tag );

			if ( tag.get_count() )
			{
				file::ptr tag_file;
				filesystem::g_open_tempmem( tag_file, p_abort );
				tag_file->write_object( tag.get_ptr(), tag.get_count(), p_abort );

				p_info.meta_remove_all();

				tag_processor::read_trailing( tag_file, p_info, p_abort );
				p_info.info_set( "tagtype", "apev2 db" );
			}

			const char * midi_preset = p_info.meta_get( field_preset, 0 );
			if ( midi_preset )
			{
				thePreset.unserialize( midi_preset );
			}

			const char * midi_syx = p_info.meta_get( field_syx, 0 );
			if ( midi_syx )
			{
				theDumps.unserialize( midi_syx, m_path );
			}
		}

		midi_file.set_track_count( original_track_count );
		theDumps.merge_into_file( midi_file, p_abort );

		plugin = thePreset.plugin;

		first_block = true;

		midi_meta_data meta_data;

		midi_file.get_meta_data( p_subsong, meta_data );

		midi_meta_data_item item;
		if ( meta_data.get_item( "type", item ) && !strcmp( item.m_value.c_str(), "MT-32" ) ) plugin = 3;

		pfc::string8 file_soundfont;

		/*if ( plugin == 2 || plugin == 4 )*/
		{
			bool exists = false;
			pfc::string8_fast temp = m_path, temp_out;

			if ( !(exists = test_soundfont_extension( temp, temp_out, p_abort ) ) )
			{
				size_t dot = temp.find_last( '.' );
				if ( dot > temp.scan_filename() )
				{
					temp.truncate( dot );
					exists = test_soundfont_extension( temp, temp_out, p_abort );
				}
				if ( !exists )
				{
					// Bah. The things needed to keep the last path separator.
					temp.truncate( temp.scan_filename() );
					temp_out = "";
					temp_out.add_byte( temp[ temp.length() - 1 ] );
					temp.truncate( temp.length() - 1 );
					size_t pos = temp.scan_filename();
					if ( pos != pfc::infinite_size )
					{
						temp += temp_out;
						temp.add_string( &temp[pos], temp.length() - pos - 1 );
						exists = test_soundfont_extension( temp, temp_out, p_abort );
					}
				}
			}

			if ( exists )
			{
				file_soundfont = temp_out;
				plugin = 4;
			}
		}

		if ( plugin == 3 ) srate = MT32Player::getSampleRate();

		get_length(p_subsong);

		samples_played = 0;

		if ( p_flags & input_flag_no_looping || !loop_type )
		{
			unsigned samples_length = length_samples;
			samples_fade_begin = samples_length;
			samples_fade_end = samples_length;

			if ( loop_begin != ~0 || loop_end != ~0 || loop_type > 1 )
			{
				samples_fade_begin = MulDiv(loop_begin_ms + (loop_end_ms - loop_begin_ms) * loop_count, srate, 1000);
				samples_fade_end = samples_fade_begin + srate * fade_ms / 1000;
			}
		}
		else
		{
			samples_fade_begin = ~0;
			samples_fade_end = ~0;
		}

#ifdef DXISUPPORT
		if (plugin == 5)
		{
			pfc::array_t<t_uint8> serialized_midi_file;
			midi_file.serialize_as_standard_midi_file( serialized_midi_file );

			delete dxiProxy;
			dxiProxy = NULL;

			dxiProxy = new DXiProxy;
			if ( SUCCEEDED( dxiProxy->initialize() ) )
			{
				dxiProxy->setSampleRate( srate );
				if ( SUCCEEDED( dxiProxy->setSequence( serialized_midi_file.get_ptr(), serialized_midi_file.get_count() ) ) )
				{
					if ( SUCCEEDED( dxiProxy->setPlugin( thePreset.dxi_plugin ) ) )
					{
						dxiProxy->Stop();
						dxiProxy->Play(TRUE);

						eof = false;
						dont_loop = true;

						if (loop_type)
						{
							if (loop_type == 1)
							{
								if (loop_begin != -1 || loop_end != -1)
								{
									set_loop();
								}
							}
							else
							{
								set_loop();
							}
						}

						return;
					}
				}
			}
		}
		else
#endif
			if (plugin == 1)
			{
				delete midiPlayer;

				VSTiPlayer * vstPlayer = new VSTiPlayer;
				midiPlayer = vstPlayer;

				if (vstPlayer->LoadVST(thePreset.vst_path))
				{
					if ( thePreset.vst_config.size() )
						vstPlayer->setChunk( &thePreset.vst_config[0], thePreset.vst_config.size() );

					vstPlayer->setSampleRate(srate);

					unsigned loop_mode = 0;

					loop_mode = MIDIPlayer::loop_mode_enable;
					if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;

					if ( vstPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
					{
						eof = false;
						dont_loop = true;

						return;
					}
				}
			}
#ifdef FLUIDSYNTHSUPPORT
			else if (plugin == 2)
			{
				/*HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE );
				if ( !fsmod )
				{
					throw exception_io_data("Failed to load FluidSynth.dll");
				}
				FreeLibrary( fsmod );*/

				delete midiPlayer;

				SFPlayer * sfPlayer = new SFPlayer;
				midiPlayer = sfPlayer;

				sfPlayer->setSoundFont(thePreset.soundfont_path);
				if ( file_soundfont.length() ) sfPlayer->setFileSoundFont( file_soundfont );
				sfPlayer->setSampleRate(srate);
				sfPlayer->setInterpolationMethod(cfg_fluid_interp_method);

				unsigned loop_mode = 0;

				if ( loop_type )
				{
					loop_mode = MIDIPlayer::loop_mode_enable;
					if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;
				}

				if ( sfPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else if (plugin == 4)
#else
			else if (plugin == 2 || plugin == 4)
#endif
			{
				/*HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE );
				if ( !fsmod )
				{
					throw exception_io_data("Failed to load FluidSynth.dll");
				}
				FreeLibrary( fsmod );*/

				delete midiPlayer;

				BMPlayer * bmPlayer = new BMPlayer;
				midiPlayer = bmPlayer;

				bmPlayer->setSoundFont(thePreset.soundfont_path);
				if ( file_soundfont.length() ) bmPlayer->setFileSoundFont( file_soundfont );
				bmPlayer->setSampleRate(srate);
				bmPlayer->setSincInterpolation(!!resampling);
				bmPlayer->setEffects(thePreset.effects);

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;

				if (bmPlayer->Load(midi_file, p_subsong, loop_mode, clean_flags))
				{
					eof = false;
					dont_loop = true;

					return;
				}
				else
				{
					std::string last_error;
					if (bmPlayer->GetLastError(last_error))
						throw exception_io_data(last_error.c_str());
				}
			}
			else if ( plugin == 6 )
			{
				delete midiPlayer;

				ADLPlayer * adlPlayer = new ADLPlayer;
				midiPlayer = adlPlayer;

				adlPlayer->setBank( thePreset.adl_bank );
				adlPlayer->setChipCount( thePreset.adl_chips );
				adlPlayer->setFullPanning( thePreset.adl_panning );
				adlPlayer->set4OpCount( thePreset.adl_chips * 4 /*cfg_adl_4op*/ );
				adlPlayer->setSampleRate(srate);

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;

				if ( adlPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else if ( plugin == 7 )
			{
				delete midiPlayer;

				fmmidiPlayer * fmPlayer = new fmmidiPlayer;
				midiPlayer = fmPlayer;

				pfc::string8 path;
				path = core_api::get_my_full_path();
				path.truncate( path.scan_filename() );
				fmPlayer->setProgramPath( path );

				fmPlayer->setSampleRate(srate);

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;

				if ( fmPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else if (plugin == 3)
			{
				midi_meta_data_item item;
				bool is_mt32 = ( meta_data.get_item( "type", item ) && !strcmp( item.m_value.c_str(), "MT-32" ) );

				delete midiPlayer;

				MT32Player * mt32Player = new MT32Player( !is_mt32, thePreset.munt_gm_set );
				midiPlayer = mt32Player;

				pfc::string8 p_base_path = cfg_munt_base_path;
				if ( !strlen( p_base_path ) )
				{
					p_base_path = core_api::get_my_full_path();
					p_base_path.truncate( p_base_path.scan_filename() );
				}
				mt32Player->setBasePath( p_base_path );
				mt32Player->setAbortCallback( &p_abort );
				mt32Player->setSampleRate( srate );

				if (!mt32Player->isConfigValid())
				{
					console::print("The Munt driver needs to be configured with a valid MT-32 or CM32L ROM set.");
					throw exception_io_data();
				}

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;

				if ( mt32Player->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else if (plugin == 9)
			{
				delete midiPlayer;

				MSPlayer * msPlayer = new MSPlayer();
				midiPlayer = msPlayer;

				msPlayer->set_synth(thePreset.ms_synth);
				msPlayer->set_bank(thePreset.ms_bank);
				msPlayer->set_extp(thePreset.ms_panning);

				msPlayer->setSampleRate(srate);

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if (loop_type > 1) loop_mode |= MIDIPlayer::loop_mode_force;

				if (msPlayer->Load(midi_file, p_subsong, loop_mode, clean_flags))
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else if (plugin == 10)
			{
				delete midiPlayer;

				SCPlayer * scPlayer = new SCPlayer();
				midiPlayer = scPlayer;

				pfc::string8 p_path;
				cfg_sc_path.get(p_path);
				if (!strlen(p_path))
				{
					p_path = core_api::get_my_full_path();
					p_path.truncate(p_path.scan_filename());
				}

				scPlayer->set_sccore_path(p_path);
				scPlayer->set_mode((SCPlayer::sc_mode)thePreset.sc_flavor);
				scPlayer->set_reverb(thePreset.sc_reverb);

				scPlayer->setSampleRate(srate);

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if (loop_type > 1) loop_mode |= MIDIPlayer::loop_mode_force;

				if (scPlayer->Load(midi_file, p_subsong, loop_mode, clean_flags))
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else
			{
				/* oh boy, yank in an external service! */
				/*
				input * i = get_external_decoder();
				if (i)
				{
				external_decoder = i;

				mem_reader = new reader_mem_temp((void*)mf->data, mf->size);

				file_info_i info;

				if (i->open(mem_reader, &info, flags))
				{
				eof = false;
				dont_loop = true;

				if (!(flags & OPEN_FLAG_NO_LOOPING) && loop_type)
				{
				if (loop_type == 1)
				{
				if (loop_begin != -1 || loop_end != -1)
				{
				set_loop();
				}
				}
				else
				{
				set_loop();
				}
				}

				samples_done = 0;

				return 1;
				}
				}
				*/
				delete midiPlayer;

				EMIDIPlayer * emidiPlayer = new EMIDIPlayer;
				midiPlayer = emidiPlayer;

				unsigned loop_mode = 0;

				loop_mode = MIDIPlayer::loop_mode_enable;
				if ( loop_type > 1 ) loop_mode |= MIDIPlayer::loop_mode_force;

				if ( emidiPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					{
						insync(sync);
						if (++g_running == 1) g_srate = srate;
						else if (srate != g_srate)
						{
							srate = g_srate;
						}
						is_emidi = true;
					}

					emidiPlayer->setSampleRate( srate );

					eof = false;
					dont_loop = true;

					return;
				}
			}

		throw exception_io_data();
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		p_abort.check();

		if (eof) return false;

		bool rv = false;

#ifdef DXISUPPORT
		if (plugin == 5)
		{
			unsigned todo = 1024;

			if (dont_loop)
			{
				if (length_samples && samples_done + todo > length_samples)
				{
					todo = length_samples - samples_done;
					if (!todo) return false;
				}
			}

#if audio_sample_size != 32
			sample_buffer.grow_size( todo * 2 );

			float * ptr = sample_buffer.get_ptr();

			thePlayer->FillBuffer(ptr, todo);

			p_chunk.set_data_32( ptr, todo, 2, srate );
#else
			p_chunk.set_data_size( todo * 2 );
			float * ptr = p_chunk.get_data();
			dxiProxy->fillBuffer( ptr, todo );
			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( todo );
#endif

			samples_done += todo;

			rv = true;
		}
		else
#endif
		if (plugin == 1)
		{
			VSTiPlayer * vstPlayer = (VSTiPlayer *) midiPlayer;

			unsigned todo = 1024;
			unsigned nch = vstPlayer->getChannelCount();

			p_chunk.set_data_size( todo * nch );

			audio_sample * out = p_chunk.get_data();

			unsigned done = vstPlayer->Play( out, todo );

			if ( ! done ) return false;

			p_chunk.set_srate( srate );
			p_chunk.set_channels( nch );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			rv = true;
		}
		else if (plugin == 3)
		{
			MT32Player * mt32Player = (MT32Player *) midiPlayer;

			unsigned todo = 1024;

			p_chunk.set_data_size( todo * 2 );

			audio_sample * out = p_chunk.get_data();

			mt32Player->setAbortCallback( &p_abort );

			unsigned done = mt32Player->Play( out, todo );

			if ( ! done ) return false;

			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			rv = true;
		}
		else if (midiPlayer)
		{
			unsigned todo = 1024;

			p_chunk.set_data_size( todo * 2 );

			audio_sample * out = p_chunk.get_data();

			unsigned done = midiPlayer->Play( out, todo );

			if ( ! done )
			{
				std::string last_error;
				if (midiPlayer->GetLastError(last_error))
					throw exception_io_data(last_error.c_str());
				return false;
			}

			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			rv = true;
		}

		if ( rv && samples_fade_begin != ~0 && samples_fade_end != ~0 )
		{
			unsigned samples_played_start = samples_played;
			unsigned samples_played_end = samples_played += p_chunk.get_sample_count();

			if ( samples_played_end >= samples_fade_begin )
			{
				for ( unsigned i = (samples_fade_begin > samples_played_start) ? samples_fade_begin : samples_played_start; i < ((samples_played > samples_fade_end) ? samples_fade_end : samples_played); i++ )
				{
					audio_sample * sample = p_chunk.get_data() + ( i - samples_played_start ) * 2;
					audio_sample scale = (audio_sample)( samples_fade_end - i ) / (audio_sample)( samples_fade_end - samples_fade_begin );
					sample[ 0 ] *= scale;
					sample[ 1 ] *= scale;
				}

				if ( samples_played_end > samples_fade_end )
				{
					p_chunk.set_sample_count( samples_fade_end - samples_played_start );
					eof = true;
				}
			}
		}

		return rv;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		unsigned seek_msec = unsigned( audio_math::time_to_samples( p_seconds, 1000 ) );

		// This value should not be wrapped to within the loop range
		samples_played = unsigned( ( t_int64( seek_msec ) * t_int64( srate ) ) / 1000 );

		if ( seek_msec > loop_end_ms )
		{
			seek_msec = ( seek_msec - loop_begin_ms ) % ( loop_end_ms - loop_begin_ms ) + loop_begin_ms;
		}

		first_block = true;
		eof = false;

		unsigned done = unsigned( ( t_int64( seek_msec ) * t_int64( srate ) ) / 1000 );
		if ( length_samples && done >= ( length_samples - srate ) )
		{
			eof = true;
			return;
		}

#ifdef DXISUPPORT
		if ( plugin == 5 )
		{
			dxiProxy->setPosition( seek_msec );

			samples_done = done;

			return;
		}
		else
#endif
		if ( midiPlayer )
		{
			midiPlayer->Seek( done );
			return;
		}
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		if ( first_block )
		{
			p_out.info_set_int( "samplerate", srate );
			p_timestamp_delta = 0.;
			first_block = false;
			return true;
		}

		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		if ( is_syx )
		{
			throw exception_io_data( "You cannot tag SysEx dumps" );
		}

		file_info_impl m_info( p_info );

		m_info.meta_remove_field( field_preset );
		const char * midi_preset = m_info.info_get( field_preset );
		if ( midi_preset )
		{
			m_info.meta_set( field_preset, midi_preset );
		}

		m_info.meta_remove_field( field_syx );
		const char * midi_syx = m_info.info_get( field_syx );
		if ( midi_syx )
		{
			m_info.meta_set( field_syx, midi_syx );
		}

		file::ptr tag_file;
		filesystem::g_open_tempmem( tag_file, p_abort );
		tag_processor::write_apev2( tag_file, m_info, p_abort );

		pfc::array_t<t_uint8> tag;
		tag_file->seek( 0, p_abort );
		tag.set_count( tag_file->get_size_ex( p_abort ) );
		tag_file->read_object( tag.get_ptr(), tag.get_count(), p_abort );

		static_api_ptr_t<metadb_index_manager>()->set_user_data( guid_midi_index, m_index_hash, tag.get_ptr(), tag.get_count() );
	}

	void retag_commit( abort_callback & p_abort )
	{
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return ! strcmp( p_content_type, "audio/midi" );
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return g_test_extension( p_extension ) || g_test_extension_syx( p_extension );
	}

	static GUID g_get_guid()
	{
		// {AE29C554-EE59-4C1A-8211-320F2A1A992B}
		static const GUID guid = { 0xae29c554, 0xee59, 0x4c1a,{ 0x82, 0x11, 0x32, 0xf, 0x2a, 0x1a, 0x99, 0x2b } };
		return guid;
	}

	static const char * g_get_name()
	{
		return "MIDI Player";
	}

	static GUID g_get_preferences_guid()
	{
		// {1623AA03-BADC-4bab-8A17-C737CF782661}
		static const GUID guid = { 0x1623aa03, 0xbadc, 0x4bab,{ 0x8a, 0x17, 0xc7, 0x37, 0xcf, 0x78, 0x26, 0x61 } };
		return guid;
	}

	static bool g_is_low_merit()
	{
		return false;
	}
};

static const char * loop_txt[] = {"Never", "When loop info detected", "Always"};

#ifdef FLUIDSYNTHSUPPORT
static const char * interp_txt[] = {"None", "Linear", "Cubic", "7th Order Sinc"};
static int interp_method[] = {FLUID_INTERP_NONE, FLUID_INTERP_LINEAR, FLUID_INTERP_4THORDER, FLUID_INTERP_7THORDER};
enum { interp_method_default = 2 };
#endif

static const char * click_to_set = "Click to set.";

static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate,16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,49716,64000,88200,96000};

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback), busy(false) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	enum {ID_REFRESH = 1000};

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_PLUGIN, CBN_SELCHANGE, OnPluginChange)
		COMMAND_HANDLER_EX(IDC_SOUNDFONT, EN_SETFOCUS, OnSetFocus)
		COMMAND_HANDLER_EX(IDC_MUNT, EN_SETFOCUS, OnSetFocus)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)
		DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, cfg_history_rate)
		COMMAND_HANDLER_EX(IDC_LOOP, CBN_SELCHANGE, OnSelectionChange)
#ifdef FLUIDSYNTHSUPPORT
		COMMAND_HANDLER_EX(IDC_FLUID_INTERPOLATION, CBN_SELCHANGE, OnSelectionChange)
#endif
		COMMAND_HANDLER_EX(IDC_RESAMPLING, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_RPGMLOOPZ, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_XMILOOPZ, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FF7LOOPZ, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_EMIDI_EX, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_PLUGIN_CONFIGURE, BN_CLICKED, OnButtonConfig)
		COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_ADL_PANNING, BN_CLICKED, OnButtonClick)
		//COMMAND_HANDLER_EX(IDC_RECOVER, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_MUNT_GM, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_MS_PRESET, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_SC_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_MS_PANNING, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_SC_EFFECTS, BN_CLICKED, OnButtonClick)
		MSG_WM_TIMER(OnTimer)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnSelectionChange(UINT, int, CWindow);
	void OnPluginChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
	void OnButtonConfig(UINT, int, CWindow);
	void OnSetFocus(UINT, int, CWindow);
	void OnTimer(UINT_PTR nIDEvent);
	bool HasChanged();
	void OnChanged();

	void enum_vsti_plugins( const char * _path = 0, puFindFile _find = 0 );

	bool check_secret_sauce();

	const preferences_page_callback::ptr m_callback;

	bool busy, secret_sauce_found;

#ifdef DXISUPPORT
	pfc::array_t< CLSID > dxi_plugins;
#endif

	struct vsti_info
	{
		std::string path, display_name;
		uint32_t unique_id;
		bool has_editor;
	};

	pfc::array_t< vsti_info > vsti_plugins;

	std::vector< uint8_t > vsti_config;

	pfc::string8 m_soundfont, m_munt_path;

	pfc::string8_fast m_cached, m_cached_current;

	struct adl_bank
	{
		int number;
		const char * name;

		adl_bank() : number( -1 ), name( "" ) { }
		adl_bank( const adl_bank& b ) : number( b.number ), name( b.name ) { }
		adl_bank( int _number, const char * _name ) : number( _number ), name( _name ) { }

		adl_bank & operator= (const adl_bank& b)
		{
			number = b.number;
			name = b.name;
			return *this;
		}

		bool operator==(const adl_bank& b) const
		{
			return number == b.number;
		}
		bool operator< (const adl_bank& b) const
		{
			int c = stricmp_utf8( name, b.name );
			if ( c ) return c < 0;
			return 0;
		}
		bool operator> (const adl_bank& b) const
		{
			int c = stricmp_utf8( name, b.name );
			if ( c ) return c > 0;
			return 0;
		}
		bool operator!=(const adl_bank& b) const { return !operator==(b); }
	};

	pfc::list_t<adl_bank> m_bank_list;
};

void CMyPreferences::enum_vsti_plugins( const char * _path, puFindFile _find )
{
	pfc::string8 ppath;
	if ( ! _find )
	{
		vsti_plugins.set_size( 0 );

		cfg_vsti_search_path.get(ppath);
		if (ppath.is_empty())
		{
			GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_SHOW );
			GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_HIDE );
			return;
		}

		console::formatter() << "Enumerating VSTi Plug-ins...";

		if (ppath[ppath.length()-1] != '\\') ppath.add_byte( '\\' );
		ppath += "*.*";
		_path = ppath;
		_find = uFindFirstFile( ppath );
	}
	if ( _find )
	{
		do
		{
			if ( _find->IsDirectory() && strcmp( _find->GetFileName(), "." ) && strcmp( _find->GetFileName(), ".." ) )
			{
				pfc::string8 npath( _path );
				npath.truncate( npath.length() - 3 );
				npath += _find->GetFileName();
				npath.add_byte( '\\' );
				npath += "*.*";
				puFindFile pfind = uFindFirstFile( npath );
				if ( pfind ) enum_vsti_plugins( npath, pfind );
			}
			else if ( _find->GetFileSize() )
			{
				pfc::string8 npath( _path );
				npath.truncate( npath.length() - 3 );
				npath += _find->GetFileName();
				if ( npath.length() > 4 && !pfc::stricmp_ascii( npath.get_ptr() + npath.length() - 4, ".dll" ) )
				{
					console::formatter() << "Trying " << npath;

					VSTiPlayer vstPlayer;
					if ( vstPlayer.LoadVST( npath ) )
					{
						vsti_info info;
						info.path = npath;

						std::string vendor, product;
						vstPlayer.getVendorString(vendor);
						vstPlayer.getProductString(product);

						if (product.length() || vendor.length())
						{
							if (!vendor.length() ||
								(product.length() >= vendor.length() &&
								!strncmp(vendor.c_str(), product.c_str(), vendor.length())))
							{
								info.display_name = product;
							}
							else
							{
								info.display_name = vendor;
								if (product.length())
								{
									info.display_name += ' ';
									info.display_name += product;
								}
							}
						}
						else info.display_name = _find->GetFileName();

						info.unique_id = vstPlayer.getUniqueID();

						info.has_editor = vstPlayer.hasEditor();

						vsti_plugins.append_single( info );
					}
				}
			}
		} while ( _find->FindNext() );
		delete _find;
	}
}

typedef struct sc_info
{
	size_t size;
	hasher_md5_result hash;
} sc_info;

static const sc_info sc_hashes[] = {

	// 1.0.3 - 32 bit
	{ 27472384, { 0xd4, 0x4d, 0x1b, 0x8c, 0x9a, 0x6f, 0x95, 0x6c, 0xa2, 0x32, 0x4f, 0x2f, 0x5d, 0x34, 0x8c, 0x44 } },

	// 1.0.7 - 32 bit
	{ 27319296, { 0x25, 0x83, 0x0a, 0x6c, 0x2f, 0xf5, 0x75, 0x1f, 0x3a, 0x55, 0x91, 0x5f, 0xb6, 0x07, 0x02, 0xf4 } },

	{ 0, { 0 } }
};

bool CMyPreferences::check_secret_sauce()
{
	size_t real_size;
	pfc::string8 path;
	cfg_sc_path.get(path);

	if (path.is_empty()) return false;

	path += "\\";
	path += g_sc_name;

	pfc::stringcvt::string_os_from_utf8 pathnative(path);

	FILE * f = _tfopen(pathnative, _T("rb"));
	if (!f) return false;

	fseek(f, 0, SEEK_END);
	real_size = ftell(f);

	bool found = false;
	for (int i = 0; sc_hashes[i].size; ++i)
	{
		if (sc_hashes[i].size == real_size)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		fclose(f);
		return false;
	}
	fseek(f, 0, SEEK_SET);

	unsigned char buffer[1024];
	static_api_ptr_t<hasher_md5> m_hasher;
	hasher_md5_state m_hasher_state;
	hasher_md5_result m_hasher_result;
	size_t bytes_total = 0;

	m_hasher->initialize(m_hasher_state);

	while (!feof(f))
	{
		size_t bytes_read = fread(buffer, 1, 1024, f);
		bytes_total += bytes_read;
		if (bytes_read)
		{
			m_hasher->process(m_hasher_state, buffer, bytes_read);
		}
		if (bytes_read < 1024) break;
	}

	fclose(f);

	if (bytes_total != real_size) return false;

	m_hasher_result = m_hasher->get_result(m_hasher_state);

	for (int i = 0; sc_hashes[i].size; ++i)
	{
		if (real_size == sc_hashes[i].size && m_hasher_result == sc_hashes[i].hash) return true;
	}

	return false;
}

static const char * chip_counts[] = {"1", "2", "5", "10", "25", "50", "100"};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	CWindow w;
	int plugin = cfg_plugin;

	secret_sauce_found = check_secret_sauce();

	if (!secret_sauce_found && plugin == 10)
		plugin = default_cfg_plugin;

	if (plugin == 8)
		plugin = default_cfg_plugin;
	
	w = GetDlgItem( IDC_PLUGIN );
	uSendMessageText( w, CB_ADDSTRING, 0, "Emu de MIDI" );
#ifdef FLUIDSYNTHSUPPORT
	uSendMessageText( w, CB_ADDSTRING, 0, "FluidSynth" );
#endif
	uSendMessageText( w, CB_ADDSTRING, 0, "BASSMIDI" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Super MUNT GM" );
	uSendMessageText( w, CB_ADDSTRING, 0, "adlmidi" );
	uSendMessageText( w, CB_ADDSTRING, 0, "fmmidi" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Nuclear Option");

	if (secret_sauce_found) uSendMessageText(w, CB_ADDSTRING, 0, "Secret Sauce");

#ifndef FLUIDSYNTHSUPPORT
	if ( plugin == 2 ) plugin = 4;
#endif
	
	/*
	if (plugin == 1)
	{
	if (!set_vsti(wnd, false)) plugin = 0;
	}
	else uSendMessageText(w, CB_ADDSTRING, 0, "VST instrument");
	*/
	
	enum_vsti_plugins();

	if ( vsti_plugins.get_size() ) console::formatter() << "Found " << vsti_plugins.get_size() << " plug-ins";
	
	unsigned vsti_count = vsti_plugins.get_size(), vsti_selected = ~0;
	
	for ( unsigned i = 0; i < vsti_count; ++i )
	{
		uSendMessageText( w, CB_ADDSTRING, 0, vsti_plugins[ i ].display_name.c_str() );
		if ( plugin == 1 && ! stricmp_utf8( vsti_plugins[ i ].path.c_str(), cfg_vst_path ) )
			vsti_selected = i;
	}

	if ( plugin == 1 && vsti_selected == ~0 ) plugin = 0;

	/*{
	HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE );
	if (fsmod)
	{
	FreeLibrary( fsmod );
	uSendMessageText(w, CB_ADDSTRING, 0, "FluidSynth");
	}
	}*/
	
	if ( plugin != 2 && plugin != 4 )
	{
		GetDlgItem( IDC_SOUNDFONT_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_SOUNDFONT ).EnableWindow( FALSE );
		GetDlgItem( IDC_RESAMPLING_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_RESAMPLING ).EnableWindow( FALSE );
		GetDlgItem( IDC_CACHED_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_CACHED ).EnableWindow( FALSE );
	}

	if ( plugin == 3 )
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_SHOW );
	}

#ifdef FLUIDSYNTHSUPPORT
	if ( plugin != 2 )
	{
		GetDlgItem( IDC_FLUID_INTERPOLATION_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_FLUID_INTERPOLATION ).EnableWindow( FALSE );
	}
#endif
	if ( plugin != 6 )
	{
		GetDlgItem( IDC_ADL_BANK_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_BANK ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_CHIPS_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_CHIPS ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_PANNING ).EnableWindow( FALSE );
	}

	if ( plugin != 1 )
	{
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).EnableWindow( FALSE );
	}
	else
	{
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).EnableWindow( vsti_plugins[ vsti_selected ].has_editor );
		vsti_config = cfg_vst_config[ vsti_plugins[ vsti_selected ].unique_id ];
	}

	if (plugin != 9)
	{
		GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
		GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
		GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
	}

	if (!secret_sauce_found)
	{
		GetDlgItem(IDC_SC_GROUP).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SC_FLAVOR_TEXT).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SC_FLAVOR).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SC_EFFECTS).ShowWindow(SW_HIDE);
	}

	if (plugin != 10)
	{
		GetDlgItem(IDC_SC_FLAVOR_TEXT).EnableWindow(FALSE);
		GetDlgItem(IDC_SC_FLAVOR).EnableWindow(FALSE);
		GetDlgItem(IDC_SC_EFFECTS).EnableWindow(FALSE);
	}

	{
		m_soundfont = cfg_soundfont_path;
		const char * filename;
		if ( m_soundfont.is_empty() ) filename = click_to_set;
		else filename = m_soundfont.get_ptr() + m_soundfont.scan_filename();
		uSetDlgItemText( m_hWnd, IDC_SOUNDFONT, filename );
	}

	{
		m_munt_path = cfg_munt_base_path;
		const char * path;
		if ( m_munt_path.is_empty() ) path = click_to_set;
		else path = m_munt_path;
		uSetDlgItemText( m_hWnd, IDC_MUNT, path );
	}

#ifdef DXISUPPORT
	unsigned dxi_selected = ~0;

	dxi_plugins.set_count( 0 );

	CoInitialize(NULL);
	{
		CPlugInInventory theInventory;
		if (SUCCEEDED(theInventory.EnumPlugIns()))
		{
			unsigned count = theInventory.GetCount();
			pfc::string8_fastalloc name;
			CLSID theClsid;
			for (unsigned i = 0; i < count; i++)
			{
				if (SUCCEEDED(theInventory.GetInfo(i, &theClsid, name)))
				{
					dxi_plugins.append_single( theClsid );
					uSendMessageText(w, CB_ADDSTRING, 0, name);

					if ( theClsid == cfg_dxi_plugin.get_value() ) dxi_selected = i;
				}
			}
		}
	}
	CoUninitialize();
#endif
	if ( plugin == 1 ) plugin += vsti_selected + 6 + (secret_sauce_found ? 1 : 0);
	else if ( plugin >= 2 && plugin <= 4 )
	{
		plugin = plugin == 2 ? 1 : plugin == 4 ? 2 : 3;
	}
	else if ( plugin == 6 )
	{
		plugin = 4;
	}
	else if ( plugin == 7 )
	{
		plugin = 5;
	}
	else if ( plugin == 9 )
	{
		plugin = 6;
	}
	else if ( plugin == 10 )
	{
		plugin = 7;
	}
#ifdef DXISUPPORT
	else if ( plugin == 5 )
	{
		if ( dxi_selected != ~0 ) plugin += vsti_count + dxi_selected - 1;
		else plugin = 0;
	}
#endif
#ifndef FLUIDSYNTHSUPPORT
	if ( plugin > 1 ) --plugin;
#endif

	::SendMessage( w, CB_SETCURSEL, plugin, 0 );
	
	{
		char temp[16];
		int n;
		for(n=_countof(srate_tab);n--;)
		{
			if (srate_tab[n] != cfg_srate)
			{
				itoa(srate_tab[n], temp, 10);
				cfg_history_rate.add_item(temp);
			}
		}
		itoa(cfg_srate, temp, 10);
		cfg_history_rate.add_item(temp);
		w = GetDlgItem( IDC_SAMPLERATE );
		cfg_history_rate.setup_dropdown( w );
		::SendMessage( w, CB_SETCURSEL, 0, 0 );
	}

	if ( !plugin )
	{
		if ( g_running ) GetDlgItem( IDC_SAMPLERATE ).EnableWindow( FALSE );
	}
	
	//if ( plugin <= vsti_count ) GetDlgItem( IDC_GM2 ).EnableWindow( FALSE );
	
	w = GetDlgItem( IDC_LOOP );
	for (unsigned i = 0; i < _countof(loop_txt); i++)
	{
		uSendMessageText( w, CB_ADDSTRING, 0, loop_txt[i] );
	}
	::SendMessage( w, CB_SETCURSEL, cfg_loop_type, 0 );

	SendDlgItemMessage( IDC_RPGMLOOPZ, BM_SETCHECK, cfg_rpgmloopz );
	SendDlgItemMessage( IDC_XMILOOPZ, BM_SETCHECK, cfg_xmiloopz );
	SendDlgItemMessage( IDC_FF7LOOPZ, BM_SETCHECK, cfg_ff7loopz );

	SendDlgItemMessage( IDC_EMIDI_EX, BM_SETCHECK, cfg_emidi_exclusion );
	SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_SETCHECK, cfg_filter_instruments );
	SendDlgItemMessage( IDC_FILTER_BANKS, BM_SETCHECK, cfg_filter_banks );
	//SendDlgItemMessage( IDC_GM2, BM_SETCHECK, cfg_gm2 );
	//SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, cfg_recover_tracks );
	//SendDlgItemMessage( IDC_NOSYSEX, BM_SETCHECK, cfg_nosysex );
	//SendDlgItemMessage( IDC_HACK_XG_DRUMS, BM_SETCHECK, cfg_hack_xg_drums );

	SendDlgItemMessage( IDC_MS_PANNING, BM_SETCHECK, cfg_ms_panning );
	if (secret_sauce_found)
		SendDlgItemMessage( IDC_SC_EFFECTS, BM_SETCHECK, !cfg_sc_reverb );

	const char * const * banknames = adl_getBankNames();
	unsigned bank_count = adl_getBanksCount();

	for ( unsigned i = 0; i < bank_count; i++ )
	{
		m_bank_list += adl_bank( i, banknames[ i ] );
	}
	m_bank_list.sort();

	unsigned bank_selected = 0;
	w = GetDlgItem( IDC_ADL_BANK );
	for ( unsigned i = 0; i < m_bank_list.get_count(); i++ )
	{
		uSendMessageText( w, CB_ADDSTRING, 0, m_bank_list[ i ].name );
		if ( m_bank_list[ i ].number == cfg_adl_bank ) bank_selected = i;
	}
	w.SendMessage( CB_SETCURSEL, bank_selected );

	w = GetDlgItem( IDC_ADL_CHIPS );
	for ( unsigned i = 0; i < _countof( chip_counts ); i++ )
	{
		uSendMessageText( w, CB_ADDSTRING, 0, chip_counts[ i ] );
	}
	SetDlgItemInt( IDC_ADL_CHIPS, cfg_adl_chips, 0 );

	SendDlgItemMessage( IDC_ADL_PANNING, BM_SETCHECK, cfg_adl_panning );

#ifdef FLUIDSYNTHSUPPORT
	w = GetDlgItem( IDC_FLUID_INTERPOLATION );
	for (unsigned i = 0; i < _countof(interp_txt); i++)
	{
		uSendMessageText( w, CB_ADDSTRING, 0, interp_txt[i] );
	}

	for (unsigned i = 0; i < _countof(interp_method); i++)
	{
		if ( cfg_fluid_interp_method == interp_method[i] )
		{
			::SendMessage( w, CB_SETCURSEL, i, 0 );
			break;
		}
	}
#endif

	w = GetDlgItem( IDC_RESAMPLING );
	uSendMessageText( w, CB_ADDSTRING, 0, "Linear interpolation" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Sinc interpolation" );
	::SendMessage( w, CB_SETCURSEL, cfg_resampling, 0 );

	w = GetDlgItem( IDC_MUNT_GM );
	for ( unsigned i = 0, j = _countof( munt_bank_names ); i < j; i++ )
	{
		uSendMessageText( w, CB_ADDSTRING, 0, munt_bank_names[ i ] );
	}
	::SendMessage( w, CB_SETCURSEL, cfg_munt_gm, 0 );

	size_t preset_number = 0;
	w = GetDlgItem(IDC_MS_PRESET);
	for ( size_t i = 0, j = g_ms_presets.get_count(); i < j; i++ )
	{
		const ms_preset & preset = g_ms_presets[i];
		uSendMessageText( w, CB_ADDSTRING, 0, preset.name );
		if (preset.synth == cfg_ms_synth && preset.bank == cfg_ms_bank)
			preset_number = i;
	}
	::SendMessage(w, CB_SETCURSEL, preset_number, 0);

	if ( secret_sauce_found )
	{
		w = GetDlgItem( IDC_SC_FLAVOR );
		uSendMessageText( w, CB_ADDSTRING, 0, "Default" );
		uSendMessageText( w, CB_ADDSTRING, 0, "G" );
		uSendMessageText( w, CB_ADDSTRING, 0, "G2" );
		uSendMessageText( w, CB_ADDSTRING, 0, "55");
		uSendMessageText( w, CB_ADDSTRING, 0, "88" );
		uSendMessageText( w, CB_ADDSTRING, 0, "88P" );
		uSendMessageText( w, CB_ADDSTRING, 0, "885");
		uSendMessageText( w, CB_ADDSTRING, 0, "X");
		::SendMessage( w, CB_SETCURSEL, cfg_sc_flavor, 0 );
	}

	SetTimer( ID_REFRESH, 20 );

	busy = false;

	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnSelectionChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnButtonConfig(UINT, int, CWindow) {
	int plugin = GetDlgItem( IDC_PLUGIN ).SendMessage( CB_GETCURSEL, 0, 0 );
	int base = secret_sauce_found ? 8 : 7;
#ifndef FLUIDSYNTHSUPPORT
	if ( plugin > 0 ) ++plugin;
#endif
	if ( plugin >= base && plugin < base + vsti_plugins.get_count() )
	{
		busy = true;
		OnChanged();

		VSTiPlayer vstPlayer;
		if ( vstPlayer.LoadVST( vsti_plugins[ plugin - base ].path.c_str() ) )
		{
			if ( vsti_config.size() )
				vstPlayer.setChunk( &vsti_config[0], vsti_config.size() );
			vstPlayer.displayEditorModal();
			vstPlayer.getChunk( vsti_config );
		}

		busy = false;
		OnChanged();
	}
}

void CMyPreferences::OnPluginChange(UINT, int, CWindow w) {
	//t_size vsti_count = vsti_plugins.get_size();
	int plugin = ::SendMessage( w, CB_GETCURSEL, 0, 0 );
#ifndef FLUIDSYNTHSUPPORT
	if ( plugin > 0 ) ++plugin;
#endif
	
	GetDlgItem( IDC_SAMPLERATE ).EnableWindow( plugin || !g_running );
	
	GetDlgItem( IDC_SOUNDFONT_TEXT ).EnableWindow( plugin == 1 || plugin == 2 );
	GetDlgItem( IDC_SOUNDFONT ).EnableWindow( plugin == 1 || plugin == 2 );
	GetDlgItem( IDC_RESAMPLING_TEXT ).EnableWindow( plugin == 1 || plugin == 2 );
	GetDlgItem( IDC_RESAMPLING ).EnableWindow( plugin == 1 || plugin == 2 );
	GetDlgItem( IDC_CACHED_TEXT ).EnableWindow( plugin == 1 || plugin == 2 );
	GetDlgItem( IDC_CACHED ).EnableWindow( plugin == 1 || plugin == 2 );
	GetDlgItem( IDC_ADL_BANK_TEXT ).EnableWindow( plugin == 4 );
	GetDlgItem( IDC_ADL_BANK ).EnableWindow( plugin == 4 );
	GetDlgItem( IDC_ADL_CHIPS_TEXT ).EnableWindow( plugin == 4 );
	GetDlgItem( IDC_ADL_CHIPS ).EnableWindow( plugin == 4 );
	GetDlgItem( IDC_ADL_PANNING ).EnableWindow( plugin == 4 );
	GetDlgItem( IDC_MS_PRESET_TEXT ).EnableWindow( plugin == 6 );
	GetDlgItem( IDC_MS_PRESET ).EnableWindow( plugin == 6 );
	GetDlgItem( IDC_MS_PANNING ).EnableWindow( plugin == 6 );
	if (secret_sauce_found)
	{
		GetDlgItem( IDC_SC_FLAVOR_TEXT ).EnableWindow( plugin == 7 );
		GetDlgItem( IDC_SC_FLAVOR ).EnableWindow( plugin == 7 );
		GetDlgItem( IDC_SC_EFFECTS ).EnableWindow( plugin == 7 );
	}

#ifdef FLUIDSYNTHSUPPORT
	GetDlgItem( IDC_FLUID_INTERPOLATION_TEXT ).EnableWindow( plugin == 1 );
	GetDlgItem( IDC_FLUID_INTERPOLATION ).EnableWindow( plugin == 1 );
#endif

	if ( plugin == 3 )
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_SHOW );
	}
	else if ( vsti_plugins.get_count() == 0 )
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_SHOW );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_HIDE );
	}
	else
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_SHOW );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_HIDE );
	}

	//GetDlgItem( IDC_GM2 ).EnableWindow( plugin > vsti_count + 1 );

	int base = secret_sauce_found ? 8 : 7;

	GetDlgItem( IDC_PLUGIN_CONFIGURE ).EnableWindow( plugin >= base && plugin < base + vsti_plugins.get_count() && vsti_plugins[ plugin - base ].has_editor );

	if ( plugin >= base && plugin < base + vsti_plugins.get_count() )
	{
		vsti_config = cfg_vst_config[ vsti_plugins[ plugin - base ].unique_id ];
	}

	OnChanged();
}

void CMyPreferences::OnSetFocus(UINT, int, CWindow w) {
	SetFocus();

	if ( w == GetDlgItem( IDC_SOUNDFONT ) )
	{
		pfc::string8 directory, filename;
		directory = m_soundfont;
		filename = m_soundfont;
		directory.truncate( directory.scan_filename() );
		if (uGetOpenFileName(m_hWnd, "SoundFont and list files|*.sf2;"
#ifdef SF2PACK
			"*.sf2pack;"
#endif
			"*.sflist|SoundFont files|*.sf2"
#ifdef SF2PACK
			";*.sf2pack"
#endif
			"|SoundFont list files|*.sflist;*.json", 0, "sf2", "Choose a SoundFont bank or list...", directory, filename, FALSE))
		{
			m_soundfont = filename;
			uSetWindowText( w, filename.get_ptr() + filename.scan_filename() );
			OnChanged();
		}
	}
	else if ( w == GetDlgItem( IDC_MUNT ) )
	{
		pfc::string8 path;
		if ( uBrowseForFolder( m_hWnd, "Locate MT-32 or CM-32L ROM set...", path ) )
		{
			m_munt_path = path;
			unsigned length = m_munt_path.length();
			if ( length >= 1 && !pfc::is_path_separator( *( m_munt_path.get_ptr() + length - 1 ) ) ) m_munt_path.add_byte( '\\' );
			const char * display_path;
			if ( length ) display_path = m_munt_path;
			else display_path = click_to_set;
			uSetWindowText( w, display_path );
			OnChanged();
		}
	}
}

void CMyPreferences::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent == ID_REFRESH )
	{
		GetDlgItem( IDC_SAMPLERATE ).EnableWindow( cfg_plugin || !g_running );

		m_cached.reset();

		uint64_t total_sample_size, samples_loaded_size;

		if ( g_get_soundfont_stats( total_sample_size, samples_loaded_size ) )
		{
			m_cached = pfc::format_file_size_short( samples_loaded_size );
			m_cached += " / ";
			m_cached += pfc::format_file_size_short( total_sample_size );
		}
		else
		{
			m_cached = "BASS not loaded.";
		}

		if ( strcmp( m_cached, m_cached_current ) )
		{
			m_cached_current = m_cached;
			uSetWindowText( GetDlgItem( IDC_CACHED ), m_cached );
		}
	}
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	if (busy) state |= preferences_state::busy;
	return state;
}

void CMyPreferences::reset() {
	SendDlgItemMessage( IDC_PLUGIN, CB_SETCURSEL, 7 /* Nuclear Option */ );
	if ( default_cfg_plugin != 2 && default_cfg_plugin != 4 )
	{
		GetDlgItem( IDC_SOUNDFONT_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_SOUNDFONT ).EnableWindow( FALSE );
		GetDlgItem( IDC_RESAMPLING_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_RESAMPLING ).EnableWindow( FALSE );
		GetDlgItem( IDC_CACHED_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_CACHED ).EnableWindow( FALSE );
	}
	if ( default_cfg_plugin != 6 )
	{
		GetDlgItem( IDC_ADL_BANK_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_BANK ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_CHIPS_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_CHIPS ).EnableWindow( FALSE );
		GetDlgItem( IDC_ADL_PANNING ).EnableWindow( FALSE );
	}
#ifdef FLUIDSYNTHSUPPORT
	if ( default_cfg_plugin != 2 )
	{
		GetDlgItem( IDC_FLUID_INTERPOLATION_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_FLUID_INTERPOLATION ).EnableWindow( FALSE );
	}
#endif
	if ( default_cfg_plugin == 3 )
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_SHOW );
	}
	else if ( vsti_plugins.get_count() == 0 )
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_SHOW );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_HIDE );
	}
	else
	{
		GetDlgItem( IDC_VST_WARNING ).ShowWindow( SW_HIDE );
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).ShowWindow( SW_SHOW );
		GetDlgItem( IDC_MUNT_WARNING ).ShowWindow( SW_HIDE );
	}
	if ( default_cfg_plugin != 9 )
	{
		GetDlgItem( IDC_MS_PRESET_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_MS_PRESET ).EnableWindow( FALSE );
		GetDlgItem( IDC_MS_PANNING ).EnableWindow( FALSE );
	}
	if ( default_cfg_plugin != 10 )
	{
		GetDlgItem( IDC_SC_FLAVOR_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_SC_FLAVOR ).EnableWindow( FALSE );
		GetDlgItem( IDC_SC_EFFECTS ).EnableWindow( FALSE );
	}

	uSetDlgItemText( m_hWnd, IDC_SOUNDFONT, click_to_set );
	uSetDlgItemText( m_hWnd, IDC_MUNT, click_to_set );
	m_soundfont.reset();
	m_munt_path.reset();
	SetDlgItemInt( IDC_SAMPLERATE, default_cfg_srate, FALSE );
	if ( !default_cfg_plugin )
	{
		if ( g_running ) GetDlgItem( IDC_SAMPLERATE ).EnableWindow( FALSE );
	}
	SendDlgItemMessage( IDC_LOOP, CB_SETCURSEL, default_cfg_loop_type );
	SendDlgItemMessage( IDC_RPGMLOOPZ, BM_SETCHECK, default_cfg_rpgmloopz );
	SendDlgItemMessage( IDC_XMILOOPZ, BM_SETCHECK, default_cfg_xmiloopz );
	SendDlgItemMessage( IDC_FF7LOOPZ, BM_SETCHECK, default_cfg_ff7loopz );
	SendDlgItemMessage( IDC_EMIDI_EX, BM_SETCHECK, default_cfg_emidi_exclusion );
	SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_SETCHECK, default_cfg_filter_instruments );
	SendDlgItemMessage( IDC_FILTER_BANKS, BM_SETCHECK, default_cfg_filter_banks );
	SendDlgItemMessage( IDC_MS_PANNING, BM_SETCHECK, default_cfg_ms_panning );
	SendDlgItemMessage( IDC_SC_EFFECTS, BM_SETCHECK, !default_cfg_sc_reverb );
	unsigned bank_selected = 0;
	for ( unsigned i = 0; i < m_bank_list.get_count(); i++ )
	{
		if ( m_bank_list[ i ].number == default_cfg_adl_bank )
		{
			bank_selected = i;
			break;
		}
	}
	SendDlgItemMessage( IDC_ADL_BANK, CB_SETCURSEL, bank_selected );
	SendDlgItemMessage( IDC_ADL_PANNING, BM_SETCHECK, default_cfg_adl_panning );
	SetDlgItemInt( IDC_ADL_CHIPS, default_cfg_adl_chips, 0 );
	//SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, default_cfg_recover_tracks );
#ifdef FLUIDSYNTHSUPPORT
	SendDlgItemMessage( IDC_FLUID_INTERPOLATION, CB_SETCURSEL, interp_method_default );
#endif
	SendDlgItemMessage( IDC_RESAMPLING, CB_SETCURSEL, default_cfg_resampling );

	size_t preset_number = 0;
	for (size_t i = 0, j = g_ms_presets.get_count(); i < j; i++)
	{
		const ms_preset & preset = g_ms_presets[i];
		if (preset.synth == default_cfg_ms_synth && preset.bank == default_cfg_ms_bank)
		{
			preset_number = i;
			break;
		}
	}
	SendDlgItemMessage( IDC_MS_PRESET, CB_SETCURSEL, preset_number );

	if ( secret_sauce_found )
		SendDlgItemMessage( IDC_SC_FLAVOR, CB_SETCURSEL, cfg_sc_flavor );

	vsti_config.resize( 0 );

	OnChanged();
}

void CMyPreferences::apply() {
	char temp[16];
	int t = GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE );
	if ( t < 6000 ) t = 6000;
	else if ( t > 192000 ) t = 192000;
	SetDlgItemInt( IDC_SAMPLERATE, t, FALSE );
	itoa( t, temp, 10 );
	cfg_history_rate.add_item( temp );
	cfg_srate = t;
	t = SendDlgItemMessage( IDC_ADL_BANK, CB_GETCURSEL );
	if ( t < 0 || t >= m_bank_list.get_count() ) t = 0;
	cfg_adl_bank = m_bank_list[ t ].number;
	t = GetDlgItemInt( IDC_ADL_CHIPS, NULL, FALSE );
	if ( t < 1 ) t = 1;
	if ( t > 100 ) t = 100;
	SetDlgItemInt( IDC_ADL_CHIPS, t, FALSE );
	cfg_adl_chips = t;
	cfg_adl_panning = SendDlgItemMessage( IDC_ADL_PANNING, BM_GETCHECK );
	cfg_munt_gm = SendDlgItemMessage( IDC_MUNT_GM, CB_GETCURSEL );
	{
		unsigned int preset_number = SendDlgItemMessage( IDC_MS_PRESET, CB_GETCURSEL );
		if (preset_number >= g_ms_presets.get_count()) preset_number = 0;
		const ms_preset & preset = g_ms_presets[preset_number];
		cfg_ms_synth = preset.synth;
		cfg_ms_bank = preset.bank;
	}
	{
		t_size base = secret_sauce_found ? 8 : 7;
		t_size vsti_count = vsti_plugins.get_size();
		int plugin = SendDlgItemMessage( IDC_PLUGIN, CB_GETCURSEL );
#ifdef DXISUPPORT
		t_size dxi_count = dxi_plugins.get_count();
#endif
#ifndef FLUIDSYNTHSUPPORT
		if ( plugin > 0 ) ++plugin;
#endif		
		cfg_vst_path = "";
		
		if ( ! plugin )
		{
			cfg_plugin = 0;
		}
		else if ( plugin >= 1 && plugin <= 3 )
		{
			cfg_plugin = plugin == 1 ? 2 : plugin == 2 ? 4 : 3;
			//cfg_plugin = plugin - vsti_count + 1;
		}
		else if ( plugin == 4 )
		{
			cfg_plugin = 6;
		}
		else if ( plugin == 5 )
		{
			cfg_plugin = 7;
		}
		else if (plugin == 6)
		{
			cfg_plugin = 9;
		}
		else if (secret_sauce_found && plugin == 7)
		{
			cfg_plugin = 10;
		}
		else if ( plugin <= vsti_count + base - 1 )
		{
			cfg_plugin = 1;
			cfg_vst_path = vsti_plugins[ plugin - base ].path.c_str();
			cfg_vst_config[ vsti_plugins[ plugin - base ].unique_id ] = vsti_config;
		}
#ifdef DXISUPPORT
		else if ( plugin <= vsti_count + dxi_count + base - 2 )
		{
			cfg_plugin = 5;
			cfg_dxi_plugin = dxi_plugins[ plugin - vsti_count - base ];
		}
#endif
	}
	cfg_soundfont_path = m_soundfont;
	cfg_munt_base_path = m_munt_path;
	cfg_loop_type = SendDlgItemMessage( IDC_LOOP, CB_GETCURSEL );
	cfg_rpgmloopz = SendDlgItemMessage( IDC_RPGMLOOPZ, BM_GETCHECK );
	cfg_xmiloopz = SendDlgItemMessage( IDC_XMILOOPZ, BM_GETCHECK );
	cfg_ff7loopz = SendDlgItemMessage( IDC_FF7LOOPZ, BM_GETCHECK );
	cfg_emidi_exclusion = SendDlgItemMessage( IDC_EMIDI_EX, BM_GETCHECK );
	cfg_filter_instruments = SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_GETCHECK );
	cfg_filter_banks = SendDlgItemMessage( IDC_FILTER_BANKS, BM_GETCHECK );
	cfg_ms_panning = SendDlgItemMessage( IDC_MS_PANNING, BM_GETCHECK );
	//cfg_recover_tracks = SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK );
#ifdef FLUIDSYNTHSUPPORT
	cfg_fluid_interp_method = interp_method[ SendDlgItemMessage( IDC_FLUID_INTERPOLATION, CB_GETCURSEL ) ];
#endif
	cfg_resampling = SendDlgItemMessage( IDC_RESAMPLING, CB_GETCURSEL );
	if (secret_sauce_found)
	{
		cfg_sc_flavor = SendDlgItemMessage( IDC_SC_FLAVOR, CB_GETCURSEL );
		cfg_sc_reverb = !SendDlgItemMessage( IDC_SC_EFFECTS, BM_GETCHECK );
	}
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;
	if ( !changed && GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE ) != cfg_srate ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_LOOP, CB_GETCURSEL ) != cfg_loop_type ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_RPGMLOOPZ, BM_GETCHECK ) != cfg_rpgmloopz ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_XMILOOPZ, BM_GETCHECK ) != cfg_xmiloopz ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FF7LOOPZ, BM_GETCHECK ) != cfg_ff7loopz ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_EMIDI_EX, BM_GETCHECK ) != cfg_emidi_exclusion ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_GETCHECK ) != cfg_filter_instruments ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FILTER_BANKS, BM_GETCHECK ) != cfg_filter_banks ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_MS_PANNING, BM_GETCHECK ) != cfg_ms_panning ) changed = true;
	//if ( !changed && SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK ) != cfg_recover_tracks ) changed = true;
#ifdef FLUIDSYNTHSUPPORT
	if ( !changed && interp_method[ SendDlgItemMessage( IDC_FLUID_INTERPOLATION, CB_GETCURSEL ) ] != cfg_fluid_interp_method ) changed = true;
#endif
	if ( !changed && SendDlgItemMessage( IDC_RESAMPLING, CB_GETCURSEL ) != cfg_resampling ) changed = true;
	if ( secret_sauce_found )
	{
		if ( !changed && SendDlgItemMessage( IDC_SC_FLAVOR, CB_GETCURSEL ) != cfg_sc_flavor ) changed = true;
		if ( !changed && !SendDlgItemMessage( IDC_SC_EFFECTS, BM_GETCHECK ) != cfg_sc_reverb ) changed = true;
	}
	if ( !changed )
	{
		unsigned int preset_number = SendDlgItemMessage( IDC_MS_PRESET, CB_GETCURSEL );
		if (preset_number >= g_ms_presets.get_count()) preset_number = 0;
		const ms_preset & preset = g_ms_presets[preset_number];
		if ( !(preset.synth == cfg_ms_synth && preset.bank == cfg_ms_bank) ) changed = true;
	}
	if ( !changed )
	{
		int t = SendDlgItemMessage( IDC_ADL_BANK, CB_GETCURSEL );
		if ( t < 0 || t >= m_bank_list.get_count() ) t = 0;
		if ( m_bank_list[ t ].number != cfg_adl_bank ) changed = true;
	}
	if ( !changed && GetDlgItemInt( IDC_ADL_CHIPS, NULL, FALSE ) != cfg_adl_chips ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_ADL_PANNING, BM_GETCHECK ) != cfg_adl_panning ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_MUNT_GM, CB_GETCURSEL ) != cfg_munt_gm ) changed = true;
	if ( !changed )
	{
		t_size base = secret_sauce_found ? 8 : 7;
		t_size vsti_count = vsti_plugins.get_size();
		int plugin = SendDlgItemMessage( IDC_PLUGIN, CB_GETCURSEL );
#ifdef DXISUPPORT
		t_size dxi_count = dxi_plugins.get_count();
#endif
#ifndef FLUIDSYNTHSUPPORT
		if ( plugin > 0 ) ++plugin;
#endif

		if ( ! plugin )
		{
			if ( cfg_plugin != 0 ) changed = true;
		}
		else if ( plugin >= 1 && plugin <= 3 )
		{
			int plugin_compare = plugin == 1 ? 2 : plugin == 2 ? 4 : 3;
			if ( cfg_plugin != plugin_compare ) changed = true;
		}
		else if ( plugin == 4 )
		{
			if ( cfg_plugin != 6 ) changed = true;
		}
		else if ( plugin == 5 )
		{
			if ( cfg_plugin != 7 ) changed = true;
		}
		else if ( plugin == 6 )
		{
			if ( cfg_plugin != 9 ) changed = true;
		}
		else if ( secret_sauce_found && plugin == 7 )
		{
			if ( cfg_plugin != 10 ) changed = true;
		}
		else if ( plugin <= vsti_count + base - 1 )
		{
			if ( cfg_plugin != 1 || stricmp_utf8( cfg_vst_path, vsti_plugins[ plugin - base ].path.c_str() ) ) changed = true;
			if ( !changed )
			{
				t_uint32 unique_id = vsti_plugins[ plugin - base ].unique_id;
				if ( vsti_config.size() != cfg_vst_config[ unique_id ].size() || (vsti_config.size() && memcmp( &vsti_config[0], &cfg_vst_config[ unique_id ][0], vsti_config.size() ) ) ) changed = true;
			}
		}
#ifdef DXISUPPORT
		else if ( plugin <= vsti_count + dxi_count + base - 1 )
		{
			if ( cfg_plugin != 5 || dxi_plugins[ plugin - vsti_count - base ] != cfg_dxi_plugin.get_value() ) changed = true;
		}
#endif
	}
	if ( !changed )
	{
		if ( stricmp_utf8( m_soundfont, cfg_soundfont_path ) )
			changed = true;
	}
	if ( !changed )
	{
		if ( stricmp_utf8( m_munt_path, cfg_munt_base_path ) )
			changed = true;
	}
	return changed;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return input_midi::g_get_name();}
	GUID get_guid() {return input_midi::g_get_preferences_guid();}
	GUID get_parent_guid() {return guid_input;}
};

class midi_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 2;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		if (idx > 1) return false;
		out = idx == 0 ? "MIDI files" : "SysEx dump files";
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx > 1) return false;
		out.reset();
		int count = idx == 0 ? _countof(exts) : _countof(exts_syx);
		const char ** the_exts = idx == 0 ? exts : exts_syx;
		for (int n = 0; n < count; n++)
		{
			if (n) out.add_byte(';');
			out << "*." << the_exts[n];
		}
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static const char * context_names[5] = {"Convert to General MIDI", "Save synthesizer preset", "Remove synthesizer preset", "Assign SysEx dumps", "Clear SysEx dumps"};

class midi_preset_filter : public file_info_filter
{
	pfc::string8 m_midi_preset;

	metadb_handle_list m_handles;

public:
	midi_preset_filter( const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const char * p_midi_preset )
	{
		m_midi_preset = p_midi_preset ? p_midi_preset : "";

		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(),order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		m_handles.set_count(order.get_size());
		for(t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
		}
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,p_location,index))
		{
			if ( m_midi_preset.get_length() ) p_info.info_set( field_preset, m_midi_preset );
			else p_info.info_remove( field_preset );
			return true;
		}
		else
		{
			return false;
		}
	}
};

class midi_syx_filter : public file_info_filter
{
	midi_syx_dumps m_dumps;

	metadb_handle_list m_handles;

public:
	midi_syx_filter( const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const midi_syx_dumps & p_dumps )
	{
		m_dumps = p_dumps;

		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(),order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		m_handles.set_count(order.get_size());
		for(t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
		}
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		pfc::string_extension m_ext( p_location->get_path() );

		for ( unsigned j = 0, k = _countof(exts_syx); j < k; j++ )
		{
			if ( !pfc::stricmp_ascii( m_ext, exts_syx[ j ] ) ) return false;
		}

		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,p_location,index))
		{
			pfc::string8 m_dump_serialized;
			m_dumps.serialize( p_location->get_path(), m_dump_serialized );
			if ( m_dump_serialized.get_length() ) p_info.info_set( field_syx, m_dump_serialized );
			else p_info.info_remove( field_syx );
			return true;
		}
		else
		{
			return false;
		}
	}
};

class context_midi : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 5; }

	virtual void get_item_name( unsigned n, pfc::string_base & out )
	{
		if ( n > 4 ) uBugCheck();
		out = context_names[ n ];
	}

	/*
	virtual void get_item_default_path( unsigned n, pfc::string_base & out )
	{
		out = "Utils";
	}
	*/
	GUID get_parent() {return contextmenu_groups::utilities;}

	virtual bool get_item_description( unsigned n, pfc::string_base & out )
	{
		if ( n > 4 ) uBugCheck();
		static const char * descriptions[5] = { "Converts the selected files into General MIDI files in the same path as the source.",
			"Applies the current synthesizer setup to this track for future playback.",
			"Removes a saved synthesizer preset from this track.",
			"Assigns the selected SysEx dumps to the selected MIDI files.",
			"Clears all assigned SysEx dumps from the selected MIDI files."
		};
		out = descriptions[ n ];
		return true;
	}

	virtual GUID get_item_guid( unsigned n )
	{
		static const GUID guids[5] = {
			{ 0x70985c72, 0xe77e, 0x4bbb, { 0xbf, 0x11, 0x3c, 0x90, 0x2b, 0x27, 0x39, 0x9d } },
			{ 0xeb3f3ab4, 0x60b3, 0x4579, { 0x9f, 0xf8, 0x38, 0xda, 0xc0, 0x91, 0x2c, 0x82 } },
			{ 0x5bcb6efe, 0x2eb5, 0x4331, { 0xb9, 0xc1, 0x92, 0x4b, 0x77, 0xba, 0xcc, 0x10 } },
			{ 0xd0e4a166, 0x10c, 0x41f0, { 0xad, 0x5a, 0x51, 0x84, 0x44, 0xa3, 0x92, 0x9c } },
			{ 0x2aa8c082, 0x5d84, 0x4982, { 0xb4, 0x5d, 0xde, 0x51, 0xcb, 0x75, 0xff, 0xf2 } }
		};
		if ( n > 4 ) uBugCheck();
		return guids[ n ];
	}

	virtual bool context_get_display( unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out,unsigned & displayflags, const GUID & )
	{
		if ( n > 4 ) uBugCheck();
		unsigned matches, matches_syx, i, j, k, l;
		i = data.get_count();
		for (matches = 0, matches_syx = 0, j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			pfc::string_extension ext(foo.get_path());
			for ( k = ((n == 0) ? 3 : 0), l = _countof(exts); k < l; k++ )
			{
				if ( !pfc::stricmp_ascii( ext, exts[ k ] ) )
				{
					matches++;
					break;
				}
			}
			if ( k < l ) continue;
			if ( n == 3 )
			{
				for ( unsigned k = 0, l = _countof(exts_syx); k < l; k++ )
				{
					if ( !pfc::stricmp_ascii( ext, exts_syx[ k ] ) )
					{
						matches_syx++;
						break;
					}
				}
			}
		}
		if ( ( n != 3 && matches == i ) || ( n == 3 && ( matches + matches_syx ) == i ) )
		{
			out = context_names[ n ];
			return true;
		}
		return false;
	}

	virtual void context_command( unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, const GUID & )
	{
		if ( n > 4 ) uBugCheck();
		if ( !n )
		{
			unsigned i = data.get_count();
			abort_callback_dummy m_abort;
			std::vector<uint8_t> file_data;
			for ( unsigned i = 0, j = data.get_count(); i < j; i++ )
			{
				const playable_location & loc = data.get_item( i )->get_location();
				pfc::string8 out_path = loc.get_path();
				out_path += ".mid";

				service_ptr_t<file> p_file;
				filesystem::g_open( p_file, loc.get_path(), filesystem::open_mode_read, m_abort );

				t_filesize size = p_file->get_size_ex( m_abort );

				file_data.resize( size );
				p_file->read_object( &file_data[0], size, m_abort );

				midi_container midi_file;
				if ( ! midi_processor::process_file( file_data, pfc::string_extension( loc.get_path() ), midi_file ) )
					continue;

				file_data.resize( 0 );
				midi_file.serialize_as_standard_midi_file( file_data );

				filesystem::g_open( p_file, out_path, filesystem::open_mode_write_new, m_abort );
				p_file->write_object( &file_data[0], file_data.size(), m_abort );
			}
		}
		else if ( n < 3 )
		{
			const char * p_midi_preset;
			pfc::string8 preset_serialized;

			if ( n == 1 )
			{
				midi_preset thePreset;
				thePreset.serialize( preset_serialized );
				p_midi_preset = preset_serialized.get_ptr();
			}
			else p_midi_preset = 0;

			static_api_ptr_t<metadb_io_v2> p_imgr;
			service_ptr_t<midi_preset_filter> p_filter = new service_impl_t< midi_preset_filter >( data, p_midi_preset );
			p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
		}
		else
		{
			midi_syx_dumps m_dumps;

			if ( n == 3 )
			{
				for ( unsigned i = 0; i < data.get_count(); i++ )
				{
					const char * path = data[ i ]->get_path();
					if ( g_test_extension_syx( pfc::string_extension( path ) ) )
					{
						m_dumps.dumps.append_single( path );
					}
				}
			}

			static_api_ptr_t<metadb_io_v2> p_imgr;
			service_ptr_t<midi_syx_filter> p_filter = new service_impl_t< midi_syx_filter >( data, m_dumps );
			p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
		}
	}
};

static input_factory_t<input_midi>                          g_input_midi_factory;
static preferences_page_factory_t <preferences_page_myimpl> g_config_midi_factory;
static service_factory_single_t   <midi_file_types>         g_input_file_type_midi_factory;
static contextmenu_item_factory_t <context_midi>            g_contextmenu_item_midi_factory;
static initquit_factory_t         <initquit_midi>           g_initquit_midi_factory;

#include "../patrons.h"

DECLARE_COMPONENT_VERSION("MIDI Player", MYVERSION, "Special thanks go to DEATH's cat.\n\nEmu de MIDI alpha - Copyright (C) Mitsutaka Okazaki 2004\n\nVST Plug-In Technology by Steinberg.\n\n"
"My main man left the Crimson Lance to bring you the Secret Sauce!\n\n"
"https://www.patreon.com/kode54""\n\n"
MY_PATRONS
"\n\n"
"Notice for json-parser:\n"
"Copyright (C) 2012, 2013, 2014 James McLaughlin et al.  All rights reserved.\n"
"https://github.com/udp/json-parser\n"
"\n"
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions\n"
"are met :\n"
"\n"
"1. Redistributions of source code must retain the above copyright\n"
" notice, this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"  notice, this list of conditions and the following disclaimer in the\n"
"  documentation and / or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n"
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
"ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n"
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
"DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
"OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
"LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
"OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
"SUCH DAMAGE.\n"
"\n\n"
"Notice for json-builder:\n"
"Copyright(C) 2014 James McLaughlin.All rights reserved.\n"
"https://github.com/udp/json-builder\n"
"\n"
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions\n"
"are met :\n"
"\n"
"1. Redistributions of source code must retain the above copyright\n"
" notice, this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"  notice, this list of conditions and the following disclaimer in the\n"
"  documentation and / or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n"
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
"ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n"
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
"DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
"OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
"LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
"OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
"SUCH DAMAGE.\n"
"\n\n"
"Notice for sflist loader:\n"
"Copyright(C) 2017 Christopher Snowhill.All rights reserved.\n"
"https://github.com/kode54/sflist\n"
"https://gist.github.com/kode54/a7bb01a0db3f2e996145b77f0ca510d5\n"
"\n"
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions\n"
"are met :\n"
"\n"
"1. Redistributions of source code must retain the above copyright\n"
" notice, this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"  notice, this list of conditions and the following disclaimer in the\n"
"  documentation and / or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n"
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
"ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n"
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
"DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
"OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
"LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
"OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
"SUCH DAMAGE.\n"
);

VALIDATE_COMPONENT_FILENAME("foo_midi.dll");
