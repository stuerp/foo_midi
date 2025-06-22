
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

### fmmidi (yuno) (Built-in)

[fmmidi](https://web.archive.org/web/20120823072908/http://milkpot.sakura.ne.jp/fmmidi/index.html) emulates the [Yamaha YM2608 (OPNA)](https://en.wikipedia.org/wiki/Yamaha_YM2608) FM synthesis sound chip.

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

---

## Usage

### Playing files

*Work in Progress*

#### Loops

The component supports 6 loop modes that can be selected in the Preferences dialog:

| Type | Description |
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

For further assistance:

🌐 Home page: [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)
🌐 Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)
🌐 Forum: [Hydrogenaudio foo_midi (foobar v2.0)](https://hydrogenaud.io/index.php/topic,123301.25.html)
