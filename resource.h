
/** $VER: Resource.h (2023.07.30) **/

#pragma once

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

/** Component specific **/

#define STR_COMPONENT_NAME      "MIDI Player"
#define STR_COMPONENT_VERSION   TOSTRING(NUM_FILE_MAJOR) "." TOSTRING(NUM_FILE_MINOR) "." TOSTRING(NUM_FILE_PATCH) "." TOSTRING(NUM_FILE_PRERELEASE)
#define STR_COMPONENT_BASENAME  "foo_midi"
#define STR_COMPONENT_FILENAME  STR_COMPONENT_BASENAME ".dll"

/** Generic **/

#define STR_COMPANY_NAME        TEXT("LoSno.co")
#define STR_INTERNAL_NAME       TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS            TEXT("Written by Christopher Snowhill, P. Stuer")
#define STR_COPYRIGHT           TEXT("Copyright (c) 2022-2023 ") STR_COMPANY_NAME TEXT(". All rights reserved.")

#define NUM_FILE_MAJOR          2
#define NUM_FILE_MINOR          8
#define NUM_FILE_PATCH          6
#define NUM_FILE_PRERELEASE     0

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT("Adds playback of MIDI files to foobar2000")

#define NUM_PRODUCT_MAJOR       2
#define NUM_PRODUCT_MINOR       8
#define NUM_PRODUCT_PATCH       6
#define NUM_PRODUCT_PRERELEASE  0

#define STR_PRODUCT_NAME        STR_COMPANY_NAME TEXT(" ") STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT("https://github.com/stuerp/foo_midi")
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Dialog Preferences Root **/

#define IDD_PREFERENCES_ROOT 106
#define IDD_PREFERENCES_ROOT_NAME STR_COMPONENT_NAME

#define IDC_PLAYER_TYPE                     1000
#define IDC_SAMPLERATE                      1001

#define IDC_CONFIGURE                       1002

#define IDC_LOOP_PLAYBACK                   1003
#define IDC_LOOP_OTHER                      1004

#define IDC_RPGMAKER_LOOPS                  1005
#define IDC_XMI_LOOPS                       1006
#define IDC_TOUHOU_LOOPS                    1007
#define IDC_FF7_LOOPS                       1008

#define IDC_MUNT_WARNING                    1009

#define IDC_EMIDI_EXCLUSION                 1010
#define IDC_FILTER_INSTRUMENTS              1011
#define IDC_FILTER_BANKS                    1012

#define IDC_RESAMPLING_MODE                 1013
#define IDC_RESAMPLING_TEXT                 1014

#define IDC_CACHED                          1015
#define IDC_CACHED_TEXT                     1016

#define IDC_ADL_BANK                        1017
#define IDC_ADL_BANK_TEXT                   1018
#define IDC_ADL_CHIPS                       1019
#define IDC_ADL_CHIPS_TEXT                  1020
#define IDC_ADL_PANNING                     1021

#define IDC_MUNT_GM_TEXT                    1022
#define IDC_MUNT_GM_SET                     1023

#define IDC_NUKE_PRESET_TEXT                1024
#define IDC_NUKE_PRESET                     1025
#define IDC_NUKE_PANNING                    1026

#define IDC_MIDI_FLAVOR_TEXT                1027
#define IDC_MIDI_FLAVOR                     1028
#define IDC_MIDI_EFFECTS                    1029
#define IDC_MIDI_USE_SUPER_MUNT             1030
#define IDC_MIDI_USE_VSTI_WITH_XG           1031

#define IDC_FLUIDSYNTH_INTERPOLATION_TEXT   1032
#define IDC_FLUIDSYNTH_INTERPOLATION        1033

/** Dialog: Preferences Paths **/

#define IDD_PREFERENCES_PATHS 2000
#define IDD_PREFERENCES_PATHS_NAME "Paths"

#define IDC_VST_PATH                    IDD_PREFERENCES_PATHS + 1
#define IDC_VST_PATH_SELECT             IDC_VST_PATH + 1

#define IDC_SOUNDFONT_FILE_PATH         IDC_VST_PATH_SELECT + 1
#define IDC_SOUNDFONT_FILE_PATH_SELECT  IDC_SOUNDFONT_FILE_PATH + 1

#define IDC_MUNT_FILE_PATH              IDC_SOUNDFONT_FILE_PATH_SELECT + 1
#define IDC_MUNT_FILE_PATH_SELECT       IDC_MUNT_FILE_PATH + 1

#define IDC_SECRET_SAUCE_PATH           IDC_MUNT_FILE_PATH_SELECT + 1
#define IDC_SECRET_SAUCE_PATH_SELECT    IDC_SECRET_SAUCE_PATH + 1

#define IDC_FLUIDSYNTH_PATH             IDC_SECRET_SAUCE_PATH_SELECT + 1
#define IDC_FLUIDSYNTH_PATH_SELECT      IDC_FLUIDSYNTH_PATH + 1

#define IDC_PATHS_MESSAGE               IDC_FLUIDSYNTH_PATH_SELECT + 1
