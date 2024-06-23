
/** $VER: PreferencesProcessingLayout.h (2024.05.19) **/

#pragma once

#define W_A00    332 // Dialog width as set by foobar2000, in dialog units
#define H_A00    288 // Dialog height as set by foobar2000, in dialog units

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
#define IY           3

#pragma region Recomposer

// Groupbox
#define X_C00    0
#define Y_C00    0

    #pragma region Loop expansion

    // Label
    #define X_C11    X_C00 + 5
    #define Y_C11    Y_C00 + 11
    #define W_C11    52
    #define H_C11    H_LBL

    // EditBox
    #define X_C12    X_C11 + W_C11 + IX
    #define Y_C12    Y_C11
    #define W_C12    24
    #define H_C12    H_EBX

    // Label
    #define X_C13    X_C12 + W_C12 + IX
    #define Y_C13    Y_C11
    #define W_C13    18
    #define H_C13    H_LBL

    #pragma endregion

    // Checkbox
    #define X_C20    X_C11
    #define Y_C20    Y_C12 + H_C12 + IY
    #define W_C20    86
    #define H_C20    H_CHB

    // Checkbox
    #define X_C21    X_C20
    #define Y_C21    Y_C20 + H_C20 + IY
    #define W_C21    86
    #define H_C21    H_CHB

    // Checkbox
    #define X_C22    X_C21
    #define Y_C22    Y_C21 + H_C21 + IY
    #define W_C22    86
    #define H_C22    H_CHB

    // Checkbox
    #define X_C23    X_C22
    #define Y_C23    Y_C22 + H_C22 + IY
    #define W_C23    86
    #define H_C23    H_CHB

    // Checkbox
    #define X_C24    X_C23
    #define Y_C24    Y_C23 + H_C23 + IY
    #define W_C24    86
    #define H_C24    H_CHB

    // Checkbox
    #define X_C25    X_C24
    #define Y_C25    Y_C24 + H_C24 + IY
    #define W_C25    86
    #define H_C25    H_CHB

#define W_C00    5 + W_C11 + IX + W_C12 + IX + W_C13 + 5
#define H_C00   11 + H_C12 + IY + H_C20 + IY + H_C21 + IY + H_C22 + IY + H_C23 + IY + H_C24 + IY + H_C25 + 7

#pragma endregion

#pragma region HMI / HMP

// Groupbox
#define X_D00    X_C00
#define Y_D00    Y_C00 + H_C00 + IY

    #pragma region Default tempo

    // Label
    #define X_D11    X_D00 + 5
    #define Y_D11    Y_D00 + 11
    #define W_D11    52
    #define H_D11    H_LBL

    // EditBox
    #define X_D12    X_D11 + W_D11 + IX
    #define Y_D12    Y_D11
    #define W_D12    24
    #define H_D12    H_EBX

    // Label
    #define X_D13    X_D12 + W_D12 + IX
    #define Y_D13    Y_D11
    #define W_D13    18
    #define H_D13    H_LBL

    #pragma endregion

#define W_D00   5 + W_D11 + IX + W_D12 + IX + W_D13 + 5
#define H_D00   11 + H_D12 + 7

#pragma endregion
