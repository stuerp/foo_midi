
/** $VER: PreferencesProcessingLayout.h (2024.10.08) **/

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
#define IY           2

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

#pragma region Channels

// Groupbox
#define X_E00    X_D00
#define Y_E00    Y_D00 + H_D00 + IY

    // Button: Channel 01
    #define X_E11    X_E00 + 5
    #define Y_E11    Y_E00 + 11
    #define W_E11    16
    #define H_E11    H_BTN

    // Button: Channel 02
    #define X_E12    X_E11 + W_E11 + IX
    #define Y_E12    Y_E00 + 11
    #define W_E12    16
    #define H_E12    H_BTN

    // Button: Channel 03
    #define X_E13    X_E12 + W_E12 + IX
    #define Y_E13    Y_E00 + 11
    #define W_E13    16
    #define H_E13    H_BTN

    // Button: Channel 04
    #define X_E14    X_E13 + W_E13 + IX
    #define Y_E14    Y_E00 + 11
    #define W_E14    16
    #define H_E14    H_BTN

    // Button: Channel 05
    #define X_E15    X_E14 + W_E14 + IX
    #define Y_E15    Y_E00 + 11
    #define W_E15    16
    #define H_E15    H_BTN

    // Button: Channel 06
    #define X_E16    X_E15 + W_E15 + IX
    #define Y_E16    Y_E00 + 11
    #define W_E16    16
    #define H_E16    H_BTN

    // Button: Channel 07
    #define X_E17    X_E16 + W_E16 + IX
    #define Y_E17    Y_E00 + 11
    #define W_E17    16
    #define H_E17    H_BTN

    // Button: Channel 08
    #define X_E18    X_E17 + W_E17 + IX
    #define Y_E18    Y_E00 + 11
    #define W_E18    16
    #define H_E18    H_BTN

    // Button: Channel 09
    #define X_E19    X_E18 + W_E18 + IX
    #define Y_E19    Y_E00 + 11
    #define W_E19    16
    #define H_E19    H_BTN

    // Button: Channel 10
    #define X_E20    X_E19 + W_E19 + IX
    #define Y_E20    Y_E00 + 11
    #define W_E20    16
    #define H_E20    H_BTN

    // Button: Channel 11
    #define X_E21    X_E20 + W_E20 + IX
    #define Y_E21    Y_E00 + 11
    #define W_E21    16
    #define H_E21    H_BTN

    // Button: Channel 12
    #define X_E22    X_E21 + W_E21 + IX
    #define Y_E22    Y_E00 + 11
    #define W_E22    16
    #define H_E22    H_BTN

    // Button: Channel 13
    #define X_E23    X_E22 + W_E22 + IX
    #define Y_E23    Y_E00 + 11
    #define W_E23    16
    #define H_E23    H_BTN

    // Button: Channel 14
    #define X_E24    X_E23 + W_E23 + IX
    #define Y_E24    Y_E00 + 11
    #define W_E24    16
    #define H_E24    H_BTN

    // Button: Channel 15
    #define X_E25    X_E24 + W_E24 + IX
    #define Y_E25    Y_E00 + 11
    #define W_E25    16
    #define H_E25    H_BTN

    // Button: Channel 16
    #define X_E26    X_E25 + W_E25 + IX
    #define Y_E26    Y_E00 + 11
    #define W_E26    16
    #define H_E26    H_BTN

    // Button: All
    #define X_E30    X_E00 + 5
    #define Y_E30    Y_E11 + H_E11 + IY
    #define W_E30    W_BTN
    #define H_E30    H_BTN

    // Button: None
    #define X_E32    X_E30 + W_E30 + IX
    #define Y_E32    Y_E30
    #define W_E32    W_BTN
    #define H_E32    H_BTN

    // Button: 1 - 10
    #define X_E34    X_E32 + W_E32 + IX
    #define Y_E34    Y_E32
    #define W_E34    W_BTN
    #define H_E34    H_BTN

    // Button: 11 - 16
    #define X_E36    X_E34 + W_E34 + IX
    #define Y_E36    Y_E34
    #define W_E36    W_BTN
    #define H_E36    H_BTN

    #pragma endregion

#define W_E00   5 + W_E11 + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + (IX + W_E12) + 5
#define H_E00   11 + H_E11 + IY + H_E30 + 7

#pragma endregion
