
# foo_midi

[foo_midi](https://github.com/stuerp/foo_midi/releases) is a [foobar2000](https://www.foobar2000.org/) component that adds playback of MIDI files to foobar2000.

It is based on [foo_midi](https://gitlab.com/kode54/foo_midi) by [kode54](https://gitlab.com/kode54).

## Features

* Decodes General MIDI files (.MID, .MIDI, .RMI, .KAR) and several MIDI based formats. (.MIDS, .MDS, .HMI, .HMP, .MUS, .XMI, .LDS).
* Supports several synthesizers, several of which do not require any additional files to play back music. The bundled synthesizers which do not require additional files may sound rather basic, though.
* Supports FluidSynth SoundFont (.sf2) based synthesizer, including support for the newer compressed format. (.sf3). SoundFonts may be loaded in a simple, or even complex setup, using either basic .sflist text files encoded in UTF-8 format, but for now, it only supports a bare list of files.
* Supports 32 and 64-bit VST instruments.
* Supports dark mode.

## Requirements

* Tested on Microsoft Windows 10 and later.
* [foobar2000](https://www.foobar2000.org/download) v1.6.16 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)

## Getting started

* Double-click `foo_midi.fbk2-component`.

or

* Import `foo_midi.fbk2-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.

## Developing

To build the code you need:

* [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later
* [foobar2000 SDK](https://www.foobar2000.org/SDK) 2023-05-10
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320

The following libraries are included in the code:

* [BASS](https://www.un4seen.com/) 2.4.17
  * [BASSFLAC](https://www.un4seen.com/) 2.4.5.3
  * [BASSMIDI](https://www.un4seen.com/) 2.4.14.1
  * [BASSWV](https://www.un4seen.com/) 2.4.7.3
  * [BASSOPUS](https://www.un4seen.com/) 2.4.2.3
  * [BASSMPC](https://www.un4seen.com/) 2.4.1.2
* [LibADLMIDI](https://github.com/Wohlstand/libADLMIDI) 1.5.1, Yamaha YMF262 (OPL3)
* [LibOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) 1.5.1, Yamaha YM2612 (OPN2)
* [Nuke.YKT](http://nukeykt.retrohost.net/)
  * [WinOPL3Driver](https://github.com/nukeykt/WinOPL3Driver)
  * [Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3) Yamaha YMF262 and CT1747 (OPL3)
  * [Nuked-OPLL](https://github.com/nukeykt/Nuked-OPLL) Yamaha YM2413 and VRC7 (OPLL)
  * [Nuked-OPM](https://github.com/nukeykt/Nuked-OPM) Yamaha YM2151
  * [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2) Yamaha YM3438 (YM2612) 1.0.9
  * [Nuked-OPNB](https://github.com/nukeykt/Nuked-OPNB) Yamaha YM2610
  * [Nuked-PSG](https://github.com/nukeykt/Nuked-PSG) Yamaha YM7101
* [emu2149](https://github.com/digital-sound-antiques/emu2149) Yamaha YM2149 (PSG)
* [emu2212](https://github.com/digital-sound-antiques/emu2212) Konami SCC
* [emu8950](https://github.com/digital-sound-antiques/emu8950) Yamaha Y8950, YM3526 and YM3812
* [emu76489](https://github.com/digital-sound-antiques/emu76489) SN76489
* [Munt win32drv](https://github.com/munt/munt/releases/tag/mt32emu_win32drv_1_8_1) Roland MT-32, CM-32L and LAPC-I synthesiser modules 1.8.1
* [FluidSynth](https://github.com/FluidSynth/fluidsynth/) 2.3.2

To create the deployment package you need:

* [PowerShell 7.2](https://github.com/PowerShell/PowerShell) or later

### Setup

Create the following directory structure:

    3rdParty
        WTL10_10320
    bin
        x86
    foo_midi
    out
    sdk

* `3rdParty/WTL10_10320` contains WTL 10.0.10320.
* `bin` contains a portable version of foobar2000 64-bit for debugging purposes.
* `bin/x86` contains a portable version of foobar2000 32-bit for debugging purposes.
* `foo_midi` contains the [Git](https://github.com/stuerp/foo_midi) repository.
* `out` receives a deployable version of the component.
* `sdk` contains the foobar2000 SDK.

### Building

Open `foo_midi.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Contributing

If you'd like to contribute, please fork the repository and use a feature
branch. Pull requests are warmly welcome.

## Change Log

v2.9.0.0, 2023-08-xx, *""*

* New: Added FluidSynth player again.
  * It can be selected after setting the path to the directory that contains the FluidSynth libraries. You can download FluidSynth [here](https://github.com/FluidSynth/fluidsynth/releases). Make sure you download the version that has the same CPU architecture as foobar2000 (x64 or x86).
* New: Added .XFM as an alternative file extension for XMI files.
* Changed: Renamed dynamic info tags *bassmidi_voices* and *bassmidi_voices_max* to *midi_active_voices* and *midi_peak_voices*. The FluidSynth player also sets those tags while playing.
* Bug Fix: Some XMI files could not be read.

TODO: Does FluidSynth use all CC's?
TODO: Does FluidSynth switch between GM, GS and XG?

v2.8.5.0, 2023-07-23, *""*

* New: Added a configuration option to always use Super Munt when playing an MT-32 file. Default is on.
* New: Added an configuration option to always use a VSTi to play an XG file. Default is off.
  * Don't forget to specify the path name of the VSTi in the Advanced preferences.
* Bug Fix: Saving MIDI presets was broken.
* Bug Fix: Loop detection was broken for some files.

v2.8.4.0, 2023-06-26, *"Beat the Drum"*

* New: A new Info tag "MIDI_PLAYER" contains the name of the MIDI player playing the current track.
* New: Added detection of an extra percussion channel in Standard MIDI Files (*.MID). When meta data messages of type Text, Track Name or Instrument Name containing the word "drum" preceed the first message for channel 16 it will be used as an extra percussion channel in addition to channel 10.
* New: A new Info tag "MIDI_EXTRA_PERCUSSION_CHANNEL" contains the (1-based) number of the channel that acts as an additional percussion channel (only channel 16 for now).
* Improved: Error reporting in general and in case of parsing MIDI files with problems.
* Bug Fix: Playing loops did not always work. (Regression)

v2.8.3.1, 2023-06-03, *"Do you want lyrics with that? Redux"*

* Bug Fix: The user selection of the MIDI player was no longer honored. (Regression) A SOLID bug...

v2.8.3.0, 2023-06-01, *"Do you want lyrics with that?"*

* Cue Marker, Lyrics, Time Signature and Key Signature meta events are now converted to tags.
  * Soft Karaoke lyrics are stored in an SYNCEDLYRICS tag; other lyrics in a LYRICS tag. The tags can be edited but will not be written back to the MIDI file.
  * Info tag MIDI_LYRICS_TYPE contains the name of the Karaoke standard. For now, only Soft Karaoke format is recognized.
* Bug Fix: vshost process was not stopped when checking for presets. (Regression)

v2.8.2.0, 2023-05-20, *"Spring Cleaning"*

* Bug Fix: MIDI files with malformed tracks caused a crash.
* Bug Fix: MIDI files with malformed SysEx events caused a crash.
* Bug Fix: Work-around for weird rendering problem in Dark mode.
* Bug Fix: Restored access to the Preferences page from the Decoding page in the Preferences dialog.
* Improved: Tweaked the size of some of the labels of the Preferences dialog.
* Improved: Added a message to re-open the Preferences dialog after any of the paths were changed.
* Improved: Changed Shift-JIS detection in meta data. A copyright sign (©) was interpreted as Shift-JIS.

v2.8.1.0, 2023-05-01, *"A New Beginning...? Redux"*

* Bug Fix: The dialog now properly resizes on systems with High DPI settings (> 100% scaling)

v2.8.0.0, 2023-04-30, *"A New Beginning...?"*

* Major refactoring of the source code.
* Builds with foobar2000 SDK 2023-04-18.
* Tried to make the preferences page a bit more accessible to new users.
  * Moved all the path controls to a separate preferences page.
  * Added the configuration of the VTSi plugin path to the preferences page (in addition of the Advanced Preferences, for backwards compatibility).
  * Added the configuration of the Secret Sauce path to the preferences page (in addition of the Advanced Preferences, for backwards compatibility).

v2.7.4.4, 2022-11-21, *"I'm SoundFont of it"*

* Bug Fix: The new scpipe32 and scpipe64 in the previous version had issues. Secret Sauce is back.
* Added support for compressed SoundFonts (.sf3) to BASS MIDI player.
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
* Tom Moebert for [FluidSynth](https://www.fluidsynth.org/)

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

### SoundFonts

* [Musical Artifacts](https://musical-artifacts.com/artifacts?tags=soundfont)

### FluidSynth

* [FluidSynth Documentation](https://github.com/FluidSynth/fluidsynth/wiki/Documentation)

### MIDI

#### XMI

* [XMI Format](https://moddingwiki.shikadi.net/wiki/XMI_Format)

## Links

* Home page: [https://github.com/stuerp/foo_midi](https://github.com/stuerp/foo_midi)
* Repository: [https://github.com/stuerp/foo_midi.git](https://github.com/stuerp/foo_midi.git)
* Issue tracker: [https://github.com/stuerp/foo_midi/issues](https://github.com/stuerp/foo_midi/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
