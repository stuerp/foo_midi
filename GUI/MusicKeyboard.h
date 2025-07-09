
/** $VER: MusicKeyboard.h (2023.12.24) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 4514 5045 5262 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>

#include "Keyboard.h"
#include "API.h"

namespace foo_vis_midi
{
class MusicKeyboard : public IMusicKeyboard
{
public:
    explicit MusicKeyboard() { }

    MusicKeyboard(const MusicKeyboard &) = delete;
    MusicKeyboard(MusicKeyboard &&) = delete;
    MusicKeyboard & operator=(const MusicKeyboard &) = delete;
    MusicKeyboard & operator=(MusicKeyboard &&) = delete;

    virtual ~MusicKeyboard() { }

    #pragma region IMusicKeyboard

    [[nodiscard]] void Initialize(uint32_t) noexcept override;
    [[nodiscard]] void ProcessMessage(uint32_t, uint32_t) noexcept override;
    [[nodiscard]] void SetPosition(uint32_t) noexcept override;

    #pragma endregion

private:
};
}
