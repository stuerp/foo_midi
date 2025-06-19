
/** $VER: PreferencesPathsLayout.h (2025.06.19) **/

#pragma once

#define W_B00   332 // Dialog width as set by foobar2000, in dialog units
#define H_B00   288 // Dialog height as set by foobar2000, in dialog units

#define H_LBL        8 // Label

#define W_BTN       50 // Button
#define H_BTN       14 // Button

#define H_EBX       12 // Edit box
#define H_CBX       14 // Combo box

#define W_CHB       10 // Check box
#define H_CHB       10 // Check box

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           2

#pragma region VSTi Plugin path

// Label
#define X_B11    0
#define Y_B11    0
#define W_B11    64
#define H_B11    8

// EditBox
#define X_B12    X_B11 + W_B11 + IX
#define Y_B12    Y_B11
#define W_B12    200
#define H_B12    H_EBX

// Button
#define X_B13    X_B12 + W_B12 + IX
#define Y_B13    Y_B12
#define W_B13    16
#define H_B13    H_BTN

#pragma endregion

#pragma region SoundFont path

// Label
#define X_B21    X_B11
#define Y_B21    Y_B12 + H_B12 + 4
#define W_B21    W_B11
#define H_B21    8

// EditBox
#define X_B22    X_B21 + W_B21 + IX
#define Y_B22    Y_B21
#define W_B22    200
#define H_B22    H_EBX

// Button
#define X_B23    X_B22 + W_B22 + IX
#define Y_B23    Y_B22
#define W_B23    16
#define H_B23    H_BTN

#pragma endregion

#pragma region Munt path

// Label
#define X_B31    X_B21
#define Y_B31    Y_B22 + H_B22 + 4
#define W_B31    W_B21
#define H_B31    8

// EditBox
#define X_B32    X_B31 + W_B31 + IX
#define Y_B32    Y_B31
#define W_B32    200
#define H_B32    H_EBX

// Button
#define X_B33    X_B32 + W_B32 + IX
#define Y_B33    Y_B32
#define W_B33    16
#define H_B33    H_BTN

#pragma endregion

#pragma region SecretSauce path

// Label
#define X_B41    X_B31
#define Y_B41    Y_B32 + H_B32 + 4
#define W_B41    W_B31
#define H_B41    8

// EditBox
#define X_B42    X_B41 + W_B41 + IX
#define Y_B42    Y_B41
#define W_B42    200
#define H_B42    H_EBX

// Button
#define X_B43    X_B42 + W_B42 + IX
#define Y_B43    Y_B42
#define W_B43    16
#define H_B43    H_BTN

#pragma endregion

#pragma region FluidSynth path
// Label
#define X_B51    X_B41
#define Y_B51    Y_B42 + H_B42 + 4
#define W_B51    W_B41
#define H_B51    8

// EditBox
#define X_B52    X_B51 + W_B51 + IX
#define Y_B52    Y_B51
#define W_B52    200
#define H_B52    H_EBX

// Button
#define X_B53    X_B52 + W_B52 + IX
#define Y_B53    Y_B52
#define W_B53    16
#define H_B53    H_BTN

#pragma endregion

#pragma region fmmidi path
// Label
#define X_B54    X_B51
#define Y_B54    Y_B52 + H_B52 + 4
#define W_B54    W_B51
#define H_B54    8

// EditBox
#define X_B55    X_B54 + W_B54 + IX
#define Y_B55    Y_B54
#define W_B55    200
#define H_B55    H_EBX

// Button
#define X_B56    X_B55 + W_B55 + IX
#define Y_B56    Y_B55
#define W_B56    16
#define H_B56    H_BTN

#pragma endregion
