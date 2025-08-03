
# foo_midi User Guide

Welcome to [foo_midi](https://github.com/stuerp/foo_midi/releases). This guide will help you understand how to use the component effectively and get the most out of its features.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Features](#features)
3. [Playing files](#playing-files)
4. [Configuring](#configuring)
5. [Troubleshooting](#troubleshooting)
6. [FAQs](#faqs)
7. [Reference Material](#reference-material)
8. [History](#history)
9. [Acknowledgements and Credits](#acknowledgements-and-credits)
10. [Support](#support)

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

> [!Important]
> Updating the component from within foobar2000 does not work.

### Installation

- Double-click `foo_midi.fbk2-component` or import `foo_midi.fbk2-component` using the foobar2000 Preferences dialog. Select the **File / Preferences / Components** menu item, select the **Components** page and click the **Install...** button.
- Follow the foobar2000 instructions.

> [!Tip]
> To verify if the installation was successful open the foobar2000 Preferences using the **File / Preferences / Components** menu item and look for **MIDI Player** in the **Installed components** list.

---

## Features

### File Formats

foo_midi can decode the following file formats:

| Name                             | Extensions  | Description |
| -------------------------------- | ----------- | ----------- |
| Standard&nbsp;MIDI&nbsp;File     | .MID, .MIDI | MIDI file conforming to the [MIDI Association](https://midi.org/standard-midi-files) standard |
| Standard&nbsp;MIDI&nbsp;File     | .KAR        | MIDI file with embedded song lyrics for karaoke |
| Extensible&nbsp;Music&nbsp;Format | .XMF, .MXMF | Format created by the [MIDI Association](https://midi.org/extensible-music-format-xmf). See also [Video Game Music Preservation Foundation (VGMPF)](https://www.vgmpf.com/Wiki/index.php?title=XMF). |
| RIFF-based MIDI File             | .RMI        | Format created by Microsoft and later extended by MIDI.org (an arm of the MIDI Manufacturers Association) to permit the bundling of both MIDI files and Downloadable Sounds (DLS) files. See [Library of Congress](https://www.loc.gov/preservation/digital/formats/fdd/fdd000120.shtml). foo_midi also implements the more recent [SF2 RMIDI Format Extension Specification](https://github.com/spessasus/sf2-rmidi-specification). |
| Game Music Format ![Proposed](https://img.shields.io/badge/proposed-blue) | .GMF | Created by Adventure Soft for their Adventure Game Operating System (AGOS) engine. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=GMF). |
| MIDI Stream                      | .MIDS, .MDS | Format created by Microsoft with the release of Windows 95. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=MDS). |
| Human Machine Interface MIDI P/R | .HMP        | Format used by Human Machine Interface's Sound Operating System. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=HMP). |
| Human Machine Interface          | .HMI, .HMQ  | Format used by Human Machine Interface's Sound Operating System. It is a revision of the HMP format. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=HMI). |
| MUS                              | .MUS        | Format created by Paul Radek for his DMX audio library. Used by id Software for Doom and several other games.  See [Modding Wiki](https://moddingwiki.shikadi.net/wiki/MUS_Format) and [MUS (DMX)](http://www.vgmpf.com/Wiki/index.php?title=MUS_(DMX)). |
| Extended MIDI                    | .XMI, .XFM  | Format used by the Miles Sound System (MSS) for storing game music. Supports multiple songs in one file. See [Modding Wiki](https://moddingwiki.shikadi.net/wiki/XMI_Format) and [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=XMI). |
| Loudness Sound System            | .LDS        | File created with the Loudness Sound System by Andras Molnar. See [Video Game Music Preservation Foundation (VGMPF)](http://www.vgmpf.com/Wiki/index.php?title=LDS). |
| Recomposer                       | .RCP, .R36, .G18, .G36 | Formats used by Recomposer, a popular music editing application on the Japanese [PC-98](https://en.wikipedia.org/wiki/PC-98) platform developed by Come-On Music during the 1990s. It became a cult favorite among Japanese composers—most famously ZUN, the creator of the Touhou Project series. |
| Mobile Music File                | .MMF        | Synthetic-music Mobile Application Format (SMAF). See [FileFormat.com](https://docs.fileformat.com/audio/mmf/). |
| System Exclusive data file       | .SYX, .DMP  | File containing System Exclusive (SysEx) messages. |

### Players

foo_midi implements and supports several players. A player emulates an FM or sample-based synthesizer. The built-in players do not require you to download and install any additional files to play back music. Additional players become available when you install and configure the required support files.

### LibADLMIDI (Built-in, FM Synthesis)

This player uses the [libADLMIDI](https://github.com/Wohlstand/libADLMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM3812 (OPL2)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL2) and the [Yamaha YMF262 and CT1747 (OPL3)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL3) FM synthesis sound chip used in Sound Blaster cards.

Key features are:

- **OPL3 Emulation**: Supports both 2-operator and 4-operator FM synthesis for rich, expressive tones.
- **Custom Instrument Banks**: Uses the .wopl format to define FM patches, allowing full control over timbre and instrument behavior.
- **Embedded & External Banks**: Comes with built-in banks from classic games and tools, plus support for loading custom banks.
- **Stereo Sound & Panning**: Offers full stereo output with binary panning (left/right).
- **Advanced MIDI Support**:
  - Pitch bend, vibrato, sustain, and sostenuto
  - Portamento and brightness control (CC74)
  - GS/XG partial compatibility for extended instrument sets
  - SysEx support for select GS/XG features
- **Automatic Arpeggios**: Simulates chords when channel limits are reached.

It can be configured on the **[MIDI Player / FM Synthesis](#fm-synthesis)** preferences page.

### LibOPNMIDI (Built-in, FM Synthesis)

This player uses the [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM2612 (OPN2)](https://en.wikipedia.org/wiki/Yamaha_YM2612) and [Yamaha YM2608 (OPNA)](https://en.wikipedia.org/wiki/Yamaha_YM2608) FM synthesis sound chip&mdash;famously used in the Sega Genesis and NEC PC-98 systems.

Key features are:

- **Customizable FM Patch Bank**: Users can create their own timbre banks using the OPN2 Bank Editor, allowing for unique instrument sounds.
- **WOPN Format Support**: Instrument banks are stored in .wopn files, which define the FM parameters for each instrument.
- **Partial GS/XG Compatibility**: Banks can include more than 128 melodic and 80 percussion instruments, supporting extended MIDI standards.
- **Automatic Arpeggios**: Helps relieve channel pressure by simulating chords with rapid note sequences.
- **Brightness Control (CC74)**: Modulates the FM tone to simulate filter cutoff, mimicking subtractive synthesis behavior.
- **Stereo Panning**: Full-panning stereo is supported, especially in emulator-based playback.

It can be configured on the **[MIDI Player / FM Synthesis](#fm-synthesis)** preferences page.

### LibEDMIDI aka Emu de MIDI (Built-in, FM Synthesis)

This player uses the [libEDMIDI](https://github.com/Wohlstand/libEDMIDI) library by [Vitaly Novichkov](https://github.com/Wohlstand) to emulate the [Yamaha YM2413 and VRC7 (OPLL)](https://en.wikipedia.org/wiki/Yamaha_YM2413) FM synthesis sound chip, the [Sega Programmable Sound Generator (PSG, SN76496)](https://segaretro.org/SN76489) and the [Konami SCC](http://bifi.msxnet.org/msxnet/tech/scc).

### Nuked OPL3 (Built-in, FM Synthesis)

This player uses the [Nuked OPL3](https://github.com/nukeykt/Nuked-OPL3) library by [Alexey Khokholov (Nuke.YKT)](http://nukeykt.retrohost.net/) to emulate the [Yamaha YMF262 and CT1747 (OPL3)](https://en.wikipedia.org/wiki/Yamaha_OPL#OPL3) FM synthesis sound chip.

It can be configured on the **[MIDI Player / FM Synthesis](#fm-synthesis)** preferences page.

### LibMT32EMU (MT-32) (Built-in, FM Synthesis / Wavetable)

This player uses the [LibMT32Emu](https://github.com/munt/munt) library to emulate the [Roland MT-32, CM-32L and LAPC-I synthesiser modules](https://en.wikipedia.org/wiki/Roland_MT-32).

> [!Important]
> You have to specify the location of the MT-32 or CM-32L PCM and control ROMS on the **[MIDI Player / Paths](#paths)** preferences page before you can use this player.

It can be configured on the **[MIDI Player / Wavetable](#wavetable)** preferences page.

### FMMIDI (Built-in, FM Synthesis)

[FMMIDI](https://web.archive.org/web/20120823072908/http://milkpot.sakura.ne.jp/fmmidi/index.html) emulates the [Yamaha YM2608 (OPNA)](https://en.wikipedia.org/wiki/Yamaha_YM2608) FM synthesis sound chip and was created by Yuno in 2004.

> [!Important]
> FMMIDI requires a text file that specifies the programs or instrument definitions. A default **Programs.txt** file is installed with the component in component directory. This file can be overriden by selecting a different one on the **[MIDI Player / Paths](#paths)** preferences page.

### BASSMIDI (Built-in, Wavetable)

This player is a wrapper for the BASSMIDI library by [Un4seen](https://www.un4seen.com/). The libraries required to use it are included with the component.

It requires an SF2, SF2Pack or SFZ soundfont to provide the instrument samples. See [Soundfonts](#soundfonts).

It can be configured on the **[MIDI Player / Wavetable](#wavetable)** preferences page.


### FluidSynth (Optional, Wavetable)

This player is a wrapper for the [FluidSynth](https://www.fluidsynth.org/) library.

It requires an SF2, SF2Pack, SFZ or SF3 soundfont or a DLS-compatible wave set to provide the instrument samples. See [Soundfonts](#soundfonts).

> [!Important]
> You need to download the libraries from [GitHub](https://github.com/FluidSynth/fluidsynth/releases/) and configure their path on the **[MIDI Player / Paths](#paths)** preferences page before FluidSynth becomes available as a player.

It can be configured on the **[MIDI Player / Wavetable](#wavetable)** preferences page.

### Secret Sauce (Optional)

Secret Sauce is a wrapper for the SCCore.dll that comes bundled with Roland’s [Sound Canvas VA](https://www.roland.com/us/products/rc_sound_canvas_va/).

> [!Important]
> You need to specify the path of the SCCore.dll on the **[MIDI Player / Paths](#paths)** preferences page before Secret Sauce becomes available as a player.

### VSTi (VST Instruments) (Optional)

[Virtual Studio Technology](https://en.wikipedia.org/wiki/Vsti) (VST&reg;) instruments are plug-ins that provide extra functionality to a digital audio workstation (DAW). foo_midi can use both 32 and 64-bit VST instruments that are virtual  synthesizers.

> [!Important]
> You need to specify location of the VSTi plug-ins on the **[MIDI Player / Paths](#paths)** preferences page. Any compatible plug-in will be added to the player list with a `VSTi` prefix.

### CLAP (Optional)

The CLAP player allows you to use [CLAP (CLever Audio Plug-in API)](https://u-he.com/community/clap/) synthesizer plug-ins to render the audio stream. The plug-in must meet a few requirements:

- Implement the Note Ports and Audio Ports extension.
- Have only 1 MIDI input port and no MIDI output ports.
- Support MIDI dialect.
- Have no audio input ports and only 1 audio output port.
- Have only 2 output channels in stereo configuration.

> [!Important]
> You need to specify location of the CLAP plug-ins on the **[MIDI Player / Paths](#paths)** preferences page. Any compatible plug-in with extension `.dll` or `.clap` will be added to the player list with a `CLAP` prefix. A CLAP plug-in file can contain multiple plug-in entries.

Here are some examples of CLAP synthesizer plug-ins:

| Name | Description | Status |
| ---- | ----------- | ------ |
| [Dexed](https://asb2m10.github.io/dexed/) | Plug-in modeled on the [Yamaha DX7](https://en.wikipedia.org/wiki/Yamaha_DX7) | ![Supported](https://img.shields.io/badge/supported-green) |
| [Nuked SC-55 CLAP](https://github.com/johnnovak/Nuked-SC55-CLAP/) | [Roland SC-55](https://en.wikipedia.org/wiki/Roland_SC-55) emulator | ![Supported](https://img.shields.io/badge/supported-green) |
| [OctaSine](https://github.com/greatest-ape/OctaSine) | Frequency modulation synthesizer plugin | ![Supported](https://img.shields.io/badge/supported-green) |
| [Odin 2](https://thewavewarden.com/pages/odin-2/) | 24-voice polyphonic powerhouse | ![Supported](https://img.shields.io/badge/supported-green) |

---

## Playing files

To play a supported MIDI file simply add it to foobar2000 playlist and press play.

If the file format is not supported or there's an error in the file an error message will appear in the Playback Error popup and in the foobar2000 console.

### Configuring the output

By default the [LibADLMIDI (Built-in)](#libadlmidi-built-in-fm-synthesis) player is used. You can change the player on the **Playback / Decoding / MIDI Player** page of the foobar2000 Preferences. The **Player** droplist contains all available players.

If you have configured [VSTi](#vsti-vst-instruments-optional) the compatible instruments will added to the list as a player prefixed with `VSTi`. [CLAP](#clap-optional) plug-ins are prefixed with `CLAP`.

> [!Note]
> Don't forget to press the **Apply** button. The newly selected player will be used when you start a new track.

The **Configure** button will be enabled if the player has an additional dialog to configure settings specific to that player.

The **Sample rate** combobox allows you to specify the sample rate the player will be asked to create samples. Select any of the predefined values or enter a custom value between 8000Hz and 192000Hz.

> [!Important]
> Some players may ignore this setting because player-specific parameters cause it to render at a higher or lower sample rate. F.e. [FluidSynth](#fluidsynth-optional-wavetable) may get a different sample rate from a configuration file or [LibMT32EMU](#libmt32emu-mt-32-built-in-fm-synthesis--wavetable) will use a different sample rate resulting from a combination of its settings. The actual sample rate being used can be read from the [samplerate](#information-fields) information field.

### Looping

Some MIDI files contain track markers to specify which part of the message stream should be played in a loop. These markers are not part of the Standard MIDI Format standard. If the file does not contain markers foo_midi can also play the entire file in a loop.

The following loop markers are supported:

| Name | Description|
| --- | --- |
| EMIDI&nbsp;/&nbsp;XMI | Used by [EMIDI / XMI](http://www.shikadi.net/moddingwiki/XMI_Format#MIDI_controller_assignments) files with Control Change 116 and 117 as loop markers. This format also can specify the number of times the sequence should be looped. |
| Final&nbsp;Fantasy | Used by Final Fantasy files (starting with [Final Fantasy VII](http://www.midishrine.com/index.php?id=85)) with "loopStart" en "loopEnd" meta events as a marker. |
| LeapFrog | Used by [LeapFrog](https://leapfrog.fandom.com/wiki/The_Leap-font_(Music)) files with Control Change 110 and 111. |
| RPG&nbsp;Maker | Used by [RPG Maker](https://en.wikipedia.org/wiki/RPG_Maker) files. Control Change 111 marks the loop start. The loop end is always the end of the stream. |
| Touhou | Used by [Touhou Project](https://en.wikipedia.org/wiki/Touhou_Project) files with Control Change 2 and 4. |

The component supports 6 loop modes that can be selected in the foobar2000 Preferences dialog:

| Type                            | Description |
| ------------------------------- | ----------- |
| Never&nbsp;loop | The song will be played once ignoring any loop information. |
| Never&nbsp;loop.&nbsp;Use&nbsp;decay&nbsp;time | The song will be played once ignoring any loop information with a customizable decay period at the end for the sound to die down. |
| Loop&nbsp;when&nbsp;detected&nbsp;and&nbsp;fade | The song will be played and any defined loop will be repeated a customizable number of times. At the end of the last loop the song will fade out. |
| Repeat&nbsp;and&nbsp;fade | The complete song will be played and repeated a customizable number of times. At the end of the last repetition the song will fade out. |
| Loop&nbsp;indefinitely&nbsp;when&nbsp;detected | The song will be played and the loop, when detected, will play until stopped. |
| Repeat&nbsp;indefinitely | The complete song will be played and repeated until stopped. |

The **Playback** droplist specifies how loops are processed during normal playback. The **Other** droplist determines how loops are processed during other foobar2000 operations such as converting a MIDI file to another format.

The **Decay time** setting specifies the time in milliseconds that the player will wait before starting to play another track. This allows the last MIDI notes of a stream to decay instead of being abruptly cut when the new track starts playing.

The **Loop count** setting determines how many times a loop will be played before the song ends.

The **Fade-Out time** setting specifies the time in milliseconds that the player will start to fade-out the song before starting to play another track.

---

### Soundfonts

A soundfont is a file that contains samples of instruments. Players like [BASSMIDI](#bassmidi-built-in-wavetable) and [FluidSynth](#fluidsynth-optional-wavetable) require it to play MIDI files. When more than one soundfont is specified or available the soundfonts will be layered on top of each other with the first soundfont being used as a base.

Any of the supported formats can be used (Soundfont lists `.sflist`, `.sf2pack`, `.sfogg`, `.sf2`, `.sf3` and `.dls`). Each added soundfont replaces the patches with the same bank and program number in the previously added soundfonts.

> [!Note]
> foo_midi will switch to the FluidSynth player if a DLS soundfont is part of the soundfont list.

If the file has an embedded soundfont that file will be extracted and added to the soundfont list. The `.RMI` file format was designed for that purpose.

Next if a soundfont with the same basename exists in the directory of the MIDI file, the soundfont will be added to the list. F.e. if `Example.sf2` will be used to play `Example.mid` if they exist in the same directory.

Then if a soundfont with the same name as the directory exists, it will be added to the list. F.e. `C:\Chip Tunes\Chip Tunes.sf2` will be loaded when MIDI file in `C:\Chip Tunes` are played if it exists.

Lastly, the global soundfont specified on the [Paths](#paths) preferences page will be added.

> [!Tip]
> foo_midi will report the soundfonts it uses and in which order in the foobar2000 console if you set the component log level to at least `Info` level.

#### SFList

The BASSMIDI and FluidSynth player also accept a Soundfont List file. It's an UTF-8 text file that lists the location of the soundfonts that should be used. Any of the soundfont formats (including nested soundfont lists) can be specified.

> [!Important]
> The soundfonts should be listed with the least specialized soundfont first (the base soundfont) followed by more specialized soundfonts that override the presets of any of the previous ones.

##### Simple format

The simple format is just a flat UTF-8 text file where each line contains the location of a soundfont file. Empty lines are ignored. If you specify a only a file name or a relative path foo_midi will look for the file relative to the directory of the list file.

Here's an example:

``` Text
X:\Simple\SoundFont File.sf2

..\Advanced\SoundFont With Mappings.sf2
```

##### Advanced format

The most flexible format is an UTF-8 JSON file with a `.sflist` or `.json` extension that allows you to layer the soundfonts that the player will use.

The root object is a `soundFonts` array that contains 1 or more soundfont objects. A soundfont object can have the following properties:

- `fileName` specifies the location of the soundfont. If you specify only a file name or a relative path foo_midi will look for the file relative to the directory of the list file. `BASSMIDI` `FluidSynth`
- `gain` specifies the overall gain that will be applied to the samples in the soundfont. `optional` `BASSMIDI`
- `channels` is an array that contains the MIDI channels that will use the soundfont. `optional` `BASSMIDI`
- `patchMappings` is an array of mapping objects that allows you to remap samples within a soundfont without modifying the soundfont file. `optional` `BASSMIDI`

Here's an example:

``` JSON
{
  "soundFonts": [
    {"fileName": "/Simple/SoundFont File.sf2"},
    {
      "fileName": "x:\\Advanced\\SoundFont With Mappings.sf2",
      "gain": 6.0,
      "channels": [1,3,5],
      "patchMappings": [
        {"source": {"bank": 0, "program": 1}, "destination": {"bank": 0, "program": 20}},
        {"source": {"bank": 20, "program": 5}, "destination": {"bank": 0, "program": 5}}
      ]
    },
    {
      "fileName":  "../Another Simple/SoundFont File.sf2",
      "patchMappings": [
        {"destination": {"bank": 1}}
      ]
    },
    {"fileName":  "/SoundFont List.sflist.json"},
    {
      "fileName":  "/Patch.sfz",
      "patchMappings": [
        {"destination": {"bank": 0, "program": 2}},
        {"destination": {"bank": 0, "program": 3}}
      ]
    }
  ]
}
```

For a more detailed explanation of the properties please consult the documentation about [BASS_MIDI_FONTEX](https://www.un4seen.com/doc/#bassmidi/BASS_MIDI_FONTEX.html).

---

### Metadata

foo_midi supports the following tags:

| Name | Contents  |
| ---- | --------- |
| midi_preset      | The settings that will be used to play the file, overriding any settings selected in Preferences. |
| midi_sysex_dumps | MIDI SysEx messages that will be sent to the player everytime playback starts. |

### Information fields

foo_midi provides you with two types of information fields.

Available in rest and during playback:

| Name                    | Contents |
| ----------------------- | -------- |
| midi_format             | The MIDI format of the file: 0 = single track, 1 = multiple tracks, 2 = multiple songs |
| midi_tracks             | The number of tracks in the file |
| midi_channels           | The number of channels used in the file |
| midi_ticks              | The duration of the file in [MIDI ticks](https://www.recordingblogs.com/wiki/midi-tick) |
| midi_type               | The MIDI flavor: `MT-32`, `GM`, `GM2`, `GS`, `XG`. Defaults to `GM` when the flavor can't be detected |
| midi_loop_start         | The start of a loop in MIDI ticks |
| midi_loop_end           | The end of a loop in MIDI ticks |
| midi_loop_start_ms      | The start of a loop in milliseconds |
| midi_loop_end_ms        | The end of a loop in milliseconds|
| midi_lyrics_type        | The type of lyrics found in the file. Currently only `Soft Karaoke` is recognized. |
| midi_hash               | Unique fingerprint of the file used by the foobar2000 media library. |
| midi_embedded_soundfont | The type of the soundfont embedded in the file (if any). Can be `SF` a SoundFont 1.0-2.x-3.x bank or `DLS` for a Downloadable Sound collection. |

> [!Tip]
> You can format `midi_loop_start_ms` and `midi_loop_endt_ms` to hh:mm notation and use it as a custom column in playlist View like this:
>
> `[$num($div($info(MIDI_LOOP_START_MS),60000),2):$num($div($mod($info(MIDI_LOOP_START_MS),60000),1000),2)]`
> `[$num($div($info(MIDI_LOOP_END_MS),60000),2):$num($div($mod($info(MIDI_LOOP_END_MS),60000),1000),2)]`

Only available during playback:

| Name                          | Contents |
| ----------------------------- | -------- |
| samplerate                    | The sample rate currently being used to generate the samples. This can be different from sample rate specified in the preferences. |
| midi_player                   | The name of the MIDI player  |
| midi_player_ext               | The name of the extension, if any, the MIDI player is using. LibADLMIDI and LibOPNMIDI use it to report the emulator core they are using. The VSTi player and the CLAP player report the plug-in they have loaded. |
| midi_active_voices            | The number of active voices used by a wave table player |
| midi_peak_voices              | The highest number of voices used by a wave table player |
| midi_extra_percussion_channel | The number of the MIDI channel being used as an extra percussion channel (1-based). See [MIDI](#midi) preferences.  |

> [!Tip]
> You can configure the foobar2000 status bar on the  **Display / Default User Interface** preferences page. Here's an example of how to use the information fields:
>
> `%samplerate%Hz, [, "$info(midi_player)"][, "$info(midi_player_ext)"][, $info(midi_active_voices) voices '(peak ' $info(midi_peak_voices)')'][, Extra percussion channel $info(midi_extra_percussion_channel)]`

---

## Configuring

The settings of foo_midi are spread over different *pages*.

### MIDI Player

 The main preferences page is called **MIDI Player** and can be found in the **Playback / Decoding** section of the Preferences dialog. These settings in general control the playback of MIDI files. Some of the settings have already been explaing in the [Playing files](#playing-files) section.

For an explanation of the loop settings refer to [Looping](#looping).

#### MIDI settings

You can force a player to start playback using a particular *flavor* of MIDI. The term groups the common MIDI specifications and particular model specific configurations.

> [!Note]
> MIDI files may contain SysEx messages that overrule the flavor you specify.

| Name        | Purpose |
| ----------- | ------- |
| Default     | Same as SC88. |
| GM          | General MIDI 1 specification. A GM System On SysEx is sent before playback starts. |
| GM2         | General MIDI 2 specification. A GM2 System On SysEx is sent before playback starts. |
| GS&nbsp;SC&#8209;55    | General Sound specification with Roland SC-55 initialization. |
| GS&nbsp;SC&#8209;88    | General Sound specification with Roland SC-88 initialization. |
| GS&nbsp;SC&#8209;88Pro | General Sound specification with Roland SC-88Pro initialization. |
| GS&nbsp;SC&#8209;8820  | General Sound specification with Roland SC-8820 initialization. |
| XG          | Extended General MIDI specification created by Yamaha |
| None        | The player starts playing with its default configuration. Please the consult the player specific documentation for more information. |

#### Filter effects

This setting turns off the reverb and chorus effect and prevents reverb and chorus messages from being sent to the player.

#### Use LibMT32Emu with MT-32

When you enable this setting the selected player will be ignored and LibMT32Emu will always be used whenever an MT-32 MIDI file is played.

#### Use VSTi with XG

This setting will ignore the selected player and use the VSTi specified on the **[MIDI Player / Paths](#paths)** page whenever an XG MIDI file is played. Typically the Yamaha S-YXG50 VSTi is configured there.

#### Extra percussion channel

This setting will assign channel 16 as an extra percussion channel whenever a track is found in the MIDI file that contains metadata of type Text, Trackname or Instrumentname that contains the word `drum` (case-insensitive).

#### Exclude unsupported EMIDI track designation

This setting will ignore tracks in an [Apogee Expanded MIDI (EMIDI)](https://moddingwiki.shikadi.net/wiki/Apogee_Expanded_MIDI) file with Track Designation control change messages (`CC 110`) with unsupported instrument definitions.

#### Disable instrument changes

Enabling this setting will remove all Program Change messages from the MIDI stream.

#### Disable bank changes

Enabling this setting will remove all Control Change bank selection messages from the MIDI stream. (`CC 0` and `CC 32`)

---

### FM Synthesis

This sub-page of **MIDI Player** contains the settings specific to configuring the FM synthesizer-based players.

#### LibADLMIDI

##### ADL Bank

The library provides many built-in FM patches from various known PC games using AIL (Miles Sound System), DMX, HMI (Human Machine Interfaces) or Creative IBK (Instrument Bank). With this setting you select the bank that will be used during playback.

##### ADL Emulator Core

To render the MIDI file a synthesizer chip is emulated. These are the eumlators that the current version of the library supports:

| Name              | Description |
| ----------------- | ----------- |
| Nuked OPL3 v1.8   | Slowest but most accurate |
| Nuked OPL3 v1.7.4 | Slow, slightly less accurate |
| DOSBox            | Fast, mostly accurate |
| Opal OPL3         | Only suitable for Reality Adlib Tracker tunes |
| Java OPL3         | Written by Robson Cozendey |
| ESFMu             | [ESS ESFM](https://en.wikipedia.org/wiki/Yamaha_OPL#ESS_ESFM), an enhanced 72-operator OPL3-compatible clone, written by [Nuke.YKT](https://github.com/nukeykt) and [Kagamiin~](https://github.com/Kagamiin) |
| MAME OPL2         | Written by Jarek Burczynski and Tatsuyuki Satoh |
| YMFM OPL2         | Written by Aaron Giles |
| YMFM OPL3         | Written by Aaron Giles |
| Nuked OPL2 LLE    | Low-Level Emulator, CPU heavy |
| Nuked OPL3 LLE    | Low-Level Emulator, CPU heavy |

##### ADL Chips

You can specify the number of chips (1 to 100) that are available to the player. Emulation of multiple chips extends polyphony limits when rendering a MIDI file.

##### ADL Soft panning

Enables or disables soft panning.

By default the library uses binary panning where a sound is either fully left, fully right, or center. It's a simple on/off stereo placement.

Soft panning (also called full-panning stereo) allows for gradual placement of sound across the stereo field, enabling smoother and more realistic spatial positioning of instruments.

##### ADL Bank File

A bank can be loaded from a file in the [WOPL format](https://github.com/Wohlstand/OPL3BankEditor/blob/master/Specifications/WOPL-and-OPLI-Specification.pdf). You can create a bank using Wohlstand's [OPL3 Bank Editor](https://github.com/Wohlstand/OPL3BankEditor). The loaded bank will take precendence over the select bank.

#### LibOPNMIDI

##### OPN Bank

The library provides a couple of built-in instrument or patch banks. With this setting you select the bank that will be used during playback.

##### OPN Emulator Core

To render the MIDI file a synthesizer chip is emulated. These are the eumlators that the current version of the library supports:

| Name | Description |
| ---- | ----------- |
| MAME&nbsp;YM2612 | Accurate and fast on slow devices |
| MAME&nbsp;YM2608 | Accurate and fast on slow devices |
| Nuked&nbsp;OPN2&nbsp;([YM3438](https://en.wikipedia.org/wiki/YM3438#Yamaha_YM3438) mode) | Very accurate, requires a powerful CPU |
| Nuked&nbsp;OPN2&nbsp;(YM2612&nbsp;mode) | Very accurate, requires a powerful CPU |
| GENS/GS&nbsp;II&nbsp;OPN2 | GENS 2.10 emulator. Very outdated and inaccurate, but fastest. |
| Neko&nbsp;Project&nbsp;II&nbsp;Kai&nbsp;OPNA | Neko Project 2 YM2608 emulator. Semi-accurate, but fast on slow devices. |
| YMFM&nbsp;OPN2 | [YMFM emulators](https://github.com/aaronsgiles/ymfm) written by Aaron Giles |
| YMFM&nbsp;OPNA | [YMFM emulators](https://github.com/aaronsgiles/ymfm) written by Aaron Giles |

The following emulator cores have been deprecated in the current version of the library and are no longer available:

- [Genesis Plus GX](https://github.com/ekeeke/Genesis-Plus-GX)
- [PMDWin](https://c60.la.coocan.jp/) OPNA

##### OPN Chips

You can specify the number of chips (1 to 100) that are available to the player. Emulation of multiple chips extends polyphony limits when rendering a MIDI file.

##### OPN Soft panning

Enables or disables soft panning.

By default the library uses binary panning where a sound is either fully left, fully right, or center. It's a simple on/off stereo placement.

Soft panning (also called full-panning stereo) allows for gradual placement of sound across the stereo field, enabling smoother and more realistic spatial positioning of instruments.

##### OPN Bank File

A bank can be loaded from a file in the [WOPN format](https://github.com/Wohlstand/OPN2BankEditor/blob/master/Specifications/WOPN-and-OPNI-Specification.txt). You can create a bank using Wohlstand's [OPN2 Bank Editor](https://github.com/Wohlstand/OPN2BankEditor). The loaded bank will take precendence over the select bank.

#### Nuked OPL3

##### Preset

Allows you to select one of the built-in presets. A preset is a combination of an emulator code (Doom, Windows or Apogee) and a patch bank.

##### Soft panning

Enables or disables soft panning.

By default the library uses binary panning where a sound is either fully left, fully right, or center. It's a simple on/off stereo placement.

Soft panning (also called full-panning stereo) allows for gradual placement of sound across the stereo field, enabling smoother and more realistic spatial positioning of instruments.

---

### Paths

This sub-page of **MIDI Player** contains the various directory and file paths that some of the players require to work.

| Name | Description |
| ---- | ----------- |
| VSTi&nbsp;Plug&#8209;Ins | The location of the VSTi plug-ins. The root and all subdirectories will be searched for compatible plug-ins. |
| VSTi&nbsp;XG&nbsp;Plug&#8209;In | The location of the VSTi plug-in that will be used when the [Use VSTi with XG](#use-vsti-with-xg) setting is enabled. |
| CLAP&nbsp;Plug&#8209;Ins | The location of the CLAP plug-ins. The root and all subdirectories will be searched for compatible plug-ins. |
| Soundfont | The location of the soundfont to be used by BASSMIDI and FluidSynth. |
| MT&#8209;32&nbsp;ROMs | The location of the MT-32 ROM files to be used by LibMT32Emu. |
| Secret&nbsp;Sauce | The location of the Secret Sauce library. |
| FluidSynth | The location where the FluidSynth libraries can be found. |
| FluidSynth&nbsp;Config | The location of the FluidSynth configuration file. |
| FMMIDI&nbsp;Programs | The location of the file containing the FMMIDI programs. Leave this empty to use the file included in the component directory. |

---

### Processing

This sub-page of **MIDI Player** contains the settings to configure the MIDI processing.

#### Recomposer

##### Expand loops

##### Write bar marker

##### Write SysEx names

##### Extend loops

##### Wolfteam loops

Wolfteam loops refer to musical sequences used in games developed by Wolfteam, a Japanese game studio known for titles on platforms like the Sharp X68000.

##### Keep muted channels

##### Include control data

#### HMI / HMP

HMI and HMP file have no way to indicate the tempo at which they should be played. This settings allows you to modify the tempo by entering the number of *beats per minute* (bpm). The default value is 160 bpm.

#### Channels

This setting allows you disable and re-enable any of the 16 MIDI channels per port. Use the *Port&nbsp;number* slider to select the port. Click the button corresponding the channel to disable or re-enable it. These settings become immediately active during playback.

A few shortcut buttons are available to assist in the selection:

| Name | Description |
| ---- | ----------- |
| All | Enables all channels on all ports |
| None | Disables all channels on all ports |
| 1&#8209;10 ^*^ | Toggles channels 1 through 10 on all ports |
| 11&#8209;16 ^*^ | Toggles channels 11 through 16 on all ports |

^*^ See MSKB article 84817 [Using the MIDI Mapper](https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/84817) for an explanation of the 1&#8209;10 and 11&#8209;16 buttons.

#### Component settings

##### Log Level

Allows you to control the messages foo_midi will write to the foobar2000 console.

> [!Warning]
> The **Debug** and **Trace** levels may generate a lot of technical output.

---

### Wavetable

This sub-page of **MIDI Player** contains the settings specific to configuring the wavetable-based players.

#### BASSMIDI settings

##### Gain

##### Resampling

##### Max. Voices

##### Process reverb and chorus

Cache status

---

#### FluidSynth settings

🔧 *Work in Progress*

##### Interpolation

##### Max. Voices (FluidSynth)

##### Process reverb and chorus (FluidSynth)

##### Dynamic sample loading

##### Configuration file

FluidSynth has many more settings to configure. You can use a text file to change those settings. The file must have the following format:

- Empty lines and comment lines starting with a '#' character are ignored.
- Each line contains one setting in the following format: `name` *spaces* `value`.
- Settings with invalid values are ignored.
- Any valid setting will override the foo_midi defaults and the values set in the Preferences.

Refer to [FluidSettings](https://www.fluidsynth.org/api/fluidsettings.xml) for the available settings.

Specify the path to the file on the **[MIDI Player / Paths](#paths)** preferences page. An example file is included in the component directory.

---

#### LibMT32Emu (Munt) settings

##### Resampling (LibMT32Emu)

Determines the quality of the sample rate conversion: Fastest, Fast, Good or Best.

##### Max. Partials

Allows you to override the maximum number of partials playing simultaneously within the emulation session.

> In the MT-32's Linear Arithmetic (LA) synthesis, each tone is made up of up to four partials. These partials can be PCM samples or synthesized waveforms, and they are mixed together to form a complete sound.

##### Analog Output Mode

The original MT-32 had analog components (like filters and amplifiers) that subtly colored the sound. This setting affects the final coloration and warmth of the sound, especially when aiming for an authentic retro audio experience.

It is especially important if you're trying to replicate the exact sound of classic DOS games or MIDI compositions as they were heard in the late '80s and early '90s.

| Name     | Description |
| -------- | ----------- |
| Digital&nbsp;Only | Only the digital path is emulated. The output samples correspond to the digital signal at the DAC entrance. |
| Coarse       | Coarse emulation of the LPF circuit. High frequencies are boosted, sample rate remains unchanged. |
| Accurate     | Finer emulation of the LPF circuit. Output signal is upsampled to 48 kHz to allow emulation of audible mirror spectra above 16 kHz, * which is passed through the LPF circuit without significant attenuation. |
| Oversampled  | Same as Accurate but the output signal is 2x oversampled, i.e. the output sample rate is 96 kHz. This makes subsequent resampling easier. Besides, due to nonlinear passband of the LPF emulated, it takes fewer number of MACs  compared to a regular LPF FIR implementations. |

##### DAC Input Mode

The DAC (Digital-to-Analog Converter) in the original MT-32 had specific limitations and nonlinearities.

The library emulates this behavior, and the DAC Input Mode determines how audio is prepared before it's "sent" to the virtual DAC.

| Name         | Description |
| ------------ | ----------- |
| Nice         | Produces samples at double the volume, without tricks. Higher quality than the real devices. |
| Pure         | Produces samples that exactly match the bits output from the emulated LA32. Much less likely to overdrive than any other mode. Half the volume of any of the other modes. |
| Generation&nbsp;1 | Re-orders the LA32 output bits as in early generation MT-32s. |
| Generation&nbsp;2 | Re-orders the LA32 output bits as in later generations MT-32s. |

##### Reverb

Enables or disable reverb generation.

##### Nice Amp Ramp

On real Roland MT-32 hardware, sudden changes in volume or expression can cause abrupt "jumps" in amplitude. This setting makes these changes gradual, resulting in a more natural and musical sound. This is particularly useful when MIDI tracks include quick volume fades or expression shifts.

> [!Note]
> This is a quality improvement that sacrifices emulation accuracy.

##### Nice Panning

The original MT-32 had some idiosyncrasies in how it handled panning. This feature mimics those subtleties for a more authentic sound. Instead of abrupt left/right shifts when a MIDI track changes pan values, "Nice Panning" makes these transitions more gradual and natural. Instruments sound more like they're moving across a stereo field, rather than jumping from one side to the other.

##### Nice Partial Mixing

The original MT-32 had subtle nonlinearities and quirks in how it mixed partials. This setting mimics those characteristics. It ensures that the partials are mixed in a way that avoids harsh digital artifacts or unnatural volume spikes.
Especially noticeable in complex tones like orchestral instruments or layered synths, where multiple partials interact.

##### Reverse Stereo

Early MT-32 units (especially those with ROM version 1.07 or earlier) are known to have reversed stereo channels compared to modern expectations.

Enable this setting to reverse the channels.

##### GM Set

Determines which General MIDI configuration is used by the player: Roland or King's Quest 6.

---

## Troubleshooting

🔧 *Work in Progress*

---

## FAQs

**Q:** Common question 1  
**A:** Clear, concise answer.

**Q:** Common question 2  
**A:** Clear, concise answer.

---

## Reference Material

This chapter contains a lot of reference material I consulted during the development of foo_midi.

### foobar2000

- [foobar2000 Development](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Development:Overview)

### Windows User Interface

- [Desktop App User Interface](https://learn.microsoft.com/en-us/windows/win32/windows-application-ui-development)
- [Windows User Experience Interaction Guidelines](https://learn.microsoft.com/en-us/windows/win32/uxguide/guidelines)
- [Windows Controls](https://learn.microsoft.com/en-us/windows/win32/controls/window-controls)
- [Control Library](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info)
- [Resource-Definition Statements](https://learn.microsoft.com/en-us/windows/win32/menurc/resource-definition-statements)
- [Visuals, Layout](https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-layout)

### Electronic Music

- [Electronic Music Wiki](https://electronicmusic.fandom.com/wiki/Main_Page)
- [File format samples](https://telparia.com/fileFormatSamples/)

### SoundFonts

- [Musical Artifacts](https://musical-artifacts.com/artifacts?tags=soundfont)
- [SoundFont Bank Offset](https://fluid-dev.nongnu.narkive.com/Fv2kQsxA/new-bank-offset-feature-and-selecting-sfont-bank-preset)
- [SoundFont Spec Test](https://github.com/mrbumpy409/SoundFont-Spec-Test)

### MIDI

- [Introduction to Computer Music: MIDI](https://cmtext.indiana.edu/MIDI/chapter3_MIDI.php), Jeffrey Hass
- [MIDI is the language of the gods](http://midi.teragonaudio.com/), Teragon Audio
  - [Running Status](http://midi.teragonaudio.com/tech/midispec/run.htm)
  - [Device Name](http://midi.teragonaudio.com/tech/midifile/port.htm)
  - [Obsolete Meta Events](http://midi.teragonaudio.com/tech/midifile/obsolete.htm)
- [Comparison of MIDI standards](https://en.wikipedia.org/wiki/Comparison_of_MIDI_standards), Wikipedia
- [Yamaha XG Programming](http://www.studio4all.de/htmle/frameset090.html), Studio 4 All
- [The Unofficial Yamaha Keyboard Resource Site Articles](http://www.jososoft.dk/yamaha/articles.htm)
- [Recording Blogs](https://www.recordingblogs.com/wiki/musical-instrument-digital-interface-midi)
  - [Status Byte](https://www.recordingblogs.com/wiki/status-byte-of-a-midi-message)
  - [Voice Messages](https://www.recordingblogs.com/wiki/midi-voice-messages)
  - [System Common Messages](https://www.recordingblogs.com/wiki/midi-system-common-messages)
  - [System Real-time Messages](https://www.recordingblogs.com/wiki/midi-system-realtime-messages)
  - [Meta Messages](https://www.recordingblogs.com/wiki/midi-meta-messages)
  - [Standards in Music](https://www.recordingblogs.com/wiki/standards-in-music-index)
- [Notes about MIDI Specifications](https://www.lim.di.unimi.it/IEEE/MIDI/INDEX.HTM), Goffredo Haus, IEEE Computer Society Press, Fall 1995
  - [Channel Mode Messages](https://www.lim.di.unimi.it/IEEE/MIDI/SOT0.HTM)
  - [Channel Voice Messages](https://www.lim.di.unimi.it/IEEE/MIDI/SOT1.HTM)
  - [System Common Messages](https://www.lim.di.unimi.it/IEEE/MIDI/SOT2.HTM)
  - [System Real-time Messages](https://www.lim.di.unimi.it/IEEE/MIDI/SOT3.HTM)
  - [System Exclusive Messages](https://www.lim.di.unimi.it/IEEE/MIDI/SOT4.HTM)
  - [Additional Explanations and Applications Notes](https://www.lim.di.unimi.it/IEEE/MIDI/SOT5.HTM)
  - [Instructions for MIDI Implementation Chart](https://www.lim.di.unimi.it/IEEE/MIDI/SOT6.HTM)
- [Somascape](http://www.somascape.org/)
  - [General MIDI (GM) Instrument and Drum Mapping](http://www.somascape.org/midi/basic/gmins.html)
  - [Guide to the MIDI Software Specification](http://www.somascape.org/midi/tech/spec.html)
  - [MIDI Files Specification](http://www.somascape.org/midi/tech/mfile.html)

### SFList JSON

- [SFList JSON](https://gist.github.com/kode54/a7bb01a0db3f2e996145b77f0ca510d5)

### IFF

- ["EA IFF 85" Standard for Interchange Format Files](https://www.martinreddy.net/gfx/2d/IFF.txt)
- [Track Properties](https://www.mediamonkey.com/sw/webhelp/frame/index.html?abouttrackproperties.htm)
- [RIFF Info Tags](https://exiftool.org/TagNames/RIFF.html#Info)

### Apogee Expanded MIDI (EMIDI)

- [Apogee Expanded MIDI (EMIDI) API v1.1](https://moddingwiki.shikadi.net/wiki/Apogee_Expanded_MIDI)

### HMP / HMI

- [ZDoom HMP](https://zdoom.org/wiki/HMP)
- [WildMIDI](https://github.com/Mindwerks/wildmidi/issues/75)

### RMI

- [RIFF RMI](https://www.loc.gov/preservation/digital/formats/fdd/fdd000120.shtml)
- Embedded Soundfonts
  - [About RMIDI](https://github.com/spessasus/SpessaSynth/wiki/About-RMIDI)
  - [SF2 RMIDI Format Extension Specification](https://github.com/spessasus/sf2-rmidi-specification)
  - [Proposal to support rmid files with embedded SF2 soundfonts](https://www.un4seen.com/forum/?topic=20475.0)
  - FluidSynth [Support for SF2 RMIDI files with embedded soundfonts](https://github.com/FluidSynth/fluidsynth/issues/1356)

### XMF (Extensible Music Format)

- [Media Type](https://www.rfc-editor.org/rfc/rfc4723.html)
- [MIDI Manufacturers Association Tech Specs & Info](https://web.archive.org/web/20080618001530/http://www.midi.org/techspecs/index.php)
- [Library of Congress](https://www.loc.gov/preservation/digital/formats/fdd/fdd000121.shtml)
- [FileFormats](http://fileformats.archiveteam.org/wiki/Extensible_Music_Format)
- [MultimediaWiki](https://wiki.multimedia.cx/index.php/Extensible_Music_Format_(XMF))
- [Introducing the Interactive XMF Audio File Format](https://www.gamedeveloper.com/audio/introducing-the-interactive-xmf-audio-file-format)
- [XmfExtractor](https://github.com/benryves/XmfExtractor)

### BASS MIDI

- [Documentation](https://www.un4seen.com/doc/)
- [MIDI Implementation Chart](https://www.un4seen.com/doc/#bassmidi/implementation.html)

### FluidSynth

- [FluidSynth User Manual](https://github.com/FluidSynth/fluidsynth/wiki/UserManual)
- [FluidSynth SoundFont](https://github.com/FluidSynth/fluidsynth/wiki/SoundFont)
- [FluidSynth Documentation](https://github.com/FluidSynth/fluidsynth/wiki/Documentation)
- Development
  - FluidSynth [GitHub](https://github.com/FluidSynth/fluidsynth)
  - [FluidSynth Developer Documentation](https://www.fluidsynth.org/api/index.html)

### SC-55

- EmuSC, [GitHub](https://github.com/skjelten/emusc)
- Nuked SC-55, Nuke-YKT [GitHub](https://github.com/nukeykt/Nuked-SC55)
- Nuked SC-55, J.C. Moyer [GitHub](https://github.com/jcmoyer/Nuked-SC55)
- Nuked SC-55, Falcosoft [GitHub](https://github.com/Falcosoft/Nuked-SC55.git)
- Nuked SC-55 GUI Float, [GitHub](https://github.com/linoshkmalayil/Nuked-SC55-GUI-Float)

### FMMIDI (yuno)

- [fmmidi](https://web.archive.org/web/20120823072908/http://milkpot.sakura.ne.jp/fmmidi/index.html)

### VST

- Hosts
  - [How To Make Your Own VST Host](https://teragonaudio.com/article/How-to-make-your-own-VST-host.html) `VST 2`
  - [MrsWatson](https://github.com/teragonaudio/MrsWatson), A command-line VST plugin host `VST 2`
  - Steinberg [AudioHost](https://github.com/steinbergmedia/vst3_public_sdk/tree/master/samples/vst-hosting/audiohost) `VST 3`
  - [A Minimal VST 3 Host in C](https://stackoverflow.com/questions/24478173/build-a-minimal-vst3-host-in-c) `VST 3`
  - [How To Instantiate a VST 3 Plug-in in Code?](https://stackoverflow.com/questions/55640822/how-to-instantiate-a-vst3-plugin-in-code-for-a-vst3-host-app) `VST 3`
  - [EasyVst](https://github.com/iffyloop/EasyVst) `VST 3`
- Plug-Ins
  - Synthesizers
    - [Yamaha S-YXG50 Portable VSTi v1.0.0](https://veg.by/en/projects/syxg50/) `VST 2`
    - [OPL3GM_VSTi](https://github.com/datajake1999/OPL3GM_VSTi) `VST 2`
    - [Firefly-Synth](https://github.com/sjoerdvankreel/firefly-synth), Sjoer van Kreel, Semi-modular synthesizer and FX plugin for Windows, Linux and Mac, VST3 and CLAP `VST 3`
    - [Firefly-Synth 2](https://github.com/sjoerdvankreel/firefly-synth-2), Sjoer van Kreel, Yet another VST3 and CLAP plugin, Windows, Linux, and MacOS `VST 3`
    - [FluidSynth VST](https://github.com/AZSlow3/FluidSynthVST) `VST 3`
    - [OS-251](https://github.com/utokusa/OS-251), utokusa `VST 3`
    - [RdPiano](https://github.com/giulioz/rdpiano) `VST 3`
    - [Virtual JV](https://github.com/giulioz/jv880_juce), JV-880 emulator as VST/AU plugin `VST 3`
    - [VSTSID](https://github.com/igorski/VSTSID), VST plugin version of the WebSID Commodore 64 synthesizer `VST 3`
- Development
  - [VST 3 Developer Portal](https://steinbergmedia.github.io/vst3_dev_portal/pages/index.html)
  - [VST 3 API Documentation](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/API+Documentation/Index.html)
  - [VST 3 C API](https://github.com/steinbergmedia/vst3_c_api/tree/master)
  - [VST 3 SDK](https://github.com/steinbergmedia/vst3sdk)
  - [VST 3 Example Plugin Hello World](https://github.com/steinbergmedia/vst3_example_plugin_hello_world) `VST 3`

### CLAP

- [CLAP Audio Software Database](https://clapdb.tech/)
- Development
  - [Getting Started](https://cleveraudio.org/developers-getting-started/)
- Hosts
  - [BaconPaul's Test microhost](https://github.com/baconpaul/micro-clap-host)
  - [The Anklang Project](https://anklang.testbit.eu/)
    - [Anklang](https://github.com/tim-janik/anklang)
    - [CLAP Host](https://github.com/free-audio/clap-host), Free-Audio
- Plug-ins
  - [Dexed](https://asb2m10.github.io/dexed/), [GitHub](https://github.com/asb2m10/dexed)
  - [Nuked SC-55 CLAP](https://github.com/johnnovak/Nuked-SC55-CLAP), [Roland SC-55](https://en.wikipedia.org/wiki/Roland_SC-55) emulator, John Novak
    - Nuked SC-55, [Vogons announcement](https://www.vogons.org/viewtopic.php?t=99447), 2024-03-14
  - [Odin 2](https://thewavewarden.com/pages/odin-2), 24-voice polyphonic powerhouse, [The WaveWarden](https://thewavewarden.com/), [GitHub](https://github.com/TheWaveWarden/odin2)
  - [SW10-Plug](https://github.com/djtuBIG-MaliceX/sw10_plug), Casio SW-10
  - [JV880-Juce](https://github.com/giulioz/jv880_juce)
  - [Mini-JV880](https://github.com/giulioz/mini-jv880)
  - [OctaSince](https://github.com/greatest-ape/OctaSine), Joakim Frostegård
  - Development
    - [CLAP Plug-ins](https://github.com/free-audio/clap-plugins), Free-Audio

---

## History

The history of foo_midi development is available in a separate [document](History.md).

---

## Acknowledgements and Credits

- Peter Pawlowski for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
- [kode54](https://gitlab.com/kode54/) for the original [foo_midi](https://gitlab.com/kode54/foo_midi) component.
- [Un4seen Developments](https://www.un4seen.com/) for the BASS audio library.
- [Munt](https://github.com/munt/munt/) for a multi-platform software synthesiser emulating pre-GM MIDI devices such as the Roland MT-32, CM-32L, CM-64 and LAPC-I.
- [Alexey Khokholov (Nuke.YKT)](http://nukeykt.retrohost.net/) for [Nuked OPL3](https://github.com/nukeykt/Nuked-OPL3).
- [Vitaly Novichkov](https://github.com/Wohlstand) for [libADLMIDI](https://github.com/Wohlstand/libADLMIDI), [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) and [libEDMIDI](https://github.com/Wohlstand/libEDMIDI).
- [Mitsutaka Okazaki](https://github.com/Wohlstand/scc) for Emu de MIDI.
- [arch21](https://hydrogenaud.io/index.php?action=profile;u=123058) for testing and pointing me in the right direction with Secret Sauce and SF3 SoundFonts.
- Tom Moebert for [FluidSynth](https://www.fluidsynth.org/).
- [Valley Bell](https://github.com/ValleyBell) for [MidiConverters](https://github.com/ValleyBell/MidiConverters).
- [Jean-loup Gailly](http://gailly.net/) and [Mark Adler](http://en.wikipedia.org/wiki/Mark_Adler) for [zlib](https://www.zlib.net/).
- [Spessasus](https://github.com/spessasus) for testing, advice and [SpessaSynth](https://github.com/spessasus/SpessaSynth).
- [Zoltán Bacskó](https://github.com/Falcosoft) for testing, advice and [MIDI Player](https://www.vogons.org/viewtopic.php?f=5&t=48207).
- [Murachue](https://murachue.sytes.net/web/) for [MMFTool](https://murachue.sytes.net/web/softlist.cgi?mode=desc&title=mmftool).
- [yuno (Yoshio Uno)](yuno@users.sourceforge.jp) for [fmmidi](http://milkpot.sakura.ne.jp/fmmidi/).
- [John Novak](https://github.com/johnnovak) for [Nuked-SC55-CLAP](https://github.com/johnnovak/Nuked-SC55-CLAP).

---

## Support

For further assistance:

🌐 Home page: [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)  
🌐 Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)  
🌐 Forum: [Hydrogenaudio foo_midi (foobar v2.0)](https://hydrogenaud.io/index.php/topic,123301.25.html)
