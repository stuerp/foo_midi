
/** $VER: PreferencesRootLayout.h (2023.08.14) **/

#pragma once

#define W_A00    332 // Dialog width as set by foobar2000, in dialog units
#define H_A00    288 // Dialog height as set by foobar2000, in dialog units

#pragma region("Output")
// Groupbox
#define X_A10    7
#define Y_A10    7
#define W_A10    W_A00 - 7 - 7

// Label
#define X_A11    X_A10 + 5
#define Y_A11    Y_A10 + 11
#define W_A11    26
#define H_A11    8

// Combobox
#define W_A15    40
#define H_A15    14
#define X_A15    X_A10 + W_A10 - 7 - W_A15
#define Y_A15    Y_A11

// Label
#define W_A14    40
#define H_A14    8
#define X_A14    X_A15 - W_A14 - 3
#define Y_A14    Y_A13

// Button
#define W_A13    50
#define H_A13    14
#define X_A13    X_A14 - W_A13 - 4
#define Y_A13    Y_A12

// Combobox
#define X_A12    X_A11 + W_A11 + 3
#define Y_A12    Y_A11
#define W_A12    W_A10 - 5 - W_A11 - 3 - W_A13 - 4 - W_A14 - 3 - W_A15 - 7
#define H_A12    14

#define H_A10    11 + H_A12 + 4
#pragma endregion

#pragma region("Looping")
// Groupbox
#define X_A20    7
#define Y_A20    Y_A10 + H_A10 + 4
#define W_A20    157

    // Label
    #define X_A21    X_A20 + 5
    #define Y_A21    Y_A20 + 11 + 2
    #define W_A21    40
    #define H_A21    8

    // Combobox
    #define X_A22    X_A21 + W_A21 + 3
    #define Y_A22    Y_A21 - 2
    #define W_A22    W_A20 - 5 - W_A21 - 3 - 5
    #define H_A22    14

    // Label
    #define X_A23    X_A21
    #define Y_A23    Y_A22 + H_A22 + 4 + 2
    #define W_A23    W_A21
    #define H_A23    8

    // Combobox
    #define X_A24    X_A22
    #define Y_A24    Y_A23 - 2
    #define W_A24    W_A22
    #define H_A24    14

    // Label (Decay)
    #define X_A29    X_A21
    #define Y_A29    Y_A24 + H_A24 + 4 + 2
    #define W_A29    W_A21
    #define H_A29    8

    // Textbox (Decay)
    #define X_A2A    X_A22
    #define Y_A2A    Y_A24 + H_A24 + 4
    #define W_A2A    32
    #define H_A2A    12

    // Label (ms)
    #define X_A2B    X_A2A + W_A2A + 3
    #define Y_A2B    Y_A24 + H_A24 + 4 + 2
    #define W_A2B    10
    #define H_A2B    8

    // Checkbox
    #define X_A25    X_A23
    #define Y_A25    Y_A29 + H_A29 + 4
    #define W_A25    W_A20 - 5 - 6
    #define H_A25    10

    // Checkbox
    #define X_A26    X_A25
    #define Y_A26    Y_A25 + H_A25 + 4
    #define W_A26    W_A25
    #define H_A26    H_A25

    // Checkbox
    #define X_A27    X_A26
    #define Y_A27    Y_A26 + H_A26 + 4
    #define W_A27    W_A26
    #define H_A27    H_A26

    // Checkbox
    #define X_A28    X_A27
    #define Y_A28    Y_A27 + H_A27 + 4
    #define W_A28    W_A27
    #define H_A28    H_A27

#define H_A20    11 + H_A22 + 4 + H_A2A + 4 + H_A24 + 4 + H_A25 + 4 + H_A26 + 4 + H_A27 + 4 + H_A28 + 4
#pragma endregion

#pragma region("MIDI")
// Groupbox
#define X_A80    7
#define Y_A80    Y_A20 + H_A20 + 4
#define W_A80    W_A20

    // Label
    #define X_A81    X_A80 + 5
    #define Y_A81    Y_A80 + 11
    #define W_A81    24
    #define H_A81    8

    // Combobox
    #define X_A82    X_A81 + W_A81 + 3
    #define Y_A82    Y_A81
    #define W_A82    W_A80 - 5 - W_A81 - 3 - 6
    #define H_A82    14

    // Checkbox
    #define X_A83    X_A81
    #define Y_A83    Y_A82 + H_A82 + 4
    #define W_A83    W_A80 - 5 - 6
    #define H_A83    8

    // Checkbox: Use Super Munt with MT-32
    #define X_A84    X_A81
    #define Y_A84    Y_A83 + H_A83 + 4
    #define W_A84    W_A80 - 5 - 6
    #define H_A84    8

    // Checkbox: Use Secret Sauce with XG
    #define X_A85    X_A81
    #define Y_A85    Y_A84 + H_A84 + 4
    #define W_A85    W_A80 - 5 - 6
    #define H_A85    8

#define H_A80    11 + H_A82 + 4 + H_A83 + 4 + H_A84 + 4 + H_A85 + 4
#pragma endregion

#pragma region("Miscellaneous")
// Groupbox
#define X_A30    7
#define Y_A30    Y_A80 + H_A80 + 4
#define W_A30    W_A80

    // Checkbox
    #define X_A31    X_A30 + 5
    #define Y_A31    Y_A30 + 11
    #define W_A31    W_A30 - 5 - 6
    #define H_A31    10

    // Checkbox
    #define X_A32    X_A31
    #define Y_A32    Y_A31 + H_A31 + 4
    #define W_A32    W_A31
    #define H_A32    10

    // Checkbox
    #define X_A33    X_A32
    #define Y_A33    Y_A32 + H_A32 + 4
    #define W_A33    W_A32
    #define H_A33    10

#define H_A30    11 + H_A31 + 4 + H_A32 + 4 + H_A33 + 4
#pragma endregion

#pragma region("FluidSynth")
// Groupbox
#define X_A40    X_A20 + W_A20 + 4
#define Y_A40    Y_A20
#define W_A40    W_A20

    // Label
    #define X_A41    X_A40 + 5
    #define Y_A41    Y_A40 + 11
    #define W_A41    48
    #define H_A41    8

    // Combobox
    #define X_A42    X_A41 + W_A41 + 3
    #define Y_A42    Y_A41
    #define W_A42    W_A40 - 5 - W_A41 - 3 - 5
    #define H_A42    14

#define H_A40    11 + H_A42 + 4
#pragma endregion

#pragma region("BASS MIDI")
// Groupbox
#define X_A50    X_A40
#define Y_A50    Y_A40 + H_A40 + 4
#define W_A50    W_A40

    // Label
    #define X_A53    X_A50 + 5
    #define Y_A53    Y_A50 + 11
    #define W_A53    42
    #define H_A53    8

    // Combobox
    #define X_A54    X_A53 + W_A53 + 3
    #define Y_A54    Y_A53
    #define W_A54    W_A50 - 5 - W_A53 - 3 - 5
    #define H_A54    14

    // Label
    #define X_A55    X_A53
    #define Y_A55    Y_A54 + H_A54 + 4
    #define W_A55    W_A53
    #define H_A55    8

    // EditBox
    #define X_A56    X_A55 + W_A55 + 3
    #define Y_A56    Y_A55
    #define W_A56    W_A50 - 5 - W_A55 - 3 - 5
    #define H_A56    8

#define H_A50    11 + H_A54 + 4 + 14 + 4
#pragma endregion

#pragma region("Munt")
// Groupbox
#define X_A60    X_A50
#define Y_A60    Y_A50 + H_A50 + 4
#define W_A60    W_A50

    // Label
    #define X_A63    X_A60 + 5
    #define Y_A63    Y_A60 + 11
    #define W_A63    40
    #define H_A63    8

    // Combobox
    #define X_A64    X_A63 + W_A63 + 3
    #define Y_A64    Y_A63
    #define W_A64    W_A60 - 5 - W_A63 - 3 - 5
    #define H_A64    14

#define H_A60    11 + H_A64 + 4
#pragma endregion

#pragma region("ADL")
// ADL
#define X_A90    X_A60
#define Y_A90    Y_A60 + H_A60 + 4
#define W_A90    W_A60

    #define X_A91    X_A90 + 5
    #define Y_A91    Y_A90 + 11
    #define W_A91    22
    #define H_A91    8

    #define X_A92    X_A91 + W_A91 + 3
    #define Y_A92    Y_A91
    #define W_A92    W_A90 - 5 - W_A91 - 3 - 6
    #define H_A92    14

    #define X_A93    X_A91
    #define Y_A93    Y_A92 + H_A92 + 4
    #define W_A93    W_A91
    #define H_A93    8

    #define X_A94    X_A93 + W_A93 + 3
    #define Y_A94    Y_A93
    #define W_A94    40
    #define H_A94    14

    #define X_A95    X_A93
    #define Y_A95    Y_A94 + H_A94 + 4
    #define W_A95    W_A90 - 5 - 6
    #define H_A95    10

#define H_A90    11 + H_A92 + 4 + H_A94 + 4 + H_A95 + 4
#pragma endregion

#pragma region("Nuke")
// Groupbox
#define X_A70    X_A90
#define Y_A70    Y_A90 + H_A90 + 4
#define W_A70    W_A90

    // Label
    #define X_A71    X_A70 + 5
    #define Y_A71    Y_A70 + 11
    #define W_A71    24
    #define H_A71    8

    // Combobox
    #define X_A72    X_A71 + W_A71 + 3
    #define Y_A72    Y_A71
    #define W_A72    W_A70 - 5 - W_A71 - 3 - 6
    #define H_A72    14

    // Checkbox
    #define X_A73    X_A71
    #define Y_A73    Y_A72 + H_A72 + 4
    #define W_A73    W_A70 - 5 - 6
    #define H_A73    8

#define H_A70    11 + H_A72 + 4 + H_A73 + 4
#pragma endregion
