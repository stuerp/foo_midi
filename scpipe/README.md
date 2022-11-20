
# SCPipe

[SCPipe](https://github.com/stuerp/SCPipe/releases) is a SCCore pipe bridge.

It is based on [SCPipe](https://gitlab.com/kode54/foobar2000/-/tree/main/plugins) by [kode54](https://gitlab.com/kode54).

## Features

* Supports foobar2000 2.0.

## Requirements

* Microsoft Windows 10 or later
* [foobar2000](https://www.foobar2000.org/download) v2.0 or later

## Getting started

SCPipe is part of `foo_midi.fbk2-component`.

## Developing

The code builds out-of-the box with Visual Studio.

### Requirements

To build the code:

* [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/)

To create the deployment package:

* [PowerShell 7.2](https://github.com/PowerShell/PowerShell)

### Setup

Create the following directory structure:

    bin
    SCPipe
    out
    sdk

* `bin` contains a portable version of foobar2000 for debugging purposes.
* `SCPipe` contains the [Git](https://github.com/stuerp/SCPipe) repository.
* `out` receives a deployable version of the plugin.

### Building

Open `SCPipe.sln` with Visual Studio and build the solution.

## Contributing

If you'd like to contribute, please fork the repository and use a feature
branch. Pull requests are warmly welcome.

## Change Log

v1.0.0.0.3, 2022-11-05, *"The Fix"*

* Added compatibility with foobar2000 v2.0.
* Fixed all Visual Studio 2022 build warnings.

v1.0.0.0.2, 2022-02-xx, *"The Original"*

* Original version by [kode54](https://gitlab.com/kode54).

## Acknowledgements / Credits

* Peter Pawlowski, for the  [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [kode54](https://gitlab.com/kode54), for the original [SCPipe](https://gitlab.com/kode54/SCPipe) plugin.

## Links

* Home page: https://github.com/stuerp/SCPipe
* Repository: https://github.com/stuerp/SCPipe.git
* Issue tracker: https://github.com/stuerp/SCPipe/issues

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
