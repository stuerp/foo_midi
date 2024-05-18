
/** $VER: PreferencesRecomposer.h (2024.05.16) **/

#pragma once

#define W_A00    332 // Dialog width as set by foobar2000, in dialog units
#define H_A00    288 // Dialog height as set by foobar2000, in dialog units

#define IX      3
#define IY      4

#pragma region("Loop expansion")

// Label
#define X_C11    7
#define Y_C11    7
#define W_C11    58
#define H_C11    8

// EditBox
#define X_C12    X_C11 + W_C11 + 3
#define Y_C12    Y_C11
#define W_C12    24
#define H_C12    14

#pragma endregion

// Checkbox
#define X_C20    X_C11
#define Y_C20    Y_C12 + H_C12 + IY
#define W_C20    86
#define H_C20    8

// Checkbox
#define X_C21    X_C20
#define Y_C21    Y_C20 + H_C20 + IY
#define W_C21    86
#define H_C21    8

// Checkbox
#define X_C22    X_C21
#define Y_C22    Y_C21 + H_C21 + IY
#define W_C22    86
#define H_C22    8

// Checkbox
#define X_C23    X_C22
#define Y_C23    Y_C22 + H_C22 + IY
#define W_C23    86
#define H_C23    8

// Checkbox
#define X_C24    X_C23
#define Y_C24    Y_C23 + H_C23 + IY
#define W_C24    86
#define H_C24    8

// Checkbox
#define X_C25    X_C24
#define Y_C25    Y_C24 + H_C24 + IY
#define W_C25    86
#define H_C25    8
