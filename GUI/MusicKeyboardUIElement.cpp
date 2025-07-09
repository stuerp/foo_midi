
/** $VER: MusicKeyboardUIElement.cpp (2025.07.09) P. Stuer **/

#include "pch.h"

#include "API.h"
#include "MusicKeyboardUIElement.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
MusicKeyboardWindow::MusicKeyboardWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr callback) : m_bMsgHandled(FALSE), m_callback(callback),
    _Config(config)
{
    _This = this;

    _Keyboard = std::make_unique<Keyboard>();
}

/// <summary>
/// Deletes this instance.
/// </summary>
MusicKeyboardWindow::~MusicKeyboardWindow()
{
    Dispose();

    _This = nullptr;
}

#pragma region CWindowImpl

/// <summary>
/// Creates the window.
/// </summary>
LRESULT MusicKeyboardWindow::OnCreate(LPCREATESTRUCT)
{
    ::InitializeCriticalSection(&_Lock);

    HRESULT hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Unable to create Direct2D device independent resources.";

    // Create the timer.
    _ThreadPoolTimer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void MusicKeyboardWindow::OnDestroy()
{
    ::EnterCriticalSection(&_Lock);

    if (_ThreadPoolTimer)
    {
        ::CloseThreadpoolTimer(_ThreadPoolTimer);
        _ThreadPoolTimer = nullptr;
    }

    _D2DFactory.Release();

    ::LeaveCriticalSection(&_Lock);
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void MusicKeyboardWindow::OnResize(UINT, CSize size)
{
    if (_RenderTarget == nullptr)
        return;

    _RenderTarget->Resize(D2D1::SizeU((UINT32)size.cx, (UINT32)size.cy));

    SetScale(_RenderTarget);
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void MusicKeyboardWindow::OnPaint(CDCHandle)
{
    Render();
    ValidateRect(nullptr);
}

/// <summary>
/// Handles a timer tick.
/// </summary>
void CALLBACK MusicKeyboardWindow::TimerCallback(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_TIMER /*timer*/) noexcept
{
    ((MusicKeyboardWindow *) context)->Render();
}

/// <summary>
/// Updates the refresh rate.
/// </summary>
void MusicKeyboardWindow::UpdateRefreshRateLimit() const noexcept
{
    if (_ThreadPoolTimer)
    {
        FILETIME DueTime = { };

        ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) 50, 0);
    }
}

#pragma endregion

#pragma region Rendering

HRESULT MusicKeyboardWindow::Render()
{
    if (!::TryEnterCriticalSection(&_Lock))
        return E_ABORT;

    MIDIEvent me;

    while ((_Events.size() > 0) && ((_Events.front().Timestamp + 64000) < _Position))
    {
        me = _Events.front();
        _Events.pop();

    #ifdef _DEBUG
//      wchar_t Line[256]; ::swprintf_s(Line, _countof(Line), L"%6d/%6d: %08X, %3d\n", (int) me.Timestamp, (int) _Position, me.Message, (int) _Events.size()); ::OutputDebugStringW(Line);
    #endif

        uint32_t Status = me.Message & 0xF0;

        if ((Status != 0x80) && (Status != 0x90))
            continue;

        uint32_t Channel = me.Message & 0x0F;

        if (Channel == 0x09)
            continue; // Don't render the percussion channel.

        uint32_t Note     = (me.Message >>  8) & 0xFF;
        uint32_t Velocity = (me.Message >> 16) & 0xFF;

        if (Status == 0x80)
            _Keyboard->ReleaseKey(Note);
        else
        if (Status == 0x90)
        {
            if (Velocity > 0)
                _Keyboard->PushKey(Note);
            else
                _Keyboard->ReleaseKey(Note);
        }
    }

    ::LeaveCriticalSection(&_Lock);

    HRESULT hr = CreateDeviceDependentResources();

    if (!SUCCEEDED(hr) || (_RenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
        return hr;

    D2D1_SIZE_F RenderTargetSize = _RenderTarget->GetSize();

    _RenderTarget->BeginDraw();

    {
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        _RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        // Paint the window background.
        _RenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, RenderTargetSize.width, RenderTargetSize.height), _GridPatternBrush);
    }

    {
        float x = 0.f;
        float y = RenderTargetSize.height - _Keyboard->GetHeight();

        hr = _Keyboard->Render(_RenderTarget, x, y);
    }

    hr = _RenderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        Dispose();
    }

    return hr;
}

/// <summary>
/// Recalculates the scale of the keyboard.
/// </summary>
void MusicKeyboardWindow::SetScale(ID2D1RenderTarget * renderTarget)
{
    if (_Keyboard == nullptr)
        return;

    float KeyboardWidth = ::ceilf(_Keyboard->GetWidth());

    D2D1_SIZE_F RenderTargetSize = renderTarget->GetSize();

    float Scale = RenderTargetSize.width / KeyboardWidth;

    _Keyboard->SetScale(Scale);
}

#pragma endregion

#pragma region Direct2D

/// <summary>
/// Creates resources which are not bound to any device.
/// </summary>
HRESULT MusicKeyboardWindow::CreateDeviceIndependentResources()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_D2DFactory);

    if (SUCCEEDED(hr))
        _Keyboard->CreateDeviceIndependentResources(_D2DFactory);

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular Direct3D device.
/// </summary>
HRESULT MusicKeyboardWindow::CreateDeviceDependentResources()
{
    if (_RenderTarget != nullptr)
        return S_OK;

    RECT rc;

    ::GetClientRect(m_hWnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT>(rc.right - rc.left), static_cast<UINT>(rc.bottom - rc.top));

    // Create a Direct2D render target.
    HRESULT hr = _D2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hWnd, size), &_RenderTarget);

    if (SUCCEEDED(hr))
        hr = CreateGridPatternBrush(_RenderTarget, &_GridPatternBrush);

    if (SUCCEEDED(hr))
        hr = _Keyboard->CreateDeviceDependentResources(_RenderTarget);

    if (SUCCEEDED(hr))
        SetScale(_RenderTarget);

    return hr;
}

/// <summary>
/// Creates a pattern brush.
/// </summary>
HRESULT MusicKeyboardWindow::CreateGridPatternBrush(ID2D1RenderTarget * renderTarget, ID2D1BitmapBrush ** bitmapBrush)
{
    CComPtr<ID2D1BitmapRenderTarget> brt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(10.0f, 10.0f), &brt);

    if (!SUCCEEDED(hr))
        return hr;

    CComPtr<ID2D1SolidColorBrush> Brush;

    hr = brt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)), &Brush);

    if (!SUCCEEDED(hr))
        return hr;

    brt->BeginDraw();
    brt->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), Brush);
    brt->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), Brush);
    brt->EndDraw();

    // Retrieve the bitmap from the render target.
    CComPtr<ID2D1Bitmap> bm;

    hr = brt->GetBitmap(&bm);

    if (!SUCCEEDED(hr))
        return hr;

    // Choose the tiling mode for the bitmap brush.
    D2D1_BITMAP_BRUSH_PROPERTIES bbp = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

    // Create the bitmap brush.
    hr = _RenderTarget->CreateBitmapBrush(bm, bbp, bitmapBrush);

    return hr;
}

/// <summary>
/// Discards device-specific resources which need to be recreated when a Direct3D device is lost.
/// </summary>
void MusicKeyboardWindow::Dispose()
{
    _Keyboard->Dispose();
}

#pragma endregion

#pragma region play_callback

/// <summary>
/// Playback advanced to new track.
/// </summary>
void MusicKeyboardWindow::on_playback_new_track(metadb_handle_ptr track)
{
    std::queue<MIDIEvent> Empty; std::swap(_Events, Empty);
    _Position = 0;

    _Keyboard->Reset();
    Invalidate();
}

/// <summary>
/// Playback stopped.
/// </summary>
void MusicKeyboardWindow::on_playback_stop(play_control::t_stop_reason)
{
    std::queue<MIDIEvent> Empty; std::swap(_Events, Empty);
    _Position = 0;

    _Keyboard->Reset();
    Invalidate();
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void MusicKeyboardWindow::on_playback_pause(bool)
{
    Invalidate();
}

#pragma endregion

#pragma region IMIDIReceiver

/// <summary>
/// Processes a MIDI message.
/// </summary>
void MusicKeyboardWindow::ProcessMessage(uint32_t message, uint32_t timestamp) noexcept
{
    if (_InterfaceVersion != InterfaceVersion)
        return;

    if (!::TryEnterCriticalSection(&_Lock))
        return;

    _Events.push(MIDIEvent(message, timestamp));

    ::LeaveCriticalSection(&_Lock);
}

#pragma endregion

MusicKeyboardWindow * _This = nullptr;

service_factory_single_t<MusicKeyboardUIElement> _MusicKeyboardUIElement;
