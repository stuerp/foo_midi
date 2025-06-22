
/** $VER: PreferencesRootLayout.h (2024.06.09) **/

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

#pragma region Output

// Groupbox
#define X_A10    0
#define Y_A10    0

    // Label
    #define X_A11    X_A10 + 5
    #define Y_A11    Y_A10 + 11
    #define W_A11    26
    #define H_A11    8

    // Combobox
    #define X_A12    X_A11 + W_A11 + IX
    #define Y_A12    Y_A11
    #define W_A12    134
    #define H_A12    H_CBX

    // Button
    #define X_A13    X_A12 + W_A12 + IX
    #define Y_A13    Y_A12 - 1
    #define W_A13    50
    #define H_A13    H_BTN

    // Label
    #define X_A14    X_A13 + W_A13 + IX
    #define Y_A14    Y_A13
    #define W_A14    42
    #define H_A14    H_LBL

    // Combobox
    #define X_A15    X_A14 + W_A14 + IX
    #define Y_A15    Y_A11
    #define W_A15    40
    #define H_A15    H_CBX

#define W_A10    5 + W_A11 + IX + W_A12 + IX + W_A13 + IX + W_A14 + IX + W_A15 + 5
#define H_A10    11 + H_A12 + 7
#pragma endregion

#pragma region Looping

// Groupbox
#define X_A20    X_A10
#define Y_A20    Y_A10 + H_A10 + IY
#define W_A20    157

    // Label: Playback
    #define X_A21    X_A20 + 5
    #define Y_A21    Y_A20 + 11
    #define W_A21    40
    #define H_A21    H_LBL

    // Combobox: Playback
    #define X_A22    X_A21 + W_A21 + IX
    #define Y_A22    Y_A21
    #define W_A22    W_A20 - 5 - W_A21 - IX - 5
    #define H_A22    H_CBX

    // Label: Other
    #define X_A23    X_A21
    #define Y_A23    Y_A22 + H_A22 + IY
    #define W_A23    W_A21
    #define H_A23    H_LBL

    // Combobox: Other
    #define X_A24    X_A22
    #define Y_A24    Y_A23
    #define W_A24    W_A22
    #define H_A24    H_CBX

    // Label: Decay
    #define X_A29    X_A21
    #define Y_A29    Y_A24 + H_A24 + IY
    #define W_A29    W_A23
    #define H_A29    H_LBL

    // Textbox: Decay
    #define X_A2A    X_A24
    #define Y_A2A    Y_A24 + H_A24 + IY
    #define W_A2A    32
    #define H_A2A    H_EBX

    // Label (ms)
    #define X_A2B    X_A2A + W_A2A + IX
    #define Y_A2B    Y_A2A
    #define W_A2B    10
    #define H_A2B    H_LBL

    // Checkbox
    #define X_A25    X_A29
    #define Y_A25    Y_A2A + H_A2A + IY
    #define W_A25    W_A20 - 5 - 6
    #define H_A25    H_CHB

    // Checkbox
    #define X_A26    X_A25
    #define Y_A26    Y_A25 + H_A25 + IY
    #define W_A26    W_A25
    #define H_A26    H_CHB

    // Checkbox
    #define X_A27    X_A26
    #define Y_A27    Y_A26 + H_A26 + IY
    #define W_A27    W_A26
    #define H_A27    H_CHB

    // Checkbox
    #define X_A28    X_A27
    #define Y_A28    Y_A27 + H_A27 + IY
    #define W_A28    W_A27
    #define H_A28    H_CHB

    // Checkbox
    #define X_A2C    X_A28
    #define Y_A2C    Y_A28 + H_A28 + IY
    #define W_A2C    W_A28
    #define H_A2C    H_CHB

#define H_A20    11 + H_A22 + IY + H_A2A + IY + H_A24 + IY + H_A25 + IY + H_A26 + IY + H_A27 + IY + H_A28 + IY + H_A2C + 7

#pragma endregion

#pragma region MIDI

// Groupbox
#define X_A80    X_A20
#define Y_A80    Y_A20 + H_A20 + 4
#define W_A80    W_A20

    // Label: Flavor
    #define X_A81    X_A80 + 5
    #define Y_A81    Y_A80 + 11
    #define W_A81    24
    #define H_A81    H_LBL

    // Combobox: Flavor
    #define X_A82    X_A81 + W_A81 + IX
    #define Y_A82    Y_A81
    #define W_A82    W_A80 - 5 - W_A81 - IX - 5
    #define H_A82    H_CBX

    // Checkbox: Filter effects
    #define X_A83    X_A81
    #define Y_A83    Y_A82 + H_A82 + IY
    #define W_A83    W_A80 - 5 - 5
    #define H_A83    H_CHB

    // Checkbox: Use Super Munt with MT-32
    #define X_A84    X_A81
    #define Y_A84    Y_A83 + H_A83 + 4
    #define W_A84    W_A80 - 5 - 5
    #define H_A84    H_CHB

    // Checkbox: Use VSTi with XG
    #define X_A85    X_A81
    #define Y_A85    Y_A84 + H_A84 + 4
    #define W_A85    W_A80 - 5 - 5
    #define H_A85    H_CHB

#define H_A80    11 + H_A82 + IY + H_A83 + IY + H_A84 + IY + H_A85 + 7

#pragma endregion

#pragma region Miscellaneous

// Groupbox
#define X_A30    X_A80
#define Y_A30    Y_A80 + H_A80 + IY
#define W_A30    W_A80

    // Checkbox
    #define X_A31    X_A30 + 5
    #define Y_A31    Y_A30 + 11
    #define W_A31    W_A30 - 5 - 5
    #define H_A31    10

    // Checkbox
    #define X_A32    X_A31
    #define Y_A32    Y_A31 + H_A31 + IY
    #define W_A32    W_A31
    #define H_A32    10

    // Checkbox
    #define X_A33    X_A32
    #define Y_A33    Y_A32 + H_A32 + IY
    #define W_A33    W_A32
    #define H_A33    10

#define H_A30    11 + H_A31 + IY + H_A32 + IY + H_A33 + 7

#pragma endregion

#pragma region FluidSynth 

// Groupbox
#define X_A40    X_A20 + W_A20 + 4
#define Y_A40    Y_A20
#define W_A40    W_A20

    // Label
    #define X_A41    X_A40 + 5
    #define Y_A41    Y_A40 + 11
    #define W_A41    48
    #define H_A41    H_LBL

    // Combobox
    #define X_A42    X_A41 + W_A41 + 3
    #define Y_A42    Y_A41
    #define W_A42    W_A40 - 5 - W_A41 - IX - 5
    #define H_A42    H_CBX

#define H_A40    11 + H_A42 + 7

#pragma endregion

#pragma region BASS MIDI

// Groupbox
#define X_A50    X_A40
#define Y_A50    Y_A40 + H_A40 + IY
#define W_A50    W_A40

    // Label
    #define X_A57    X_A50 + 5
    #define Y_A57    Y_A50 + 11
    #define W_A57    42
    #define H_A57    H_LBL

    // EditBox: Volume
    #define X_A58    X_A57 + W_A57 + IX
    #define Y_A58    Y_A57
    #define W_A58    20
    #define H_A58    H_EBX

    // Label
    #define X_A53    X_A57
    #define Y_A53    Y_A58 + H_A58 + IY
    #define W_A53    42
    #define H_A53    H_LBL

    // Combobox: Resampling
    #define X_A54    X_A53 + W_A53 + IX
    #define Y_A54    Y_A53
    #define W_A54    W_A50 - 5 - W_A53 - IX - 5
    #define H_A54    H_CBX

    // Label
    #define X_A55    X_A53
    #define Y_A55    Y_A54 + H_A54 + IY
    #define W_A55    W_A53
    #define H_A55    H_LBL

    // Label: Cached
    #define X_A56    X_A55 + W_A55 + IX
    #define Y_A56    Y_A55
    #define W_A56    W_A50 - 5 - W_A55 - IX - 5
    #define H_A56    H_LBL

#define H_A50    11 + H_A58 + IY + H_A54 + IY + H_A56 + 7

#pragma endregion

#pragma region Munt 

// Groupbox
#define X_A60    X_A50
#define Y_A60    Y_A50 + H_A50 + IY
#define W_A60    W_A50

    // Label: GM Set
    #define X_A63    X_A60 + 5
    #define Y_A63    Y_A60 + 11
    #define W_A63    40
    #define H_A63    H_LBL

    // Combobox: GM Set
    #define X_A64    X_A63 + W_A63 + IX
    #define Y_A64    Y_A63
    #define W_A64    W_A60 - 5 - W_A63 - IX - 5
    #define H_A64    H_CBX

#define H_A60    11 + H_A64 + 7

#pragma endregion

#pragma region Nuke

// Groupbox
#define X_A70    X_A60
#define Y_A70    Y_A60 + H_A60 + IY
#define W_A70    W_A60

    // Label: Preset
    #define X_A71    X_A70 + 5
    #define Y_A71    Y_A70 + 11
    #define W_A71    24
    #define H_A71    H_LBL

    // Combobox: Preset
    #define X_A72    X_A71 + W_A71 + IX
    #define Y_A72    Y_A71
    #define W_A72    W_A70 - 5 - W_A71 - IX - 5
    #define H_A72    H_CBX

    // Checkbox
    #define X_A73    X_A71
    #define Y_A73    Y_A72 + H_A72 + IY
    #define W_A73    W_A70 - 5 - 5
    #define H_A73    H_CHB

#define H_A70    11 + H_A72 + IY + H_A73 + 7

#pragma endregion
