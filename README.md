
# foo_midi

[foo_midi](https://github.com/stuerp/foo_midi/releases) is a [foobar2000](https://www.foobar2000.org/) component that adds playback of MIDI files to foobar2000.

It is based on [foo_midi](https://gitlab.com/kode54/foo_midi) by [kode54](https://gitlab.com/kode54).

## Features

* Supports foobar2000 2.0
* 32-bit and 64-bit version
* Supports dark mode.

## Requirements

* Microsoft Windows 10 or later
* [foobar2000](https://www.foobar2000.org/download) v2.0 or later

## Getting started

* Double-click *foo_midi.fbk2-component*.
* Import *foo_midi.fbk2-component* into foobar2000 using "File / Preferences / Components / Install...".

## Developing

The code builds out-of-the box with Visual Studio.

### Requirements

* [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/)
* [foobar2000 SDK](https://www.foobar2000.org/SDK)
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320

### Setup

Create the following directory structure:

    3rdParty
        WTL10_10320
    bin
    foo_midi
    out
    sdk

* `3rdParty/WTL10_10320` contains WTL 10.0.10320.
* `bin` contains a portable version of foobar2000 for debugging purposes.
* `foo_midi` contains the [Git](https://github.com/stuerp/foo_midi) repository.
* `out` receives a deployable version of the component.
* `sdk` contains the foobar2000 SDK.

### Building

Open *foo_midi.sln* with Visual Studio and build the solution.

## Contributing

If you'd like to contribute, please fork the repository and use a feature
branch. Pull requests are warmly welcome.

## Change Log

v2.7.4, 2022-10-03, *"Scratchin' the itch"*

* Initial release of x64 version for [foobar2000](https://www.foobar2000.org/) v2.0.

## Acknowledgements / Credits

* Peter Pawlowski, for the  [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [kode54](https://gitlab.com/kode54), for the original [foo_midi](https://gitlab.com/kode54/foo_midi) component.

## Links

* Home page: https://github.com/stuerp/foo_midi
* Repository: https://github.com/stuerp/foo_midi.git
* Issue tracker: https://github.com/stuerp/foo_midi/issues

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
