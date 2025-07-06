
/** $VER: foo_midi.cpp (2025.07.06) **/

#include "pch.h"

#include <sdk/foobar2000-lite.h>
#include <sdk/componentversion.h>

#include "History.h"

#include "Resource.h"

#include <bass/c/bass.h>
#include <clap/version.h>
#include <FluidSynth/include/fluidsynth.h>
#include <mt32emu/config.h>
#include <libADLMIDI/repo/include/adlmidi.h>
#include <libOPNMIDI/repo/include/opnmidi.h>

#pragma hdrstop

#define CLAP_SDK_VERSION TOSTRING(CLAP_VERSION_MAJOR) "." TOSTRING(CLAP_VERSION_MINOR) "." TOSTRING(CLAP_VERSION_REVISION)

namespace
{
    #pragma warning(disable: 4265 4625 4626 5026 5027 26433 26436 26455)
    DECLARE_COMPONENT_VERSION
    (
        STR_COMPONENT_NAME,
        STR_COMPONENT_VERSION,
        STR_COMPONENT_BASENAME " " STR_COMPONENT_VERSION "\n"
            STR_COMPONENT_COPYRIGHT "\n"
            STR_COMPONENT_COMMENTS "\n"
            "\n"
            STR_COMPONENT_DESCRIPTION "\n"
            "\n"
            "Built with foobar2000 SDK " TOSTRING(FOOBAR2000_SDK_VERSION) "\n"
            "on " __DATE__ " " __TIME__ ".\n"
            "\n"
            "BASS SDK " BASSVERSIONTEXT "\n"
            "FluidSynth SDK " FLUIDSYNTH_VERSION "\n"
            "mt32emu " MT32EMU_VERSION "\n"
            "CLAP SDK " CLAP_SDK_VERSION "\n"
            "LibADLMIDI " ADLMIDI_VERSION "\n"
            "LibOPNMIDI " OPNMIDI_VERSION "\n"
    );

    VALIDATE_COMPONENT_FILENAME(STR_COMPONENT_FILENAME);
}
