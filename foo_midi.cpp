
/** $VER: foo_midi.cpp (2022.12.30) **/

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/componentversion.h>

#include "History.h"

#include "resource.h"

#pragma warning(disable: 4265 4625 4626 5026 5027 26433 26436 26455)
DECLARE_COMPONENT_VERSION
(
    COMPONENT_NAME, COMPONENT_VERSION,
    COMPONENT_BASENAME " " COMPONENT_VERSION "\n"
    "\n"
    COMPONENT_DESCRIPTION
);

VALIDATE_COMPONENT_FILENAME(COMPONENT_FILENAME);

