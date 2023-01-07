
/** $VER: PreferencesPathsLayout.h (2023.01.07) **/

#pragma once

#define W_B00   332 // Dialog width as set by foobar2000, in dialog units
#define H_B00   288 // Dialog height as set by foobar2000, in dialog units

#pragma region("VSTi Plugin path")
// Label
#define X_B11    7
#define Y_B11    7
#define W_B11    48
#define H_B11    8

// Button
#define W_B13    16
#define H_B13    14
#define X_B13    W_B00 - 7 - W_B13
#define Y_B13    Y_B12

// EditBox
#define X_B12    X_B11 + W_B11 + 3
#define Y_B12    Y_B11
#define W_B12    W_B00 - 7 - W_B11 - 3 - W_B13 - 3 - 7
#define H_B12    14
#pragma endregion

#pragma region("SoundFont path")
// Label
#define X_B21    X_B11
#define Y_B21    Y_B12 + H_B12 + 4
#define W_B21    W_B11
#define H_B21    8

// Button
#define W_B23    16
#define H_B23    14
#define X_B23    W_B00 - 7 - W_B23
#define Y_B23    Y_B22

// EditBox
#define X_B22    X_B21 + W_B21 + 3
#define Y_B22    Y_B21
#define W_B22    W_B00 - 7 - W_B21 - 3 - W_B23 - 3 - 7
#define H_B22    14
#pragma endregion

#pragma region("Munt path")
// Label
#define X_B31    X_B21
#define Y_B31    Y_B22 + H_B22 + 4
#define W_B31    W_B21
#define H_B31    8

// Button
#define W_B33    16
#define H_B33    14
#define X_B33    W_B00 - 7 - W_B33
#define Y_B33    Y_B32

// EditBox
#define X_B32    X_B31 + W_B31 + 3
#define Y_B32    Y_B31
#define W_B32    W_B00 - 7 - W_B31 - 3 - W_B33 - 3 - 7
#define H_B32    14
#pragma endregion

#pragma region("SecretSauce path")
// Label
#define X_B41    X_B31
#define Y_B41    Y_B32 + H_B32 + 4
#define W_B41    W_B31
#define H_B41    8

// Button
#define W_B43    16
#define H_B43    14
#define X_B43    W_B00 - 7 - W_B43
#define Y_B43    Y_B42

// EditBox
#define X_B42    X_B41 + W_B41 + 3
#define Y_B42    Y_B41
#define W_B42    W_B00 - 7 - W_B41 - 3 - W_B43 - 3 - 7
#define H_B42    14
#pragma endregion
