#define MYVERSION "1.158"

// #define DXISUPPORT
// #define FLUIDSYNTHSUPPORT

/*
	change log

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

#define _WIN32_WINNT 0x0501

#include <foobar2000.h>
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "nu_processing/midi_processor.h"

#include <shlobj.h>
#include <shlwapi.h>

#include "VSTiPlayer.h"
#ifdef FLUIDSYNTHSUPPORT
#include "SFPlayer.h"
#endif
#include "BMPlayer.h"
#include "MT32Player.h"
#include "EMIDIPlayer.h"

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

// {49B4E0E6-E572-4d5d-88DB-78CEA20EADC1}
static const GUID guid_cfg_xmiloopz = 
{ 0x49b4e0e6, 0xe572, 0x4d5d, { 0x88, 0xdb, 0x78, 0xce, 0xa2, 0xe, 0xad, 0xc1 } };
// {D1E8D624-C2F8-4fe1-A13E-B1F19A6F2CB6}
static const GUID guid_cfg_ff7loopz = 
{ 0xd1e8d624, 0xc2f8, 0x4fe1, { 0xa1, 0x3e, 0xb1, 0xf1, 0x9a, 0x6f, 0x2c, 0xb6 } };
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
// {8FFE9127-579E-46B8-951D-3C785930307F}
static const GUID guid_cfg_munt_debug_info = 
{ 0x8ffe9127, 0x579e, 0x46b8, { 0x95, 0x1d, 0x3c, 0x78, 0x59, 0x30, 0x30, 0x7f } };
#ifdef FLUIDSYNTHSUPPORT
// {A395C6FD-492A-401B-8BDB-9DF53E2EF7CF}
static const GUID guid_cfg_fluid_interp_method = 
{ 0xa395c6fd, 0x492a, 0x401b, { 0x8b, 0xdb, 0x9d, 0xf5, 0x3e, 0x2e, 0xf7, 0xcf } };
#endif
// {A1097E84-09B6-4708-9A58-8B1247D54299}
static const GUID guid_cfg_vst_config = 
{ 0xa1097e84, 0x9b6, 0x4708, { 0x9a, 0x58, 0x8b, 0x12, 0x47, 0xd5, 0x42, 0x99 } };
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


template <typename TObj>
class cfg_array : public cfg_var, public pfc::array_t<TObj> {
public:
	cfg_array(const GUID& guid) : cfg_var(guid), pfc::array_t<TObj>() {}

	pfc::array_t<TObj> & val() {return *this;}
	pfc::array_t<TObj> const & val() const {return *this;}

	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
		stream_writer_formatter<> out(*p_stream,p_abort);
		const pfc::array_t<TObj> * ptr = this;
		out.write_array( *ptr );
	}
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		stream_reader_formatter<> in(*p_stream,p_abort);
		pfc::array_t<TObj> * ptr = this;
		in.read_array( *ptr );
	}
};

enum
{
	default_cfg_xmiloopz = 0,
	default_cfg_ff7loopz = 0,
	default_cfg_emidi_exclusion = 1,
	default_cfg_filter_instruments = 0,
	default_cfg_filter_banks = 0,
	//default_cfg_recover_tracks = 0,
	default_cfg_loop_type = 0,
	default_cfg_srate = 44100,
	default_cfg_plugin = 0,
	default_cfg_resampling = 1,
#ifdef FLUIDSYNTHSUPPORT
	default_cfg_fluid_interp_method = FLUID_INTERP_DEFAULT
#endif
};

#ifdef DXISUPPORT
static const GUID default_cfg_dxi_plugin = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
#endif

cfg_int cfg_xmiloopz(guid_cfg_xmiloopz, default_cfg_xmiloopz), cfg_ff7loopz(guid_cfg_ff7loopz, default_cfg_ff7loopz),
		cfg_emidi_exclusion(guid_cfg_emidi_exclusion, default_cfg_emidi_exclusion), /*cfg_hack_xg_drums("yam", 0),*/
		cfg_filter_instruments(guid_cfg_filter_instruments, default_cfg_filter_instruments),
		cfg_filter_banks(guid_cfg_filter_banks, default_cfg_filter_banks),
		/*cfg_recover_tracks(guid_cfg_recover_tracks, default_cfg_recover_tracks),*/ cfg_loop_type(guid_cfg_loop_type, default_cfg_loop_type),
		/*cfg_nosysex("sux", 0),*/ /*cfg_gm2(guid_cfg_gm2, 0),*/
		cfg_srate(guid_cfg_srate, default_cfg_srate), cfg_plugin(guid_cfg_plugin, default_cfg_plugin),
		cfg_resampling(guid_cfg_resampling, default_cfg_resampling)
#ifdef FLUIDSYNTHSUPPORT
		,cfg_fluid_interp_method(guid_cfg_fluid_interp_method, default_cfg_fluid_interp_method)
#endif
		;

#ifdef DXISUPPORT
cfg_guid cfg_dxi_plugin(guid_cfg_dxi_plugin, default_cfg_dxi_plugin);
#endif

cfg_string cfg_vst_path(guid_cfg_vst_path, "");

cfg_array<t_uint8> cfg_vst_config(guid_cfg_vst_config);

cfg_string cfg_soundfont_path(guid_cfg_soundfont_path, "");

cfg_string cfg_munt_base_path(guid_cfg_munt_base_path, "");

advconfig_branch_factory cfg_midi_parent("MIDI Decoder", guid_cfg_midi_parent, advconfig_branch::guid_branch_playback, 0);

advconfig_checkbox_factory cfg_munt_debug_info("MUNT - display debug information", guid_cfg_munt_debug_info, guid_cfg_midi_parent, 0, false);

advconfig_string_factory cfg_vsti_search_path("VSTi search path", guid_cfg_vsti_search_path, guid_cfg_midi_parent, 0, "");

static const char * exts[]=
{
	"MID",
	"MIDI",
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

static critical_section sync;
static int g_running = 0;
static int g_srate;

class input_midi
{
#ifdef DXISUPPORT
	DXiProxy * dxiProxy;
#endif

	VSTiPlayer * vstPlayer;
#ifdef FLUIDSYNTHSUPPORT
	SFPlayer * sfPlayer;
#endif
	BMPlayer * bmPlayer;
	MT32Player * mt32Player;
	EMIDIPlayer * emidiPlayer;

	midi_container midi_file;

	unsigned srate;
	unsigned plugin;
	unsigned resampling;

	bool b_xmiloopz;
	bool b_ff7loopz;
	unsigned clean_flags;
	//bool b_gm2;

	unsigned length_samples;
	unsigned length_ticks;
	unsigned samples_done;

	unsigned loop_begin;
	unsigned loop_end;

	bool eof;
	bool dont_loop;

	bool first_block;

	pfc::string8 m_path;

	t_filestats m_stats;

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
	input_midi() : srate(cfg_srate), plugin(cfg_plugin), resampling(cfg_resampling),
		b_xmiloopz(!!cfg_xmiloopz), b_ff7loopz(!!cfg_ff7loopz) //, b_gm2(!!cfg_gm2)
	{
#ifdef DXISUPPORT
		dxiProxy = NULL;
#endif

		vstPlayer = NULL;
#ifdef FLUIDSYNTHSUPPORT
		sfPlayer = NULL;
#endif
		bmPlayer = NULL;
		mt32Player = NULL;
		emidiPlayer = NULL;

		length_samples = 0;
		length_ticks = 0;

		/*
		external_decoder = 0;
		mem_reader = 0;
		*/

		clean_flags = (cfg_emidi_exclusion ? midi_container::clean_flag_emidi : 0) |
			(cfg_filter_instruments ? midi_container::clean_flag_instruments : 0) |
			(cfg_filter_banks ? midi_container::clean_flag_banks : 0);
	}

	~input_midi()
	{
		/*if (external_decoder) external_decoder->service_release();
		if (mem_reader) mem_reader->reader_release();*/
		if (emidiPlayer)
		{
			delete emidiPlayer;
			insync(sync);
			g_running--;
		}
#ifdef DXISUPPORT
		if (dxiProxy) delete dxiProxy;
#endif
		if (vstPlayer) delete vstPlayer;
#ifdef FLUIDSYNTHSUPPORT
		if (sfPlayer) delete sfPlayer;
#endif
		if (bmPlayer) delete bmPlayer;
		if (mt32Player) delete mt32Player;
	}

private:
	double get_length( unsigned p_subsong )
	{
		unsigned len = midi_file.get_timestamp_end( p_subsong, true );
		double length = len * .001 + 1.;
		length_ticks = midi_file.get_timestamp_end( p_subsong ); //theSequence->m_tempoMap.Sample2Tick(len, 1000);
		length_samples = (unsigned)(((__int64)len * (__int64)srate) / 1000) + srate;
		loop_begin = midi_file.get_timestamp_loop_start( p_subsong );
		loop_end = midi_file.get_timestamp_loop_end( p_subsong );
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
		if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

		if ( p_file.is_empty() )
		{
			filesystem::g_open( p_file, p_path, filesystem::open_mode_read, p_abort );
		}

		m_path = p_path;

		m_stats = p_file->get_stats( p_abort );
		if ( ! m_stats.m_size || m_stats.m_size > ( 1 << 30 ) ) throw exception_io_unsupported_format();

		midi_processor::process_file( p_file, pfc::string_extension( p_path ), midi_file, p_abort );

		midi_file.scan_for_loops( b_xmiloopz, b_ff7loopz );
	}

	unsigned get_subsong_count()
	{
		return midi_file.get_subsong_count();
	}

	t_uint32 get_subsong( unsigned p_index )
	{
		return midi_file.get_subsong( p_index );
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		midi_meta_data meta_data;
		midi_file.get_meta_data( p_subsong, meta_data );

		midi_meta_data_item item;
		bool remap_display_name = !meta_data.get_item( "title", item );

		for ( t_size i = 0; i < meta_data.get_count(); ++i )
		{
			const midi_meta_data_item & item = meta_data[ i ];
			if ( pfc::stricmp_ascii( item.m_name, "type" ) )
			{
				const char * name = item.m_name;
				if ( remap_display_name && !pfc::stricmp_ascii( name, "display_name" ) ) name = "title";
				p_info.meta_add( name, item.m_value );
			}
		}

		p_info.info_set_int("midi_format", midi_file.get_format());
		p_info.info_set_int("midi_tracks", midi_file.get_format() == 2 ? 1 : midi_file.get_track_count());
		p_info.info_set_int("midi_channels", midi_file.get_channel_count(p_subsong));
		p_info.info_set_int("midi_ticks", midi_file.get_timestamp_end(p_subsong));
		if (meta_data.get_item("type", item)) p_info.info_set("midi_type", item.m_value);

		unsigned loop_begin = midi_file.get_timestamp_loop_start( p_subsong );
		unsigned loop_end = midi_file.get_timestamp_loop_end( p_subsong );

		if (loop_begin != ~0) p_info.info_set_int("midi_loop_start", loop_begin );
		if (loop_end != ~0) p_info.info_set_int("midi_loop_end", loop_end );
		//p_info.info_set_int("samplerate", srate);
		p_info.info_set_int("channels", 2);
		p_info.info_set( "encoding", "synthesized" );
		p_info.set_length( double( midi_file.get_timestamp_end( p_subsong, true ) ) * 0.001 + 1.0 );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		first_block = true;

		get_length(p_subsong);

		midi_meta_data meta_data;

		midi_file.get_meta_data( p_subsong, meta_data );

		midi_meta_data_item item;
		if ( meta_data.get_item( "type", item ) && !strcmp( item.m_value, "MT-32" ) ) plugin = 3;

		pfc::string8 file_soundfont;

		if ( plugin == 2 || plugin == 4 )
		{
			pfc::string8_fast temp;

			if ( !pfc::strcmp_partial( m_path, "file://" ) )
			{
				temp = m_path;
				temp += ".sf2";
				if ( !filesystem::g_exists( temp, p_abort ) )
				{
					temp = pfc::string_replace_extension( m_path, "sf2" );
					if ( !filesystem::g_exists( temp, p_abort ) )
					{
						temp.reset();
					}
				}

				if ( temp.length() )
				{
					file_soundfont = temp.get_ptr() + 7;
				}
			}
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
					if ( SUCCEEDED( dxiProxy->setPlugin( cfg_dxi_plugin.get_value() ) ) )
					{
						dxiProxy->Stop();
						dxiProxy->Play(TRUE);

						eof = false;
						dont_loop = true;

						if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type)
						{
							if (cfg_loop_type == 1)
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
				delete vstPlayer;
				vstPlayer = new VSTiPlayer;
				if (vstPlayer->LoadVST(cfg_vst_path))
				{
					vstPlayer->setChunk( cfg_vst_config.val().get_ptr(), cfg_vst_config.val().get_count() );

					vstPlayer->setSampleRate(srate);

					unsigned loop_mode = 0;

					if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
					{
						loop_mode = VSTiPlayer::loop_mode_enable;
						if ( cfg_loop_type > 1 ) loop_mode |= VSTiPlayer::loop_mode_force;
					}

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

				delete sfPlayer;
				sfPlayer = new SFPlayer;
				sfPlayer->setSoundFont(cfg_soundfont_path);
				if ( file_soundfont.length() ) sfPlayer->setFileSoundFont( file_soundfont );
				sfPlayer->setSampleRate(srate);
				sfPlayer->setInterpolationMethod(cfg_fluid_interp_method);

				unsigned loop_mode = 0;

				if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
				{
					loop_mode = SFPlayer::loop_mode_enable;
					if ( cfg_loop_type > 1 ) loop_mode |= SFPlayer::loop_mode_force;
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

				delete bmPlayer;
				bmPlayer = new BMPlayer;
				bmPlayer->setSoundFont(cfg_soundfont_path);
				if ( file_soundfont.length() ) bmPlayer->setFileSoundFont( file_soundfont );
				bmPlayer->setSampleRate(srate);
				bmPlayer->setSincInterpolation(!!resampling);

				unsigned loop_mode = 0;

				if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
				{
					loop_mode = BMPlayer::loop_mode_enable;
					if ( cfg_loop_type > 1 ) loop_mode |= BMPlayer::loop_mode_force;
				}

				if ( bmPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					eof = false;
					dont_loop = true;

					return;
				}
			}
			else if (plugin == 3)
			{
				midi_meta_data_item item;
				bool is_mt32 = ( meta_data.get_item( "type", item ) && !strcmp( item.m_value, "MT-32" ) );
				bool mt32_debug_info = cfg_munt_debug_info;
				delete mt32Player;
				mt32Player = new MT32Player( !is_mt32, mt32_debug_info );
				pfc::string8 p_base_path = cfg_munt_base_path;
				if ( !strlen( p_base_path ) )
				{
					p_base_path = core_api::get_my_full_path();
					p_base_path.truncate( p_base_path.scan_filename() );
				}
				mt32Player->setBasePath( p_base_path );
				mt32Player->setAbortCallback( &p_abort );
				mt32Player->setSampleRate( srate );

				unsigned loop_mode = 0;

				if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
				{
					loop_mode = MT32Player::loop_mode_enable;
					if ( cfg_loop_type > 1 ) loop_mode |= MT32Player::loop_mode_force;
				}

				if ( mt32Player->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
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

				if (!(flags & OPEN_FLAG_NO_LOOPING) && cfg_loop_type)
				{
				if (cfg_loop_type == 1)
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
				delete emidiPlayer;
				emidiPlayer = new EMIDIPlayer;

				unsigned loop_mode = 0;

				if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
				{
					loop_mode = EMIDIPlayer::loop_mode_enable;
					if ( cfg_loop_type > 1 ) loop_mode |= EMIDIPlayer::loop_mode_force;
				}

				if ( emidiPlayer->Load( midi_file, p_subsong, loop_mode, clean_flags ) )
				{
					{
						insync(sync);
						if (++g_running == 1) g_srate = srate;
						else if (srate != g_srate)
						{
							srate = g_srate;
						}
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

			return true;
		}
		else
#endif
		if (plugin == 1)
		{
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

			return true;
		}
#ifdef FLUIDSYNTHSUPPORT
		else if (plugin == 2)
		{
			unsigned todo = 1024;

			p_chunk.set_data_size( todo * 2 );

			audio_sample * out = p_chunk.get_data();

			unsigned done = sfPlayer->Play( out, todo );

			if ( ! done )
			{
				const char * err = sfPlayer->GetLastError();
				if ( err ) throw exception_io_data( err );
				return false;
			}

			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			return true;
		}
		else if (plugin == 4)
#else
		else if (plugin == 2 || plugin == 4)
#endif
		{
			unsigned todo = 1024;

			p_chunk.set_data_size( todo * 2 );

			audio_sample * out = p_chunk.get_data();

			unsigned done = bmPlayer->Play( out, todo );

			if ( ! done )
			{
				return false;
			}

			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			return true;
		}
		else if (plugin == 3)
		{
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

			return true;
		}
		else
		{
			/*
			int rval = external_decoder->run(chunk);
			if (rval >= 0)
			{
				if (!dont_loop)
				{
					if (rval)
					{
						unsigned new_rate = chunk->get_srate();
						if (srate != new_rate)
						{
							ULONGLONG meh = UInt32x32To64(samples_done, new_rate);
							samples_done = (unsigned)(meh / (ULONGLONG)srate);
							srate = new_rate;
							set_loop();
							get_length();
						}
					}

					unsigned done = chunk->get_sample_count();
					if (!rval || (samples_done + done >= sample_loop_end))
					{
						if (!external_decoder->seek((double)sample_loop_start / (double)srate))
						{
							goto fagotry;
						}

						if (rval)
						{
							done = sample_loop_end - samples_done;
							if (done)
							{
								chunk->set_sample_count(done);
								samples_done = sample_loop_start;
								return 1;
							}
						}

						rval = external_decoder->run(chunk);

						if (!rval)
						{
							/* gee, looks like input needs a bit more coaxing to reset, damnit
fagotry:
							external_decoder->service_release();
							external_decoder = get_external_decoder();
							if (!external_decoder) return 0;

							file_info_i info;
							mem_reader->seek(0);
							if (!external_decoder->open(mem_reader, &info, OPEN_FLAG_DECODE)) return 0;
							if (!external_decoder->seek((double)sample_loop_start / (double)srate)) return 0;

							rval = external_decoder->run(chunk);
						}

						samples_done = sample_loop_start;
						done = chunk->get_sample_count();
					}
					samples_done += done;
				}
			}
			return rval;
			*/
			unsigned todo = 1024;

			p_chunk.set_data_size( todo * 2 );

			audio_sample * out = p_chunk.get_data();

			unsigned done = emidiPlayer->Play( out, todo );

			if ( ! done ) return false;

			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			return true;
		}
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		unsigned seek_msec = unsigned( audio_math::time_to_samples( p_seconds, 1000 ) );

		first_block = true;
		eof = false;

		unsigned done = unsigned( ( t_int64( seek_msec ) * t_int64( srate) ) / 1000 );
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
		if ( plugin == 1 )
		{
			vstPlayer->Seek( done );
			return;
		}
#ifdef FLUIDSYNTHSUPPORT
		else if ( plugin == 2 )
		{
			sfPlayer->Seek( done );
			const char * err = sfPlayer->GetLastError();
			if ( err ) throw exception_io_data( err );
			return;
		}
		else if ( plugin == 4 )
#else
		else if ( plugin == 2 || plugin == 4 )
#endif
		{
			bmPlayer->Seek( done );
			return;
		}
		else if ( plugin == 3 )
		{
			mt32Player->Seek( done );
			return;
		}
		else
		{
			emidiPlayer->Seek( done );
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
		throw exception_io_unsupported_format();
	}

	void retag_commit( abort_callback & p_abort )
	{
		throw exception_io_unsupported_format();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return ! strcmp( p_content_type, "audio/midi" );
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		for( unsigned n=0; n< _countof( exts ); ++n )
		{
			if ( ! pfc::stricmp_ascii( p_extension, exts[ n ] ) ) return true;
		}
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

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

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
		COMMAND_HANDLER_EX(IDC_XMILOOPZ, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FF7LOOPZ, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_EMIDI_EX, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_PLUGIN_CONFIGURE, BN_CLICKED, OnButtonConfig)
		//COMMAND_HANDLER_EX(IDC_RECOVER, BN_CLICKED, OnButtonClick)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnSelectionChange(UINT, int, CWindow);
	void OnPluginChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
	void OnButtonConfig(UINT, int, CWindow);
	void OnSetFocus(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	void enum_vsti_plugins( const char * _path = 0, puFindFile _find = 0 );

	const preferences_page_callback::ptr m_callback;

	bool busy;

#ifdef DXISUPPORT
	pfc::array_t< CLSID > dxi_plugins;
#endif

	struct vsti_info
	{
		pfc::string8 path, display_name;
		bool has_editor;
	};

	pfc::array_t< vsti_info > vsti_plugins;

	pfc::array_t< t_uint8 > vsti_config;

	pfc::string8 m_soundfont, m_munt_path;
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

		ppath.add_byte( '\\' );
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

						pfc::string8 vendor, product;
						vstPlayer.getVendorString(vendor);
						vstPlayer.getProductString(product);

						if (product.length() || vendor.length())
						{
							if (!vendor.length() ||
								(product.length() >= vendor.length() &&
								!strncmp(vendor, product, vendor.length())))
							{
								info.display_name = product;
							}
							else
							{
								info.display_name = vendor;
								if (product.length())
								{
									info.display_name.add_byte(' ');
									info.display_name += product;
								}
							}
						}
						else info.display_name = _find->GetFileName();

						info.has_editor = vstPlayer.hasEditor();

						vsti_plugins.append_single( info );
					}
				}
			}
		} while ( _find->FindNext() );
		delete _find;
	}
}

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	CWindow w;
	int plugin = cfg_plugin;
	
	w = GetDlgItem( IDC_PLUGIN );
	uSendMessageText( w, CB_ADDSTRING, 0, "Emu de MIDI" );
#ifdef FLUIDSYNTHSUPPORT
	uSendMessageText( w, CB_ADDSTRING, 0, "FluidSynth" );
#endif
	uSendMessageText( w, CB_ADDSTRING, 0, "BASSMIDI" );
	uSendMessageText( w, CB_ADDSTRING, 0, "MUNT" );

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
		uSendMessageText( w, CB_ADDSTRING, 0, vsti_plugins[ i ].display_name );
		if ( plugin == 1 && ! stricmp_utf8( vsti_plugins[ i ].path, cfg_vst_path ) )
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
	}

#ifdef FLUIDSYNTHSUPPORT
	if ( plugin != 2 )
	{
		GetDlgItem( IDC_FLUID_INTERPOLATION_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_FLUID_INTERPOLATION ).EnableWindow( FALSE );
	}
#endif

	if ( plugin != 1 )
	{
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).EnableWindow( FALSE );
	}
	else
	{
		GetDlgItem( IDC_PLUGIN_CONFIGURE ).EnableWindow( vsti_plugins[ vsti_selected ].has_editor );
		vsti_config = cfg_vst_config.val();
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
	if ( plugin == 1 ) plugin += vsti_selected + 3;
	else if ( plugin >= 2 && plugin <= 4 )
	{
		plugin = plugin == 2 ? 1 : plugin == 4 ? 2 : 3;
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

	SendDlgItemMessage( IDC_XMILOOPZ, BM_SETCHECK, cfg_xmiloopz );
	SendDlgItemMessage( IDC_FF7LOOPZ, BM_SETCHECK, cfg_ff7loopz );

	SendDlgItemMessage( IDC_EMIDI_EX, BM_SETCHECK, cfg_emidi_exclusion );
	SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_SETCHECK, cfg_filter_instruments );
	SendDlgItemMessage( IDC_FILTER_BANKS, BM_SETCHECK, cfg_filter_banks );
	//SendDlgItemMessage( IDC_GM2, BM_SETCHECK, cfg_gm2 );
	//SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, cfg_recover_tracks );
	//SendDlgItemMessage( IDC_NOSYSEX, BM_SETCHECK, cfg_nosysex );
	//SendDlgItemMessage( IDC_HACK_XG_DRUMS, BM_SETCHECK, cfg_hack_xg_drums );

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
#ifndef FLUIDSYNTHSUPPORT
	if ( plugin > 0 ) ++plugin;
#endif
	if ( plugin >= 4 && plugin < 4 + vsti_plugins.get_count() )
	{
		busy = true;
		OnChanged();

		VSTiPlayer vstPlayer;
		if ( vstPlayer.LoadVST( vsti_plugins[ plugin - 4 ].path ) )
		{
			vstPlayer.setChunk( vsti_config.get_ptr(), vsti_config.get_count() );
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
#ifdef FLUIDSYNTHSUPPORT
	GetDlgItem( IDC_FLUID_INTERPOLATION_TEXT ).EnableWindow( plugin == 1 );
	GetDlgItem( IDC_FLUID_INTERPOLATION ).EnableWindow( plugin == 1 );
#endif

	//GetDlgItem( IDC_GM2 ).EnableWindow( plugin > vsti_count + 1 );

	GetDlgItem( IDC_PLUGIN_CONFIGURE ).EnableWindow( plugin >= 4 && plugin < 4 + vsti_plugins.get_count() && vsti_plugins[ plugin - 4 ].has_editor );

	vsti_config.set_count( 0 );

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
		if ( uGetOpenFileName( m_hWnd, "SoundFont and list files|*.sf2;*.sflist|SoundFont files|*.sf2|SoundFont list files|*.sflist", 0, "sf2", "Choose a SoundFont bank or list...", directory, filename, FALSE ) )
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
	SendDlgItemMessage( IDC_PLUGIN, CB_SETCURSEL, default_cfg_plugin );
	if ( default_cfg_plugin != 2 && default_cfg_plugin != 4 )
	{
		GetDlgItem( IDC_SOUNDFONT_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_SOUNDFONT ).EnableWindow( FALSE );
		GetDlgItem( IDC_RESAMPLING_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_RESAMPLING ).EnableWindow( FALSE );
	}
#ifdef FLUIDSYNTHSUPPORT
	if ( default_cfg_plugin != 2 )
	{
		GetDlgItem( IDC_FLUID_INTERPOLATION_TEXT ).EnableWindow( FALSE );
		GetDlgItem( IDC_FLUID_INTERPOLATION ).EnableWindow( FALSE );
	}
#endif
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
	SendDlgItemMessage( IDC_XMILOOPZ, BM_SETCHECK, default_cfg_xmiloopz );
	SendDlgItemMessage( IDC_FF7LOOPZ, BM_SETCHECK, default_cfg_ff7loopz );
	SendDlgItemMessage( IDC_EMIDI_EX, BM_SETCHECK, default_cfg_emidi_exclusion );
	SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_SETCHECK, default_cfg_filter_instruments );
	SendDlgItemMessage( IDC_FILTER_BANKS, BM_SETCHECK, default_cfg_filter_banks );
	//SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, default_cfg_recover_tracks );
#ifdef FLUIDSYNTHSUPPORT
	SendDlgItemMessage( IDC_FLUID_INTERPOLATION, CB_SETCURSEL, interp_method_default );
#endif
	SendDlgItemMessage( IDC_RESAMPLING, CB_SETCURSEL, default_cfg_resampling );

	vsti_config.set_count( 0 );

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
	{
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
			cfg_vst_config.val().set_count( 0 );
		}
		else if ( plugin >= 1 && plugin <= 3 )
		{
			cfg_plugin = plugin == 1 ? 2 : plugin == 2 ? 4 : 3;
			//cfg_plugin = plugin - vsti_count + 1;
			cfg_vst_config.val().set_count( 0 );
		}
		else if ( plugin <= vsti_count + 3 )
		{
			cfg_plugin = 1;
			cfg_vst_path = vsti_plugins[ plugin - 4 ].path;
			cfg_vst_config.val() = vsti_config;
		}
#ifdef DXISUPPORT
		else if ( plugin <= vsti_count + dxi_count + 3 )
		{
			cfg_plugin = 5;
			cfg_dxi_plugin = dxi_plugins[ plugin - vsti_count - 4 ];
		}
#endif
	}
	cfg_soundfont_path = m_soundfont;
	cfg_munt_base_path = m_munt_path;
	cfg_loop_type = SendDlgItemMessage( IDC_LOOP, CB_GETCURSEL );
	cfg_xmiloopz = SendDlgItemMessage( IDC_XMILOOPZ, BM_GETCHECK );
	cfg_ff7loopz = SendDlgItemMessage( IDC_FF7LOOPZ, BM_GETCHECK );
	cfg_emidi_exclusion = SendDlgItemMessage( IDC_EMIDI_EX, BM_GETCHECK );
	cfg_filter_instruments = SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_GETCHECK );
	cfg_filter_banks = SendDlgItemMessage( IDC_FILTER_BANKS, BM_GETCHECK );
	//cfg_recover_tracks = SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK );
#ifdef FLUIDSYNTHSUPPORT
	cfg_fluid_interp_method = interp_method[ SendDlgItemMessage( IDC_FLUID_INTERPOLATION, CB_GETCURSEL ) ];
#endif
	cfg_resampling = SendDlgItemMessage( IDC_RESAMPLING, CB_GETCURSEL );
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;
	if ( !changed && GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE ) != cfg_srate ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_LOOP, CB_GETCURSEL ) != cfg_loop_type ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_XMILOOPZ, BM_GETCHECK ) != cfg_xmiloopz ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FF7LOOPZ, BM_GETCHECK ) != cfg_ff7loopz ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_EMIDI_EX, BM_GETCHECK ) != cfg_emidi_exclusion ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FILTER_INSTRUMENTS, BM_GETCHECK ) != cfg_filter_instruments ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FILTER_BANKS, BM_GETCHECK ) != cfg_filter_banks ) changed = true;
	//if ( !changed && SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK ) != cfg_recover_tracks ) changed = true;
#ifdef FLUIDSYNTHSUPPORT
	if ( !changed && interp_method[ SendDlgItemMessage( IDC_FLUID_INTERPOLATION, CB_GETCURSEL ) ] != cfg_fluid_interp_method ) changed = true;
#endif
	if ( !changed && SendDlgItemMessage( IDC_RESAMPLING, CB_GETCURSEL ) != cfg_resampling ) changed = true;
	if ( !changed )
	{
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
		else if ( plugin <= vsti_count + 3 )
		{
			if ( cfg_plugin != 1 || stricmp_utf8( cfg_vst_path, vsti_plugins[ plugin - 4 ].path ) ) changed = true;
			if ( !changed )
			{
				if ( vsti_config.get_count() != cfg_vst_config.val().get_count() || memcmp( vsti_config.get_ptr(), cfg_vst_config.val().get_ptr(), vsti_config.get_count() ) ) changed = true;
			}
		}
#ifdef DXISUPPORT
		else if ( plugin <= vsti_count + dxi_count + 3 )
		{
			if ( cfg_plugin != 5 || dxi_plugins[ plugin - vsti_count - 4 ] != cfg_dxi_plugin.get_value() ) changed = true;
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
	const char * get_name() {return "MIDI synthesizer host";}
	GUID get_guid() {
		// {1623AA03-BADC-4bab-8A17-C737CF782661}
		static const GUID guid = { 0x1623aa03, 0xbadc, 0x4bab, { 0x8a, 0x17, 0xc7, 0x37, 0xcf, 0x78, 0x26, 0x61 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
};

class midi_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		if (idx > 0) return false;
		out = "MIDI files";
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx > 0) return false;
		out.reset();
		for (int n = 0; n < tabsize(exts); n++)
		{
			if (n) out.add_byte(';');
			out << "*." << exts[n];
		}
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

class context_midi : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name( unsigned n, pfc::string_base & out )
	{
		if ( n ) uBugCheck();
		out = "Convert to General MIDI";
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
		if ( n ) uBugCheck();
		out = "Converts the selected files into General MIDI files in the same path as the source.";
		return true;
	}

	virtual GUID get_item_guid( unsigned n )
	{
		static const GUID guid = { 0x70985c72, 0xe77e, 0x4bbb, { 0xbf, 0x11, 0x3c, 0x90, 0x2b, 0x27, 0x39, 0x9d } };
		if ( n ) uBugCheck();
		return guid;
	}

	virtual bool context_get_display( unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out,unsigned & displayflags, const GUID & )
	{
		if ( n ) uBugCheck();
		unsigned matches, i, j;
		i = data.get_count();
		for (matches = 0, j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			pfc::string_extension ext(foo.get_path());
			for ( unsigned k = 2, l = tabsize(exts); k < l; k++ )
			{
				if ( !pfc::stricmp_ascii( ext, exts[ k ] ) )
				{
					matches++;
					break;
				}
			}
		}
		if ( matches == i )
		{
			out = "Convert to General MIDI";
			return true;
		}
		return false;
	}

	virtual void context_command( unsigned n, const pfc::list_base_const_t<metadb_handle_ptr> & data, const GUID & )
	{
		if ( n ) uBugCheck();
		unsigned i = data.get_count();
		abort_callback_impl m_abort;
		for ( unsigned i = 0, j = data.get_count(); i < j; i++ )
		{
			const playable_location & loc = data.get_item( i )->get_location();
			pfc::string8 out_path = loc.get_path();
			out_path += ".mid";

			service_ptr_t<file> p_file;
			filesystem::g_open( p_file, loc.get_path(), filesystem::open_mode_read, m_abort );

			midi_container midi_file;
			midi_processor::process_file( p_file, pfc::string_extension( loc.get_path() ), midi_file, m_abort );

			pfc::array_t<t_uint8> data;
			midi_file.serialize_as_standard_midi_file( data );

			filesystem::g_open( p_file, out_path, filesystem::open_mode_write_new, m_abort );
			p_file->write_object( data.get_ptr(), data.get_count(), m_abort );
		}
	}
};

static input_factory_t<input_midi>                          g_input_midi_factory;
static preferences_page_factory_t <preferences_page_myimpl> g_config_midi_factory;
static service_factory_single_t   <midi_file_types>         g_input_file_type_midi_factory;
static contextmenu_item_factory_t <context_midi>            g_contextmenu_item_midi_factory;

DECLARE_COMPONENT_VERSION("MIDI synthesizer host", MYVERSION, "Special thanks go to DEATH's cat.\n\nEmu de MIDI alpha - Copyright (C) Mitsutaka Okazaki 2004\n\nVST Plug-In Technology by Steinberg.");

VALIDATE_COMPONENT_FILENAME("foo_midi.dll");
