
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

* Tested on Microsoft Windows 10 or later.
* [foobar2000](https://www.foobar2000.org/download) v1.6.13 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)

## Getting started

* Double-click `foo_midi.fbk2-component`.

or

* Import `foo_midi.fbk2-component` into foobar2000 using "File / Preferences / Components / Install...".

## Developing

The code builds out-of-the box with Visual Studio.

### Requirements

To build the code you need:

* [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later
* [foobar2000 SDK](https://www.foobar2000.org/SDK) 2022-10-20
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320
* [BASS](https://www.un4seen.com/) 2.4.17
* [BASSFLAC](https://www.un4seen.com/) 2.4.5
* [BASSMIDI](https://www.un4seen.com/) 2.4.14.1
* [BASSWV](https://www.un4seen.com/) 2.4.7.3
* [BASSOPUS](https://www.un4seen.com/) 2.4.2.1
* [BASSMPC](https://www.un4seen.com/) 2.4.1.2
* [LibADLMIDI](https://github.com/Wohlstand/libADLMIDI) 1.5.1

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

* Peter Pawlowski, for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [kode54](https://gitlab.com/kode54), for the original [foo_midi](https://gitlab.com/kode54/foo_midi) component.
* [Munt](https://munt.sourceforge.net/), for a multi-platform software synthesiser emulating (somewhat inaccurately) pre-GM MIDI devices such as the Roland MT-32, CM-32L, CM-64 and LAPC-I.
* [arch21](https://hydrogenaud.io/index.php?action=profile;u=123058) for testing and pointing me in the right direction with Secret Sauce and SF3 SoundFonts.

## Reference Material

* foobar2000
  * [foobar2000 Development](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Development:Overview)

* Windows User Interface
  * [Desktop App User Interface](https://learn.microsoft.com/en-us/windows/win32/windows-application-ui-development)
  * [Windows User Experience Interaction Guidelines](https://learn.microsoft.com/en-us/windows/win32/uxguide/guidelines)
  * [Windows Controls](https://learn.microsoft.com/en-us/windows/win32/controls/window-controls)
  * [Control Library](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info)
  * [Resource-Definition Statements](https://learn.microsoft.com/en-us/windows/win32/menurc/resource-definition-statements)
  * [Visuals, Layout](https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-layout)

## Links

* Home page: https://github.com/stuerp/foo_midi
* Repository: https://github.com/stuerp/foo_midi.git
* Issue tracker: https://github.com/stuerp/foo_midi/issues

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
