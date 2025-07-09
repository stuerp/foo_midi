
/** $VER: Key.cpp (2023.01.14) P. Stuer **/

#include "pch.h"

#include "Key.h"

#pragma hdrstop

HRESULT LeftWhiteKey::Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float& dx) const noexcept
{
    renderTarget->FillGeometry(_Geometry, _IsDown ? pushedBrush : whiteBrush);
    renderTarget->DrawGeometry(_Geometry, blackBrush);

    dx = WhiteKeyWidth - (BlackKeyWidth / 2) + 1.f;

    return S_OK;
};

HRESULT MiddleWhiteKey::Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float& dx) const noexcept
{
    renderTarget->FillGeometry(_Geometry, _IsDown ? pushedBrush : whiteBrush);
    renderTarget->DrawGeometry(_Geometry, blackBrush);

    dx = WhiteKeyWidth - (BlackKeyWidth / 2) + 1.f;

    return S_OK;
};

HRESULT RightWhiteKey::Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float& dx) const noexcept
{
    renderTarget->FillGeometry(_Geometry, _IsDown ? pushedBrush : whiteBrush);
    renderTarget->DrawGeometry(_Geometry, blackBrush);

    dx = WhiteKeyWidth + 1.f;

    return S_OK;
};

HRESULT BlackKey::Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush *, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float& dx) const noexcept
{
    renderTarget->FillRectangle(&_Rect, _IsDown ? pushedBrush : blackBrush);

    dx = BlackKeyWidth / 2 + 1.f;

    return S_OK;
};

HRESULT WhiteKey::Render(ID2D1HwndRenderTarget * renderTarget, ID2D1Brush * whiteBrush, ID2D1Brush * blackBrush, ID2D1Brush * pushedBrush, float& dx) const noexcept
{
    renderTarget->FillRectangle(&_Rect, _IsDown ? pushedBrush : whiteBrush);
    renderTarget->DrawRectangle(&_Rect, blackBrush);

    dx = WhiteKeyWidth + 1.f;

    return S_OK;
};
