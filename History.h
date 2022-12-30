#pragma once
/*
    change log

2022-11-20 19:00 UTC - stuerp
- Re-added missing BASS libraries.
- Started fixing compiler warnings.
- No added functionality.
- Version is now 2.7.4.3

2022-11-14 19:00 UTC - stuerp
- Fixed VST support for foobar2000 v1.6.x and foobar2000 v2.0, both for the 32-bit and 64-bit executables and the plugins.
- Version is now 2.7.4.2

2022-11-04 18:45 UTC - stuerp
- Fixed x86 build.
- Updated Bass to v2.4.17.
- Added Dark Mode support for foobar2000 v2.0.
- Version is now 2.7.4.1

2022-11-03 01:17 UTC - stuerp
- Created x64 version compatible with foobar2000 v2.0.
- Version is now 2.7.4

2022-02-19 01:17 UTC - kode54
- Fix System Exclusive messages across different ports
- Version is now 2.7.3

2022-02-10 11:07 UTC - kode54
- Fix the filtering subsystem to work properly
- Version is now 2.7.2

2022-02-10 02:19 UTC - kode54
- Fix a heap overrun in the playback code's leftover samples handler
- Version is now 2.7.1

2022-02-09 22:08 UTC - kode54
- Replace FluidSynth with BASSMIDI again. This is slightly
  regrettable, if only because it's replacing FOSS with proprietary
  software, but BASSMIDI is a way better implementation, and
  definitely way more stable.
- Version is now 2.7

2022-02-08 10:31 UTC - kode54
- Implemented synthesizer reset support for seeking, which should
  fix FluidSynth crashing when seeking around a lot, and should
  also improve responsiveness of seeking backwards.
- Version is now 2.6.2

2022-02-05 12:52 UTC - kode54
- Hopefully fix seeking in files with blocked decoding, like VSTi
- Also fix seeking in non-blocked decoding files, because some
  synthesizers need time rendered to accept timed events
- Version is now 2.6.1

2022-02-03 15:59 UTC - kode54
- Actually get it working
- Update SDK
- Fix a fatal flaw in the 64 bit VST host, regarding event lists.
  This presented itself as me using sizeof(long) for the two initial
  members of the structure, which is plainly wrong, and only compiled
  correctly for 32 bit architecture. This fixes Roland Sound Canvas VA,
  and possibly any other 64 bit plugins.
- Version is now 2.6.0

2022-02-03 10:02 UTC - kode54
- Replaced main MIDI player with common implementation from my other
  player, Cog. This implementation supports renderers that require a
  fixed block size, which I implemented for Secret Sauce and VSTi.
- Replaced Secret Sauce flavor and reverb filtering with a generic
  filter that can be applied to all drivers.

2021-08-08 20:06 UTC - kode54
- Changed zero duration check to require at least one track with a
  valid duration
- Version is now 2.5.7

2021-08-01 20:27 UTC - kode54
- Removed FluidSynth thread count option, as it's apparently broken
  with the build I use and supply
- Version is now 2.5.6

2021-05-21 07:16 UTC - kode54
- Implemented options for FluidSynth to disable effects processing,
  change the polyphony count, and increase the CPU core count
- Version is now 2.5.5

2021-05-09 01:38 UTC - kode54
- Implemented option for FluidSynth to load samples dynamically,
  enabled by default
- Rebuild FluidSynth with libsndfile support, to enable compressed
  SF3 support
- Version is now 2.5.4

2021-05-07 02:41 UTC - kode54
- Switch FluidSynth to version 3.x from HEAD, with an important
  change for support for GM/GS/XG reset messages
- Change how FluidSynth is configured, so device-id is set
  properly, so GS/XG reset messages are handled correctly
- New version of FluidSynth supports SoundFont banks greater
  than 4GB in size
- Version is now 2.5.3

2021-05-06 21:09 UTC - kode54
- Rewrite FluidSynth handler a bit, implementing its own SysEx
  handler, and changing the gain level
- Implemented support for looping indefinitely on playback only
  if a loop is detected
- Version is now 2.5.2

2021-05-05 00:39 UTC - kode54
- Fix preferences section name
- Version is now 2.5.1

2021-05-04 09:13 UTC - kode54
- Disabled BASSMIDI support and reinstated FluidSynth support
- Refactored preferences dialog code
- Version is now 2.5

2021-04-27 02:35 UTC - kode54
- Remove BASS_Free call, as it will most likely be happening during
  application shutdown anyway
- Update BASS and BASSMIDI versions, as well as decoder plugins
- Version is now 2.4.12

2021-04-12 00:13 UTC - kode54
- Fixed a potential crash from opening a file with no valid events
- Version is now 2.4.11

2021-04-10 07:13 UTC - kode54
- Finally attempt to fix Standard MIDI files with truncated tracks
  causing inconsistent track length reporting due to uninitialized
  data and other random factors
- Version is now 2.4.10

2021-04-09 21:57 UTC - kode54
- Fix Standard MIDI to truncate track ends to the last event in
  the track, rather than the timestamp of the track end meta event
- Version is now 2.4.9

2021-04-09 21:08 UTC - kode54
- Forgot to rebuild the project after making changes to the MIDI
  processor
- Version is now 2.4.8

2021-04-09 07:02 UTC - kode54
- Made Standard MIDI processor handle another bad file case better
- Version is now 2.4.7

2021-04-09 00:35 UTC - kode54
- Made Standard MIDI processor way more lenient about broken files
- Version is now 2.4.6

2021-04-08 04:35 UTC - kode54
- Updated libADLMIDI and libOPNMIDI
- Stopped using personal fork of libADLMIDI
- Version is now 2.4.5

2021-03-29 07:37 UTC - kode54
- Fix Standard MIDI file handling to support the edge case of
  files with non-standard chunks in them
- Updated libADLMIDI and libOPNMIDI
- Version is now 2.4.4

2021-01-11 08:57 UTC - kode54
- Updated libADLMIDI and libOPNMIDI
- Version is now 2.4.3

2020-12-09 02:34 UTC - kode54
- Added HMQ extension for HMP format
- Version is now 2.4.2

2020-10-18 08:24 UTC - kode54
- Fixed metadata service init
- Version is now 2.4.1

2020-10-07 02:49 UTC - kode54
- Replaced fmmidi/midisynth with libOPNMIDI
- Updated BASS/BASSMIDI/BASSWV
- Updated libADLMIDI
- Version is now 2.4.0

2020-08-30 07:09 UTC - kode54
- Hopefully fix end of file issues
- Version is now 2.3.6

2020-07-24 00:29 UTC - kode54
- Implemented BASSMIDI voices control in Advanced Preferences
- Moved existing BASSMIDI Advanced Preferences option to its
  own tree with the new option
- Dynamic info now returns current and maximum BASSMIDI voices
- Version is now 2.3.5

2020-03-25 02:06 UTC - kode54
- Added another VST plugin import function name
- Version is now 2.3.4

2020-03-05 05:12 UTC - kode54
- Add another Secret Sauce hash set
- Version is now 2.3.3

2020-02-24 12:05 UTC - kode54
- Forget cross compiling, let's use old Visual Studio 2015 instead!
- Version is now 2.3.2

2020-02-24 04:27 UTC - kode54
- Attempt to cross compile external executables, to try to minimize
  my own code footprint and just bundle widely distributed FOSS dependencies
  instead
- Version is now 2.3.1

2020-02-15 11:49 UTC - kode54
- Rebuild everything with Clang! That makes the VirusTotal go away! Mostly!
- Version is now 2.3

2020-02-02 03:54 UTC - kode54
- Now possible to load Halion and Halion Sonic, and possibly
  other plugins that had this weird quirk. No guarantees
  that any of them will even work properly.
- Version is now 2.2.14

2020-01-27 02:51 UTC - kode54
- Correctly process piped input and ignore messages
- Version is now 2.2.13

2020-01-26 01:05 UTC - kode54
- Remove message pumps from VST and SC pipe handlers
- Version is now 2.2.12

2020-01-02 03:31 UTC - kode54
- Added new Secret Sauce version
- Changed how Touhou loops are handled slightly
- Version is now 2.2.11

2019-11-24 01:11 UTC - kode54
- Implemented support for Touhou loops
- Version is now 2.2.10

2019-09-26 21:50 UTC - kode54
- Fixed Secret Sauce not sending GS reset on Default mode
- Version is now 2.2.9

2019-09-26 03:06 UTC - kode54
- Made Secret Sauce main and GS flavor separately configurable
- Version is now 2.2.8

2019-09-26 01:53 UTC - kode54
- Overhauled the looping and timing system, now with configurable presets
  for both playback and conversion, and separating the indefinite playback
  mode from the other looping modes, since it's also only applicable to
  playback
- Version is now 2.2.7

2019-09-26 00:25 UTC - kode54
- Added another Secret Sauce
- Version is now 2.2.6

2019-08-19 03:23 UTC - kode54
- Made external executables use static runtime again, since they can't import
  bundled runtime like components can
- Updated libADLMIDI
- Removed unnecessary source file from repository
- Fixed invalid MIDI files with division ticks count of 0 from slipping through
  validation and causing division by zero errors on tick count to timestamp
  calculations
- Version is now 2.2.5

2019-06-06 00:04 UTC - kode54
- Added a new Secret Sauce hash, still looking for v1.0.7 64 bit
- Better commented the MD5 table
- Version is now 2.2.4

2019-03-11 02:10 UTC - kode54
- Added further checks to Secret Sauce and VSTi players for empty plugin paths
- Added more console error notices for various states where outputs were selected
  without configuring them with ROMs or instruments
- Version is now 2.2.3

2019-03-09 00:12 UTC - kode54
- Added limited ANSI metadata handling
- Version is now 2.2.2

2019-02-25 08:08 UTC - kode54
- Changed Secret Sauce plugin interface to emit silence if the cores
  have crashed
- Version is now 2.2.1

2019-02-25 04:21 UTC - kode54
- Rewrote Secret Sauce interface into a pipe exchange system, which allows:
  - Both 32 and 64 bit modules
  - Multiple instances without copying the DLL to a temporary file
- Version is now 2.2

2019-02-08 01:56 UTC - kode54
- Updated BASS and BASSMIDI
- Added support for new BASSMIDI interpolation mode
- Added support for .SFOGG extension, which is the same
  format as sf2pack, only using Ogg Vorbis sample data
- Version is now 2.1.9

2019-01-29 01:15 UTC - kode54
- Updated libADLMIDI
- Version is now 2.1.8

2018-09-30 00:59 UTC - kode54
- Implemented libADLMIDI core selection, defaulting to the fast Dosbox core
- Version is now 2.1.7

2018-09-16 03:11 UTC - kode54
- Updated libADLMIDI
- Version is now 2.1.6

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

#include "Patrons.h"

#define COMPONENT_DESCRIPTION \
    "Special thanks go to DEATH's cat.\n" \
    "\n" \
    "Emu de MIDI alpha - Copyright (C) Mitsutaka Okazaki 2004\n\nVST Plug-In Technology by Steinberg.\n" \
    "\n" \
    "My main man left the Crimson Lance to bring you the Secret Sauce!\n" \
    "\n" \
    "https://www.patreon.com/kode54" "\n" \
    "\n" \
    KODE54_PATRONS \
    "\n" \
    "Notice for json-parser:\n" \
    "Copyright (C) 2012, 2013, 2014 James McLaughlin et al.  All rights reserved.\n" \
    "https://github.com/udp/json-parser\n" \
    "\n" \
    "Redistribution and use in source and binary forms, with or without\n" \
    "modification, are permitted provided that the following conditions\n" \
    "are met :\n" \
    "\n" \
    "1. Redistributions of source code must retain the above copyright\n" \
    " notice, this list of conditions and the following disclaimer.\n" \
    "\n" \
    "2. Redistributions in binary form must reproduce the above copyright\n" \
    "  notice, this list of conditions and the following disclaimer in the\n" \
    "  documentation and / or other materials provided with the distribution.\n" \
    "\n" \
    "THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n" \
    "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n" \
    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n" \
    "ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n" \
    "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n" \
    "DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n" \
    "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n" \
    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n" \
    "LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n" \
    "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n" \
    "SUCH DAMAGE.\n" \
    "\n" \
    "\n" \
    "Notice for json-builder:\n" \
    "Copyright(C) 2014 James McLaughlin.All rights reserved.\n" \
    "https://github.com/udp/json-builder\n" \
    "\n" \
    "Redistribution and use in source and binary forms, with or without\n" \
    "modification, are permitted provided that the following conditions\n" \
    "are met :\n" \
    "\n" \
    "1. Redistributions of source code must retain the above copyright\n" \
    " notice, this list of conditions and the following disclaimer.\n" \
    "\n" \
    "2. Redistributions in binary form must reproduce the above copyright\n" \
    "  notice, this list of conditions and the following disclaimer in the\n" \
    "  documentation and / or other materials provided with the distribution.\n" \
    "\n" \
    "THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n" \
    "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n" \
    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n" \
    "ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n" \
    "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n" \
    "DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n" \
    "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n" \
    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n" \
    "LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n" \
    "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n" \
    "SUCH DAMAGE.\n" \
    "\n" \
    "\n" \
    "Notice for sflist loader:\n" \
    "Copyright(C) 2017 Christopher Snowhill.All rights reserved.\n" \
    "https://github.com/kode54/sflist\n" \
    "https://gist.github.com/kode54/a7bb01a0db3f2e996145b77f0ca510d5\n" \
    "\n" \
    "Redistribution and use in source and binary forms, with or without\n" \
    "modification, are permitted provided that the following conditions\n" \
    "are met :\n" \
    "\n" \
    "1. Redistributions of source code must retain the above copyright\n" \
    " notice, this list of conditions and the following disclaimer.\n" \
    "\n" \
    "2. Redistributions in binary form must reproduce the above copyright\n" \
    "  notice, this list of conditions and the following disclaimer in the\n" \
    "  documentation and / or other materials provided with the distribution.\n" \
    "\n" \
    "THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n" \
    "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n" \
    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n" \
    "ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n" \
    "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n" \
    "DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n" \
    "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n" \
    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n" \
    "LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n" \
    "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n" \
    "SUCH DAMAGE.\n"
