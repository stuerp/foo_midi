
# foo_midi User Guide

Welcome to [foo_midi](https://github.com/stuerp/foo_midi/releases). This guide will help you understand how to use the component effectively and get the most out of its features.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Features](#features)
3. [Playing files](#playing-files)
4. [Troubleshooting](#troubleshooting)
5. [FAQs](#faqs)
6. [Reference Material](#reference-material)
7. [Support](#support)

---

## Introduction

[foo_midi](https://github.com/stuerp/foo_midi/releases) is a [foobar2000](https://www.foobar2000.org/) component that adds playback of MIDI files to foobar2000. It's based on [foo_midi](https://gitlab.com/kode54/foo_midi) by [kode54](https://gitlab.com/kode54).

---

### System Requirements

- [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)
- Tested on Microsoft Windows 10 and later.
- To use FluidSynth you need to [download](https://github.com/FluidSynth/fluidsynth/releases/) and install the latest libraries from its GitHub page.

### Download foo_midi

You can download the component from the foobar2000 [Components](https://www.foobar2000.org/components/view/foo_midi+%28x64%29) repository or from the GitHub [Releases](https://github.com/stuerp/foo_midi/releases) page.

> [!IMPORTANT]
> Updating the component from within foobar2000 does not work.

### Installation

- Double-click `foo_midi.fbk2-component` or import `foo_midi.fbk2-component` using the foobar2000 Preferences dialog. Select the **File / Preferences / Components** menu item, select the **Components** page and click the **Install...** button.
- Follow the foobar2000 instructions.

> [!TIP]
> To verify if the installation was successful open the foobar2000 Preferences using the **File / Preferences / Components** menu item and look for **MIDI Player** in the **Installed components** list.

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
| Extended MIDI |.XMI | Used by the Miles Sound System (MSS) for storing game music. See [Modding Wiki](https://moddingwiki.shikadi.net/wiki/XMI_Format) and [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=XMI). |
| Loudness Sound System | .LDS | Created with the Loudness Sound System by Andras Molnar. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=LDS). |
| Recomposer |.RCP, .R36, .G18, .G36 | Created with Recomposer, a popular music editing application on the Japanese [PC-98](https://en.wikipedia.org/wiki/PC-98) platform. |
| Extensible Music Format | .XMF, .MXMF | Created by the [MIDI Association](https://midi.org/extensible-music-format-xmf). See also [Video Game Music Preservation Foundation (VGMPF)](https://www.vgmpf.com/Wiki/index.php?title=XMF). |
| Mobile Music File |.MMF| Synthetic-music Mobile Application Format (SMAF). See [FileFormat.com](https://docs.fileformat.com/audio/mmf/). |

### Players

foo_midi implements and suports several players. A player is emulates an FM or sample-based synthesizer. The build-in players do not require any additional files to play back music. Additional players become available when you install and configure the required support files.

### LibADLMIDI (Built-in)

This player uses the [libADLMIDI](https://github.com/Wohlstand/libADLMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YMF262 and CT1747 (OPL3)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL3) FM synthesis sound chip.

You can choose which emulator core this player uses:

- Nuked OPL3 v1.8: Slowest but most accurate
- Nuked OPL3 v1.7.4: Slow, slightly less accurate
- DOSBox: Fast, mostly accurate
- Opal: Only suitable for Reality Adlib Tracker tunes
- Java
- ESFMu: ESS “ESFM” enhanced OPL3 clone
- MAME OPL2
- YMFM OPL2: Yamaha FM sound cores (OPM, OPN, OPL, and others), written by Aaron Giles
- YMFM OPL3
- Nuked OPL2 LLE
- Nuked OPL3 LLE

### LibOPNMIDI (Built-in)

This player uses the [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM2612 (OPN2)](https://en.wikipedia.org/wiki/Yamaha_YM2612) and [Yamaha YM2608 (OPNA)](https://en.wikipedia.org/wiki/Yamaha_YM2608) FM synthesis sound chip.

You can choose which emulator core this player uses:

- MAME YM2612
- Nuked OPN2
- GENS
- Genesis Plus GX
- Neko Project II OPNA
- MAME YM2608
- PMDWin OPNA

### LibEDMIDI aka Emu de MIDI (Built-in)

This player uses the [libEDMIDI](https://github.com/Wohlstand/libEDMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM2413 and VRC7 (OPLL)](https://en.wikipedia.org/wiki/Yamaha_YM2413) FM synthesis sound chip, the [Sega Programmable Sound Generator (PSG, SN76496)](https://segaretro.org/SN76489) and the [Konami SCC](http://bifi.msxnet.org/msxnet/tech/scc).

### Nuked OPL3 (Built-in)

This player uses the [Nuked OPL3](https://github.com/nukeykt/Nuked-OPL3) library by [Alexey Khokholov (Nuke.YKT)](http://nukeykt.retrohost.net/) to emulate the [Yamaha YMF262 and CT1747 (OPL3)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL3) FM synthesis sound chip.

### LibMT32EMU (MT-32) (Built-in)

This player uses the [libMT32Emu](https://github.com/munt/munt) library to emulated the [Roland MT-32, CM-32L and LAPC-I synthesiser modules](https://en.wikipedia.org/wiki/Roland_MT-32).

### FMMIDI (yuno) (Built-in)

[FMMIDI](https://web.archive.org/web/20120823072908/http://milkpot.sakura.ne.jp/fmmidi/index.html) emulates the [Yamaha YM2608 (OPNA)](https://en.wikipedia.org/wiki/Yamaha_YM2608) FM synthesis sound chip.

It requires a text file that specifies the programs or instrument definitions. A Programs.txt file is installed with the component in component directory.

### BASSMIDI (Built-in)

This player is a wrapper for the BASSMIDI library by [Un4seen](https://www.un4seen.com/). The required libraries to use it are included with the component.

It requires an SF2, SF2Pack, SFZ or SF3 soundfont to provide the instrument samples. See [Sound Fonts](#sound-fonts).

### FluidSynth (Optional)

This player is a wrapper for the [FluidSynth](https://www.fluidsynth.org/) library. You need to download the libraries from [GitHub](https://github.com/FluidSynth/fluidsynth/releases/) and configure their path the Preferences dialog to use it.

### VST Instruments (VSTi) (Optional)

Supports 32 and 64-bit VST instruments.

### Secret Sauce (Optional)

Secret Sauce is a wrapper for the SCCore.dll that comes bundled with Roland’s [Sound Canvas VA](https://www.roland.com/us/products/rc_sound_canvas_va/).

### CLAP (Optional)

The CLAP player allows you to use [CLAP ((CLever Audio Plug-in API))](https://u-he.com/community/clap/) plug-ins to render the audio stream.

---

## Playing files

To play a supported MIDI file simply add it to foobar2000 playlist and press play.

If the file format is not supported or there's an error in the file an error message will appear in the Playback Error popup and in the foobar2000 console.

### Configuring the output

By default the [LibADLMIDI (Built-in)](#libadlmidi-built-in) player is used. You can change the player on the **Playback / Decoding / MIDI Player** page of the foobar2000 Preferences. The **Player** droplist contains all available players.

If you have configured VSTi the compatible instruments will added to the list as a player prefixed with *VSTi*.

The **Configure** button will be enabled if the player has an additional dialog in which to configure settings specific to that player.

The **Sample rate** combobox allows you to specify the frequency the player will use to synthesize the samples. Select any of the predefined values or enter a custom value between 6000Hz and 192000Hz.

### Looping

Some MIDI files contain markers to specify which part of the message stream can be played in a loop. These markers are not part of the Standard MIDI Format standard. If the file does not contain markers foo_midi can also play the entire file in a loop.

The following loop markers are supported:

| Name | Description|
| --- | --- |
| EMIDI / XMI | Used by [EMIDI / XMI](http://www.shikadi.net/moddingwiki/XMI_Format#MIDI_controller_assignments) files with Control Change 116 and 117 as loop markers. This format also can specify the number of times the sequence should be looped. |
| Final Fantasy | Used by Final Fantasy files (starting with [Final Fantasy VII](http://www.midishrine.com/index.php?id=85)) with "loopStart" en "loopEnd" meta events as a marker |
| LeapFrog | Used by [LeapFrog](https://leapfrog.fandom.com/wiki/The_Leap-font_(Music)) files with Control Change 110 and 111 |
| RPG Maker | Used by [RPG Maker](https://en.wikipedia.org/wiki/RPG_Maker) files. Control Change 111 marks the loop start. The loop end is always the end of the stream. |
| Touhou | Used by [Touhou Project](https://en.wikipedia.org/wiki/Touhou_Project) files with Control Change 2 and 4 |

The component supports 6 loop modes that can be selected in the foobar2000 Preferences dialog:

| Type | Description |
| --- | --- |
| Never loop | The song will be played once ignoring any loop information. |
| Never loop. Use decay time | The song will be played once ignoring any loop information with a customizable decay period at the end for the sound to die down. |
| Loop and fade when detected |The song will be played and any defined loop will be repeated a customizable number of times (defined in Advanced Preferences by **Loop count**). At the end of the last loop the song will fade out over the period defined by the **Fade time** settings in Advanced Preferences. |
| Loop and fade always |The song will be played and looped a customizable number of times (defined in Advanced Preferences by **Loop count**). At the end of the last loop the song will fade out over the period defined by the **Fade time** settings in Advanced Preferences. |
| Play indefinitely when detected | The song will be played and the loop, when detected, will play until stopped. |
| Play indefinitely | The song will be played and loop until stopped. |

The **Playback** droplist specifies how loops are processed during normal playback. The **Other** droplist determines how loops are processed during other foobar2000 operations such as converting a MIDI file to another format.

The **Decay time** setting specifies the time in milliseconds that the player will wait before starting to play another track. This allows the last MIDI notes of a stream to decay instead of being abruptly cut when the new track starts playing.

### Sound Fonts


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

## Reference Material

### SoundFonts

- [Musical Artifacts](https://musical-artifacts.com/artifacts?tags=soundfont)

### Electronic Music

- [Electronic Music Wiki](https://electronicmusic.fandom.com/wiki/Main_Page)
- [File format samples](https://telparia.com/fileFormatSamples/)

### SoundFonts

- [SoundFont](https://musical-artifacts.com/artifacts?tags=soundfont), Musical Artifacts

### MIDI

- [Introduction to Computer Music: MIDI](https://cmtext.indiana.edu/MIDI/chapter3_MIDI.php), Jeffrey Hass
- [MIDI is the language of the gods](http://midi.teragonaudio.com/), Teragon Audio
- [Standards in Music](https://www.recordingblogs.com/wiki/standards-in-music-index), Recording Blogs
- [Comparison of MIDI standards](https://en.wikipedia.org/wiki/Comparison_of_MIDI_standards), Wikipedia
- [Yamaha XG Programming](http://www.studio4all.de/htmle/frameset090.html), Studio 4 All

---

## Support

For further assistance:

🌐 Home page: [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)  
🌐 Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)  
🌐 Forum: [Hydrogenaudio foo_midi (foobar v2.0)](https://hydrogenaud.io/index.php/topic,123301.25.html)
