#include "stdafx.h"
#include "resource.h"

#include <pluginterfaces/vst2.x/aeffect.h>
#include <pluginterfaces/vst2.x/aeffectx.h>

#include <dwmapi.h>
#include <shlobj.h>

#include <atomic>
#include <string>

#pragma comment(lib, "dwmapi.lib")

typedef AEffect * (VSTCALLBACK * main_func)(audioMasterCallback audioMaster);

// #define LOG_EXCHANGE

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

enum
{
    BUFFER_SIZE = 4096
};

std::atomic<bool> need_idle{false};
bool idle_started = false;

static char * _DirectoryPath = nullptr;

static HANDLE _hFileNUL = nullptr;
static HANDLE _hPipeIn = nullptr;
static HANDLE _hPipeOut = nullptr;
static HANDLE _hEditorRequest = nullptr;
static HANDLE _hAutoEditorRequest = nullptr;
static HANDLE _hDarkTheme = nullptr;

struct EditorIdentity
{
    const wchar_t * FilePath;
    std::wstring Title;
};

struct EditorDialog
{
    AEffect * Effect = nullptr;
    const EditorIdentity * Identity = nullptr;
    HWND Window = nullptr;
    HWND ContentWindow = nullptr;
    LONG ContentWidth = 0;
    LONG ContentHeight = 0;
    DPI_HOSTING_BEHAVIOR PreviousHosting = DPI_HOSTING_BEHAVIOR_INVALID;
    HICON LargeIcon = nullptr;
    HICON SmallIcon = nullptr;
    UINT IconDpi = 0;
    bool IsModal = false;
    bool DarkRequested = false;
    bool IsDark = false;
    bool UseFallbackIcons = false;
};

static constexpr COLORREF EditorDarkBackground = RGB(32, 32, 32);
static constexpr int LegacyEditorDpi = 96;
static constexpr UINT DpiChangedMessage = 0x02E0; // WM_DPICHANGED

struct EditorDpi
{
    bool CanHostLegacyContent() const
    {
        return SetContext && SetHosting && GetDpi && GetMetric && AdjustRect;
    }

private:
    const HMODULE User32 = ::GetModuleHandleW(L"user32.dll");

    template <typename Function>
    Function Resolve(const char * name) const
    {
        return reinterpret_cast<Function>(::GetProcAddress(User32, name));
    }

public:
    DPI_AWARENESS_CONTEXT (WINAPI * const SetContext)(DPI_AWARENESS_CONTEXT) =
        Resolve<decltype(SetContext)>("SetThreadDpiAwarenessContext");
    DPI_HOSTING_BEHAVIOR (WINAPI * const SetHosting)(DPI_HOSTING_BEHAVIOR) =
        Resolve<decltype(SetHosting)>("SetThreadDpiHostingBehavior");
    UINT (WINAPI * const GetDpi)(HWND) = Resolve<decltype(GetDpi)>("GetDpiForWindow");
    int (WINAPI * const GetMetric)(int, UINT) = Resolve<decltype(GetMetric)>("GetSystemMetricsForDpi");
    BOOL (WINAPI * const AdjustRect)(LPRECT, DWORD, BOOL, DWORD, UINT) =
        Resolve<decltype(AdjustRect)>("AdjustWindowRectExForDpi");
};

static const EditorDpi DpiApi;

class ScopedDpiContext
{
public:
    explicit ScopedDpiContext(DPI_AWARENESS_CONTEXT context)
        : _Previous(context ? DpiApi.SetContext(context) : nullptr)
    {
    }

    ~ScopedDpiContext()
    {
        if (_Previous)
            DpiApi.SetContext(_Previous);
    }

    bool Changed() const
    {
        return _Previous != nullptr;
    }

private:
    const DPI_AWARENESS_CONTEXT _Previous;
};

static void RestoreEditorHosting(EditorDialog & editor)
{
    if (editor.PreviousHosting == DPI_HOSTING_BEHAVIOR_INVALID)
        return;

    DpiApi.SetHosting(editor.PreviousHosting);
    editor.PreviousHosting = DPI_HOSTING_BEHAVIOR_INVALID;
}

static DPI_AWARENESS_CONTEXT PluginContext(const EditorDialog & editor)
{
    return editor.ContentWindow ? DPI_AWARENESS_CONTEXT_UNAWARE : nullptr;
}

static void ResizeEditor(const EditorDialog & editor, UINT dpi, const RECT * position = nullptr)
{
    if (!editor.ContentWidth || !editor.ContentHeight)
        return;

    const HWND Window = editor.Window;
    const DWORD Style = static_cast<DWORD>(::GetWindowLongW(Window, GWL_STYLE));
    const DWORD ExStyle = static_cast<DWORD>(::GetWindowLongW(Window, GWL_EXSTYLE));
    RECT Bounds = { 0, 0, editor.ContentWidth, editor.ContentHeight };

    if (editor.ContentWindow)
    {
        Bounds.right = ::MulDiv(Bounds.right, dpi, LegacyEditorDpi);
        Bounds.bottom = ::MulDiv(Bounds.bottom, dpi, LegacyEditorDpi);
        ::SetWindowPos(editor.ContentWindow, nullptr, 0, 0, Bounds.right, Bounds.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
        DpiApi.AdjustRect(&Bounds, Style, FALSE, ExStyle, dpi);
    }
    else
    {
        ::AdjustWindowRectEx(&Bounds, Style, FALSE, ExStyle);
    }

    const LONG Width = Bounds.right - Bounds.left;
    const LONG Height = Bounds.bottom - Bounds.top;
    LONG X = 0;
    LONG Y = 0;
    UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;

    if (position)
    {
        X = position->left;
        Y = position->top;
    }
    else
    {
        MONITORINFO Monitor = { sizeof(Monitor) };
        if (::GetMonitorInfoW(::MonitorFromWindow(Window, MONITOR_DEFAULTTONEAREST), &Monitor))
        {
            // DS_CENTER positions the empty dialog before we know its final size.
            const RECT & Work = Monitor.rcWork;
            X = Work.left + max(0, (Work.right - Work.left - Width) / 2);
            Y = Work.top + max(0, (Work.bottom - Work.top - Height) / 2);
        }
        else
        {
            Flags |= SWP_NOMOVE;
        }
    }

    ::SetWindowPos(Window, nullptr, X, Y, Width, Height, Flags);
}

static void SetEditorIcons(HWND window, EditorDialog & editor, HICON largeIcon, HICON smallIcon)
{
    ::SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon ? largeIcon : smallIcon));
    ::SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon ? smallIcon : largeIcon));

    if (editor.LargeIcon)
        ::DestroyIcon(editor.LargeIcon);
    if (editor.SmallIcon)
        ::DestroyIcon(editor.SmallIcon);

    editor.LargeIcon = largeIcon;
    editor.SmallIcon = smallIcon;
}

static void UpdateEditorIcons(HWND window, EditorDialog & editor, bool themeChanged = false)
{
    const UINT Dpi = DpiApi.GetDpi ? DpiApi.GetDpi(window) : 0;
    const bool KeepIcons = Dpi == editor.IconDpi && (editor.LargeIcon || editor.SmallIcon);
    if (KeepIcons && (!themeChanged || !editor.UseFallbackIcons))
        return;

    const auto Metric = [Dpi](int metric)
    {
        return Dpi && DpiApi.GetMetric ? DpiApi.GetMetric(metric, Dpi) : ::GetSystemMetrics(metric);
    };
    const int LargeWidth = Metric(SM_CXICON);
    const int SmallWidth = Metric(SM_CXSMICON);
    HICON LargeIcon = nullptr;
    HICON SmallIcon = nullptr;

    if (!editor.UseFallbackIcons)
    {
        ::SHDefExtractIconW(editor.Identity->FilePath, 0, 0, &LargeIcon, &SmallIcon,
            MAKELONG(LargeWidth, SmallWidth));
        editor.UseFallbackIcons = !LargeIcon && !SmallIcon;
    }

    if (editor.UseFallbackIcons)
    {
        const auto Resource = MAKEINTRESOURCEW(editor.IsDark ? IDI_VSTI_DARK : IDI_VSTI_NORMAL);
        const HINSTANCE Instance = ::GetModuleHandleW(nullptr);
        LargeIcon = static_cast<HICON>(::LoadImageW(Instance, Resource, IMAGE_ICON,
            LargeWidth, Metric(SM_CYICON), LR_DEFAULTCOLOR));
        SmallIcon = static_cast<HICON>(::LoadImageW(Instance, Resource, IMAGE_ICON,
            SmallWidth, Metric(SM_CYSMICON), LR_DEFAULTCOLOR));
    }

    if (LargeIcon || SmallIcon)
    {
        editor.IconDpi = Dpi;
        SetEditorIcons(window, editor, LargeIcon, SmallIcon);
    }
}

static void UpdateEditorTheme(HWND window, EditorDialog & editor, bool systemChanged = false)
{
    const bool Requested = _hDarkTheme && ::WaitForSingleObject(_hDarkTheme, 0) == WAIT_OBJECT_0;
    if (!systemChanged && Requested == editor.DarkRequested)
        return;

    editor.DarkRequested = Requested;

    HIGHCONTRASTW Contrast = { sizeof(Contrast) };
    const bool HighContrast = Requested && ::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(Contrast), &Contrast, 0) &&
                              (Contrast.dwFlags & HCF_HIGHCONTRASTON);
    const BOOL Dark = Requested && !HighContrast;
    if (editor.IsDark == (Dark != FALSE))
        return;

    editor.IsDark = Dark != FALSE;
    if (FAILED(::DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &Dark, sizeof(Dark))))
    {
        // Windows 10 builds before 20H1 used attribute 19.
        ::DwmSetWindowAttribute(window, 19, &Dark, sizeof(Dark));
    }

    // foobar2000 can choose dark mode independently of the system theme.
    const COLORREF Caption = Dark ? EditorDarkBackground : DWMWA_COLOR_DEFAULT;
    const COLORREF Text = Dark ? RGB(243, 243, 243) : DWMWA_COLOR_DEFAULT;
    ::DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &Caption, sizeof(Caption));
    ::DwmSetWindowAttribute(window, DWMWA_TEXT_COLOR, &Text, sizeof(Text));
    if (editor.UseFallbackIcons)
        UpdateEditorIcons(window, editor, true);
    ::RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
}

static std::wstring DecodePluginText(const char * text)
{
    UINT CodePage = CP_UTF8;
    DWORD Flags = MB_ERR_INVALID_CHARS;
    int Length = ::MultiByteToWideChar(CodePage, Flags, text, -1, nullptr, 0);
    if (Length == 0)
    {
        CodePage = CP_ACP;
        Flags = 0;
        Length = ::MultiByteToWideChar(CodePage, Flags, text, -1, nullptr, 0);
    }

    if (Length <= 1)
        return {};

    std::wstring Result(static_cast<size_t>(Length), L'\0');
    ::MultiByteToWideChar(CodePage, Flags, text, -1, &Result[0], Length);
    Result.pop_back();
    return Result;
}

static std::wstring GetEditorTitle(const char * name, const char * vendor, const char * product, const wchar_t * filePath)
{
    auto Title = DecodePluginText(*product ? product : name);
    if (!Title.empty())
    {
        const auto Vendor = DecodePluginText(vendor);
        if (!Vendor.empty() && ::_wcsnicmp(Title.c_str(), Vendor.c_str(), Vendor.size()) != 0)
            Title = Vendor + L' ' + Title;

        return Title;
    }

    const std::wstring Path(filePath);
    const auto Separator = Path.find_last_of(L"\\/");
    Title = Separator == std::wstring::npos ? Path : Path.substr(Separator + 1);
    const auto Extension = Title.find_last_of(L'.');
    if (Extension != std::wstring::npos && Extension != 0)
        Title.resize(Extension);

    return Title.empty() ? L"VST Editor" : Title;
}

#pragma pack(push, 8)
#pragma warning(disable: 4820) // x bytes padding added after data member

struct vst_event_t
{
    struct vst_event_t * next;

    unsigned port;

    union
    {
        VstMidiEvent midiEvent;
        VstMidiSysexEvent sysexEvent;
    } ev;
}* _EventsHead = nullptr, * _EventsTail = nullptr;

#pragma warning(default: 4820) // x bytes padding added after data member
#pragma pack(pop)

void FreeEvents()
{
    vst_event_t * Event = _EventsHead;

    while (Event)
    {
        vst_event_t * Next = Event->next;

        if (Event->port && Event->ev.sysexEvent.type == kVstSysExType)
            ::free(Event->ev.sysexEvent.sysexDump);

        ::free(Event);

        Event = Next;
    }

    _EventsHead = nullptr;
    _EventsTail = nullptr;
}

void WriteBytes(const void * data, uint32_t size)
{
    DWORD BytesWritten;

    ::WriteFile(_hPipeOut, data, size, &BytesWritten, NULL);

#ifdef LOG_EXCHANGE
    TCHAR logfile[MAX_PATH];
    _stprintf_s(logfile, _T("C:\\temp\\log\\bytes_%08u.out"), exchange_count++);
    FILE * f = _tfopen(logfile, _T("wb"));
    fwrite(out, 1, size, f);
    fclose(f);
#endif
}

void WriteCode(uint32_t code)
{
    WriteBytes(&code, sizeof(code));
}

void ReadBytes(void * in, uint32_t size)
{
    DWORD BytesRead;

    if (!::ReadFile(_hPipeIn, in, size, &BytesRead, NULL) || (BytesRead < size))
    {
        memset(in, 0, size);

    #ifdef LOG_EXCHANGE
        TCHAR logfile[MAX_PATH];
        _stprintf_s(logfile, _T("C:\\temp\\log\\bytes_%08u.err"), exchange_count++);
        FILE * f = _tfopen(logfile, _T("wb"));
        _ftprintf(f, _T("Wanted %u bytes, got %u"), size, BytesRead);
        fclose(f);
    #endif
    }
    else
    {
    #ifdef LOG_EXCHANGE
        TCHAR logfile[MAX_PATH];
        _stprintf_s(logfile, _T("C:\\temp\\log\\bytes_%08u.in"), exchange_count++);
        FILE * f = _tfopen(logfile, _T("wb"));
        fwrite(in, 1, size, f);
        fclose(f);
    #endif
    }
}

uint32_t ReadCode()
{
    uint32_t code;

    ReadBytes(&code, sizeof(code));

    return code;
}

void ReadChunk(AEffect * effect, std::vector<uint8_t> & out)
{
    out.resize(0);

    uint32_t unique_id = (uint32_t) effect->uniqueID;

    append_be(out, unique_id);

    bool type_chunked = !!(effect->flags & effFlagsProgramChunks);

    append_be(out, type_chunked);

    if (!type_chunked)
    {
        uint32_t num_params = (uint32_t) effect->numParams;

        append_be(out, num_params);

        for (uint32_t i = 0; i < num_params; ++i)
        {
            float parameter = effect->getParameter(effect, (VstInt32) i);

            append_be(out, parameter);
        }
    }
    else
    {
        void * chunk;

        uint32_t size = (uint32_t) effect->dispatcher(effect, effGetChunk, 0, 0, &chunk, 0);

        append_be(out, size);

        size_t chunk_size = out.size();

        out.resize(chunk_size + size);

        memcpy(&out[chunk_size], chunk, size);
    }
}

void setChunk(AEffect * pEffect, std::vector<uint8_t> const & in)
{
    uint32_t size = (uint32_t) in.size();

    if (pEffect == nullptr || size == 0)
        return;

    const uint8_t * inc = in.data();

    uint32_t effect_id;

    retrieve_be(effect_id, inc, size);

    if (effect_id != (uint32_t) pEffect->uniqueID)
        return;

    bool type_chunked;

    retrieve_be(type_chunked, inc, size);

    if (type_chunked != !!(pEffect->flags & effFlagsProgramChunks))
        return;

    if (!type_chunked)
    {
        uint32_t num_params;

        retrieve_be(num_params, inc, size);

        if (num_params != (uint32_t) pEffect->numParams)
            return;

        for (uint32_t i = 0; i < num_params; ++i)
        {
            float parameter;

            retrieve_be(parameter, inc, size);

            pEffect->setParameter(pEffect, (VstInt32) i, parameter);
        }
    }
    else
    {
        uint32_t chunk_size;

        retrieve_be(chunk_size, inc, size);

        if (chunk_size > size)
            return;

        pEffect->dispatcher(pEffect, effSetChunk, 0, (VstIntPtr) chunk_size, (void *) inc, 0);
    }
}

struct MyDLGTEMPLATE : DLGTEMPLATE
{
    WORD ext[3];

    MyDLGTEMPLATE()
    {
        memset(this, 0, sizeof(*this));
    };
};

static LRESULT CALLBACK EditorContentProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
    UINT_PTR subclass, DWORD_PTR) noexcept
{
    if (message == WM_CLOSE)
    {
        // Legacy editors can request closure through the parent HWND passed to effEditOpen.
        ::PostMessageW(::GetParent(window), WM_CLOSE, 0, 0);
        return 0;
    }

    if (message == WM_NCDESTROY)
        ::RemoveWindowSubclass(window, EditorContentProc, subclass);

    return ::DefSubclassProc(window, message, wParam, lParam);
}

INT_PTR CALLBACK EditorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    auto Editor = reinterpret_cast<EditorDialog *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, lParam);

            Editor = reinterpret_cast<EditorDialog *>(lParam);
            Editor->Window = hwnd;
            Editor->DarkRequested = false;
            Editor->IsDark = false;
            if (Editor->PreviousHosting != DPI_HOSTING_BEHAVIOR_INVALID)
            {
                RestoreEditorHosting(*Editor);

                // Give the frame native DPI while Windows scales the legacy VST editor.
                const ScopedDpiContext Context(DPI_AWARENESS_CONTEXT_UNAWARE);
                if (Context.Changed())
                {
                    Editor->ContentWindow = ::CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", nullptr,
                        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0, hwnd,
                        nullptr, ::GetModuleHandleW(nullptr), nullptr);
                    if (Editor->ContentWindow)
                        ::SetWindowSubclass(Editor->ContentWindow, EditorContentProc, 0, 0);
                }
            }

            UpdateEditorTheme(hwnd, *Editor);
            if (!Editor->IsModal)
                ::SetPropW(hwnd, L"foo_midi.playback_editor", reinterpret_cast<HANDLE>(1));

            ::SetTimer(hwnd, 1, 20, 0);

            if (auto effect = Editor->Effect)
            {
                {
                    const ScopedDpiContext Context(PluginContext(*Editor));
                    HWND Parent = Editor->ContentWindow ? Editor->ContentWindow : hwnd;
                    effect->dispatcher(effect, effEditOpen, 0, 0, Parent, 0);

                    ERect * eRect = nullptr;
                    effect->dispatcher(effect, effEditGetRect, 0, 0, &eRect, 0);

                    if (eRect)
                    {
                        Editor->ContentWidth = max(50, eRect->right - eRect->left);
                        Editor->ContentHeight = max(50, eRect->bottom - eRect->top);
                    }
                    else if (Editor->ContentWindow)
                    {
                        RECT Content = {};
                        ::GetClientRect(Editor->ContentWindow, &Content);
                        Editor->ContentWidth = Content.right;
                        Editor->ContentHeight = Content.bottom;
                    }
                }

                if (Editor->Window == hwnd)
                    ResizeEditor(*Editor, DpiApi.GetDpi ? DpiApi.GetDpi(hwnd) : 0);
            }

            if (Editor->Window != hwnd)
                return FALSE;

            ::SetWindowTextW(hwnd, Editor->Identity->Title.c_str());
            UpdateEditorIcons(hwnd, *Editor);
        } break;

        case WM_TIMER:
            if (Editor)
            {
                // The command loop cannot detect a broken pipe while the modal dialog is open.
                if (Editor->IsModal && !::PeekNamedPipe(_hPipeIn, nullptr, 0, nullptr, nullptr, nullptr))
                {
                    const DWORD Error = ::GetLastError();
                    if (Error == ERROR_BROKEN_PIPE || Error == ERROR_PIPE_NOT_CONNECTED)
                    {
                        ::EndDialog(hwnd, IDCANCEL);
                        return TRUE;
                    }
                }

                UpdateEditorTheme(hwnd, *Editor);
                if (Editor->Effect)
                {
                    const ScopedDpiContext Context(PluginContext(*Editor));
                    Editor->Effect->dispatcher(Editor->Effect, effEditIdle, 0, 0, 0, 0);
                }
            }
            break;

        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
            if (Editor)
                UpdateEditorTheme(hwnd, *Editor, true);
            break;

        case DpiChangedMessage:
            if (Editor)
            {
                UpdateEditorIcons(hwnd, *Editor);
                if (Editor->ContentWindow && Editor->ContentWidth && Editor->ContentHeight)
                {
                    ResizeEditor(*Editor, HIWORD(wParam), reinterpret_cast<const RECT *>(lParam));
                    return TRUE;
                }
            }
            break;

        case WM_CTLCOLORDLG:
            if (Editor && Editor->IsDark)
            {
                ::SetDCBrushColor(reinterpret_cast<HDC>(wParam), EditorDarkBackground);
                return reinterpret_cast<INT_PTR>(::GetStockObject(DC_BRUSH));
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL)
                break;
            // Fall through to close the dialog.
        case WM_CLOSE:
            if (Editor && Editor->IsModal)
                ::EndDialog(hwnd, IDOK);
            else
                ::DestroyWindow(hwnd);
            return TRUE;

        case WM_DESTROY:
        {
            ::KillTimer(hwnd, 1);
            if (Editor)
            {
                if (Editor->Effect)
                {
                    const ScopedDpiContext Context(PluginContext(*Editor));
                    Editor->Effect->dispatcher(Editor->Effect, effEditClose, 0, 0, 0, 0);
                }
                Editor->ContentWindow = nullptr;
                Editor->ContentWidth = 0;
                Editor->ContentHeight = 0;
                if (!Editor->IsModal)
                    ::RemovePropW(hwnd, L"foo_midi.playback_editor");
                Editor->Window = nullptr;
            }
            break;
        }

        case WM_NCDESTROY:
        {
            if (Editor)
            {
                Editor->UseFallbackIcons = false;
                SetEditorIcons(hwnd, *Editor, nullptr, nullptr);
            }
            break;
        }
    }

    return 0;
}

static void ShowEditor(EditorDialog & editor, bool activate = true)
{
    if (!editor.Window)
    {
        const ScopedDpiContext Context(DpiApi.CanHostLegacyContent() ? DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 : nullptr);
        if (Context.Changed())
            editor.PreviousHosting = DpiApi.SetHosting(DPI_HOSTING_BEHAVIOR_MIXED);

        MyDLGTEMPLATE Template;
        Template.style = WS_POPUPWINDOW | WS_DLGFRAME | DS_CENTER | WS_CLIPCHILDREN;

        if (editor.IsModal)
            ::DialogBoxIndirectParamW(nullptr, &Template, ::GetDesktopWindow(), EditorProc, reinterpret_cast<LPARAM>(&editor));
        else
            ::CreateDialogIndirectParamW(::GetModuleHandleW(nullptr), &Template, nullptr, EditorProc, reinterpret_cast<LPARAM>(&editor));

        RestoreEditorHosting(editor);
    }

    if (editor.IsModal)
        return;

    if (editor.Window)
    {
        ::ShowWindow(editor.Window, activate ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
        if (activate)
            ::SetForegroundWindow(editor.Window);
    }
}

static void DispatchEditorMessages(const EditorDialog & editor)
{
    MSG Message;
    while (::PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE))
    {
        if (!editor.Window || !::IsDialogMessageW(editor.Window, &Message))
        {
            ::TranslateMessage(&Message);
            ::DispatchMessageW(&Message);
        }
    }
}

class PlaybackEditor
{
public:
    bool Start(AEffect * effect, const EditorIdentity & identity)
    {
        if (!_hEditorRequest || !(effect->flags & effFlagsHasEditor))
            return true;

        _Editor.Effect = effect;
        _Editor.Identity = &identity;
        _Stop = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!_Stop)
            return false;

        _Thread = ::CreateThread(nullptr, 0, Run, this, 0, &_ThreadId);
        return _Thread != nullptr;
    }

    void Stop()
    {
        if (_Thread)
        {
            ::SetEvent(_Stop);

            // A plug-in can start a modal dialog whose loop cannot see the stop event.
            while (::WaitForSingleObject(_Thread, 50) == WAIT_TIMEOUT)
                ::EnumThreadWindows(_ThreadId, CloseEnabledWindow, 0);

            ::CloseHandle(_Thread);
            _Thread = nullptr;
        }

        if (_Stop)
        {
            ::CloseHandle(_Stop);
            _Stop = nullptr;
        }
    }

private:
    static BOOL CALLBACK CloseEnabledWindow(HWND window, LPARAM)
    {
        if (::IsWindowVisible(window) && ::IsWindowEnabled(window))
            ::PostMessageW(window, WM_CLOSE, 0, 0);

        return TRUE;
    }

    static DWORD WINAPI Run(void * context)
    {
        auto & Self = *static_cast<PlaybackEditor *>(context);
        const HRESULT COM = ::CoInitialize(nullptr);
        const HANDLE Handles[] = { Self._Stop, _hEditorRequest, _hAutoEditorRequest };
        const DWORD Count = _hAutoEditorRequest ? _countof(Handles) : _countof(Handles) - 1;
        for (;;)
        {
            const DWORD Ready = ::MsgWaitForMultipleObjectsEx(Count, Handles, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            if (Ready == WAIT_OBJECT_0 + 1)
                ShowEditor(Self._Editor);
            else if (Ready == WAIT_OBJECT_0 + Count)
                DispatchEditorMessages(Self._Editor);
            else if (Ready == WAIT_OBJECT_0 + 2)
                ShowEditor(Self._Editor, false);
            else
                break;
        }

        if (Self._Editor.Window)
            ::DestroyWindow(Self._Editor.Window);

        if (SUCCEEDED(COM))
            ::CoUninitialize();
        return 0;
    }

    EditorDialog _Editor;
    HANDLE _Stop = nullptr;
    HANDLE _Thread = nullptr;
    DWORD _ThreadId = 0;
};

struct audioMasterData
{
    VstIntPtr effect_number;
};

static VstIntPtr VSTCALLBACK audioMaster(AEffect * effect, VstInt32 opcode, VstInt32, VstIntPtr, void * ptr, float)
{
    audioMasterData * data = nullptr;

    if (effect)
        data = (audioMasterData *) effect->user;

    switch (opcode)
    {
        case audioMasterVersion:
            return kVstVersion;

        case audioMasterCurrentId:
            if (data)
                return data->effect_number;
            break;

        case audioMasterGetVendorString:
            strncpy((char *) ptr, "NoWork, Inc.", 64);
            break;

        case audioMasterGetProductString:
            strncpy((char *) ptr, "VSTi Host Bridge", 64);
            break;

        case audioMasterGetVendorVersion:
            return 1000;

        case audioMasterGetLanguage:
            return kVstLangEnglish;

        case audioMasterVendorSpecific: // Steinberg HACK
            if (ptr)
            {
                uint32_t * blah = (uint32_t *) (((char *) ptr) - 4);
                if (*blah == 0x0737bb68)
                {
                    *blah ^= 0x5CC8F349;
                    blah[2] = 0x19E;
                    return 0x1E7;
                }
            }
            break;

        case audioMasterGetDirectory:
            return (VstIntPtr) _DirectoryPath;

            /* More crap */
        case DECLARE_VST_DEPRECATED(audioMasterNeedIdle):
            need_idle = true;
            return 0;
    }

    return 0;
}

LONG __stdcall FilterUnhandledException(LPEXCEPTION_POINTERS param)
{
    if (::IsDebuggerPresent())
        return ::UnhandledExceptionFilter(param);

    // DumpCrashInfo( param );
    ::TerminateProcess(::GetCurrentProcess(), 0);

    return 0; // never reached
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    int argc = 0;

    LPWSTR * argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    if (argv == nullptr || argc < 3 || argc > 6)
        return 1;

    wchar_t * end_char = nullptr;

#ifdef NDEBUG
    unsigned Cookie = ::wcstoul(argv[2], &end_char, 16);

    if (end_char == argv[2] || *end_char)
        return 2;
#endif

    uint32_t Sum = 0;

    end_char = argv[1];

    while (*end_char)
        Sum += (WCHAR) (*end_char++ * 820109);

#ifdef NDEBUG
    if (Sum != Cookie)
        return 3;
#endif

    int FirstEditorRequest = 3;
    constexpr wchar_t ThemeOption[] = L"--theme=";
    constexpr size_t ThemeOptionLength = _countof(ThemeOption) - 1;
    if (argc > FirstEditorRequest && ::wcsncmp(argv[FirstEditorRequest], ThemeOption, ThemeOptionLength) == 0)
    {
        const auto Value = argv[FirstEditorRequest++] + ThemeOptionLength;
        const auto Handle = ::wcstoull(Value, &end_char, 16);
        if (end_char == Value || *end_char || Handle == 0)
            return 2;

        _hDarkTheme = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(Handle));
    }

    HANDLE * const EditorRequests[] = { &_hEditorRequest, &_hAutoEditorRequest };
    if (argc - FirstEditorRequest > static_cast<int>(_countof(EditorRequests)))
        return 1;

    for (int i = FirstEditorRequest; i < argc; ++i)
    {
        const auto Handle = ::wcstoull(argv[i], &end_char, 16);
        if (end_char == argv[i] || *end_char || Handle == 0)
            return 2;

        *EditorRequests[i - FirstEditorRequest] = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(Handle));
    }

    uint32_t Code = 0;

    AEffect * Effect[3] = { 0, 0, 0 };
    EditorIdentity Identity{ argv[1], {} };
    PlaybackEditor LiveEditor;

    audioMasterData effectData[3] = { { 0 }, { 1 }, { 2 } };

    std::vector<uint8_t> State;

    uint32_t SampleRate = 44100;
    uint32_t EditorPort = 0;

    std::vector<uint8_t> chunk;
    std::vector<float> sample_buffer;

    _hFileNUL = ::CreateFileW(_T("NUL"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    _hPipeIn  = ::GetStdHandle(STD_INPUT_HANDLE);
    _hPipeOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    ::SetStdHandle(STD_INPUT_HANDLE, _hFileNUL);
    ::SetStdHandle(STD_OUTPUT_HANDLE, _hFileNUL);

    {
        INITCOMMONCONTROLSEX icc =
        {
            sizeof(icc),
            ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_STANDARD_CLASSES
        };

        if (!::InitCommonControlsEx(&icc))
            return 4;
    }

    if (FAILED(::CoInitialize(NULL)))
        return 5;

#ifndef _DEBUG
    ::SetUnhandledExceptionFilter(FilterUnhandledException);
#endif

    {
        size_t l = ::wcslen(argv[1]);

        _DirectoryPath = (char *) ::malloc(l + 1);

        ::wcstombs(_DirectoryPath, argv[1], l);

        _DirectoryPath[l] = '\0';

        char * Slash = ::strrchr(_DirectoryPath, '\\');

        *Slash = '\0';
    }

    HMODULE hModule = ::LoadLibraryW(argv[1]);

    if (hModule == 0)
    {
        Code = 6;
        goto exit;
    }

    #pragma warning(disable: 4191) //unsafe conversion from 'FARPROC' to 'main_func'

    // Find the DLL entry point.
    auto Main = (main_func) ::GetProcAddress(hModule, "VSTPluginMain");

    if (Main == nullptr)
    {
        Main = (main_func) ::GetProcAddress(hModule, "main");

        if (Main == nullptr)
        {
            Main = (main_func) ::GetProcAddress(hModule, "MAIN");

            if (Main == nullptr)
            {
                Code = 7;
                goto exit;
            }
        }
    }

    {
        Effect[0] = Main(&audioMaster);

        if ((Effect[0] == nullptr) || (Effect[0]->magic != kEffectMagic))
        {
            Code = 8;
            goto exit;
        }

        Effect[0]->user = &effectData[0];
        Effect[0]->dispatcher(Effect[0], effOpen, 0, 0, 0, 0);

        if ((Effect[0]->dispatcher(Effect[0], effGetPlugCategory, 0, 0, 0, 0) != kPlugCategSynth) || (Effect[0]->dispatcher(Effect[0], effCanDo, 0, 0, (void *) "receiveVstMidiEvent", 0) < 1))
        {
            Code = 9;
            goto exit;
        }
    }

    uint32_t MaxOutputs = (uint32_t) min(Effect[0]->numOutputs, 2);

    {
        char name_string[256] = { 0 };
        char vendor_string[256] = { 0 };
        char product_string[256] = { 0 };

        uint32_t name_string_length;
        uint32_t vendor_string_length;
        uint32_t product_string_length;
        uint32_t vendor_version;
        uint32_t unique_id;

        Effect[0]->dispatcher(Effect[0], effGetEffectName, 0, 0, &name_string, 0);
        Effect[0]->dispatcher(Effect[0], effGetVendorString, 0, 0, &vendor_string, 0);
        Effect[0]->dispatcher(Effect[0], effGetProductString, 0, 0, &product_string, 0);

        name_string_length = (uint32_t) ::strlen(name_string);
        vendor_string_length = (uint32_t) ::strlen(vendor_string);
        product_string_length = (uint32_t) ::strlen(product_string);
        vendor_version = (uint32_t) Effect[0]->dispatcher(Effect[0], effGetVendorVersion, 0, 0, 0, 0);
        unique_id = (uint32_t) Effect[0]->uniqueID;

        if (Effect[0]->flags & effFlagsHasEditor)
            Identity.Title = GetEditorTitle(name_string, vendor_string, product_string, Identity.FilePath);

        WriteCode(0);
        WriteCode(name_string_length);
        WriteCode(vendor_string_length);
        WriteCode(product_string_length);
        WriteCode(vendor_version);
        WriteCode(unique_id);
        WriteCode(MaxOutputs);

        if (name_string_length)
            WriteBytes(name_string, name_string_length);

        if (vendor_string_length)
            WriteBytes(vendor_string, vendor_string_length);

        if (product_string_length)
            WriteBytes(product_string, product_string_length);
    }

    float ** float_list_in = nullptr;
    float ** float_list_out = nullptr;
    float * float_null = nullptr;
    float * float_out = nullptr;

    for (;;)
    {
        Code = ReadCode();

        if (Code == 0)
            break;

        switch (Code)
        {
            case 1: // Get Chunk
            {
                ReadChunk(Effect[0], chunk);

                WriteCode(0);
                WriteCode((uint32_t) chunk.size());
                WriteBytes(chunk.data(), (uint32_t) chunk.size());
                break;
            }

            case 2: // Set Chunk
            {
                uint32_t size = ReadCode();
                chunk.resize(size);
                if (size) ReadBytes(chunk.data(), size);

                setChunk(Effect[0], chunk);
                setChunk(Effect[1], chunk);
                setChunk(Effect[2], chunk);

                WriteCode(0);
                break;
            }

            case 3: // Has Editor
            {
                uint32_t has_editor = (Effect[0]->flags & effFlagsHasEditor) ? 1u : 0u;

                WriteCode(0);
                WriteCode(has_editor);
                break;
            }

            case 4: // Display Editor Modal
            {
                if (Effect[0]->flags & effFlagsHasEditor)
                {
                    EditorDialog ModalEditor{ Effect[0], &Identity };
                    ModalEditor.IsModal = true;
                    ShowEditor(ModalEditor);

                    ReadChunk(Effect[0], chunk);
                    setChunk(Effect[1], chunk);
                    setChunk(Effect[2], chunk);
                }

                WriteCode(0);
                break;
            }

            case 5: // Set Sample Rate and optional playback editor port
            {
                const uint32_t Size = ReadCode();
                const bool HasEditorPort = Size == sizeof(SampleRate) + sizeof(EditorPort);
                if (Size != sizeof(SampleRate) && !HasEditorPort)
                {
                    Code = 10;
                    goto exit;
                }

                SampleRate = ReadCode();
                EditorPort = HasEditorPort ? ReadCode() : 0;
                if (EditorPort >= _countof(Effect))
                {
                    Code = 10;
                    goto exit;
                }

                WriteCode(0);
                break;
            }

            case 6: // Reset
            {
                LiveEditor.Stop();

                if (Effect[2])
                {
                    if (State.size())
                        Effect[2]->dispatcher(Effect[2], effStopProcess, 0, 0, 0, 0);

                    Effect[2]->dispatcher(Effect[2], effClose, 0, 0, 0, 0);
                    Effect[2] = nullptr;
                }

                if (Effect[1])
                {
                    if (State.size())
                        Effect[1]->dispatcher(Effect[1], effStopProcess, 0, 0, 0, 0);

                    Effect[1]->dispatcher(Effect[1], effClose, 0, 0, 0, 0);
                    Effect[1] = nullptr;
                }

                if (State.size())
                    Effect[0]->dispatcher(Effect[0], effStopProcess, 0, 0, 0, 0);

                Effect[0]->dispatcher(Effect[0], effClose, 0, 0, 0, 0);

                State.resize(0);

                FreeEvents();

                Effect[0] = Main(&audioMaster);

                if (!Effect[0])
                {
                    Code = 8;
                    goto exit;
                }

                Effect[0]->user = &effectData[0];
                Effect[0]->dispatcher(Effect[0], effOpen, 0, 0, 0, 0);
                setChunk(Effect[0], chunk);

                WriteCode(0);
                break;
            }

            case 7: // Send MIDI Event
            {
                vst_event_t * ev = (vst_event_t *) calloc(sizeof(vst_event_t), 1);

                if (ev != nullptr)
                {
                    if (_EventsTail)
                        _EventsTail->next = ev;

                    _EventsTail = ev;

                    if (!_EventsHead)
                        _EventsHead = ev;

                    uint32_t b = ReadCode();

                    ev->port = (b & 0x7F000000) >> 24;

                    if (ev->port > 2)
                        ev->port = 2;

                    ev->ev.midiEvent.type = kVstMidiType;
                    ev->ev.midiEvent.byteSize = sizeof(ev->ev.midiEvent);

                    memcpy(&ev->ev.midiEvent.midiData, &b, 3);

                    WriteCode(0);
                }
                break;
            }

            case 8: // Send System Exclusive Event
            {
                vst_event_t * ev = (vst_event_t *) calloc(sizeof(vst_event_t), 1);

                if (ev != nullptr)
                {
                    if (_EventsTail)
                        _EventsTail->next = ev;

                    _EventsTail = ev;

                    if (!_EventsHead)
                        _EventsHead = ev;

                    uint32_t size = ReadCode();
                    uint32_t port = size >> 24; size &= 0xFFFFFF;

                    ev->port = port;

                    if (ev->port > 2)
                        ev->port = 2;

                    ev->ev.sysexEvent.type = kVstSysExType;
                    ev->ev.sysexEvent.byteSize = sizeof(ev->ev.sysexEvent);
                    ev->ev.sysexEvent.dumpBytes = (VstInt32) size;
                    ev->ev.sysexEvent.sysexDump = (char *) ::malloc(size);

                    ReadBytes(ev->ev.sysexEvent.sysexDump, size);

                    WriteCode(0);
                }
                break;
            }

            case 9: // Render Samples
            {
                if (Effect[1] == nullptr)
                {
                    Effect[1] = Main(&audioMaster);

                    if (Effect[1] == nullptr)
                    {
                        Code = 11;
                        goto exit;
                    }

                    Effect[1]->user = &effectData[1];
                    Effect[1]->dispatcher(Effect[1], effOpen, 0, 0, 0, 0);

                    setChunk(Effect[1], chunk);
                }

                if (Effect[2] == nullptr)
                {
                    Effect[2] = Main(&audioMaster);

                    if (Effect[2] == nullptr)
                    {
                        Code = 11;
                        goto exit;
                    }

                    Effect[2]->user = &effectData[2];
                    Effect[2]->dispatcher(Effect[2], effOpen, 0, 0, 0, 0);

                    setChunk(Effect[2], chunk);
                }

                // Initialize the lists and the sample buffer.
                if (State.size() == 0)
                {
                    Effect[0]->dispatcher(Effect[0], effSetSampleRate, 0, 0, 0, float(SampleRate));
                    Effect[0]->dispatcher(Effect[0], effSetBlockSize, 0, BUFFER_SIZE, 0, 0);
                    Effect[0]->dispatcher(Effect[0], effMainsChanged, 0, 1, 0, 0);
                    Effect[0]->dispatcher(Effect[0], effStartProcess, 0, 0, 0, 0);

                    Effect[1]->dispatcher(Effect[1], effSetSampleRate, 0, 0, 0, float(SampleRate));
                    Effect[1]->dispatcher(Effect[1], effSetBlockSize, 0, BUFFER_SIZE, 0, 0);
                    Effect[1]->dispatcher(Effect[1], effMainsChanged, 0, 1, 0, 0);
                    Effect[1]->dispatcher(Effect[1], effStartProcess, 0, 0, 0, 0);

                    Effect[2]->dispatcher(Effect[2], effSetSampleRate, 0, 0, 0, float(SampleRate));
                    Effect[2]->dispatcher(Effect[2], effSetBlockSize, 0, BUFFER_SIZE, 0, 0);
                    Effect[2]->dispatcher(Effect[2], effMainsChanged, 0, 1, 0, 0);
                    Effect[2]->dispatcher(Effect[2], effStartProcess, 0, 0, 0, 0);

                    {
                        {
                            size_t buffer_size = sizeof(float *) * (Effect[0]->numInputs + (Effect[0]->numOutputs * 3)); // float lists

                            buffer_size += sizeof(float) * BUFFER_SIZE; // null input
                            buffer_size += sizeof(float) * BUFFER_SIZE * Effect[0]->numOutputs * 3; // outputs

                            State.resize(buffer_size);
                        }

                        float_list_in  = (float **) State.data();
                        float_list_out =            float_list_in + Effect[0]->numInputs;
                        float_null     = (float *) (float_list_out + Effect[0]->numOutputs * 3);
                        float_out      =            float_null + BUFFER_SIZE;

                        for (uint32_t i = 0; i < (uint32_t) Effect[0]->numInputs;      ++i)
                            float_list_in[i] = float_null;

                        for (uint32_t i = 0; i < (uint32_t) Effect[0]->numOutputs * 3; ++i)
                            float_list_out[i] = float_out + (BUFFER_SIZE * i);

                        ::memset(float_null, 0, BUFFER_SIZE * sizeof(float));

                        size_t NewSize = BUFFER_SIZE * MaxOutputs * sizeof(float);

                        sample_buffer.resize(NewSize);
                    }

                    // The nonzero-port instances are created on the first render call.
                    if (!LiveEditor.Start(Effect[EditorPort], Identity))
                    {
                        Code = 13;
                        goto exit;
                    }
                }

                if (need_idle && float_list_in && float_list_out)
                {
                    Effect[0]->dispatcher(Effect[0], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
                    Effect[1]->dispatcher(Effect[1], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
                    Effect[2]->dispatcher(Effect[2], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);

                    if (!idle_started)
                    {
                        unsigned idle_run = BUFFER_SIZE * 200;

                        while (idle_run)
                        {
                            uint32_t count_to_do = min(idle_run, BUFFER_SIZE);
                            uint32_t num_outputs = (uint32_t) Effect[0]->numOutputs;

                            Effect[0]->processReplacing(Effect[0], float_list_in, float_list_out, (VstInt32) count_to_do);
                            Effect[1]->processReplacing(Effect[1], float_list_in, float_list_out + num_outputs, (VstInt32) count_to_do);
                            Effect[2]->processReplacing(Effect[2], float_list_in, float_list_out + num_outputs * 2, (VstInt32) count_to_do);

                            Effect[0]->dispatcher(Effect[0], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
                            Effect[1]->dispatcher(Effect[1], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
                            Effect[2]->dispatcher(Effect[2], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);

                            idle_run -= count_to_do;
                        }
                    }
                }

                VstEvents * events[3] = { 0 };

                if (_EventsHead)
                {
                    unsigned event_count[3] = { 0 };

                    vst_event_t * ev = _EventsHead;

                    while (ev)
                    {
                        event_count[ev->port]++;

                        ev = ev->next;
                    }

                    if (event_count[0] != 0)
                    {
//                      events[0] = (VstEvents *) malloc(sizeof(VstInt32) + sizeof(VstIntPtr) + (sizeof(VstEvent *) * event_count[0]));
                        events[0] = (VstEvents *) malloc(offsetof(struct VstEvents, events) - offsetof(struct VstEvents, numEvents) + (sizeof(VstEvent *) * event_count[0]));

                        events[0]->numEvents = (VstInt32) event_count[0];
                        events[0]->reserved = 0;

                        ev = _EventsHead;

                        for (unsigned i = 0; ev;)
                        {
                            if (!ev->port)
                                events[0]->events[i++] = (VstEvent *) &ev->ev;

                            ev = ev->next;
                        }

                        Effect[0]->dispatcher(Effect[0], effProcessEvents, 0, 0, events[0], 0);
                    }

                    if (event_count[1] != 0)
                    {
//                      events[1] = (VstEvents *) malloc(sizeof(VstInt32) + sizeof(VstIntPtr) + (sizeof(VstEvent *) * event_count[1]));
                        events[1] = (VstEvents *) malloc(offsetof(struct VstEvents, events) - offsetof(struct VstEvents, numEvents) + (sizeof(VstEvent *) * event_count[1]));

                        events[1]->numEvents = (VstInt32) event_count[1];
                        events[1]->reserved = 0;

                        ev = _EventsHead;

                        for (unsigned i = 0; ev;)
                        {
                            if (ev->port == 1)
                                events[1]->events[i++] = (VstEvent *) &ev->ev;

                            ev = ev->next;
                        }

                        Effect[1]->dispatcher(Effect[1], effProcessEvents, 0, 0, events[1], 0);
                    }

                    if (event_count[2] != 0)
                    {
//                      events[2] = (VstEvents *) malloc(sizeof(VstInt32) + sizeof(VstIntPtr) + (sizeof(VstEvent *) * event_count[2]));
                        events[2] = (VstEvents *) malloc(offsetof(struct VstEvents, events) - offsetof(struct VstEvents, numEvents) + (sizeof(VstEvent *) * event_count[2]));

                        events[2]->numEvents = (VstInt32) event_count[2];
                        events[2]->reserved = 0;

                        ev = _EventsHead;

                        for (unsigned i = 0; ev;)
                        {
                            if (ev->port == 2)
                                events[2]->events[i++] = (VstEvent *) &ev->ev;

                            ev = ev->next;
                        }

                        Effect[2]->dispatcher(Effect[2], effProcessEvents, 0, 0, events[2], 0);
                    }
                }

                if (need_idle)
                {
                    Effect[0]->dispatcher(Effect[0], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
                    Effect[1]->dispatcher(Effect[1], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
                    Effect[2]->dispatcher(Effect[2], DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);

                    if (!idle_started)
                    {
                        if (events[0]) Effect[0]->dispatcher(Effect[0], effProcessEvents, 0, 0, events[0], 0);
                        if (events[1]) Effect[1]->dispatcher(Effect[1], effProcessEvents, 0, 0, events[1], 0);
                        if (events[2]) Effect[2]->dispatcher(Effect[2], effProcessEvents, 0, 0, events[2], 0);

                        idle_started = true;
                    }
                }

                uint32_t SampleCount = ReadCode();

                WriteCode(0);

                if (float_list_out)
                {
                    while (SampleCount)
                    {
                        unsigned SamplesToDo = min(SampleCount, BUFFER_SIZE);

                        uint32_t num_outputs = (uint32_t) Effect[0]->numOutputs;
//                      unsigned sample_start = 0;

                        Effect[0]->processReplacing(Effect[0], float_list_in, float_list_out,                   (VstInt32) SamplesToDo);
                        Effect[1]->processReplacing(Effect[1], float_list_in, float_list_out + num_outputs,     (VstInt32) SamplesToDo);
                        Effect[2]->processReplacing(Effect[2], float_list_in, float_list_out + num_outputs * 2, (VstInt32) SamplesToDo);

                        float * out = sample_buffer.data();

                        if (MaxOutputs == 2)
                        {
                            for (unsigned i = 0; i < SamplesToDo; ++i)
                            {
                                float sample = (float_out[i] +
                                                float_out[i + BUFFER_SIZE * num_outputs] +
                                                float_out[i + BUFFER_SIZE * num_outputs * 2]);
                                out[0] = sample;

                                sample = (float_out[i + BUFFER_SIZE] +
                                          float_out[i + BUFFER_SIZE + BUFFER_SIZE * num_outputs] +
                                          float_out[i + BUFFER_SIZE + BUFFER_SIZE * num_outputs * 2]);

                                out[1] = sample;

                                out += 2;
                            }
                        }
                        else
                        {
                            for (unsigned i = 0; i < SamplesToDo; ++i)
                            {
                                float sample = (float_out[i] +
                                                float_out[i + BUFFER_SIZE * num_outputs] +
                                                float_out[i + BUFFER_SIZE * num_outputs * 2]);
                                out[0] = sample;

                                out++;
                            }
                        }

                        WriteBytes(sample_buffer.data(), SamplesToDo * MaxOutputs * sizeof(float));

                        SampleCount -= SamplesToDo;
                    }
                }

                if (events[0])
                    free(events[0]);

                if (events[1])
                    free(events[1]);

                if (events[2])
                    free(events[2]);

                FreeEvents();
                break;
            }

            case 10: // Send MIDI Event, with timestamp
            {
                vst_event_t * ev = (vst_event_t *) calloc(sizeof(vst_event_t), 1);

                if (_EventsTail) _EventsTail->next = ev;

                _EventsTail = ev;

                if (!_EventsHead) _EventsHead = ev;

                uint32_t b = ReadCode();
                uint32_t timestamp = ReadCode();

                ev->port = (b & 0x7F000000) >> 24;

                if (ev->port > 2) ev->port = 2;

                ev->ev.midiEvent.type = kVstMidiType;
                ev->ev.midiEvent.byteSize = sizeof(ev->ev.midiEvent);
                memcpy(&ev->ev.midiEvent.midiData, &b, 3);
                ev->ev.midiEvent.deltaFrames = (VstInt32) timestamp;

                WriteCode(0);
                break;
            }

            case 11: // Send System Exclusive Event, with timestamp
            {
                vst_event_t * ev = (vst_event_t *) calloc(sizeof(vst_event_t), 1);

                if (_EventsTail) _EventsTail->next = ev;

                _EventsTail = ev;

                if (!_EventsHead) _EventsHead = ev;

                uint32_t size = ReadCode();
                uint32_t port = size >> 24;
                size &= 0xFFFFFF;

                uint32_t timestamp = ReadCode();

                ev->port = port;
                if (ev->port > 2) ev->port = 0;
                ev->ev.sysexEvent.type = kVstSysExType;
                ev->ev.sysexEvent.byteSize = sizeof(ev->ev.sysexEvent);
                ev->ev.sysexEvent.dumpBytes = (VstInt32) size;
                ev->ev.sysexEvent.sysexDump = (char *) malloc(size);
                ev->ev.sysexEvent.deltaFrames = (VstInt32) timestamp;

                ReadBytes(ev->ev.sysexEvent.sysexDump, size);

                WriteCode(0);
                break;
            }

            default:
            {
                Code = 12;
                goto exit;
            }
        }
    }

exit:
    LiveEditor.Stop();

    if (Effect[2])
    {
        if (State.size())
            Effect[2]->dispatcher(Effect[2], effStopProcess, 0, 0, 0, 0);

        Effect[2]->dispatcher(Effect[2], effClose, 0, 0, 0, 0);
    }

    if (Effect[1])
    {
        if (State.size())
            Effect[1]->dispatcher(Effect[1], effStopProcess, 0, 0, 0, 0);

        Effect[1]->dispatcher(Effect[1], effClose, 0, 0, 0, 0);
    }

    if (Effect[0])
    {
        if (State.size())
            Effect[0]->dispatcher(Effect[0], effStopProcess, 0, 0, 0, 0);

        Effect[0]->dispatcher(Effect[0], effClose, 0, 0, 0, 0);
    }

    FreeEvents();

    if (hModule)
        ::FreeLibrary(hModule);

    ::CoUninitialize();

    if (_DirectoryPath)
        free(_DirectoryPath);

    if (argv)
        ::LocalFree(argv);

    WriteCode(Code);

    if (_hEditorRequest)
        ::CloseHandle(_hEditorRequest);
    if (_hAutoEditorRequest)
        ::CloseHandle(_hAutoEditorRequest);
    if (_hDarkTheme)
        ::CloseHandle(_hDarkTheme);

    if (_hFileNUL)
    {
        ::CloseHandle(_hFileNUL);

        ::SetStdHandle(STD_INPUT_HANDLE, _hPipeIn);
        ::SetStdHandle(STD_OUTPUT_HANDLE, _hPipeOut);
    }

    return (int) Code;
}
