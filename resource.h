
/** $VER: Resource.h (2023.05.14) **/

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
#define NUM_FILE_PATCH          2
#define NUM_FILE_PRERELEASE     0

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT("Adds playback of MIDI files to foobar2000")

#define NUM_PRODUCT_MAJOR       2
#define NUM_PRODUCT_MINOR       8
#define NUM_PRODUCT_PATCH       2
#define NUM_PRODUCT_PRERELEASE  0

#define STR_PRODUCT_NAME        STR_COMPANY_NAME TEXT(" ") STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT("https://github.com/stuerp/foo_midi")
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Dialog Preferences Root **/

#define IDD_PREFERENCES_ROOT 106
#define IDD_PREFERENCES_ROOT_NAME STR_COMPONENT_NAME

#define IDC_PLAYER_TYPE 1000
#define IDC_SAMPLERATE 1001

#define IDC_CONFIGURE 1013

#define IDC_LOOP_PLAYBACK 1002
#define IDC_RPGMAKERLOOPS 1003
#define IDC_XMILOOPS 1004
#define IDC_FF7LOOPS 1005
#define IDC_EMIDI_EX 1006
#define IDC_RECOVER 1007
#define IDC_LOOP2 1007
#define IDC_LOOP_OTHER 1007
#define IDC_NOSYSEX 1008

#define IDC_MUNT_WARNING 1014

#define IDC_FILTER_INSTRUMENTS 1015
#define IDC_FILTER_BANKS 1016

#define IDC_RESAMPLING_MODE 1017
#define IDC_RESAMPLING_TEXT 1018

#define IDC_CACHED 1019
#define IDC_CACHED_TEXT 1020

#define IDC_ADL_BANK 1021
#define IDC_ADL_BANK_TEXT 1022
#define IDC_ADL_CHIPS 1023
#define IDC_ADL_CHIPS_TEXT 1024
#define IDC_ADL_PANNING 1025

#define IDC_MUNT_GM_TEXT 1026

#define IDC_RESAMPLING2 1027

#define IDC_NUKE_PRESET_TEXT 1027
#define IDC_MUNT_GM_SET 1028
#define IDC_NUKE_PRESET 1029

#define IDC_SC_FLAVOR_TEXT 1030
#define IDC_MIDI_FLAVOR_TEXT 1030
#define IDC_SC_FLAVOR 1031

#define IDC_MIDI_FLAVOR 1031
#define IDC_MIDI_FLAVOR 1031

#define IDC_NUKE_PANNING 1033
#define IDC_MS_PANNING2 1034
#define IDC_SC_EFFECTS 1034
#define IDC_MIDI_EFFECTS 1034
#define IDC_SC_FLAVOR2 1035
#define IDC_GS_FLAVOR 1035
#define IDC_CHECK1 1036
#define IDC_TOUHOULOOPS 1036

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
