
/** $VER: foo_midi.cpp (2023.06.12) **/

#include "framework.h"

#include <sdk/foobar2000-lite.h>
#include <sdk/componentversion.h>

#include "History.h"

#include "resource.h"

#pragma hdrstop

namespace
{
    #pragma warning(disable: 4265 4625 4626 5026 5027 26433 26436 26455)
    DECLARE_COMPONENT_VERSION
    (
        STR_COMPONENT_NAME,
        STR_COMPONENT_VERSION,
        STR_COMPONENT_BASENAME " " STR_COMPONENT_VERSION "\n"
            "Copyright (c) 2022-2023 LoSno.co. All rights reserved.\n"
            "Written by kode54, P. Stuer\n"
            "\n"
            "Adds playback of MIDI files.\n"
            "\n"
            "Built with foobar2000 SDK " TOSTRING(FOOBAR2000_SDK_VERSION) "\n"
            "on " __DATE__ " " __TIME__ ".\n"
        "\n"
        COMPONENT_DESCRIPTION
    );

    VALIDATE_COMPONENT_FILENAME(STR_COMPONENT_FILENAME);
}
