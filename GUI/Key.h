
/** $VER: Key.h (2025.07.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 5262 ALL_CPPCORECHECK_WARNINGS)

#include "pch.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>


const float WhiteKeyWidth = 23.5f; // mm
const float WhiteKeyHeight = 150.0f; // mm

const float BlackKeyWidth = 13.7f; // mm
const float BlackKeyHeight = 90.0f; // mm

#pragma warning(disable: 4820)

/// <summary>
/// Implements a base class for a keyboard key.
/// </summary>
class Key
{
public:
    Key(ID2D1PathGeometry * geometry) noexcept : _Geometry(geometry), _IsDown(false) { }

    Key(const Key &) = delete;
    Key & operator=(const Key &) = delete;
    Key(Key &&) = delete;
    Key & operator=(Key &&) = delete;

    virtual ~Key()
    {
    };

    virtual HRESULT Measure(D2D1_SIZE_F & size) const noexcept = 0;
    virtual HRESULT Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float & dx) const noexcept = 0;

    float GetHeight() const { return WhiteKeyHeight; }

    void Push() { _IsDown = true; }
    void Release() { _IsDown = false; }

protected:
    ID2D1PathGeometry * _Geometry;
    bool _IsDown;
};

/// <summary>
/// Implements a white, left keyboard key.
/// </summary>
class LeftWhiteKey : public Key
{
public:
    LeftWhiteKey(ID2D1PathGeometry * geometry) noexcept : Key(geometry) { }

    LeftWhiteKey(const LeftWhiteKey &) = delete;
    LeftWhiteKey & operator=(const LeftWhiteKey &) = delete;
    LeftWhiteKey(LeftWhiteKey &&) = delete;
    LeftWhiteKey & operator=(LeftWhiteKey &&) = delete;

    ~LeftWhiteKey() final { };

    virtual HRESULT Measure(D2D1_SIZE_F & size) const noexcept
    {
        size = { WhiteKeyWidth, WhiteKeyHeight };

        return S_OK;
    }

    virtual HRESULT Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float & dx) const noexcept final;
};

/// <summary>
/// Implements a white, middle keyboard key.
/// </summary>
class MiddleWhiteKey : public Key
{
public:
    MiddleWhiteKey(ID2D1PathGeometry * geometry) noexcept : Key(geometry) { }

    MiddleWhiteKey(const MiddleWhiteKey &) = delete;
    MiddleWhiteKey & operator=(const MiddleWhiteKey &) = delete;
    MiddleWhiteKey(MiddleWhiteKey &&) = delete;
    MiddleWhiteKey & operator=(MiddleWhiteKey &&) = delete;

    ~MiddleWhiteKey() final { };

    virtual HRESULT Measure(D2D1_SIZE_F & size) const noexcept
    {
        size = { WhiteKeyWidth, WhiteKeyHeight };

        return S_OK;
    }

    virtual HRESULT Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float & dx) const noexcept final;
};

/// <summary>
/// Implements a white, right keyboard key.
/// </summary>
class RightWhiteKey : public Key
{
public:
    RightWhiteKey(ID2D1PathGeometry * geometry) noexcept : Key(geometry) { }

    RightWhiteKey(const RightWhiteKey &) = delete;
    RightWhiteKey & operator=(const RightWhiteKey &) = delete;
    RightWhiteKey(RightWhiteKey &&) = delete;
    RightWhiteKey & operator=(RightWhiteKey &&) = delete;

    ~RightWhiteKey() final { };

    virtual HRESULT Measure(D2D1_SIZE_F & size) const noexcept
    {
        size = { WhiteKeyWidth, WhiteKeyHeight };

        return S_OK;
    }

    virtual HRESULT Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float & dx) const noexcept final;
};

/// <summary>
/// Implements a black keyboard key.
/// </summary>
class BlackKey : public Key
{
public:
    BlackKey() noexcept : Key(nullptr) { };

    BlackKey(const BlackKey &) = delete;
    BlackKey & operator=(const BlackKey &) = delete;
    BlackKey(BlackKey &&) = delete;
    BlackKey & operator=(BlackKey &&) = delete;

    ~BlackKey() final { };

    virtual HRESULT Measure(D2D1_SIZE_F & size) const noexcept
    {
        size = { 0.f, 0.f };

        return S_OK;
    }

    virtual HRESULT Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float & dx) const noexcept final;

private:
    const D2D1_RECT_F _Rect = { 0, 0, BlackKeyWidth, BlackKeyHeight };
};

/// <summary>
/// Implements a white keyboard key.
/// </summary>
class WhiteKey : public Key
{
public:
    WhiteKey() noexcept : Key(nullptr) { };

    WhiteKey(const WhiteKey &) = delete;
    WhiteKey & operator=(const WhiteKey &) = delete;
    WhiteKey(WhiteKey &&) = delete;
    WhiteKey & operator=(WhiteKey &&) = delete;

    ~WhiteKey() final { };

    virtual HRESULT Measure(D2D1_SIZE_F & size) const noexcept
    {
        size = { WhiteKeyWidth, WhiteKeyHeight };

        return S_OK;
    }

    virtual HRESULT Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float & dx) const noexcept final;

private:
    const D2D1_RECT_F _Rect = { 0, 0, WhiteKeyWidth, WhiteKeyHeight };
};

#pragma warning(default: 4820)
