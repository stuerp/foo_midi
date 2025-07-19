
# foo_midi

[foo_midi](https://github.com/stuerp/foo_midi/releases) is a [foobar2000](https://www.foobar2000.org/) component that adds playback of MIDI files to foobar2000.

It is based on [foo_midi](https://gitlab.com/kode54/foo_midi) by [kode54](https://gitlab.com/kode54).

## Features

- Decodes General MIDI files (.MID, .MIDI, .RMI, .KAR) and several MIDI based formats. (.MIDS, .MDS, .HMI, .HMP, .MUS, .XMI, .XFM, .LDS, .RCP, .R36, .G18, .G36, .XMF/.MXMF, .MMF).
- Supports several synthesizers, several of which do not require any additional files to play back music. The bundled synthesizers which do not require additional files may sound rather basic, though.
- Supports FluidSynth SoundFont (.sf2) based synthesizer, including support for the newer compressed format. (.sf3). SoundFonts may be loaded in a simple, or even complex setup, using either basic .sflist text files encoded in UTF-8 format, but for now, it only supports a bare list of files.
- Supports 32 and 64-bit VST instruments.
- Supports dark mode.

## Requirements

- [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)
- Tested on Microsoft Windows 10 and later.

## Getting started

- Double-click `foo_midi.fbk2-component`.

or

- Import `foo_midi.fbk2-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.

## Usage

You can find the user guide [here](docs/README.md).

## Developing

### Requirements

To build the code you need:

- [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later
- [foobar2000 SDK](https://www.foobar2000.org/SDK) 2025-03-07
- [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320

The following libraries are included in the code:

- [BASS](https://www.un4seen.com/) 2.4.17
  - [BASSFLAC](https://www.un4seen.com/) 2.4.5.5
  - [BASSMIDI](https://www.un4seen.com/) 2.4.15.3
  - [BASSWV](https://www.un4seen.com/) 2.4.7.4
  - [BASSOPUS](https://www.un4seen.com/) 2.4.3.0
  - [BASSMPC](https://www.un4seen.com/) 2.4.1.2
- [FluidSynth](https://github.com/FluidSynth/fluidsynth/) 2.4.7
- [LibADLMIDI](https://github.com/Wohlstand/libADLMIDI) 1.6.0, Yamaha YMF262 and CT1747 (OPL3)
- [LibOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) 1.6.0, Yamaha YM2612 (OPN2) and Yamaha YM2608 (OPNA)
- [LibEMIDI](https://github.com/Wohlstand/libEDMIDI), Yamaha (OPLL), PSG and SCC
- [LibMT32Emu](https://github.com/munt/munt) 2.7.2, Roland MT-32, CM-32L and LAPC-I synthesiser modules
- [Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3), Yamaha YMF262 and CT1747 (OPL3)
- [zlib](https://www.zlib.net/) 1.3.1

To create the deployment package you need:

- [PowerShell 7.2](https://github.com/PowerShell/PowerShell) or later

### Setup

Create the following directory structure:

    3rdParty
        WTL10_10320
    bin
        x86
    foo_midi
    out
    sdk

- `3rdParty/WTL10_10320` contains [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL).
- `bin` contains a portable version of foobar2000 64-bit for debugging purposes.
- `bin/x86` contains a portable version of foobar2000 32-bit for debugging purposes.
- `foo_midi` contains the [Git](https://github.com/stuerp/foo_midi) repository.
- `out` receives a deployable version of the component.
- `sdk` contains the [foobar2000 SDK](https://www.foobar2000.org/SDK).

Issue the following commands:

> git clone --recurse [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)  
> cd foo_midi  
> git submodule update --recursive --init

### Building

Open `foo_midi.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x64 configuration and next the x86 configuration.

## Change Log

- v3.0.0.0, 2025-07-19

- New: Resurrected yuno's fmmidi player.
- New: User Guide [documentation](docs/README.md) (*Work in Progress*)
  - The default Programs definition file is located in the component directory but can be changed in the Preferences dialog.
- New: CLAP host functionality. foo_midi can now use [CLAP ((CLever Audio Plug-in API))](https://u-he.com/community/clap/) MIDI plug-ins to render the audio stream.
  - Specify the directory containing CLAP plug-ins in the **Paths** preferences page.
  - Take a look at [Nuked-SC55-CLAP](https://github.com/johnnovak/Nuked-SC55-CLAP) for an example.
- New: MIDI flavor `None` sends no SysEx messages to the player before playback starts.
- New: **midi_player_ext** info field that contains the name of the active plug-in when a player supports it or the ADL, OPN and Nuked OPL3 emulator core.
- New: FluidSynth can be configured using a configuration file. See [Configuration file](docs/README.md#configuration-file)
- New: Detecting the extra percussion channel can be turned on or off in the Preferences.
- New: Log level setting determines which foo_midi messages are written to the foobar2000 console.
- Improved: Stricter interpretation of the RCP mute mode that prevents an RCP track from being included in the MIDI stream.
- Improved: Updated LibMT32Emu to v2.7.2.
- Improved: LibADLMIDI configuration
  - Moved to separate **FM Synthesis** page. All the advanced settings can now be set here.
  - Added Opal, Java, ESFMu, MAME OPL2, YMFM OPL2, YMFM OPL3, Nuked OPL2 LLE and Nuked OPL3 LLE as selectable emulator cores.
  - Fixed sorting in bank names combobox.
- Improved: LibOPNMIDI configuration
  - Moved to separate **FM Synthesis*- page. All the advanced settings can now be set here.
- Improved: Rewrote MuntPlayer to use LibMT32Emu API version 3.
  - Supports [Versioned ROMs](https://github.com/dosbox-staging/dosbox-staging/wiki/MIDI#mt-32-roms).
  - Samples are rendered as floating-point numbers.
  - Limitation: Most of options are still hardcoded.
- Improved: Added a couple of MT32Emu settings.
- Improved: ADL player uses LibADLMIDI 1.6.0 and 64-bit floating-point samples.
- Improved: OPN player uses LibOPNMIDI 1.6.0 and 64-bit floating-point samples. WOPN bank can be set from a file.
- Improved: Added setting to reverse the stereo channels of the LibMT32Emu player (default is on).
- Improved: Added enable or disable reverb processing by the LibMT32Emu player (default is on).
- Improved: Skip to First Note, Loop Count and Fade-Out Time can be set on the main Preferences page.
- Improved: Lots of little fixes, code cleanup and code polishing.
- Improved: VSTi player issues a warning whenever it's host executable `vshost32.exe` and `vshost64.exe` can't be found. Some releases of MS Defender are known to delete the file.
- Changed Moved BASS MIDI, FluidSynth and LibMT32Emu settings to new **Wavetable** preference page.
- Changed: All advanced preferences have been moved to a Preferences page. The current values will **not** be migrated.
- Changed: Loop mode descriptions should better reflect their functionality.
- Fixed: Preferences dialog should adapt to High DPI settings now.
- Fixed: An old threading issue caused by allowing the MIDI channels to be enabled or disabled during playback.
- Fixed: The LibMT32Emu player reversed the left and the right channels in the output.
- Fixed: Changing the channel mask setting did not always stick.

You can read the full history [here](docs/History.md).

## Acknowledgements / Credits

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

## Links

ðŸŒ Home page: [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)  
ðŸŒ Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)  
ðŸŒ Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)  

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
