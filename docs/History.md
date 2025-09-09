
# foo_midi History

v3.2.0.0-alpha3, 2025-09-03

- Improved: DLS to SF2 conversion
  - Added Downloadable Sounds filter to the soundfont file dialog.
  - Added work-around for DLS files with incorrect sample chunks (WSMP).
  - Added support for A-Law encoded DLS samples.
  - Added exclusiveClass generator conversion.
  - Fixed incorrect modulator generation for Chorus effect.
  - Fixed improper handling of instrument INFO chunks.
- Fixed: The Log Level drop list was not cleared after a reset.

v3.2.0.0-alpha2, 2025-09-01

- New: Option "Use SecretSauce with GS" to always use the SecretSauce player with a GS MIDI file.
- New: Context menu item to extract an embedded sound font and save it as a DLS collection or SF2 bank.
- Improved: DLS to SF2 conversion
  - BASSMIDI can now use DLS collections as a default sound font or a sound font associated with a MIDI file.
  - Added support for 8-bit DLS samples.
  - Fixed loop offset generators when the DLS sample has no loop.
- Improved: The SecretSauce player keeps trying to play files that cause the host to stop early f.e. because of an unsupported SysEx.

v3.2.0.0-alpha1, 2025-08-30

- New: The BASSMIDI player can now use DLS sound fonts. They are automatically converted to SF2 format before use.
- New: "Use DLS" option for the BASSMIDI player.
- New: "Use DLS (Custom) option for the FluidSynth player. This overrides the built-in DLS support.
- Fixed: Reduced device count of LibADLMIDI/LibOPNMIDI from 3 to 1 to prevent a rare edge case from freezing the sample render loop.
- Fixed: The sound font cache prevented temporary files from being deleted.

v3.1.2.0, 2025-08-17

- New: The VSTi that is used when the [Use VSTi with XG](docs/README.md) is enabled loads its saved configuration before playback starts.

v3.1.1.0, 2025-08-15

- Fixed: Under some conditions the BASSMIDI volume of a soundfont was set to 0.
- Fixed: Some Unicode conversions failed when looking for extra soundfonts. See [STL issue 5093](https://github.com/microsoft/STL/issues/5093). Thanks to [MagikalUnicorn](https://github.com/MagikalUnicorn) for pointing it out.

v3.1.0.0, 2025-08-05

- New: Added SFList support to the FluidSynth player. Due to limitations of the current FluidSynth API only the `fileName` property is used.
- New: Added support for flat text SFList files.
- New: Support for the editor (GUI) of CLAP plug-ins. Try f.e. [OctaSine](https://github.com/greatest-ape/OctaSine) or [Odin 2](https://thewavewarden.com/pages/odin-2/).
- New: LibEDMIDI / Emu de MIDI player now uses [libEDMIDI](https://github.com/Wohlstand/libEDMIDI) by [Wohlstand](https://github.com/Wohlstand).
- Improved: Some extra information in the console when using SFList files.
- Improved: Error handling and reporting when reading soundfont lists.
- Improved: FluidSynth handles DLS embedded in XMF files better.
- Improved: BASSMIDI and FluidSynth only create as many streams or synthesizers as there are ports in the MIDI stream resulting in a faster startup.
- Improved: Support for archives. MIDI files, soundfont files and soundfont list files can be bundled in an archive.
- Improved: Overall hardening and polishing of the CLAP host.
- Improved: Extra error handling when looking for soundfonts. Some UTF-8 conversions can fail.
- Fixed: MIDI flavor **None** did not work completely as intended.
- Fixed: 32-bit ADL and OPN player reported a wrong sample size to their respective libraries resulting in undefined behavior during playback.

v3.1.0.0-alpha6, 2025-08-02

- Improved: Multi-threading support for CLAP.
- Fixed: BASSMIDI soundfont stacking was done in reverse by the new list loader code. (3.0.0.0 regression)

v3.1.0.0-alpha5, 2025-07-31

- New: Support for the editor (GUI) of CLAP plug-ins. Try f.e. [OctaSine](https://github.com/greatest-ape/OctaSine) or [Odin 2](https://thewavewarden.com/pages/odin-2/).
- Improved: Overall hardening and polishing of the CLAP host.
- Fixed: Soundfont with the same name as the directory was not loaded anymore. (alpha4 regression)

v3.1.0.0-alpha4, 2025-07-25

- Improved: Support for archives. MIDI files, soundfont files and soundfont list files can be bundled in an archive.
- Fixed: Only the first soundfont from a text soundfont list was initialized correctly by the BASSMIDI player.
- Fixed: 32-bit ADL and OPN player reported a wrong sample size to their respective libraries resulting in undefined behavior during playback.

v3.1.0.0-alpha3, 2025-07-23

- Changed: Relative soundfont paths should now work in JSON and text soundfont lists.
- Improved: Error handling and reporting when reading soundfont lists.
- Improved: BASSMIDI and FluidSynth only create as many streams or synthesizers as there are ports in the MIDI stream resulting in a faster startup.
- Improved: FluidSynth handles DLS embedded in XMF files better.
- Fixed: MIDI flavor **None** did not work completely as intended.

v3.1.0.0-alpha2, 2025-07-21

- New: Added support for flat text SFList files.

v3.1.0.0-alpha1, 2025-07-21

- New: Added SFList support to the FluidSynth player. Due to limitations of the current FluidSynth API only the `fileName` property is used.
- Improved: Some extra information in the console when using SFList files.

v3.0.0.0, 2025-07-19

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

v3.0.0.0-rc3, 2025-07-17

- Changed: Removed OPN Scale Modulators feature from OPN player initialization. It causes a weird, unwanted effect with some files.

v3.0.0.0-rc2, 2025-07-15

- Fixed: Do a case-insensitive comparison when looking for external files. (Regression)
- Fixed: A couple of preference controls that did not update the Apply button.
- Fixed: Loop count 0 is no longer a valid value.
- Changed: Loop descriptions should better reflect their functionality.

v2.19.0.0-rc1, 2025-07-13

- New: Log level setting determines which foo_midi messages are written to the foobar2000 console.
- Changed: Moved the LibMT32Emu settings to the Wavetable page.
- Improved: VSTi player issues a warning whenever it's host executable `vshost32.exe` and `vshost64.exe` can't be found.
- Fixed: Changing the channel mask setting did not always stick.
- Fixed: The fade range was not recalculated when the player changed to another sample rate. (Regression)
- Fixed: VSTi plug-in name was not always set. (Regression)
- Fixed: A couple of labels were to truncate in High DPI modes.

v2.19.0.0-alpha7, 2025-07-11

- New: Detecting the extra percussion channel can be turned on or off in the Preferences.
- Improved: ADL player uses LibADLMIDI 1.6.0 and 64-bit floating-point samples.
- Improved: OPN player uses LibOPNMIDI 1.6.0 and 64-bit floating-point samples. WOPN bank can be set from a file.
- Improved: Added setting to reverse the stereo channels of the LibMT32Emu player (default is on).
- Improved: Added enable or disable reverb processing by the LibMT32Emu player (default is on).
- Improved: Skip to First Note, Loop Count and Fade-Out Time can be set on the main Preferences page.
- Improved: Lots of little fixes, code cleanup and code polishing.
- Changed: All advanced preferences have been moved to a Preferences page. The current values will **not** be migrated.
- Changed: Renamed `midi_plug_in` tag to `midi_player_ext`. The ADL, OPN and Nuked OPL3 player will set it to the current emulator core.
- Changed Moved BASS MIDI and FluidSynth settings to new Wavetable preference page.
- Fixed: The LibMT32Emu player reversed the left and the right channels in the output.
- Fixed: Crash when looking for the MT-32 ROMs.

v2.19.0.0-alpha6, 2025-07-05

- New: **midi_plug_in** info field that contains the name of the active plug-in when a player supports it.
- New: FluidSynth can be configured using a configuration file. See [Configuration file](docs/README.md#configuration-file)
- Improved: Support for Unicode paths.
- Improved: Hardened the CLAP Host.
- Improved: Moved MT32Emu settings to FM Synthesis preferences page and added a couple of new settings.
- Fixed: Some startup and sample rate related issues.

v2.19.0.0-alpha5, 2025-07-02

- Fixed: CLAP Player is thread safe again. ![Regression](https://img.shields.io/badge/regression-red)
- Fixed: Event timing was not always correctly sent to the CLAP plug-in.
- Improved: Rewrote MuntPlayer to use LibMT32Emu API version 3.
  - Supports [Versioned ROMs](https://github.com/dosbox-staging/dosbox-staging/wiki/MIDI#mt-32-roms).
  - Samples are rendered as floating-point numbers.
  - Limitation: Most of options are still hardcoded.

v2.19.0.0-alpha4, 2025-06-29

- Improved: CLAP host functionality.
  - Added support for sample rate and MIDI flavor preferences.
  - Added support for CLAP plug-ins with a GUI.
  - Known limitation: The GUI does not control the player yet and vice versa.
  - Known problem: The Dexed GUI does not reappear when the GUI is closed and re-opened.
  - Extended CLAP host functionality to support [Dexed](https://asb2m10.github.io/dexed/) and other plug-ins with the same requirements.
- Changed: MIDI flavor **Default** is now just that. Previous versions used GS as a default. Let me know if this breaks things.
- Fixed: Player selection in Preferences was broken. ![Regression](https://img.shields.io/badge/regression-red)
- Fixed: FluidSynth reverb processing was always disabled. ![Regression](https://img.shields.io/badge/regression-red)

v2.19.0.0-alpha3, 2025-06-25

- New: CLAP host functionality. foo_midi can now use [CLAP ((CLever Audio Plug-in API))](https://u-he.com/community/clap/) MIDI plug-ins to render the audio stream.
  - Specify the directory containing CLAP plug-ins in the Paths preferences page.
  - Take a look at [Nuked-SC55-CLAP](https://github.com/johnnovak/Nuked-SC55-CLAP) for an example.

v2.19.0.0-alpha2, 2025-06-22

- New: User Guide [documentation](docs/README.md) (*Work in Progress*)
- Improved: Updated LibMT32Emu to v2.7.2.
- Improved: LibADLMIDI configuration
  - Moved to separate **FM Synthesis** page. All the advanced settings can now be set here.
  - Added Opal, Java, ESFMu, MAME OPL2, YMFM OPL2, YMFM OPL3, Nuked OPL2 LLE and Nuked OPL3 LLE as selectable emulator cores.
  - Fixed sorting in bank names combobox.
- Improved: LibOPNMIDI configuration
  - Moved to separate **FM Synthesis*- page. All the advanced settings can now be set here.
- Fixed: An old threading issue caused by allowing the MIDI channels to be enabled or disabled during playback.

v2.19.0.0-alpha1, 2025-06-16

- New: Resurrected yuno's fmmidi player.
  - The default Programs definition file is located in the component directory but can be changed in the Preferences dialog.
- Fixed: Preferences dialog should adapt to High DPI settings now.
- Improved: Stricter interpretation of the RCP mute mode that prevents an RCP track from being included in the MIDI stream.

v2.18.0.0, 2025-04-05

- Changed: Enabled dynamic sample loading in the FluidSynth player again when using [FluidSynth 2.4.4](https://github.com/FluidSynth/fluidsynth/releases/tag/v2.4.4) or later.
- Added: BASSMIDI will ignore NRPN Vibrato Depth events in SC-88Pro mode. It overreacts to this parameter.
- Added: Support for MMF/SMAF MA-2 files.

v2.17.2.0, 2025-03-19

- Fixed: Crash while attempting to open a MIDI file with a file name containing non-ASCII characters. An old bug suddenly surfaced while attempting to open a WRD file containing external lyrics.
  - Thank you to [ha7pro](https://hydrogenaud.io/index.php?action=profile;u=163651) for reporting the bug and helping me fix it.

v2.17.1.0, 2025-03-16

- Fixed: Secret Sauce crashed due to too many port resets.

v2.17.0.0, 2025-03-16

- New: Metadata MIDI_EMBEDDED_SOUNDFONT: Contains "SF x.x" (where x.x is the version number of the SoundFont specification) or "DLS" if the MIDI file contains an embedded soundfont.
- Improved: Support for XMF/MXMF files with raw deflated content.
- Improved: Tweaked the handling of embedded sound fonts for BASSMIDI and FluidSynth again.
  - The [Official SF2 RMIDI Specification](https://github.com/spessasus/sf2-rmidi-specification) example files seem to work now.
- Improved: The RIFF IPRD chunk will also be used to add an Album tag in case an IALB chunk is not found.
- Improved: FluidSynth player understands Polyphonic Key Pressure (Aftertouch) now.
- Changed: Increased the gain of the FluidSynth player.
- Changed: Disabled dynamic sample loading in the FluidSynth player. It causes distortion when playing some very short samples.
- Fixed: A pending SysEx message would get skipped when the next event used the running status.
- Fixed: More Multi Port MIDI files play correctly in BASSMIDI now.
  - The first MIDI Port message of a track is now added at the start of a track to make sure it occurs before any Program Change events.

v2.16.0.0, 2025-02-24

- New: Support for XMF (Extensible Music Format) and MXMF (Mobile Extensible Music Format) files.
- New: Metadata XMF_META_FILE_VERSION, XMF_FILE_TYPE and XMF_FILE_TYPE_REVISION.
- Fixed: VSTi plugins did not load or save their configuration anymore (regression).
- Fixed: VSTi plugins did not always show the correct name in the Preferences dialog (regression).

v2.15.2.0, 2025-01-12

- Improved: Updated BASSMIDI to v2.4.15.3.
- Improved: Updated FluidSynth to v2.4.2.

v2.15.1.0, 2024-09-18

- Fixed: The selected player now only gets overriden to BASSMIDI/FluidSynth when an embedded soundfont or a soundfont is found with the same basename as the MIDI file.
- Fixed: The BASSMIDI Resampling combo box was not always enabled correctly when switching player types.

v2.15.0.0, 2024-09-14

- New: Support for embedded DLS sound fonts in RMI files. Works only with the FluidSynth player.
- New: Support for the DBNK chunk in RMI files.
- New: Support for SoundFont layering without using SFList files. Not perfect yet but usable.
- Improved: Tweaked the player type override logic.
- Improved: Increased the number of MIDI ports supported by the BASSMIDI player.
- Fixed: A very old bug in the MIDI parsing code when Pitch Bend control change messages were encountered.
- Fixed: Some preferences were not reset when the Reset button was clicked.

v2.14.0.0, 2024-08-17

- New: Support for embedded artwork (IPIC) and encoding (IENC) chunks in RMI files.

v2.13.2.0, 2024-08-16

- Fixed: The Apply button remained active again even if nothing was changed.

v2.13.1.0, 2024-08-15

- Fixed: 0-byte *.tmp files were not deleted after playing an RMI file with an embedded SoundFont.

v2.13.0.0, 2024-08-14

- Improved: An All Notes Off channel mode message will be sent when a channel gets disabled. The overall filtering has been fine-tuned.
- Improved: Channels can be disabled per port.
- Improved: Only sets the MIDI player to BASSMIDI with SoundFonts if FluidSynth is not used.
- Builds with foobar2000 SDK 2024-08-07.

v2.12.0.0, 2024-08-10

- New: Added support for embedded sound fonts in RMI files.
- New: MIDI channels can be enabled and disabled during playback.
  - See MSKB article 84817 [Using the MIDI Mapper](https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/84817) for an explanation of the 1 - 10 and 11 - 16 buttons.
- Changed: Dropped support for foobar2000 1.x.

v2.11.0.0, 2024-06-23

- New: Recomposer support (.RCP, .R36, .G18, .G36). Some files may still have issues.
- New: Preferences page to configure Recomposer and HMI/HMP settings. (alpha7)
- New: HMI/HMP default tempo can be configured. (alpha7)
- New: Support for LeapFrog loop markers (CC 110 and 111). (alpha8)
- Improved: Added support for Unicode paths to RCP converter (alpha3)
- Improved: Detection of mixed-encoding text in metadata (alpha5)
- Fixed: RPG Maker loops should work again.
- Fixed: Recomposer files with strange tempo changes crashed the component. (alpha4)
- Fixed: HMI conversion added a second Note On event after every note instead of a Note Off event. (alpha7)
- Fixed: The Apply button remained active even if nothing was changed. (alpha7)

v2.10.0.0, 2024-05-07, *"It's been a while"*

- New: The volume of BASSMIDI can be tweaked independently of the overall volume. Defaults to 0.15, determined experimentally to align with the other players.
- Improved: Added detection of EUC-JP encoded meta data.
- Improved: Added Shift-JIS and EUC-JP detection and conversion for lyrics.
- Fixed: Mixed ANSI and Shift-JIS wasn't detected (anymore?).
- Fixed: The BASSMIDI voice count was not initialized correctly when using a preset.
- Fixed: Emu de MIDI sysex recognition.
- Fixed: Emu de MIDI potential buffer overflow during rendering.

v2.9.2.0, 2023-12-24, *"Merry Christmas"*

- New: Compatible with foo_vis_midi v0.1.0.
- Fixed: Crash in Emu de MIDI because dynamic synthesis rate was not initialized in time.
- Fixed: Loop type was not respected when converting to other audio formats.
- Builds with foobar2000 SDK 2023-09-23.

v2.9.1.3, 2023-11-02, *"Loop de loop"*

- New: You can specify the path of an ADLMIDI bank (*.wopl or any of the other supported formats) in the Advanced branch of the Preferences dialog.
  - The bank in the file overrides any selection in the bank drop down list in Preferences.
  - The file path is not yet saved as part of a preset.
  - Only file paths with Latin-1 characters are supported (limitation of the library).
- New: Made Opal and Java OPL3 emulator core from LibADLMIDI selectable in the Advanced Preferences.
- Improved: The decay time is now configurable. The default is still 1s (1000ms).
- Improved: Looping, fading and decay has been tweaked.
- Improved: The song duration is now always calculated without taking into account the selected loop mode. So it's the absolute length of the song without any looping or decay time.
- Improved: Made the parsing of the MIDI data more robust.
- Improved: LDS file detection is more robust.
- Fixed: FluidSynth did not respect the preferred sample rate.
- Fixed: FluidSynth did not save two settings in a preset.
- Fixed: Invalid embedded karaoke lyrics were not handled correctly.
- Builds with foobar2000 SDK 2023-09-06.

v2.9.0.0, 2023-08-02, *"Revenge of the FluidSynth"*

- New: Added FluidSynth player again.
  - It can be selected after setting the path to the directory that contains the FluidSynth libraries. Here you can download [FluidSynth](https://github.com/FluidSynth/fluidsynth/releases). Make sure you download the version that has the same CPU architecture as foobar2000 (x64 or x86).
- New: Added .XFM as an alternative file extension for XMI files.
- Improved: Added FluidSynth settings to preferences page.
- Improved: MIDI standard detection
  - Some XG files were not recognized as such if the file contained any GS messages first.
  - GM 2 detection.
- Changed: Renamed dynamic info tags *bassmidi_voices* and *bassmidi_voices_max* to *midi_active_voices* and *midi_peak_voices*. The FluidSynth player also sets those tags while playing.
- Fixed: An old bug in the XMI parser prevented some XMI files from loading.

v2.8.5.0, 2023-07-23, *""*

- New: Added a configuration option to always use Super Munt when playing an MT-32 file. Default is on.
- New: Added an configuration option to always use a VSTi to play an XG file. Default is off.
  - Don't forget to specify the path name of the VSTi in the Advanced preferences.
- Fixed: Saving MIDI presets was broken.
- Fixed: Loop detection was broken for some files.

v2.8.4.0, 2023-06-26, *"Beat the Drum"*

- New: A new Info tag "MIDI_PLAYER" contains the name of the MIDI player playing the current track.
- New: Added detection of an extra percussion channel in Standard MIDI Files (*.MID). When meta data messages of type Text, Track Name or Instrument Name containing the word "drum" preceed the first message for channel 16 it will be used as an extra percussion channel in addition to channel 10.
- New: A new Info tag "MIDI_EXTRA_PERCUSSION_CHANNEL" contains the (1-based) number of the channel that acts as an additional percussion channel (only channel 16 for now).
- Improved: Error reporting in general and in case of parsing MIDI files with problems.
- Fixed: Playing loops did not always work. (Regression)

v2.8.3.1, 2023-06-03, *"Do you want lyrics with that? Redux"*

- Fixed: The user selection of the MIDI player was no longer honored. (Regression) A SOLID bug...

v2.8.3.0, 2023-06-01, *"Do you want lyrics with that?"*

- Cue Marker, Lyrics, Time Signature and Key Signature meta events are now converted to tags.
  - Soft Karaoke lyrics are stored in an SYNCEDLYRICS tag; other lyrics in a LYRICS tag. The tags can be edited but will not be written back to the MIDI file.
  - Info tag MIDI_LYRICS_TYPE contains the name of the Karaoke standard. For now, only Soft Karaoke format is recognized.
- Fixed: vshost process was not stopped when checking for presets. (Regression)

v2.8.2.0, 2023-05-20, *"Spring Cleaning"*

- Fixed: MIDI files with malformed tracks caused a crash.
- Fixed: MIDI files with malformed SysEx events caused a crash.
- Fixed: Work-around for weird rendering problem in Dark mode.
- Fixed: Restored access to the Preferences page from the Decoding page in the Preferences dialog.
- Improved: Tweaked the size of some of the labels of the Preferences dialog.
- Improved: Added a message to re-open the Preferences dialog after any of the paths were changed.
- Improved: Changed Shift-JIS detection in meta data. A copyright sign (©) was interpreted as Shift-JIS.

v2.8.1.0, 2023-05-01, *"A New Beginning...? Redux"*

- Fixed: The dialog now properly resizes on systems with High DPI settings (> 100% scaling)

v2.8.0.0, 2023-04-30, *"A New Beginning...?"*

- Major refactoring of the source code.
- Builds with foobar2000 SDK 2023-04-18.
- Tried to make the preferences page a bit more accessible to new users.
  - Moved all the path controls to a separate preferences page.
  - Added the configuration of the VTSi plugin path to the preferences page (in addition of the Advanced Preferences, for backwards compatibility).
  - Added the configuration of the Secret Sauce path to the preferences page (in addition of the Advanced Preferences, for backwards compatibility).

v2.7.4.4, 2022-11-21, *"I'm SoundFont of it"*

- Fixed: The new scpipe32 and scpipe64 in the previous version had issues. Secret Sauce is back.
- Added support for compressed SoundFonts (.sf3) to BASSMIDI player.
- Updated Munt (MT32 emulator) to v2.7.0.
- Reduced the component package size a bit. Only one copy of each vsthost and scpipe executable is included.

v2.7.4.3, 2022-11-20, *"Returning to BASS."*

- Re-added missing BASS libraries.
- Re-added recompiled versions of scpipe32 and scpipe64.
- Upgraded JSON parser and build to latest version.
- Started fixing compiler warnings.
- No added functionality.

v2.7.4.2, 2022-11-14, *"The Temple of VeSTa"*

- Added 32-bit and 64-bit VST instrument support for foobar2000 v2.0.
- Fixed 64-bit VST instrument support for foobar2000 v1.6.13.

v2.7.4.1, 2022-11-04, *"The Dark Side"*

- Added Dark Mode support for foobar2000 v2.0.
- Fixed 32-bit build.
- Updated BASS to v2.4.17.

v2.7.4, 2022-11-03, *"Scratchin' the itch"*

- Initial release of x64 version for [foobar2000](https://www.foobar2000.org/) v2.0.

2022-02-19 01:17 UTC - kode54

- Fix System Exclusive messages across different ports
- Version is now 2.7.3

2022-02-10 11:07 UTC - kode54

- Fix the filtering subsystem to work properly
- Version is now 2.7.2

2022-02-10 02:19 UTC - kode54

- Fix a heap overrun in the playback code's leftover samples handler
- Version is now 2.7.1

2022-02-09 22:08 UTC - kode54

- Replace FluidSynth with BASSMIDI again. This is slightly regrettable, if only because it's replacing FOSS with proprietary software, but BASSMIDI is a way better implementation, and definitely way more stable.
- Version is now 2.7

2022-02-08 10:31 UTC - kode54

- Implemented synthesizer reset support for seeking, which should fix FluidSynth crashing when seeking around a lot, and should also improve responsiveness of seeking backwards.
- Version is now 2.6.2

2022-02-05 12:52 UTC - kode54

- Hopefully fix seeking in files with blocked decoding, like VSTi
- Also fix seeking in non-blocked decoding files, because some synthesizers need time rendered to accept timed events
- Version is now 2.6.1

2022-02-03 15:59 UTC - kode54

- Actually get it working
- Update SDK
- Fix a fatal flaw in the 64 bit VST host, regarding event lists. This presented itself as me using sizeof(long) for the two initial members of the structure, which is plainly wrong, and only compiled correctly for 32 bit architecture. This fixes Roland Sound Canvas VA, and possibly any other 64 bit plugins.
- Version is now 2.6.0

2022-02-03 10:02 UTC - kode54

- Replaced main MIDI player with common implementation from my other player, Cog. This implementation supports renderers that require a fixed block size, which I implemented for Secret Sauce and VSTi.
- Replaced Secret Sauce flavor and reverb filtering with a generic filter that can be applied to all drivers.

2021-08-08 20:06 UTC - kode54

- Changed zero duration check to require at least one track with a valid duration
- Version is now 2.5.7

2021-08-01 20:27 UTC - kode54

- Removed FluidSynth thread count option, as it's apparently broken with the build I use and supply
- Version is now 2.5.6

2021-05-21 07:16 UTC - kode54

- Implemented options for FluidSynth to disable effects processing, change the polyphony count, and increase the CPU core count
- Version is now 2.5.5

2021-05-09 01:38 UTC - kode54

- Implemented option for FluidSynth to load samples dynamically, enabled by default
- Rebuild FluidSynth with libsndfile support, to enable compressed SF3 support
- Version is now 2.5.4

2021-05-07 02:41 UTC - kode54

- Switch FluidSynth to version 3.x from HEAD, with an important change for support for GM/GS/XG reset messages
- Change how FluidSynth is configured, so device-id is set properly, so GS/XG reset messages are handled correctly New version of FluidSynth supports SoundFont banks greater than 4GB in size
- Version is now 2.5.3

2021-05-06 21:09 UTC - kode54

- Rewrite FluidSynth handler a bit, implementing its own SysEx handler, and changing the gain level
- Implemented support for looping indefinitely on playback only if a loop is detected
- Version is now 2.5.2

2021-05-05 00:39 UTC - kode54

- Fix preferences section name
- Version is now 2.5.1

2021-05-04 09:13 UTC - kode54

- Disabled BASSMIDI support and reinstated FluidSynth support
- Refactored preferences dialog code
- Version is now 2.5

2021-04-27 02:35 UTC - kode54

- Remove BASS_Free call, as it will most likely be happening during application Shutdown anyway
- Update BASS and BASSMIDI versions, as well as decoder plugins
- Version is now 2.4.12

2021-04-12 00:13 UTC - kode54

- Fixed a potential crash from opening a file with no valid events
- Version is now 2.4.11

2021-04-10 07:13 UTC - kode54

- Finally attempt to fix Standard MIDI files with truncated tracks causing inconsistent track length reporting due to uninitialized data and other random factors
- Version is now 2.4.10

2021-04-09 21:57 UTC - kode54

- Fix Standard MIDI to truncate track ends to the last event in the track, rather than the timestamp of the track end meta event
- Version is now 2.4.9

2021-04-09 21:08 UTC - kode54

- Forgot to rebuild the project after making changes to the MIDI processor
- Version is now 2.4.8

2021-04-09 07:02 UTC - kode54

- Made Standard MIDI processor handle another bad file case better
- Version is now 2.4.7

2021-04-09 00:35 UTC - kode54

- Made Standard MIDI processor way more lenient about broken files
- Version is now 2.4.6

2021-04-08 04:35 UTC - kode54

- Updated libADLMIDI and libOPNMIDI
- Stopped using personal fork of libADLMIDI
- Version is now 2.4.5

2021-03-29 07:37 UTC - kode54

- Fix Standard MIDI file handling to support the edge case of files with non-standard chunks in them
- Updated libADLMIDI and libOPNMIDI
- Version is now 2.4.4

2021-01-11 08:57 UTC - kode54

- Updated libADLMIDI and libOPNMIDI
- Version is now 2.4.3

2020-12-09 02:34 UTC - kode54

- Added HMQ extension for HMP format
- Version is now 2.4.2

2020-10-18 08:24 UTC - kode54

- Fixed metadata service init
- Version is now 2.4.1

2020-10-07 02:49 UTC - kode54

- Replaced fmmidi/midisynth with libOPNMIDI
- Updated BASS/BASSMIDI/BASSWV
- Updated libADLMIDI
- Version is now 2.4.0

2020-08-30 07:09 UTC - kode54

- Hopefully fix end of file issues
- Version is now 2.3.6

2020-07-24 00:29 UTC - kode54

- Implemented BASSMIDI voices control in Advanced Preferences
- Moved existing BASSMIDI Advanced Preferences option to its own tree with the new option
- Dynamic info now returns current and maximum BASSMIDI voices
- Version is now 2.3.5

2020-03-25 02:06 UTC - kode54

- Added another VST plugin import function name
- Version is now 2.3.4

2020-03-05 05:12 UTC - kode54

- Add another Secret Sauce hash set
- Version is now 2.3.3

2020-02-24 12:05 UTC - kode54

- Forget cross compiling, let's use old Visual Studio 2015 instead!
- Version is now 2.3.2

2020-02-24 04:27 UTC - kode54

- Attempt to cross compile external executables, to try to minimize my own code footprint and just bundle widely distributed FOSS dependencies instead
- Version is now 2.3.1

2020-02-15 11:49 UTC - kode54

- Rebuild everything with Clang! That makes the VirusTotal go away! Mostly!
- Version is now 2.3

2020-02-02 03:54 UTC - kode54

- Now possible to load Halion and Halion Sonic, and possibly other plugins that had this weird quirk. No guarantees that any of them will even work properly.
- Version is now 2.2.14

2020-01-27 02:51 UTC - kode54

- Correctly process piped input and ignore messages
- Version is now 2.2.13

2020-01-26 01:05 UTC - kode54

- Remove message pumps from VST and SC pipe handlers
- Version is now 2.2.12

2020-01-02 03:31 UTC - kode54

- Added new Secret Sauce version
- Changed how Touhou loops are handled slightly
- Version is now 2.2.11

2019-11-24 01:11 UTC - kode54

- Implemented support for Touhou loops
- Version is now 2.2.10

2019-09-26 21:50 UTC - kode54

- Fixed Secret Sauce not sending GS reset on Default mode
- Version is now 2.2.9

2019-09-26 03:06 UTC - kode54

- Made Secret Sauce main and GS flavor separately configurable
- Version is now 2.2.8

2019-09-26 01:53 UTC - kode54

- Overhauled the looping and timing system, now with configurable presets for both playback and conversion, and separating the indefinite playback mode from the other looping modes, since it's also only applicable to playback
- Version is now 2.2.7

2019-09-26 00:25 UTC - kode54

- Added another Secret Sauce
- Version is now 2.2.6

2019-08-19 03:23 UTC - kode54

- Made external executables use static runtime again, since they can't import bundled runtime like components can
- Updated libADLMIDI
- Removed unnecessary source file from repository
- Fixed invalid MIDI files with division ticks count of 0 from slipping through validation and causing division by zero errors on tick count to timestamp calculations
- Version is now 2.2.5

2019-06-06 00:04 UTC - kode54

- Added a new Secret Sauce hash, still looking for v1.0.7 64 bit
- Better commented the MD5 table
- Version is now 2.2.4

2019-03-11 02:10 UTC - kode54

- Added further checks to Secret Sauce and VSTi players for empty plugin paths
- Added more console error notices for various states where outputs were selected without configuring them with ROMs or instruments
- Version is now 2.2.3

2019-03-09 00:12 UTC - kode54

- Added limited ANSI metadata handling
- Version is now 2.2.2

2019-02-25 08:08 UTC - kode54

- Changed Secret Sauce plugin interface to emit silence if the cores have crashed
- Version is now 2.2.1

2019-02-25 04:21 UTC - kode54

- Rewrote Secret Sauce interface into a pipe exchange system, which allows:
  - Both 32 and 64 bit modules
  - Multiple instances without copying the DLL to a temporary file
- Version is now 2.2

2019-02-08 01:56 UTC - kode54

- Updated BASS and BASSMIDI
- Added support for new BASSMIDI interpolation mode
- Added support for .SFOGG extension, which is the same
  format as sf2pack, only using Ogg Vorbis sample data
- Version is now 2.1.9

2019-01-29 01:15 UTC - kode54

- Updated libADLMIDI
- Version is now 2.1.8

2018-09-30 00:59 UTC - kode54

- Implemented libADLMIDI core selection, defaulting to the fast Dosbox core
- Version is now 2.1.7

2018-09-16 03:11 UTC - kode54

- Updated libADLMIDI
- Version is now 2.1.6

2018-08-29 10:10 UTC - kode54

- Re-enabled the VSTi enumeration message pump, as otherwise, the whole process deadlocks, except when it's running under Wine.
  - This was supposed to fix a bug where dialog creation could be interrupted
    by a switch to other preferences panes while VSTi enumeration is still
    happening, causing the MIDI prefpane to override the other. Apparently,
    that's yet another bug that doesn't occur when running the process under
    Wine.
- Version is now 2.1.5

2018-08-28 11:18 UTC - kode54

- Change optimization settings and profile optimize once again
- Updated Munt
- Updated libADLMIDI
- Changed ADLMIDI player to embed full WOPL banks for two of the presets
- Disabled VSTi player running a message pump during plugin enumeration
- Removed effects from fmmidi, they were too slow
- Version is now 2.1.4

2018-08-01 23:57 UTC - kode54

- Fixed VSTi selection on opening preferences after removing oplmidi
- Combined two cases on reset to defaults
- Version is now 2.1.3

2018-08-01 01:51 UTC - kode54

- Updated libADLMIDI
- Fixed preferences dialog description for soft panning mode (no longer requires double chips)
- Halved volume for Nuclear Option mode, since this version of NukedOPL is louder
- Updated BASS to version 2.4.13.8, BASSFLAC to 2.4.4, and BASSOPUS to 2.4.1.10
- Version is now 2.1.2

2018-07-31 00:04 UTC - kode54

- Updated libADLMIDI
- Version is now 2.1.1

2018-07-30 04:21 UTC - kode54

- Replaced bundled copy of ADLMIDI with libADLMIDI
- Adjusted Nuclear Option drivers to use libADLMIDI Nuked OPL, including panning
- Version is now 2.1

2018-05-21 01:36 UTC - kode54

- Updated Nuked OPL3 to version 1.8 and ported over my changes
- Version is now 2.0.24

2018-04-24 21:48 UTC - kode54

- Fixed Munt enabling Super / GM behavior for strictly MT-32 files
- Version is now 2.0.23

2018-02-09 10:19 UTC - kode54

- Fixed Munt path activating the Apply button
- Version is now 2.0.22

2017-12-31 04:30 UTC - kode54

- Updated input API for player version 1.4
- Version is now 2.0.21

2017-11-19 22:28 UTC - kode54

- Fix SysEx adding a random extra byte when it shouldn't, which broke synthesizers that observed the length, like Windows drivers, or VST instruments in this case.
- Version is now 2.0.20

2017-10-05 19:35 UTC - kode54

- Added support for running SysEx status in Standard MIDI files
- Version is now 2.0.19

2017-08-29 00:56 UTC - kode54

- Added support for SC-VA version 1.0.7
- Version is now 2.0.18

2017-08-28 03:52 UTC - kode54

- Updated DMXOPL to version 2.0b
- Version is now 2.0.17

2017-08-17 03:58 UTC - kode54

- Updated DMXOPL to version 1.10 Final
- Version is now 2.0.16

2017-07-18 02:03 UTC - kode54

- Updated DMXOPL to version 1.9.1
- Version is now 2.0.15

2017-06-25 23:50 UTC - kode54

- Updated BASS, BASSMIDI, and BASSFLAC
- Version is now 2.0.14

2017-06-25 23:41 UTC - kode54

- Fixed EMIDI loops, forgot to pull a midi_processing change long implemented in Cog, DeaDBeeF - Sorry, Duke Nukem 3D fans!

2017-06-24 00:03 UTC - kode54

- Updated DMXOPL to version 1.8.2
- Updated Nuclear Option Doom flavor synth
- Version is now 2.0.13

2017-06-14 01:56 UTC - kode54

- Implemented DMXOPL FM bank in both Nuclear Option and ADLmidi
- Implemented support in ADLmidi for proper Doom pseudo 4op voice pitch offset
- Version is now 2.0.12

2017-03-14 00:47 UTC - kode54

- Added configuration validation for Munt driver
- Fixed VSTHost to correctly check canDo features
- Version is now 2.0.11

2017-03-13 01:49 UTC - kode54

- Fixed support for some VSTi synthesizers, as I was incorrectly testing for sendVstMidiEvent instead of receiveVstMidiEvent
- Version is now 2.0.10

2017-02-24 02:34 UTC - kode54

- Updated BASSMIDI and BASSFLAC libraries
- Added Apogee MIDI driver to Nuclear Option set
- Added dumb cleanup to Secret Sauce Startup
- Version is now 2.0.9

2017-02-04 19:28 UTC - kode54

- Updated BASS libraries
- Version is now 2.0.8

2017-02-04 04:51 UTC - kode54

- Add link to about string
- Version is now 2.0.7

2017-01-27 22:20 UTC - kode54

- Fixed Secret Sauce XG reset
- Secret Sauce now prevents GS flavor setting if an override is in place
- Version is now 2.0.6

2017-01-26 01:46 UTC - kode54

- Fixed Secret Sauce GS reset
- Version is now 2.0.5

2016-12-21 01:03 UTC - kode54

- Fixed Secret Sauce seeking
- Version is now 2.0.4

2016-12-01 03:58 UTC - kode54

- Improved Secret Sauce reset with even heavier resetting
- Added Secret Sauce reverb/chorus disable
- Added Nuclear Option soft panning control
- Oops! Was reading Secret Sauce flavor from configuration rather than preset
- Version is now 2.0.3

2016-11-29 07:11 UTC - kode54

- Unleash the Secret Sauce!
- Version is now 2.0.2

2016-11-25 23:49 UTC - kode54

- Fix new sflist parser:
  - Handling Unicode byte order markers
  - Handling patch mapping range checks from correct JSON variable
  - Handling some miscellaneous warnings
- Version is now 2.0.1

2016-11-21 04:00 UTC - kode54

- Nuclear Option!
- New name!
- Version is now 2.0

2016-11-16 22:50 UTC - kode54

- Reworded one of the settings
- Version is now 1.256

2016-06-19 22:07 UTC - kode54

- Rewrote SFList parser, and added support for the new JSON format
- Version is now 1.255

2016-04-10 06:19 UTC - kode54

- Added an advanced preferences option to disable the chorus in adlmidi
- Version is now 1.254

2016-04-10 04:19 UTC - kode54

- Fixed trimming some type 2 MIDI files, due to trimming non-existent tempo maps
- Version is now 1.253

2016-04-10 02:43 UTC - kode54

- Fixed VSTi crashes with the latest Sound Canvas VA (v1.0.2.0+), but break compatibility with older versions
- Fix VSTi propagating saved settings to playback instances
- Version is now 1.252

2016-04-03 05:42 UTC - kode54

- Now correctly handles invalid files that have no playable tracks
- Version is now 1.251

2016-04-03 05:09 UTC - kode54

- Added an option to start playback on the first note, enabled by default
- Rearranged the preferences dialog a bit
- Version is now 1.250

2016-03-24 01:49 UTC - kode54

- Let's forget all about the secret sauce.
- Updated VSTHost to work with Sound Canvas VA
- Version is now 1.249

2015-10-04 08:26 UTC - kode54

- Rebuilt with MSVC 2010
- Widened the AdLib baank selector control
- Version is now 1.247

2015-09-23 00:56 UTC - kode54

- Hopefully fix MT-32 SysEx message handling
- Version is now 1.246

2015-09-21 01:41 UTC - kode54

- Fix accidental dependency that broke Windows XP compatibility
- Version is now 1.245

2015-09-14 03:00 UTC - kode54

- Fix Munt by updating to the latest version and correcting how its API is used
- Updated BASSMIDI to version 2.4.9.15
- Version is now 1.244

2015-07-16 00:21 UTC - kode54

- Updated BASSMIDI to version 2.4.9.12
- Relaxed XMI IFF validation
- Fixed RPG Maker loop handling, since it appears to only indicate loop start
- Version is now 1.243

2015-04-05 05:54 UTC - kode54

- Updated BASSMIDI to version 2.4.9.4
- Version is now 1.242

2015-03-17 06:17 UTC - kode54

- Fixed SFZ loading
- Added SoundFont load error reporting
- Adjusted VSTi player so process termination can't get in a recursive call loop
- Version is now 1.241

2015-03-17 05:57 UTC - kode54

- Changed BASSMIDI SoundFont loader to not use FontInitUser for local .sfz files
- Version is now 1.240

2015-02-22 21:58 UTC - kode54

- Fixed loading song specific .sflist files
- Version is now 1.239

2015-02-22 07:01 UTC - kode54

- Split up BASSMIDI into three streams, and apply port 0 SysEx messages to all ports
- Version is now 1.238

2015-02-22 01:57 UTC - kode54

- Fixed lockups on MIDI files with loops that start on the end of the file
- Fixed bank/drum/bank LSB handling
- Version is now 1.237
- Updated BASS to version 2.4.11
- Updated BASSMIDI to version 2.4.9
- Updated BASSFLAC to version 2.4.2
- Version is now 1.235

2014-11-30 01:56 UTC - kode54

- Fixed handling sequencer specific commands in adlmidi, Emu de MIDI, and BASSMIDI drivers
- Fixed midi_processing to support sequencer specific commands on import, playback, and export
- Version is now 1.234

2014-11-21 04:47 UTC - kode54

- Fixed SysEx relative path resolving to absolute path
- Version is now 1.233

2014-10-21 00:56 UTC - kode54

- Added some extra validity checks to LDS format parsing
- Version is now 1.232

2014-10-12 23:52 UTC - kode54

- Fixed BASSMIDI player to correctly release unused SoundFonts after 10 seconds, which it wasn't due to misuse of the standard difftime function
- Version is now 1.231

2014-07-08 08:16 UTC - kode54

- Updated BASSMIDI, now with support for SFZ instruments
- Fixed initialization, in case there are multiple copies of BASS resident
- Version is now 1.230

2013-12-15 05:04 UTC - kode54

- Removed CC 111 from EMIDI track exclusion check
- Added RPG Maker loop support, which is CC 111 with values of 0 for start and 1 for end, so long as no other values are used with that control change throughout the same file
- Implemented an option to disable BASSMIDI reverb and chorus processing
- Version is now 1.229

2013-11-03 07:20 UTC - kode54

- Fixed finite looping when loop commands are found
- Version is now 1.228

2013-11-03 07:07 UTC - kode54

- Fixed MIDI loop end handling

2013-10-31 23:42 UTC - kode54

- Odd Windows versions don't like forward slashes in local paths, now directory-specific SoundFonts will preserve the last path separator
- Version is now 1.227

2013-10-31 00:12 UTC - kode54

- Rewrote file- and directory-specific SoundFont handling
- Fixed SoundFont player actually using file- and directory-specific banks
- SoundFont player is now capable of loading file- and directory-specific sflist files
- Version is now 1.226

2013-10-30 19:17 UTC - kode54

- Re-added custom file accessor to SoundFont loading
- Version is now 1.225

2013-10-27 13:23 UTC - kode54

- Added an extra range check to the Standard MIDI track parser
- Fixed System Exclusive message parsing in the Standard MIDI track parser
- Version is now 1.224

2013-10-25 22:09 UTC - kode54

- Foolishly thought I could code sign BASS and company. Reverted to original versions.
- Version is now 1.223

2013-10-25 20:48 UTC - kode54

- Added safety checks to the MIDI processing loaders
- Version is now 1.222

2013-10-21 00:22 UTC - kode54

- BASSMIDI sflist preset command system now supports more than one preset (p) command per command group (once again, groups separated by &)
- BASSMIDI sflist channel command now supports ranges of multiple channels, optionally specified using a hyphen and a higher channel number: c1-4
- Version is now 1.221

2013-10-19 02:05 UTC - kode54

- Added support for extended SFList preset overrides. To use, precede any line of an
  .sflist file with a vertical bar "|" and fill in a list of commands, separated into
  groups by ampersand "&" characters. Each group may contain one preset "p" command
  and any number of non-overlapping channel "c" commands. The preset command is of
  the format "p[#,]#=[#,]#", where the four parameters, left to right, are destination
  bank, destination program, source bank, source program. The either of the bank
  numbers is optional, and the default will map all banks, or map to the default bank.
  The channel command is of the format "c#", where the one parameter is a channel
  number in the range of 1-48. The channel command is implemented by routing the paired
  programs, or all programs in the bank if none is specified, to the bank LSB of the
  channels' numbers. Thus, playback through the BASSMIDI driver ignores bank LSB
  commands in the actual files. If anyone can suggest a better way to do this, or ask
  Ian Luck to further extend the BASS_MIDI_FONTEX structure with a channel bitfield,
  be my guest.
- Version is now 1.220

2013-10-16 16:54 UTC - kode54

- Fixed MIDIPlayer length enforcement for non-looping files
- Version is now 1.216

2013-10-16 10:37 UTC - kode54

- Fixed SoundFont handle cache
- Version is now 1.215

2013-10-16 03:12 UTC - kode54

- Disabled use of std::thread in BASSMIDI Player to spare wine users the fallout of MSVC 2012+ std::thread not working there yet
- Version is now 1.214

2013-10-16 03:01 UTC - kode54

- Major overhaul to use more STL and C++11 features in the MIDI players, to hopefully ease synchronizing the code between this component and other Unixish platforms

2013-09-02 08:45 UTC - kode54

- Added back error checking that was missing after switching to the alternative midi_processing library
- Version is now 1.213

2013-08-23 01:10 UTC - kode54

- Adjusted reverb send level calculation
- Version is now 1.212

2013-08-23 01:03 UTC - kode54

- Optimized reverb and corrected wet/dry mixing

2013-08-23 00:48 UTC - kode54

- Fixed MIDI preset storage for oplmidi

2013-08-23 00:35 UTC - kode54

- Doubled oplmidi volume
- Added reverb to fmmidi and oplmidi

2013-08-13 01:08 UTC - kode54

- Migrated all common MIDI player code to a single base class
- Version is now 1.211

2013-08-12 23:28 UTC - kode54

- Moved midisynth into its own external library
- Changed oplmidi interface to borrow instruments from adlmidi
- Fixed several bounds checking exceptions which could occur in midi_processing

2013-08-05 01:57 UTC - kode54

- Replaced customized MIDI processing routines with in-memory STL based midi_processing library, improving loading speed greatly
- Version is now 1.210

2013-05-09 01:22 UTC - kode54

- Fixed midisynth coarse and fine tuning
- Version is now 1.209

2013-05-08 07:59 UTC - kode54

- Updated BASSMIDI to version 2.4.8
- Updated SoundFont support to use foobar2000 file system functions
- Version is now 1.208

2013-05-08 06:54 UTC - kode54

- Container normalizes port numbers correctly now
- Version is now 1.207

2013-05-08 02:39 UTC - kode54

- Implemented VSTHost hacks necessary to get Universal Sound Module
  working acceptably
- Version is now 1.206

2013-05-08 01:40 UTC - kode54

- Fixed a stupid VSTPlayer memory leak introduced with the new MIDI preset system

2013-05-08 01:27 UTC - kode54

- Reverted VSTHost process to use standard input and output, but reassign them to a null file for the duration of execution

2013-05-08 00:06 UTC - kode54

- Fixed external SysEx dump support for absolute paths
- Fixed external SysEx dump support for multiple items
- Version is now 1.205

2013-05-05 17:56 UTC - kode54

- Changed optimization settings
- Version is now 1.204

2013-05-05 03:52 UTC - kode54

- Minor optimization and fix to the Lanczos resampler
- Version is now 1.203

2013-05-03 02:23 UTC - kode54

- Replaced blargg's Fir_Resampler with a different Lanczos sinc resampler

2013-05-02 01:28 UTC - kode54

- Normalized MIDI port number meta commands per channel which they are applied to
- Version is now 1.202

2013-05-01 01:35 UTC - kode54

- Added King's Quest 6 GM set for MT-32 GM playback

2013-04-28 05:48 UTC - kode54

- Fixed configuration Apply button remaining active if plugin is set to oplmidi
- Verson is now 1.201

2013-04-28 03:31 UTC - kode54

- Rebased Munt to version 1.2.0

2013-04-28 01:04 UTC - kode54

- Implemented support for applying external SysEx dumps to files
- Version is now 1.200

2013-04-24 16:09 UTC - kode54

- Added .KAR to extensions list for real this time
- Version is now 1.199

2013-03-07 17:27 UTC - kode54

- Fixed adlmidi negative vibrato depth
- Version is now 1.198

2013-03-07 16:27 UTC - kode54

- Split fmmidi into separate note factory implementation
- Implemented OPL-like synthesizer based on fmmidi

2013-02-26 03:57 UTC - kode54

- Implemented database metadata storage system
- Implemented per-file synthesizer preset configuration
- Version is now 1.197

2013-02-26 00:57 UTC - kode54

- Fixed adlmidi to initialize RPN to 0x3fff

2013-01-30 19:14 UTC - kode54

- Implemented support for MIDI meta number 9 for port name
- Implemented support for four ports in adlmidi player
- Fixed the Bisqwit bank, and all the banks were renumbered
- Version is now 1.196

2013-01-20 01:06 UTC - kode54

- Added more instrument banks to adlmidi
- Version is now 1.195

2013-01-17 02:57 UTC - kode54

- Regenerated the adldata file from latest adlmidi
- Fixed fmmidi bank LSB control
- Rearranged the preferences page
- Added automatic sorting of the adlmidi bank list
- Disabled adlmidi configuration when adlmidi isn't selected
- Version is now 1.194

2013-01-15 06:34 UTC - kode54

- Implemented support for fmmidi by yuno
- Version is now 1.193

2013-01-10 03:02 UTC - kode54

- Fixed adldata fixed drum notes for OP3 bank files
- Fixed adldata note length calculation for four operator voices
- Fixed adlmidi volume control to only apply to carrier voices
- Version is now 1.192

2013-01-08 03:02 UTC - kode54

- Added precalculated attack rates table to dbopl for 49716Hz, speeding up initialization
- Fixed notes with base notes or fine tune offsets so low that they wrap into the negative by changing one note variable to a signed type, and also fixed the instrument table generator for that case
- Version is now 1.191

2013-01-08 01:48 UTC - kode54

- Disabled adlmidi resampler if sample rate matches OPL3 native rate
- Version is now 1.190

2013-01-07 21:32 UTC - kode54

- Fixed gen_adldata to handle relative note number values, and also adjust DMX note numbers so they're never relative
- Version is now 1.189

2013-01-07 04:30 UTC - kode54

- Implement adlmidi resampler so emulated chips run at their native clock rate
- Version is now 1.188

2013-01-06 03:17 UTC - kode54

- Implemented adlmidi full panning mode
- Version is now 1.187

2013-01-06 01:12 UTC - kode54

- Reduced 4OP voice count to 4 per chip
- Fixed DMX instruments, now dual voice instruments are pseudo-four-operator, with each voice playing out of a separate speaker and the second voice slightly phased

2012-12-31 21:09 UTC - kode54

- Included new OPL3 banks and changed the default bank
- Version is now 1.186

2012-12-31 00:33 UTC - kode54

- Fixed adlmidi arpeggio handling
- Version is now 1.185

2012-12-30 17:41 UTC - kode54

- Automatically switch to BASSMIDI when a file SoundFont is detected
- Version is now 1.184

2012-12-30 17:34 UTC - kode54

- Fixed VSTi configuration for real this time

2012-12-30 17:04 UTC - kode54

- Removed 4OP configuration variable and set 4OP limit to 2 voices per chip
- Version is now 1.183

2012-12-30 00:09 UTC - kode54

- Fixed VSTi configuration
- Version is now 1.182

2012-12-29 16:18 UTC - kode54

- Implemented Adlib/OPL3 synthesizer driver based on Joel Yliluoma's adlmidi
- Version is now 1.181

2012-12-16 01:00 UTC - kode54

- Fixed sample rate configuration disable state for Emu de MIDI
- Version is now 1.180

2012-12-15 18:27 UTC - kode54

- Fixed file termination when seeking
- Version is now 1.179

2012-12-14 03:38 UTC - kode54

- Renamed Munt output setting and added a warning
- Fixed enabling and disabling of BASSMIDI cache status
- Version is now 1.178

2012-12-14 01:28 UTC - kode54

- Added BASSMIDI sample cache statistics reporting to configuration dialog
- Version is now 1.177

2012-12-xx xx:xx UTC - kode54

- Updated BASSMIDI to version 2.4.7.10.
- Version is now 1.176

2012-11-26 11:24 UTC - kode54

- Added .KAR to the supported file name extensions list
- Version is now 1.175

2012-09-28 18:30 UTC - kode54

- Added support for automatically loading SoundFont banks named after the directory containing the MIDI file
- Version is now 1.174

2012-09-03 00:21 UTC - kode54

- Added storage for multiple VSTi configuration blocks
- Version is now 1.173

2012-09-02 22:47 UTC - kode54

- Fixed VSTi loading when the search path has a trailing slash
- Added exception handling around VSTi platform check
- Version is now 1.172

2012-09-02 01:58 UTC - kode54

- Implemented finite looping and fading support
- Version is now 1.171

2012-08-31 20:01 UTC - kode54

- Hotfix to include updated VSTHost.exe
- Now supports 64-bit VST instruments
- Version is now 1.170

2012-08-29 03:23 UTC - kode54

- Implemented support for 48 channel MIDI files
- Version is now 1.169

2012-08-22 19:44 UTC - kode54

- Updated BASSMIDI to version 2.4.7.9
- Hopefully fixed BASSMIDI player bank select control
- Version is now 1.168

2012-08-04 00:01 UTC - kode54

- Changed VSTi player to pass pipe names to the VST host and let it open them itself
- Version is now 1.167

2012-08-03 03:02 UTC - kode54

- Fixed VSTi driver for Edirol Hyper Canvas, stupid thing floods stdout
- Version is now 1.166

2012-08-01 01:14 UTC - kode54

- Added code page detection to MIDI metadata retrieval
- Version is now 1.165

2012-08-01 00:16 UTC - kode54

- Changed BASS plug-in loader to search using a wildcard match so I can add more plug-ins without changing the source code
- Version is now 1.164

2012-07-31 18:00 UTC - kode54

- Enabled sf2pack support with FLAC and WavPack plug-ins
- Version is now 1.163

2012-07-31 16:17 UTC - kode54

- Restructured MIDI port detection and routing
- Updated BASSMIDI to version 2.4.7.5
- Version is now 1.162

2012-07-28 10:39 UTC - kode54

- Restored and enabled tone portamento support in the LDS loader
- Version is now 1.161

2012-07-27 08:35 UTC - kode54

- Updated BASSMIDI to version 2.4.7.4
- Version is now 1.160

2012-07-27 08:24 UTC - kode54

- Made GS reset message detection more lenient
- Version is now 1.159

2012-07-27 03:36 UTC - kode54

- Added direct floating point output to Munt
- Added "Super" mode to Munt for non-MT-32 files
- Version is now 1.158

2012-07-22 15:48 UTC - kode54

- Updated Munt to git revision munt_0_1_3-484-g6807408
- Version is now 1.157

2012-07-14 02:15 UTC - kode54

- Changed MIDI delta processing to signed 32 bit format and added guards and workarounds for negative results
- Version is now 1.156

2012-03-26 18:47 UTC - kode54

- Updated BASSMIDI to version 2.4.6.11
- Implemented support for BASSMIDI sinc interpolation
- Version is now 1.155

2012-02-19 19:59 UTC - kode54

- Added abort check to decoder
- Version is now 1.154

2012-02-09 14:28 UTC - kode54

- BASSMIDI driver now correctly frees SoundFont banks when all playback stops
- Version is now 1.153

2012-01-28 22:57 UTC - kode54

- Updated BASSMIDI to version 2.4.6.10
- Version is now 1.152

2012-01-25 10:39 UTC - kode54

- Added sanity checking to loop scanner so zero-length loops are ignored
- Version is now 1.151

2012-01-08 16:13 UTC - kode54

- Implemented MIDI instrument and bank change filters
- Version is now 1.150

2012-01-08 01:30 UTC - kode54

- Improved the VSTi host bridge and its communication system

2012-01-06 09:23 UTC - kode54

- Fixed automatic paired SoundFont loading for FluidSynth
- Disabled FluidSynth support
- Version is now 1.149

2012-01-06 09:18 UTC - kode54

- Rewrote VSTi player to use an external bridge process
- Version is now 1.148

2012-01-05 20:16 UTC - kode54

- Removed default VST search path and added a warning if no path is configured
- Version is now 1.147

2011-11-27 02:45 UTC - kode54

- Implemented support for GMF format files
- Version is now 1.146

2011-09-17 03:00 UTC - kode54

- Made XMI parser more tolerant of broken files
- Version is now 1.145

2011-09-11 09:41 UTC - kode54

- Implemented drum channel control for Emu de MIDI
- Version is now 1.144

2011-09-11 07:38 UTC - kode54

- Replaced Emu de MIDI main player core with my own stream parser
- Version is now 1.143

2011-09-10 03:19 UTC - kode54

- Made VSTi search path configurable
- Version is now 1.142

2011-08-09 23:13 UTC - kode54

- Removed faulty format detection logic from the MDS/MIDS format handler
- Version is now 1.141

2011-07-24 06:57 UTC - kode54

- Limited MIDI port numbers to 0 and 1
- Version is now 1.140

2011-07-24 04:51 UTC - kode54

- Restructured VSTi player and added dual port support
- Version is now 1.139

2011-07-11 19:58 UTC - kode54

- Implemented GM2 reset handling in FluidSynth and BASSMIDI drivers
- Fixed FluidSynth and BASSMIDI GS drum channel control again for files where tracks may end up mapped to the second port
- Version is now 1.138

2011-07-10 23:19 UTC - kode54

- Added VSTi plug-in configuration
- Version is now 1.137

2011-07-05 04:03 UTC - kode54

- Limited FluidSynth and BASSMIDI GS drum channel control to 16 channels
- Updated BASSMIDI to version 2.4.6.2
- Version is now 1.136

2011-07-04 22:45 UTC - kode54

- Changed FluidSynth and BASSMIDI drivers to require GM, GS, or XG mode for relevant drum channel controls to work
- Version is now 1.135

2011-07-03 03:42 UTC - kode54

- Fixed S-YXG50 VSTi configuration preset for 'funny' versions with junk after
  the preset name
- Version is now 1.134

2011-03-31 09:31 UTC - kode54

- Updated BASSMIDI to version 2.4.6
- Version is now 1.133

2011-03-21 17:55 UTC - kode54

- Updated BASSMIDI to version 2.4.5.14
- Version is now 1.132

2011-03-06 00:53 UTC - kode54

- Fixed the consequences of odd files with lots of weird instrument name meta events losing entire tracks in playback
- Version is now 1.131

2011-03-05 07:02 UTC - kode54

- Fixed multi-port MIDI file support
- Version is now 1.130

2011-03-03 13:44 UTC - kode54

- Implemented support for multi-port MIDI files, properly supported by the
  FluidSynth and BASSMIDI players
- Version is now 1.129

2011-02-29 02:00 UTC - kode54

- Updated BASSMIDI to version 2.4.5.13
- Amended EMIDI track exclusion to include GS tracks
- Version is now 1.128

2011-02-27 16:19 UTC - kode54

- Enabled Standard MIDI File type 2 support
- Fixed type 2 tempo handling
- Version is now 1.127

2011-02-25 23:28 UTC - kode54

- Changed BASSMIDI SoundFont reference counting system
- Updated BASSMIDI to version 2.4.5.12
- Fixed SoundFont list handler loading the last item twice
- Version is now 1.126

2011-02-25 19:27 UTC - kode54

- Fixed XMI tempo
- Version is now 1.125

2011-02-25 16:11 UTC - kode54

- Implemented MIDI type 2 subsong support

2011-02-25 01:44 UTC - kode54

- Fixed drum channel handling with new BASSMIDI
- Version is now 1.124

2011-02-22 06:38 UTC - kode54

- Fixed tempo map handling multiple tempos on the same timestamp

2011-02-22 05:41 UTC - kode54

- Implemented delayed SoundFont closing in BASSMIDI player

2011-02-22 05:00 UTC - kode54

- Updated to newer version of BASSMIDI which simplifies sending MIDI data

2011-02-20 09:22 UTC - kode54

- Implemented support for per-file SoundFont banks
- Version is now 1.123

2011-02-20 08:50 UTC - kode54

- Fixed MUS note off handling

2011-02-20 04:50 UTC - kode54

- Implemented BASSMIDI support

2011-02-14 13:13 UTC - kode54

- Completed LDS file processor
- Version is now 1.122

2011-02-13 01:29 UTC - kode54

- Fixed EMIDI track exclusion
- Version is now 1.121

2011-02-13 00:33 UTC - kode54

- Changed MIDI event class to store up to 16 bytes of parameter data statically
- Version is now 1.120

2011-02-12 00:09 UTC - kode54

- Standard MIDI file processor now ignores MIDI start and stop status codes
- Version is now 1.119

2011-02-11 23:56 UTC - kode54

- Changed standard MIDI file last event handling to work around Meta/SysEx events

2011-02-11 23:27 UTC - kode54

- Changed a few stricmps and stricmp_utf8s to pfc::stricmp_ascii, and added a safety check to VSTi finding function

2011-02-11 22:53 UTC - kode54

- Added MDS to file name extensions list

2011-02-11 22:42 UTC - kode54

- Completed MIDS file processor

2011-02-11 20:26 UTC - kode54

- Changed tempo map timestamp converter to default to a tempo of 500000

2011-02-11 20:05 UTC - kode54

- Completed MUS file processor

2011-02-11 17:14 UTC - kode54

- Fixed VSTi, FluidSynth, and Munt players to correctly handle note on with velocity of zero when seeking

2011-02-11 17:05 UTC - kode54

- Removed incomplete track recovery option

2011-02-11 16:55 UTC - kode54

- Completed XMI file processor

2011-02-11 13:43 UTC - kode54

- Fixed loop end truncation in VSTi, FluidSynth, and Munt players adding note off commands for the wrong channel numbers

2011-02-11 13:22 UTC - kode54

- Fixed loop end truncation in VSTi, FluidSynth, and Munt players

2011-02-11 13:03 UTC - kode54

- Completed HMI file processor

2011-02-11 12:43 UTC - kode54

- Increased FluidSynth missing drum instrument search to include every preset below the missing preset

2011-02-11 11:40 UTC - kode54

- Completed HMP file processor

2011-02-10 12:39 UTC - kode54

- Completed RIFF MIDI file processor

2011-02-10 10:07 UTC - kode54

- Completed Standard MIDI file processor

2011-02-09 23:38 UTC - kode54

- Started rewrite of MIDI file processing routines

2011-02-09 08:22 UTC - kode54

- Fixed FluidSynth initializing channel 10 bank to 128 instead of DRUM_INST_BANK by default
- Version is now 1.118

2011-02-07 14:51 UTC - kode54

- Reverted reverb and chorus send level scale back to 20%
- Version is now 1.117

2011-02-05 06:40 UTC - kode54

- Reverted default reverb send level
- Version is now 1.116

2011-01-24 10:40 UTC - kode54

- Updated Munt with MMX/3DNow!/SSE acceleration from the DOSBox Development forum
- Version is now 1.115

2011-01-23 09:04 UTC - kode54

- Modified SoundFont loader to translate banks to MSB
- Increased reverb and chorus level scale from 20% to 100%
- Enabled default reverb send level of 40
- Version is now 1.114

2011-01-23 02:55 UTC - kode54

- Implemented FluidSynth interpolation method configuration

2010-11-25 00:59 UTC - kode54

- Fixed a typo in the SoundFont loader dialog description string
- Fixed the SoundFont player to so it fails if the specified sflist file fails to open
- Fixed the FluidSynth SoundFont loader so it won't attempt to fclose a NULL file handle
- Version is now 1.113

2010-11-21 23:51 UTC - kode54

- FluidSynth player will now correctly fail instead of crashing if any SoundFonts fail to load
- Changed mostly cosmetic error with MT32Player using SFPlayer control flags
- Version is now 1.112

2010-11-16 03:46 UTC - kode54

- Fixed a potential crash in all readers involving deltas returning negative values
- Version is now 1.111

2010-09-10 19:39 UTC - kode54

- Made Munt data file path configurable
- Made Munt debug info configurable, disabled by default
- Version is now 1.110

2010-09-06 21:13 UTC - kode54

- Added some range checks to various MIDI parsing code, should hopefully prevent some crashes on invalid files
- Version is now 1.109

2010-08-10 20:39 UTC - kode54

- Changed open function to report unsupported file type when file loading fails, to allow fallback to other inputs
- Version is now 1.108

2010-08-08 22:04 UTC - kode54

- Added error checking to some memory allocation calls in MIDI loader
- Version is now 1.107

2010-07-21 07:26 UTC - kode54

- Implemented support for SoundFont list files in the FluidSynth player
- Version is now 1.106

2010-07-20 21:54 UTC - kode54

- Plugged a memory leak in FluidSynth with a dirty workaround
- Version is now 1.105

2010-07-19 20:37 UTC - kode54

- Updated FluidSynth to revision 328
- Version is now 1.104

2010-07-18 22:55 UTC - kode54

- Implemented error checking in FluidSynth dynamic sample loader
- Version is now 1.103

2010-07-18 07:46 UTC - kode54

- Fixed FluidSynth XG and GM2 drum channel allocation
- Version is now 1.102

2010-07-17 23:14 UTC - kode54

- Implemented dynamic sample loading in FluidSynth

2010-06-30 09:45 UTC - kode54

- Fixed FluidSynth pitch bend range RPN

2010-06-22 01:14 UTC - kode54

- Implemented GM support for MT-32 player

2010-05-13 12:48 UTC - kode54

- Implemented MT-32 player based on Munt, triggered by MT-32 SysEx autodetection

2010-05-01 06:55 UTC - kode54

- Removed full path from the SoundFont selection box display, and made the file selection dialog open where the last file was selected.
- Version is now 1.101

2010-04-13 14:57 UTC - kode54

- Amended preferences WM_INITDIALOG handler
- Version is now 1.100

2010-02-18 20:14 UTC - kode54

- Fixed Emu de MIDI handling of initial track deltas
- Version is now 1.99

2010-02-16 21:17 UTC - kode54

- Fixed preferences apply for multiple settings
- Version is now 1.98

2010-02-10 09:49 UTC - kode54

- Made some parts of the MIDI processor used by VSTi and FluidSynth safer
- Version is now 1.97

2010-01-13 00:33 UTC - kode54

- Updated context menu code
- Version is now 1.96

2010-01-11 14:30 UTC - kode54

- Updated preferences page to 1.0 API
- Version is now 1.95

2009-09-01 03:13 UTC - kode54

- FluidSynth player now supports basic GS/XG drum channel control
- Version is now 1.94

2009-08-30 01:37 UTC - kode54

- Fixed loop end event handling for both VSTi and FluidSynth players
- Version is now 1.93

2009-08-23 02:20 UTC - kode54

- Fixed control change loop start position for VSTi and FluidSynth players

2009-08-22 23:28 UTC - kode54

- Now static linking to FluidSynth
- Version is now 1.92

2009-08-22 01:08 UTC - kode54

- Fixed crash bug with delay load checking by calling LoadLibraryEx instead

2009-08-22 00:49 UTC - kode54

- Fixed crash bug in FluidSynth player when seeking before starting playback
- Version is now 1.91

2009-08-21 08:02 UTC - kode54

- Added support for FluidSynth
- Version is now 1.9

2009-08-21 00:57 UTC - kode54

- Fixed memory leak in General MIDI converter context menu handler
- Version is now 1.83

2009-08-16 03:16 UTC - kode54

- Solved rounding error in MIDI do_table and Marker/SysEx map translation
- Version is now 1.82

2009-06-14 20:15 UTC - kode54

- Changed VSTi player loop end handler to simply truncate, rather than mash all end events together
- Version is now 1.81

2009-03-18 20:22 UTC - kode54

- Added General MIDI converter context menu item.
- Version is now 1.8

2006-12-24 05:15 UTC - kode54

- Fixed plug-in droplist change handler for VSTi plug-ins so the last item in the list won't enable the GS/XG to GM2 checkbox.

2006-08-21 09:22 UTC - kode54

- Added call to effMainsChanged before effStartProcess for some VST instruments which require it. (Steinberg Hypersonic 2 crashes without this.)
- Fixed a bug in VSTiPlayer Play(), where processReplace would be called even if it didn't exist.
- Added GM initialization preset for Steinberg Hypersonic 2, requires foo_unpack.
- Version is now 1.7

2005-11-14 00:00 UTC - kode54

- Fixed bug in VSTiPlayer seeking ( bleh, C-style cast flubbed changes to mem_block class )

2005-10-08 22:10 UTC - kode54

- Ported to 0.9 beta 7 + input API
- DXi support disabled because it is a bloated mess and should die
- Version is now 1.6

2005-06-06 08:02 UTC - kode54

- VSTi seeking now resets the synthesizer when seeking backwards, and drops complete note on/off pairs
- Version is now 1.55

2005-05-29 04:21 UTC - kode54

- Completed VSTi support (complete with default polyphony override for S-YXG50)
- Version is now 1.5

2005-03-12 11:02 UTC - kode54

- Added basic GS/XG to GM2 remapping for Hyper Canvas
- Version is now 1.2.3

2004-09-29 19:11 UTC - kode54

- Bug fix in first rmi_data parsing loop, forgot to enum next item uhuhuhu
- Version is now 1.2.2

2004-09-25 04:48 UTC - kode54

- Renamed internal synth to "Emu de MIDI" hopefully to prevent confusion with the EMIDI setting
- Implemented Data Increment and Decrement controller changes in CMIDIModule.cpp
- Changed default pitch bend range in CMIDIModule.cpp from 12 semitones to 2
- Version is now 1.2.1

2004-09-23 10:45 UTC - kode54

- Added internal Emu de MIDI synthesizer, for great chip style justice
- Commented out external input wrapper
- Version is now 1.2

2004-09-19 08:50 UTC - kode54

- Implemented external input wrapper with looping, for foo_emidi
- Version is now 1.1.1

2004-09-19 04:27 UTC - kode54

- Whoops, forgot that all RIFF chunks are word aligned, fixed LIST INFO parser
- Version is now 1.1

2004-09-19 03:08 UTC - kode54

- Changed grow_buf truncate from a #define to an inline function wrapper
- Implemented RIFF LIST INFO chunk parser
- Expanded metadata gathering

2004-03-31 17:53 UTC - kode54

- Fixed seeking past the end of the file
- Version is now 1.0.1
