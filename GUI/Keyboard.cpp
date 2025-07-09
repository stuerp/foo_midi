
/** $VER: Keyboard.cpp (2023.01.15) P. Stuer **/

#include "pch.h"

#include "Keyboard.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance of the class.
/// </summary>
Keyboard::Keyboard() :
    _Width(0.f)
{
    SetScale(1.0f);
}

/// <summary>
/// Deletes this instance of the class.
/// </summary>
Keyboard::~Keyboard()
{
    Dispose();
}

/// <summary>
/// Renders the object.
/// </summary>
HRESULT Keyboard::Render(ID2D1HwndRenderTarget * renderTarget, float x, float y) const noexcept
{
    for (auto& Iterator : _Keys)
    {
        float dx;

        renderTarget->SetTransform(_ScaleMatrix * D2D1::Matrix3x2F::Translation(x, y));

        Iterator->Render(renderTarget, _WhiteBrush, _BlackBrush, _PushedBrush, dx);

        x += dx * _ScaleMatrix.m11;
    }

    return S_OK;
}

/// <summary>
/// Gets the width of the keyboard.
/// </summary>
float Keyboard::MeasureWidth() const noexcept
{
    float Width = 0.f;

    for (auto& Iterator : _Keys)
    {
        D2D1_SIZE_F KeySize;

        Iterator->Measure(KeySize);

        Width += KeySize.width + 1.f;
    }

    return Width;
}

/// <summary>
/// Creates the device-independent resources.
/// </summary>
HRESULT Keyboard::CreateDeviceIndependentResources(ID2D1Factory * d2DFactory)
{
    // Create the left white key.
    HRESULT hr = d2DFactory->CreatePathGeometry(&_LeftWhitePathGeometry);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> gs;

        hr = _LeftWhitePathGeometry->Open(&gs);

        if (SUCCEEDED(hr))
        {
            gs->SetFillMode(D2D1_FILL_MODE_WINDING);

            gs->BeginFigure(D2D1::Point2F(0, 0), D2D1_FIGURE_BEGIN_FILLED);

            const D2D1_POINT_2F Points[5] =
            {
               D2D1::Point2F(WhiteKeyWidth - BlackKeyWidth / 2, 0),
               D2D1::Point2F(WhiteKeyWidth - BlackKeyWidth / 2, BlackKeyHeight),
               D2D1::Point2F(WhiteKeyWidth, BlackKeyHeight),
               D2D1::Point2F(WhiteKeyWidth, WhiteKeyHeight),
               D2D1::Point2F( 0, WhiteKeyHeight), 
            };

            gs->AddLines(Points, ARRAYSIZE(Points));
            gs->EndFigure(D2D1_FIGURE_END_CLOSED);

            hr = gs->Close();
        }
    }

    // Create the middle white key.
    hr = d2DFactory->CreatePathGeometry(&_MiddleWhitePathGeometry);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> gs;

        hr = _MiddleWhitePathGeometry->Open(&gs);

        if (SUCCEEDED(hr))
        {
            gs->SetFillMode(D2D1_FILL_MODE_WINDING);

            gs->BeginFigure(D2D1::Point2F(BlackKeyWidth / 2, 0), D2D1_FIGURE_BEGIN_FILLED);

            const D2D1_POINT_2F Points[] =
            {
               D2D1::Point2F(WhiteKeyWidth - (BlackKeyWidth / 2),   0),
               D2D1::Point2F(WhiteKeyWidth - (BlackKeyWidth / 2), BlackKeyHeight),
               D2D1::Point2F(WhiteKeyWidth, BlackKeyHeight),
               D2D1::Point2F(WhiteKeyWidth, WhiteKeyHeight),
               D2D1::Point2F( 0, WhiteKeyHeight),
               D2D1::Point2F( 0, BlackKeyHeight),
               D2D1::Point2F(BlackKeyWidth / 2, BlackKeyHeight),
            };

            gs->AddLines(Points, ARRAYSIZE(Points));
            gs->EndFigure(D2D1_FIGURE_END_CLOSED);

            hr = gs->Close();
        }
    }

    // Create the right white key.
    hr = d2DFactory->CreatePathGeometry(&_RightWhitePathGeometry);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> gs;

        hr = _RightWhitePathGeometry->Open(&gs);

        if (SUCCEEDED(hr))
        {
            gs->SetFillMode(D2D1_FILL_MODE_WINDING);

            gs->BeginFigure(D2D1::Point2F(BlackKeyWidth / 2, 0), D2D1_FIGURE_BEGIN_FILLED);

            const D2D1_POINT_2F Points[5] =
            {
               D2D1::Point2F(WhiteKeyWidth, 0),
               D2D1::Point2F(WhiteKeyWidth, WhiteKeyHeight),
               D2D1::Point2F(0, WhiteKeyHeight),
               D2D1::Point2F(0, BlackKeyHeight),
               D2D1::Point2F(BlackKeyWidth / 2, BlackKeyHeight), 
            };

            gs->AddLines(Points, ARRAYSIZE(Points));
            gs->EndFigure(D2D1_FIGURE_END_CLOSED);

            hr = gs->Close();
        }
    }
 
    // Create the keys. Middle C = MIDI 60 = Index = 39.
    _Keys.reserve(88);

    _Keys.push_back(make_unique<LeftWhiteKey>(_LeftWhitePathGeometry));
    _Keys.push_back(make_unique<BlackKey>());
    _Keys.push_back(make_unique<RightWhiteKey>(_RightWhitePathGeometry));

    for (size_t i = 0; i < 7; i++)
    {
        _Keys.push_back(make_unique<LeftWhiteKey>(_LeftWhitePathGeometry));
        _Keys.push_back(make_unique<BlackKey>());
        _Keys.push_back(make_unique<MiddleWhiteKey>(_MiddleWhitePathGeometry));
        _Keys.push_back(make_unique<BlackKey>());
        _Keys.push_back(make_unique<RightWhiteKey>(_RightWhitePathGeometry));

        _Keys.push_back(make_unique<LeftWhiteKey>(_LeftWhitePathGeometry));
        _Keys.push_back(make_unique<BlackKey>());
        _Keys.push_back(make_unique<MiddleWhiteKey>(_MiddleWhitePathGeometry));
        _Keys.push_back(make_unique<BlackKey>());
        _Keys.push_back(make_unique<MiddleWhiteKey>(_MiddleWhitePathGeometry));
        _Keys.push_back(make_unique<BlackKey>());
        _Keys.push_back(make_unique<RightWhiteKey>(_RightWhitePathGeometry));
    }

    _Keys.push_back(make_unique<WhiteKey>());

    _Width = MeasureWidth();

    return hr;
}

/// <summary>
/// Creates the device-dependent resources.
/// </summary>
HRESULT Keyboard::CreateDeviceDependentResources(ID2D1HwndRenderTarget * renderTarget)
{
    HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &_BlackBrush);

    if (SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_WhiteBrush);

    if (SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DodgerBlue), &_PushedBrush);

    return hr;
}

/// <summary>
/// Disposes the device-dependent resources.
/// </summary>
void Keyboard::Dispose() noexcept
{
}
