
# libmidi

[libmidi](https://github.com/stuerp/libmidi) is a library to read and write MIDI sequences. It is heavily based on [kode54](https://gitlab.com/kode54)'s [midiprocessing]() library.

WARNING: This code is very tightly coupled with foo_midi. The API and the implementation will change several times before it becomes stable.

## Features

## Requirements

- Tested on Microsoft Windows 10 and later.

## Getting started

## Developing

To build the code you need:

- [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later

## Change Log

v0.1.1.0, 2026-xx-xx

- Added: Support for MMD98 (MIDI Music Driver) files.
- Improved: Stricter interpretation of the RCP mute mode that prevents an RCP track from being included in the MIDI stream.

v0.1.0.0, 2025-03-19

- Initial release.

## Acknowledgements / Credits

- [kode54](https://gitlab.com/kode54) for the original [midiprocessing]() library.
- Jean-loup Gailly and Mark Adler for [zlib](http://www.zlib.net/).

## Reference Material

- [The Wonderfully Terrible World of C and C++ Encoding APIs (with Some Rust)](https://thephd.dev/the-c-c++-rust-string-text-encoding-api-landscape)
- [cuneicode, and the Future of Text in C](https://thephd.dev/cuneicode-and-the-future-of-text-in-c)

## Test Material

## Links

- Home page: [https://github.com/stuerp/libmidi](https://github.com/stuerp/libmidi)
- Repository: [https://github.com/stuerp/libmidi.git](https://github.com/stuerp/libmidi.git)
- Issue tracker: [https://github.com/stuerp/libmidi/issues](https://github.com/stuerp/libmidi/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
