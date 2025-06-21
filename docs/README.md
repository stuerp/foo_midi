
# foo_midi - End User Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Getting started](#getting-started)
3. [Features](#features)
4. [Usage](#usage)
5. [Troubleshooting](#troubleshooting)
6. [FAQs](#faqs)
7. [Support](#support)

---

## Introduction

Welcome to [foo_midi](https://github.com/stuerp/foo_midi/releases), a [foobar2000](https://www.foobar2000.org/) component that adds playback of MIDI files to foobar2000. It's based on [foo_midi](https://gitlab.com/kode54/foo_midi) by [kode54](https://gitlab.com/kode54).

This guide will help you understand how to use the product effectively and get the most out of its features.

---

## Getting started

### System Requirements

- [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)
- Tested on Microsoft Windows 10 and later.
- To use FluidSynth you need to [download](https://github.com/FluidSynth/fluidsynth/releases/) and install the latest libraries from its GitHub page.

### Installation

- Double-click `foo_midi.fbk2-component` or import `foo_midi.fbk2-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.
- Follow the foobar2000 instructions.
- Add a supported MIDI file to a foobar2000 playlist.
- Play the file.

---

## Features

### File Formats

foo_midi can decode the following file formats:

| Name | Extensions | Description |
| --- | --- | --- |
| Standard MIDI File | .MID, .MIDI, .KAR | Created by the [MIDI Association](https://midi.org/standard-midi-files). |
| RIFF-based MIDI File | .RMI | Wrapper format for MIDI data, as first specified by Microsoft, and later extended by MIDI.org (an arm of the MIDI Manufacturers Association) to permit the bundling of both MIDI files and Downloadable Sounds (DLS) files. See [Library of Congress](https://www.loc.gov/preservation/digital/formats/fdd/fdd000120.shtml). |
| Game Music Format | .GMF | Created by Adventure Soft for their Adventure Game Operating System (AGOS) engine. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=GMF). |
| MIDI Stream | .MIDS, .MDS | Created by Microsoft with the release of Windows 95. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=MDS). |
| Human Machine Interface MIDI P/R | .HMP | Used by Human Machine Interface's Sound Operating System. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=HMP). |
| Human Machine Interface |.HMI| Used by Human Machine Interface's Sound Operating System. It is a revision of the HMP format. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=HMI). |
| MUS |.MUS | Created by Paul Radek for his DMX audio library. Used by id Software for Doom and several other games.  See [Modding Wiki](https://moddingwiki.shikadi.net/wiki/MUS_Format). |
| Extended MIDI |.XMI | Used by the Miles Sound System (MSS) for storing game music. See [Modding Wiki](https://moddingwiki.shikadi.net/wiki/XMI_Format). |
| Loudness Sound System | .LDS | Created with the Loudness Sound System by Andras Molnar. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=LDS). |
| Recomposer |.RCP, .R36, .G18, .G36 | Created with Recomposer, a popular music editing application on the Japanese [PC-98](https://en.wikipedia.org/wiki/PC-98) platform. |
| Extensible Music Format | .XMF, .MXMF | Created by the [MIDI Association](https://midi.org/extensible-music-format-xmf). See also [Video Game Music Preservation Foundation (VGMPF)](https://www.vgmpf.com/Wiki/index.php?title=XMF). |
| Mobile Music File |.MMF| Synthetic-music Mobile Application Format (SMAF). See [FileFormat.com](https://docs.fileformat.com/audio/mmf/). |

### Players

foo_midi implements and suports several players. A player is emulates an FM or sample-based synthesizer. The build-in players do not require any additional files to play back music. Additional players become available when you install and configure the required support files.

### LibADLMIDI (Built-in)

LibADLMIDI uses the [libADLMIDI](https://github.com/Wohlstand/libADLMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YMF262 and CT1747 (OPL3)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL3) FM synthesis sound chip.

### LibOPNMIDI (Built-in)

LibOPNMIDI uses the [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM2612 (OPN2)](https://en.wikipedia.org/wiki/Yamaha_YM2612) FM synthesis sound chip.

### LibEDMIDI aka Emu de MIDI (Built-in)

Emu de MIDI uses the [libEDMIDI](https://github.com/Wohlstand/libEDMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM2413 and VRC7 (OPLL)](https://en.wikipedia.org/wiki/Yamaha_YM2413) FM synthesis sound chip, the [Sega Programmable Sound Generator (PSG, SN76496)](https://segaretro.org/SN76489) and the [Konami SCC](http://bifi.msxnet.org/msxnet/tech/scc).

### Nuke

[Yamaha YMF262 and CT1747 (OPL3)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL3) FM synthesis sound chip
[Yamaha YM2413 and VRC7 (OPLL)](https://en.wikipedia.org/wiki/Yamaha_YM2413) FM synthesis sound chip
Yamaha YM2151
Yamaha YM2610
Yamaha YM7101

### Super Munt (MT-32)

Roland MT-32, CM-32L and LAPC-I synthesiser modules

### fmmidi (yuno)

[fmmidi](https://web.archive.org/web/20120823072908/http://milkpot.sakura.ne.jp/fmmidi/index.html) emulates the [Yamaha YM2608 (OPNA)](https://en.wikipedia.org/wiki/Yamaha_YM2608) FM synthesis sound chip.

### BASSMIDI

This player is a wrapper for the BASSMIDI library by [Un4seen](https://www.un4seen.com/). The required libraries to use it are included with the component.

It requires an SF2, SF2Pack, SFZ or SF3 soundfont to provide the instrument samples. See [Sound Fonts](#sound-fonts).

### FluidSynth

This player is a wrapper for the [FluidSynth](https://www.fluidsynth.org/) library. You need to download the libraries from [GitHub](https://github.com/FluidSynth/fluidsynth/releases/) and configure their path the Preferences dialog to use it.

### VST Instruments (VSTi)

Supports 32 and 64-bit VST instruments.

### Secret Sauce (Roland’s Sound Canvas VA)

Secret Sauce is a wrapper for the SCCore.dll that comes bundled with Roland’s [Sound Canvas VA](https://www.roland.com/us/products/rc_sound_canvas_va/).

---

## Usage

### Playing files

*Work in Progress*

#### Loops

The component supports 6 loop modes that can be selected in the Preferences dialog:

| --- | --- |
| Never loop | The song will be played once ignoring any loop information. |
| Never loop. Use decay time | The song will be played once ignoring any loop information with a customizable decay period at the end for the sound to die down. |
| Loop and fade when detected|The song will be played and any defined loop will be repeated a customizable number of times (defined in Advanced Preferences by "Loop count"). At the end of the last loop the song will fade out over the period defined by the "Fade time" settings in Advanced Preferences. |
| Loop and fade always|The song will be played and looped a customizable number of times (defined in Advanced Preferences by "Loop count"). At the end of the last loop the song will fade out over the period defined by the "Fade time" settings in Advanced Preferences. |
| Play indefinitely when detected | The song will be played and the loop will play until stopped. |
| Play indefinitely | The song will be played and loop until stopped. |

#### Sound Fonts


### Metadata

*Work in Progress*

### Info Tags

*Work in Progress*

---

## Troubleshooting

---

## FAQs

**Q:** Common question 1  
**A:** Clear, concise answer.

**Q:** Common question 2  
**A:** Clear, concise answer.

---

## Support

For further assistance, contact our support team:  
📧 Email: [support@example.com](mailto:support@example.com)  
📞 Phone: +123-456-7890  
🌐 Website: [www.example.com/support](https://www.example.com/support)







## Change Log

v2.18.0.0, 2025-04-05
* Changed: Enabled dynamic sample loading in the FluidSynth player again when using [FluidSynth 2.4.4](https://github.com/FluidSynth/fluidsynth/releases/tag/v2.4.4) or later.
* Added: BASSMIDI will ignore NRPN Vibrato Depth events in SC-88Pro mode. It overreacts to this parameter.
* Added: Support for MMF/SMAF MA-2 files.

v2.17.2.0, 2025-03-19
* Fixed: Crash while attempting to open a MIDI file with a file name containing non-ASCII characters. An old bug suddenly surfaced while attempting to open a WRD file containing external lyrics.
  * Thank you to [ha7pro](https://hydrogenaud.io/index.php?action=profile;u=163651) for reporting the bug and helping me fix it.

v2.17.1.0, 2025-03-16
* Fixed: Secret Sauce crashed due to too many port resets.

v2.17.0.0, 2025-03-16
* New: Metadata MIDI_EMBEDDED_SOUNDFONT: Contains "SF x.x" (where x.x is the version number of the SoundFont specification) or "DLS" if the MIDI file contains an embedded soundfont.
* Improved: Support for XMF/MXMF files with raw deflated content.
* Improved: Tweaked the handling of embedded sound fonts for BASSMIDI and FluidSynth again.
  * The [Official SF2 RMIDI Specification](https://github.com/spessasus/sf2-rmidi-specification) example files seem to work now.
* Improved: The RIFF IPRD chunk will also be used to add an Album tag in case an IALB chunk is not found.
* Improved: FluidSynth player understands Polyphonic Key Pressure (Aftertouch) now.
* Changed: Increased the gain of the FluidSynth player.
* Changed: Disabled dynamic sample loading in the FluidSynth player. It causes distortion when playing some very short samples.
* Fixed: A pending SysEx message would get skipped when the next event used the running status.
* Fixed: More Multi Port MIDI files play correctly in BASSMIDI now.
  * The first MIDI Port message of a track is now added at the start of a track to make sure it occurs before any Program Change events.

v2.16.0.0, 2025-02-24
* New: Support for XMF (Extensible Music Format) and MXMF (Mobile Extensible Music Format) files.
* New: Metadata XMF_META_FILE_VERSION, XMF_FILE_TYPE and XMF_FILE_TYPE_REVISION.
* Fixed: VSTi plugins did not load or save their configuration anymore (regression).
* Fixed: VSTi plugins did not always show the correct name in the Preferences dialog (regression).

v2.15.2.0, 2025-01-12

* Improved: Updated BASSMIDI to v2.4.15.3.
* Improved: Updated FluidSynth to v2.4.2.

v2.15.1.0, 2024-09-18

* Fixed: The selected player now only gets overriden to BASSMIDI/FluidSynth when an embedded soundfont or a soundfont is found with the same basename as the MIDI file.
* Fixed: The BASSMIDI Resampling combo box was not always enabled correctly when switching player types.

v2.15.0.0, 2024-09-14

* New: Support for embedded DLS sound fonts in RMI files. Works only with the FluidSynth player.
* New: Support for the DBNK chunk in RMI files.
* New: Support for SoundFont layering without using SFList files. Not perfect yet but usable.
* Improved: Tweaked the player type override logic.
* Improved: Increased the number of MIDI ports supported by the BASSMIDI player.
* Fixed: A very old bug in the MIDI parsing code when Pitch Bend control change messages were encountered.
* Fixed: Some preferences were not reset when the Reset button was clicked.

v2.14.0.0, 2024-08-17

* New: Support for embedded artwork (IPIC) and encoding (IENC) chunks in RMI files.

v2.13.2.0, 2024-08-16

* Fixed: The Apply button remained active again even if nothing was changed.

v2.13.1.0, 2024-08-15

* Fixed: 0-byte *.tmp files were not deleted after playing an RMI file with an embedded SoundFont.

v2.13.0.0, 2024-08-14

* Improved: An All Notes Off channel mode message will be sent when a channel gets disabled. The overall filtering has been fine-tuned.
* Improved: Channels can be disabled per port.
* Improved: Only sets the MIDI player to BASSMIDI with SoundFonts if FluidSynth is not used.
* Builds with foobar2000 SDK 2024-08-07.

v2.12.0.0, 2024-08-10

* New: Added support for embedded sound fonts in RMI files.
* New: MIDI channels can be enabled and disabled during playback.
  * See MSKB article 84817 [Using the MIDI Mapper](https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/84817) for an explanation of the 1 - 10 and 11 - 16 buttons.
* Changed: Dropped support for foobar2000 1.x.

v2.11.0.0, 2024-06-23

* New: Recomposer support (.RCP, .R36, .G18, .G36). Some files may still have issues.
* New: Preferences page to configure Recomposer and HMI/HMP settings. (alpha7)
* New: HMI/HMP default tempo can be configured. (alpha7)
* New: Support for LeapFrog loop markers (CC 110 and 111). (alpha8)
* Improved: Added support for Unicode paths to RCP converter (alpha3)
* Improved: Detection of mixed-encoding text in metadata (alpha5)
* Fixed: RPG Maker loops should work again.
* Fixed: Recomposer files with strange tempo changes crashed the component. (alpha4)
* Fixed: HMI conversion added a second Note On event after every note instead of a Note Off event. (alpha7)
* Fixed: The Apply button remained active even if nothing was changed. (alpha7)

v2.10.0.0, 2024-05-07, *"It's been a while"*

* New: The volume of BASSMIDI can be tweaked independently of the overall volume. Defaults to 0.15, determined experimentally to align with the other players.
* Improved: Added detection of EUC-JP encoded meta data.
* Improved: Added Shift-JIS and EUC-JP detection and conversion for lyrics.
* Fixed: Mixed ANSI and Shift-JIS wasn't detected (anymore?).
* Fixed: The BASSMIDI voice count was not initialized correctly when using a preset.
* Fixed: Emu de MIDI sysex recognition.
* Fixed: Emu de MIDI potential buffer overflow during rendering.

v2.9.2.0, 2023-12-24, *"Merry Christmas"*

* New: Compatible with foo_vis_midi v0.1.0.
* Fixed: Crash in Emu de MIDI because dynamic synthesis rate was not initialized in time.
* Fixed: Loop type was not respected when converting to other audio formats.
* Builds with foobar2000 SDK 2023-09-23.

v2.9.1.3, 2023-11-02, *"Loop de loop"*

* New: You can specify the path of an ADLMIDI bank (*.wopl or any of the other supported formats) in the Advanced branch of the Preferences dialog.
  * The bank in the file overrides any selection in the bank drop down list in Preferences.
  * The file path is not yet saved as part of a preset.
  * Only file paths with Latin-1 characters are supported (limitation of the library).
* New: Made Opal and Java OPL3 emulator core from LibADLMIDI selectable in the Advanced Preferences.
* Improved: The decay time is now configurable. The default is still 1s (1000ms).
* Improved: Looping, fading and decay has been tweaked.
* Improved: The song duration is now always calculated without taking into account the selected loop mode. So it's the absolute length of the song without any looping or decay time.
* Improved: Made the parsing of the MIDI data more robust.
* Improved: LDS file detection is more robust.
* Fixed: FluidSynth did not respect the preferred sample rate.
* Fixed: FluidSynth did not save two settings in a preset.
* Fixed: Invalid embedded karaoke lyrics were not handled correctly.
* Builds with foobar2000 SDK 2023-09-06.

v2.9.0.0, 2023-08-02, *"Revenge of the FluidSynth"*

* New: Added FluidSynth player again.
  * It can be selected after setting the path to the directory that contains the FluidSynth libraries. You can download FluidSynth [here](https://github.com/FluidSynth/fluidsynth/releases). Make sure you download the version that has the same CPU architecture as foobar2000 (x64 or x86).
* New: Added .XFM as an alternative file extension for XMI files.
* Improved: Added FluidSynth settings to preferences page.
* Improved: MIDI standard detection
  * Some XG files were not recognized as such if the file contained any GS messages first.
  * GM 2 detection.
* Changed: Renamed dynamic info tags *bassmidi_voices* and *bassmidi_voices_max* to *midi_active_voices* and *midi_peak_voices*. The FluidSynth player also sets those tags while playing.
* Fixed: An old bug in the XMI parser prevented some XMI files from loading.

v2.8.5.0, 2023-07-23, *""*

* New: Added a configuration option to always use Super Munt when playing an MT-32 file. Default is on.
* New: Added an configuration option to always use a VSTi to play an XG file. Default is off.
  * Don't forget to specify the path name of the VSTi in the Advanced preferences.
* Fixed: Saving MIDI presets was broken.
* Fixed: Loop detection was broken for some files.

v2.8.4.0, 2023-06-26, *"Beat the Drum"*

* New: A new Info tag "MIDI_PLAYER" contains the name of the MIDI player playing the current track.
* New: Added detection of an extra percussion channel in Standard MIDI Files (*.MID). When meta data messages of type Text, Track Name or Instrument Name containing the word "drum" preceed the first message for channel 16 it will be used as an extra percussion channel in addition to channel 10.
* New: A new Info tag "MIDI_EXTRA_PERCUSSION_CHANNEL" contains the (1-based) number of the channel that acts as an additional percussion channel (only channel 16 for now).
* Improved: Error reporting in general and in case of parsing MIDI files with problems.
* Fixed: Playing loops did not always work. (Regression)

v2.8.3.1, 2023-06-03, *"Do you want lyrics with that? Redux"*

* Fixed: The user selection of the MIDI player was no longer honored. (Regression) A SOLID bug...

v2.8.3.0, 2023-06-01, *"Do you want lyrics with that?"*

* Cue Marker, Lyrics, Time Signature and Key Signature meta events are now converted to tags.
  * Soft Karaoke lyrics are stored in an SYNCEDLYRICS tag; other lyrics in a LYRICS tag. The tags can be edited but will not be written back to the MIDI file.
  * Info tag MIDI_LYRICS_TYPE contains the name of the Karaoke standard. For now, only Soft Karaoke format is recognized.
* Fixed: vshost process was not stopped when checking for presets. (Regression)

v2.8.2.0, 2023-05-20, *"Spring Cleaning"*

* Fixed: MIDI files with malformed tracks caused a crash.
* Fixed: MIDI files with malformed SysEx events caused a crash.
* Fixed: Work-around for weird rendering problem in Dark mode.
* Fixed: Restored access to the Preferences page from the Decoding page in the Preferences dialog.
* Improved: Tweaked the size of some of the labels of the Preferences dialog.
* Improved: Added a message to re-open the Preferences dialog after any of the paths were changed.
* Improved: Changed Shift-JIS detection in meta data. A copyright sign (©) was interpreted as Shift-JIS.

v2.8.1.0, 2023-05-01, *"A New Beginning...? Redux"*

* Fixed: The dialog now properly resizes on systems with High DPI settings (> 100% scaling)

v2.8.0.0, 2023-04-30, *"A New Beginning...?"*

* Major refactoring of the source code.
* Builds with foobar2000 SDK 2023-04-18.
* Tried to make the preferences page a bit more accessible to new users.
  * Moved all the path controls to a separate preferences page.
  * Added the configuration of the VTSi plugin path to the preferences page (in addition of the Advanced Preferences, for backwards compatibility).
  * Added the configuration of the Secret Sauce path to the preferences page (in addition of the Advanced Preferences, for backwards compatibility).

v2.7.4.4, 2022-11-21, *"I'm SoundFont of it"*

* Fixed: The new scpipe32 and scpipe64 in the previous version had issues. Secret Sauce is back.
* Added support for compressed SoundFonts (.sf3) to BASSMIDI player.
* Updated Munt (MT32 emulator) to v2.7.0.
* Reduced the component package size a bit. Only one copy of each vsthost and scpipe executable is included.

v2.7.4.3, 2022-11-20, *"Returning to BASS."*

* Re-added missing BASS libraries.
* Re-added recompiled versions of scpipe32 and scpipe64.
* Upgraded JSON parser and build to latest version.
* Started fixing compiler warnings.
* No added functionality.

v2.7.4.2, 2022-11-14, *"The Temple of VeSTa"*

* Added 32-bit and 64-bit VST instrument support for foobar2000 v2.0.
* Fixed 64-bit VST instrument support for foobar2000 v1.6.13.

v2.7.4.1, 2022-11-04, *"The Dark Side"*

* Added Dark Mode support for foobar2000 v2.0.
* Fixed 32-bit build.
* Updated BASS to v2.4.17.

v2.7.4, 2022-11-03, *"Scratchin' the itch"*

* Initial release of x64 version for [foobar2000](https://www.foobar2000.org/) v2.0.

## Acknowledgements / Credits

* Peter Pawlowski for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [kode54](https://gitlab.com/kode54/) for the original [foo_midi](https://gitlab.com/kode54/foo_midi) component.
* [Un4seen Developments](https://www.un4seen.com/) for the BASS audio library.
* [Munt](https://github.com/munt/munt/) for a multi-platform software synthesiser emulating pre-GM MIDI devices such as the Roland MT-32, CM-32L, CM-64 and LAPC-I.
* [Alexey Khokholov (Nuke.YKT)](http://nukeykt.retrohost.net/) for [Nuked OPL3](https://github.com/nukeykt/Nuked-OPL3).
* [Vitaly Novichkov](https://github.com/Wohlstand) for [libADLMIDI](https://github.com/Wohlstand/libADLMIDI) and [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI).
* [Mitsutaka Okazaki](https://github.com/Wohlstand/scc) for Emu de MIDI.
* [arch21](https://hydrogenaud.io/index.php?action=profile;u=123058) for testing and pointing me in the right direction with Secret Sauce and SF3 SoundFonts.
* Tom Moebert for [FluidSynth](https://www.fluidsynth.org/).
* [Valley Bell](https://github.com/ValleyBell) for [MidiConverters](https://github.com/ValleyBell/MidiConverters).
* [Jean-loup Gailly](http://gailly.net/) and [Mark Adler](http://en.wikipedia.org/wiki/Mark_Adler) for [zlib](https://www.zlib.net/).
* [Spessasus](https://github.com/spessasus) for testing, advice and [SpessaSynth](https://github.com/spessasus/SpessaSynth).
* [Zoltán Bacskó](https://github.com/Falcosoft) for testing, advice and [MIDI Player](https://www.vogons.org/viewtopic.php?f=5&t=48207).
* [Murachue](https://murachue.sytes.net/web/) for [MMFTool](https://murachue.sytes.net/web/softlist.cgi?mode=desc&title=mmftool)

## Reference Material

### foobar2000

* [foobar2000 Development](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Development:Overview)

### Windows User Interface

* [Desktop App User Interface](https://learn.microsoft.com/en-us/windows/win32/windows-application-ui-development)
* [Windows User Experience Interaction Guidelines](https://learn.microsoft.com/en-us/windows/win32/uxguide/guidelines)
* [Windows Controls](https://learn.microsoft.com/en-us/windows/win32/controls/window-controls)
* [Control Library](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info)
* [Resource-Definition Statements](https://learn.microsoft.com/en-us/windows/win32/menurc/resource-definition-statements)
* [Visuals, Layout](https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-layout)

### Electronic Music

* [Electronic Music Wiki](https://electronicmusic.fandom.com/wiki/Main_Page)
* [File format samples](https://telparia.com/fileFormatSamples/)

### SoundFonts

* [SoundFont](https://musical-artifacts.com/artifacts?tags=soundfont), Musical Artifacts

### FluidSynth

* [FluidSynth Documentation](https://github.com/FluidSynth/fluidsynth/wiki/Documentation)

### MIDI

* [Introduction to Computer Music: MIDI](https://cmtext.indiana.edu/MIDI/chapter3_MIDI.php), Jeffrey Hass
* [MIDI is the language of the gods](http://midi.teragonaudio.com/), Teragon Audio
* [Standards in Music](https://www.recordingblogs.com/wiki/standards-in-music-index), Recording Blogs
* [Comparison of MIDI standards](https://en.wikipedia.org/wiki/Comparison_of_MIDI_standards), Wikipedia
* [Yamaha XG Programming](http://www.studio4all.de/htmle/frameset090.html), Studio 4 All

#### GMF (Game Music Format)

* [GMF](http://www.vgmpf.com/Wiki/index.php?title=GMF)

#### HMI (Human Machine Interface)

* [HMI](http://www.vgmpf.com/Wiki/index.php?title=HMI)

#### HMP (Human Machine Interface P)

* [HMP](http://www.vgmpf.com/Wiki/index.php?title=HMP)

#### LDS (Loudness Sound System)

* [LDS](http://www.vgmpf.com/Wiki/index.php?title=LDS)

#### MDS (MIDI Stream)

* [MDS](http://www.vgmpf.com/Wiki/index.php?title=MDS)

#### MUS (DMX)

* [MUS (DMX)](http://www.vgmpf.com/Wiki/index.php?title=MUS_(DMX))
* [MUS Format](https://moddingwiki.shikadi.net/wiki/MUS_Format)

#### RMI

* [About RMIDI](https://github.com/spessasus/SpessaSynth/wiki/About-RMIDI)
* [Official SF2 RMIDI Specification](https://github.com/spessasus/sf2-rmidi-specification)

#### RCP (Recomposer)

* [Recomposer Format](http://www.vgmpf.com/Wiki/index.php?title=GMF)

#### XMI (Extended Multiple Instrument Digital Interface)

* [XMI](http://www.vgmpf.com/Wiki/index.php?title=XMI)
* [XMI Format](https://moddingwiki.shikadi.net/wiki/XMI_Format)

#### XMF (Extensible Music Format)

* [Media Type](https://www.rfc-editor.org/rfc/rfc4723.html)
* [MIDI Manufacturers Association Tech Specs & Info](https://web.archive.org/web/20080618001530/http://www.midi.org/techspecs/index.php)
* [Library of Congress](https://www.loc.gov/preservation/digital/formats/fdd/fdd000121.shtml)
* [FileFormats](http://fileformats.archiveteam.org/wiki/Extensible_Music_Format)
* [MultimediaWiki](https://wiki.multimedia.cx/index.php/Extensible_Music_Format_(XMF))
* [Introducing the Interactive XMF Audio File Format](https://www.gamedeveloper.com/audio/introducing-the-interactive-xmf-audio-file-format)
* [XmfExtractor](https://github.com/benryves/XmfExtractor)

## Links

* Home page: [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)
* Repository: [https://github.com/stuerp/foo_midi.git](https://github.com/stuerp/foo_midi.git)
* Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
