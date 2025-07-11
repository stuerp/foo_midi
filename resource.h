
/** $VER: Resource.h (2025.07.11) P. Stuer **/

#pragma once

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

/** Component specific **/

#define STR_COMPONENT_NAME          "MIDI Player"
#define STR_COMPONENT_VERSION       TOSTRING(NUM_FILE_MAJOR) "." TOSTRING(NUM_FILE_MINOR) "." TOSTRING(NUM_FILE_PATCH) "." TOSTRING(NUM_FILE_PRERELEASE)
#define STR_COMPONENT_BASENAME      "foo_midi"
#define STR_COMPONENT_FILENAME      STR_COMPONENT_BASENAME ".dll"
#define STR_COMPONENT_COMPANY_NAME  "LoSno.co"
#define STR_COMPONENT_COPYRIGHT     "Copyright (c) 2004-2025 " STR_COMPONENT_COMPANY_NAME ". All rights reserved."
#define STR_COMPONENT_COMMENTS      "Written by Christopher Snowhill, P. Stuer"
#define STR_COMPONENT_DESCRIPTION   "Adds playback of MIDI files to foobar2000"
#define STR_COMPONENT_URL           "https://github.com/stuerp/" STR_COMPONENT_BASENAME

/** Generic **/

#define STR_COMPANY_NAME            TEXT(STR_COMPONENT_COMPANY_NAME)
#define STR_INTERNAL_NAME           TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS                TEXT(STR_COMPONENT_COMMENTS)
#define STR_COPYRIGHT               TEXT(STR_COMPONENT_COPYRIGHT)

#define NUM_FILE_MAJOR              2
#define NUM_FILE_MINOR              19
#define NUM_FILE_PATCH              0
#define NUM_FILE_PRERELEASE         0

#define STR_FILE_NAME               TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION            TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE) TEXT("-alpha8")
#define STR_FILE_DESCRIPTION        TEXT(STR_COMPONENT_DESCRIPTION)

#define NUM_PRODUCT_MAJOR           2
#define NUM_PRODUCT_MINOR           19
#define NUM_PRODUCT_PATCH           0
#define NUM_PRODUCT_PRERELEASE      0

#define STR_PRODUCT_NAME            STR_COMPANY_NAME TEXT(" ") STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION         TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE) TEXT("-alpha8")

#define STR_ABOUT_NAME              STR_INTERNAL_NAME
#define STR_ABOUT_WEB               TEXT(STR_COMPONENT_URL)
#define STR_ABOUT_EMAIL             TEXT("mailto:peter.stuer@outlook.com")

/** Dialog Preferences Root **/

#define IDD_PREFERENCES_ROOT                106
#define IDD_PREFERENCES_ROOT_NAME           STR_COMPONENT_NAME

#define IDC_PLAYER_TYPE                     1000
#define IDC_SAMPLERATE                      1001

#define IDC_CONFIGURE                       1002

#define IDC_LOOP_PLAYBACK                   1003
#define IDC_LOOP_OTHER                      1004
#define IDC_DECAY_TIME                      1005

#define IDC_LOOP_COUNT                      1006
#define IDC_FADE_OUT_TIME                   1007

#define IDC_TOUHOU_LOOPS                    1008
#define IDC_RPGMAKER_LOOPS                  1009
#define IDC_LEAPFROG_LOOPS                  1010
#define IDC_XMI_LOOPS                       1011
#define IDC_FF7_LOOPS                       1012

#define IDC_MT32EMU_WARNING                 1013

#define IDC_EMIDI_EXCLUSION                 1014
#define IDC_FILTER_INSTRUMENTS              1015
#define IDC_FILTER_BANKS                    1016

#define IDC_SKIP_TO_FIRST_NOTE              1017

#define IDC_MIDI_FLAVOR_TEXT                1018
#define IDC_MIDI_FLAVOR                     1019
#define IDC_MIDI_EFFECTS                    1020
#define IDC_MIDI_USE_MT32EMU_WITH_MT32      1021
#define IDC_MIDI_USE_VSTI_WITH_XG           1022
#define IDC_MIDI_DETECT_EXTRA_DRUM          1023


/** Dialog: Preferences FM Synthesis **/

#define IDC_ADL_BANK                        1100
#define IDC_ADL_BANK_TEXT                   1101
#define IDC_ADL_CORE                        1102
#define IDC_ADL_CORE_TEXT                   1103
#define IDC_ADL_CHIPS                       1104
#define IDC_ADL_CHIPS_TEXT                  1105
#define IDC_ADL_SOFT_PANNING                1106
#define IDC_ADL_BANK_FILE_PATH_TEXT         1107
#define IDC_ADL_BANK_FILE_PATH              1108
#define IDC_ADL_BANK_FILE_PATH_SELECT       1109

#define IDC_OPN_BANK                        1200
#define IDC_OPN_BANK_TEXT                   1201
#define IDC_OPN_CORE                        1202
#define IDC_OPN_CORE_TEXT                   1203
#define IDC_OPN_CHIPS                       1204
#define IDC_OPN_CHIPS_TEXT                  1205
#define IDC_OPN_SOFT_PANNING                1206
#define IDC_OPN_BANK_FILE_PATH_TEXT         1207
#define IDC_OPN_BANK_FILE_PATH              1208
#define IDC_OPN_BANK_FILE_PATH_SELECT       1209

#define IDC_NUKE_PRESET_TEXT                1300
#define IDC_NUKE_PRESET                     1301
#define IDC_NUKE_PANNING                    1302

#define IDC_MT32_CONVERSION_QUALITY_TEXT    1400
#define IDC_MT32_CONVERSION_QUALITY         1401
#define IDC_MT32_MAX_PARTIALS_TEXT          1403
#define IDC_MT32_MAX_PARTIALS               1404
#define IDC_MT32_ANALOG_OUTPUT_MODE_TEXT    1405
#define IDC_MT32_ANALOG_OUTPUT_MODE         1406
#define IDC_MT32_GM_SET_TEXT                1407
#define IDC_MT32_GM_SET                     1408
#define IDC_MT32_DAC_INPUT_MODE_TEXT        1409
#define IDC_MT32_DAC_INPUT_MODE             1410

#define IDC_MT32_REVERB                     1411
#define IDC_MT32_NICE_AMP_RAMP              1412
#define IDC_MT32_NICE_PANNING               1413
#define IDC_MT32_NICE_PARTIAL_MIXING        1414
#define IDC_MT32_REVERSE_STEREO             1415

/** Dialog: Preferences Paths **/

#define IDD_PREFERENCES_PATHS               2000
#define IDD_PREFERENCES_PATHS_NAME          "Paths"

#define IDC_VSTi_PATH                       2010
#define IDC_VSTi_PATH_SELECT                2011

#define IDC_VSTi_XG_FILE_PATH               2020
#define IDC_VSTi_XG_FILE_PATH_SELECT        2021

#define IDC_SOUNDFONT_FILE_PATH             2030
#define IDC_SOUNDFONT_FILE_PATH_SELECT      2031

#define IDC_MT32EMU_FILE_PATH               2040
#define IDC_MT32EMU_FILE_PATH_SELECT        2041

#define IDC_SECRET_SAUCE_PATH               2050
#define IDC_SECRET_SAUCE_PATH_SELECT        2051

#define IDC_FLUIDSYNTH_PATH                 2060
#define IDC_FLUIDSYNTH_PATH_SELECT          2061

#define IDC_FLUIDSYNTH_CONFIG_PATH          2070
#define IDC_FLUIDSYNTH_CONFIG_PATH_SELECT   2071

#define IDC_PROGRAMS_FILE_PATH              2080
#define IDC_PROGRAMS_FILE_PATH_SELECT       2081

#define IDC_CLAP_PATH                       2090
#define IDC_CLAP_PATH_SELECT                2091

#define IDC_PATHS_MESSAGE                   2999

/** Dialog: Preferences Recomposer **/

#define IDD_PREFERENCES_PROCESSING          2100
#define IDD_PREFERENCES_PROCESSING_NAME     "Processing"

#define IDC_LOOP_EXPANSION                  2110

#define IDC_WRITE_BAR_MARKERS               2120
#define IDC_WRITE_SYSEX_NAMES               2130
#define IDC_EXTEND_LOOPS                    2140
#define IDC_WOLFTEAM_LOOPS                  2150
#define IDC_KEEP_MUTED_CHANNELS             2160
#define IDC_INCLUDE_CONTROL_DATA            2170

/** Dialog: Preferences HMI **/

#define IDD_PREFERENCES_HMI                 2200
#define IDD_PREFERENCES_HMI_NAME            "HMI / HMP"

#define IDC_DEFAULT_TEMPO                   2210

/** Dialog: Channels **/

#define IDC_CHANNEL_01                      2300
#define IDC_CHANNEL_02                      2301
#define IDC_CHANNEL_03                      2302
#define IDC_CHANNEL_04                      2303
#define IDC_CHANNEL_05                      2304
#define IDC_CHANNEL_06                      2305
#define IDC_CHANNEL_07                      2306
#define IDC_CHANNEL_08                      2307
#define IDC_CHANNEL_09                      2308
#define IDC_CHANNEL_10                      2309
#define IDC_CHANNEL_11                      2310
#define IDC_CHANNEL_12                      2311
#define IDC_CHANNEL_13                      2312
#define IDC_CHANNEL_14                      2313
#define IDC_CHANNEL_15                      2314
#define IDC_CHANNEL_16                      2315

#define IDC_CHANNEL_ALL                     2320
#define IDC_CHANNEL_NONE                    2322
#define IDC_CHANNEL_1_10                    2324
#define IDC_CHANNEL_11_16                   2326

#define IDC_PORT_LBL                        2330
#define IDC_PORT                            2331
#define IDC_PORT_SLIDER                     2332

/** Dialog: Preferences FM Synthesis **/

#define GUID_PREFS_FM { 0xbe9aebaa, 0x5b66, 0x4374, { 0xa3, 0xad, 0x94, 0x6e, 0xb6, 0xc7, 0xe2, 0x61 } }

#define IDD_PREFERENCES_FM                  2400
#define IDD_PREFERENCES_FM_NAME             "FM Synthesis"

#define IDD_PREFERENCES_WT                  2500
#define IDD_PREFERENCES_WT_NAME             "Wavetable"

#define IDC_BASSMIDI_VOLUME_LBL             2501
#define IDC_BASSMIDI_GAIN                   2502

#define IDC_RESAMPLING_LBL                  2503
#define IDC_BASSMIDI_RESAMPLING             2504

#define IDC_BASSMIDI_MAX_VOICES_LBL         2505
#define IDC_BASSMIDI_MAX_VOICES             2506

#define IDC_BASSMIDI_EFFECTS                2507

#define IDC_CACHED_LBL                      2508
#define IDC_CACHED                          2509

#define IDC_FLUIDSYNTH_INTERPOLATION_TEXT   2550
#define IDC_FLUIDSYNTH_INTERPOLATION        2251

#define IDC_FLUIDSYNTH_MAX_VOICES_LBL       2252
#define IDC_FLUIDSYNTH_MAX_VOICES           2253

#define IDC_FLUIDSYNTH_EFFECTS              2254
#define IDC_FLUIDSYNTH_DYN_LOADING          2255

#define IDD_CLAP_WINDOW                     2600
