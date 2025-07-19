
/** $VER: MusicKeyboard.cpp (2025.07.09) P. Stuer **/

#include "pch.h"

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 5262 ALL_CPPCORECHECK_WARNINGS)

#include "MusicKeyboard.h"
#include "MusicKeyboardUIElement.h"

#pragma hdrstop

extern MusicKeyboardWindow * _CLAPWindow;

#pragma region IMusicKeyboard

[[nodiscard]] void MusicKeyboard::Initialize(uint32_t interfaceVersion) noexcept
{
    if (_CLAPWindow)
        _CLAPWindow->Initialize(interfaceVersion);
}

[[nodiscard]] void MusicKeyboard::ProcessMessage(uint32_t message, uint32_t timestamp) noexcept
{
    if (_CLAPWindow)
        _CLAPWindow->ProcessMessage(message, timestamp);
}

[[nodiscard]] void MusicKeyboard::SetPosition(uint32_t position) noexcept
{
    if (_CLAPWindow)
        _CLAPWindow->SetPosition(position);
}

#pragma endregion
