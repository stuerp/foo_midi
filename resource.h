
/** $VER: Resource.h (2025.03.15) P. Stuer **/

#pragma once

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

/** Component specific **/

#define STR_COMPONENT_NAME          "MIDI Player"
#define STR_COMPONENT_VERSION       TOSTRING(NUM_FILE_MAJOR) "." TOSTRING(NUM_FILE_MINOR) "." TOSTRING(NUM_FILE_PATCH) "." TOSTRING(NUM_FILE_PRERELEASE) "-alpha4"
#define STR_COMPONENT_BASENAME      "foo_midi"
#define STR_COMPONENT_FILENAME      STR_COMPONENT_BASENAME ".dll"
#define STR_COMPONENT_COMPANY_NAME  "LoSno.co"
#define STR_COMPONENT_COPYRIGHT     "Copyright (c) 2004-2025 " STR_COMPONENT_COMPANY_NAME ". All rights reserved."
#define STR_COMPONENT_COMMENTS      "Written by Christopher Snowhill, P. Stuer"
#define STR_COMPONENT_DESCRIPTION   "Adds playback of MIDI files to foobar2000"

/** Generic **/

#define STR_COMPANY_NAME        TEXT(STR_COMPONENT_COMPANY_NAME)
#define STR_INTERNAL_NAME       TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS            TEXT(STR_COMPONENT_COMMENTS)
#define STR_COPYRIGHT           TEXT(STR_COMPONENT_COPYRIGHT)

#define NUM_FILE_MAJOR          2
#define NUM_FILE_MINOR          16
#define NUM_FILE_PATCH          1
#define NUM_FILE_PRERELEASE     0

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT(STR_COMPONENT_DESCRIPTION)

#define NUM_PRODUCT_MAJOR       2
#define NUM_PRODUCT_MINOR       16
#define NUM_PRODUCT_PATCH       1
#define NUM_PRODUCT_PRERELEASE  0

#define STR_PRODUCT_NAME        STR_COMPANY_NAME TEXT(" ") STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT("https://github.com/stuerp/") STR_COMPONENT_BASENAME
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Dialog Preferences Root **/

#define IDD_PREFERENCES_ROOT                106
#define IDD_PREFERENCES_ROOT_NAME           STR_COMPONENT_NAME

#define IDC_PLAYER_TYPE                     1000
#define IDC_SAMPLERATE                      1001

#define IDC_CONFIGURE                       1002

#define IDC_LOOP_PLAYBACK                   1003
#define IDC_LOOP_OTHER                      1004
#define IDC_DECAY_TIME                      1005

#define IDC_TOUHOU_LOOPS                    1006
#define IDC_RPGMAKER_LOOPS                  1007
#define IDC_LEAPFROG_LOOPS                  1008
#define IDC_XMI_LOOPS                       1009
#define IDC_FF7_LOOPS                       1010

#define IDC_MUNT_WARNING                    1011

#define IDC_EMIDI_EXCLUSION                 1012
#define IDC_FILTER_INSTRUMENTS              1013
#define IDC_FILTER_BANKS                    1014

#define IDC_BASSMIDI_VOLUME_LBL             1036
#define IDC_BASSMIDI_VOLUME                 1037

#define IDC_RESAMPLING_LBL                  1015
#define IDC_RESAMPLING_MODE                 1016

#define IDC_CACHED_LBL                      1017
#define IDC_CACHED                          1018

#define IDC_ADL_BANK                        1019
#define IDC_ADL_BANK_TEXT                   1020
#define IDC_ADL_CHIPS                       1021
#define IDC_ADL_CHIPS_TEXT                  1022
#define IDC_ADL_PANNING                     1023

#define IDC_MUNT_GM_TEXT                    1024
#define IDC_MUNT_GM_SET                     1025

#define IDC_NUKE_PRESET_TEXT                1026
#define IDC_NUKE_PRESET                     1027
#define IDC_NUKE_PANNING                    1028

#define IDC_MIDI_FLAVOR_TEXT                1029
#define IDC_MIDI_FLAVOR                     1030
#define IDC_MIDI_EFFECTS                    1031
#define IDC_MIDI_USE_SUPER_MUNT             1032
#define IDC_MIDI_USE_VSTI_WITH_XG           1033

#define IDC_FLUIDSYNTH_INTERPOLATION_TEXT   1034
#define IDC_FLUIDSYNTH_INTERPOLATION        1035

/** Dialog: Preferences Paths **/

#define IDD_PREFERENCES_PATHS               2000
#define IDD_PREFERENCES_PATHS_NAME          "Paths"

#define IDC_VST_PATH                        2010
#define IDC_VST_PATH_SELECT                 2011

#define IDC_SOUNDFONT_FILE_PATH             2020
#define IDC_SOUNDFONT_FILE_PATH_SELECT      2021

#define IDC_MUNT_FILE_PATH                  2030
#define IDC_MUNT_FILE_PATH_SELECT           2031

#define IDC_SECRET_SAUCE_PATH               2040
#define IDC_SECRET_SAUCE_PATH_SELECT        2041

#define IDC_FLUIDSYNTH_PATH                 2050
#define IDC_FLUIDSYNTH_PATH_SELECT          2051

#define IDC_PATHS_MESSAGE                   2060

/** Dialog: Preferences Recomposer **/

#define IDD_PREFERENCES_PROCESSING          2100
#define IDD_PREFERENCES_PROCESSING_NAME     "Processing"

#define IDC_LOOP_EXPANSION                  2110

#define IDC_WRITE_BAR_MARKERS               2120
#define IDC_WRITE_SYSEX_NAMES               2130
#define IDC_EXTEND_LOOPS                    2140
#define IDC_WOLFTEAM_LOOPS                  2150
#define IDC_KEEP_DUMMY_CHANNELS             2160
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
