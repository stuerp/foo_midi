
/** $VER: VSTiPlayer.cpp (2025.08.31) **/

#include "pch.h"

#include "VSTiPlayer.h"
#include "Resource.h"
#include "Log.h"

#include <sdk/dsp_manager.h>
#include <sdk/initquit.h>
#include <sdk/playback_control.h>
#include <sdk/timer.h>
#include <sdk/ui_element.h>
#include <memory>
#include <mutex>

namespace VSTi
{

namespace
{
    struct PlaybackPlayer
    {
        playable_location_impl Location;
        fs::path FilePath;
        HANDLE EditorRequest;
        HANDLE AutoEditorRequest;
        std::shared_ptr<void> EditorClosed;
        DWORD ProcessId = 0;
        bool EditorRequested = false;
    };

    constexpr DWORD HostSetupTimeout = 30'000;

    std::mutex PlaybackPlayersMutex;
    std::vector<PlaybackPlayer> PlaybackPlayers;

    // Main thread only. Keep the close event alive when the track gets a new decoder.
    std::shared_ptr<void> EditorClosedForTrack;

    HANDLE EditorDarkModeEvent()
    {
        // A manual-reset event lets all out-of-process editors read the current theme.
        static const std::unique_ptr<void, decltype(&::CloseHandle)> Event(
            ::CreateEventW(nullptr, TRUE, FALSE, nullptr), ::CloseHandle);
        return Event.get();
    }

    class EditorThemeObserver : public ui_config_callback_impl
    {
    public:
        EditorThemeObserver() { ui_colors_changed(); }

        void ui_colors_changed() override
        {
            if (const HANDLE Event = EditorDarkModeEvent())
            {
                if (ui_config_manager::g_is_dark_mode())
                    ::SetEvent(Event);
                else
                    ::ResetEvent(Event);
            }
        }
    };

    std::unique_ptr<EditorThemeObserver> EditorTheme;
    FB2K_RUN_ON_INIT_QUIT(
        [] { EditorTheme = std::make_unique<EditorThemeObserver>(); },
        [] { EditorTheme.reset(); });

    // Main thread only. The decoder can run ahead of the audible track in large batches.
    struct LiveEditorScope
    {
        service_ptr Scope;
        fb2k::objRef Timer;
        ULONGLONG OpenDeadline = 0;
        DWORD ProcessId = 0;
        HWND Window = nullptr;
    };

    LiveEditorScope EditorScope;

    void ClearEditorScope()
    {
        EditorScope.Timer.release();
        EditorScope.Scope.release();
        EditorScope.ProcessId = 0;
        EditorScope.Window = nullptr;
    }

    FB2K_RUN_ON_QUIT(ClearEditorScope);

    bool IsPlaybackEditorWindow(HWND window)
    {
        DWORD ProcessId = 0;
        ::GetWindowThreadProcessId(window, &ProcessId);
        return ProcessId == EditorScope.ProcessId && ::GetPropW(window, L"foo_midi.playback_editor") != nullptr;
    }

    BOOL CALLBACK FindPlaybackEditorWindow(HWND window, LPARAM)
    {
        if (!IsPlaybackEditorWindow(window))
            return TRUE;

        EditorScope.Window = window;
        return FALSE;
    }

    void UpdateEditorScope()
    {
        if (!EditorScope.Window)
        {
            ::EnumWindows(FindPlaybackEditorWindow, 0);
            if (!EditorScope.Window && ::GetTickCount64() < EditorScope.OpenDeadline)
                return;
        }

        if (!EditorScope.Window || !IsPlaybackEditorWindow(EditorScope.Window))
            ClearEditorScope();
    }

    void RequestEditorScope(DWORD processId)
    {
        if (processId == 0 || (processId == EditorScope.ProcessId && EditorScope.Timer.is_valid()))
            return;

        ClearEditorScope();
        auto Manager = dsp_config_manager_v3::tryGet();
        if (Manager.is_empty())
            return;

        EditorScope.ProcessId = processId;
        EditorScope.OpenDeadline = ::GetTickCount64() + 2000;
        EditorScope.Scope = Manager->acquire_live_edit_scope();
        EditorScope.Timer = fb2k::registerTimer(0.1, UpdateEditorScope);
    }

    void SetPlaybackHostProcess(HANDLE editorRequest, DWORD processId)
    {
        if (!editorRequest)
            return;

        const std::lock_guard Lock(PlaybackPlayersMutex);
        const auto It = std::find_if(PlaybackPlayers.begin(), PlaybackPlayers.end(),
            [editorRequest](const PlaybackPlayer & item) { return item.EditorRequest == editorRequest; });
        if (It != PlaybackPlayers.end() && It->ProcessId != processId)
        {
            It->ProcessId = processId;
            It->EditorRequested = false;
        }
    }
}

#pragma region Public

Player::Player() noexcept : player_t(), VendorVersion(), Id()
{
    _IsCOMInitialized = false;
    _IsTerminating = false;

    _hReadEvent = NULL;
    _hPipeInRead = NULL;
    _hPipeInWrite = NULL;
    _hPipeOutRead = NULL;
    _hPipeOutWrite = NULL;
    _hProcess = NULL;
    _hThread = NULL;

    _ChannelCount = 0;
    _SrcFrames = nullptr;

    _ProcessorArchitecture = 0;

}

Player::~Player()
{
    if (_hEditorRequest)
    {
        const std::lock_guard Lock(PlaybackPlayersMutex);
        std::erase_if(PlaybackPlayers, [this](const PlaybackPlayer & item) { return item.EditorRequest == _hEditorRequest; });
    }

    StopHost();

    if (_hEditorRequest)
        ::CloseHandle(_hEditorRequest);
    if (_hAutoEditorRequest)
        ::CloseHandle(_hAutoEditorRequest);

    SafeDelete(_SrcFrames);
}

bool Player::LoadVST(const fs::path & filePath, const playable_location * playbackLocation)
{
    if (filePath.empty())
        return false;

    _FilePath = filePath;
    _ProcessorArchitecture = GetProcessorArchitecture(_FilePath);

    if (_ProcessorArchitecture == 0)
        return false;

    if (playbackLocation)
        _hEditorRequest = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (_hEditorRequest)
    {
        _hAutoEditorRequest = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (_hAutoEditorRequest)
        {
            if (const HANDLE Closed = ::CreateEventW(nullptr, TRUE, FALSE, nullptr))
                _EditorClosed = std::shared_ptr<void>(Closed, ::CloseHandle);
        }

        const std::lock_guard Lock(PlaybackPlayersMutex);
        PlaybackPlayers.push_back({ *playbackLocation, _FilePath, _hEditorRequest, _hAutoEditorRequest, _EditorClosed });
    }

    return StartHost();
}

void Player::ResetPlaybackEditorDismissal()
{
    EditorClosedForTrack.reset();
}

bool Player::OpenPlaybackEditor(const fs::path & filePath, bool activate)
{
    if (!activate && EditorClosedForTrack && ::WaitForSingleObject(EditorClosedForTrack.get(), 0) == WAIT_OBJECT_0)
        return false;

    metadb_handle_ptr Track;
    if (!playback_control::get()->get_now_playing(Track))
        return false;

    DWORD ProcessId = 0;
    bool Requested = false;
    {
        const std::lock_guard Lock(PlaybackPlayersMutex);
        for (auto It = PlaybackPlayers.rbegin(); It != PlaybackPlayers.rend(); ++It)
        {
            // The decoder for the next track may already have started.
            if (It->Location != Track->get_location())
                continue;

            std::error_code Error;
            if (!filePath.empty() && It->FilePath != filePath && !fs::equivalent(It->FilePath, filePath, Error))
                continue;

            if (!activate && It->EditorRequested)
                break;

            ProcessId = It->ProcessId;
            if (activate && ProcessId != 0)
                ::AllowSetForegroundWindow(ProcessId);

            const HANDLE Request = activate ? It->EditorRequest : It->AutoEditorRequest;
            if (Request && (activate || ProcessId != 0))
            {
                Requested = ::SetEvent(Request) != FALSE;
                if (Requested)
                {
                    It->EditorRequested = true;
                    EditorClosedForTrack = It->EditorClosed;
                }
            }
            break;
        }
    }

    if (Requested)
        RequestEditorScope(ProcessId);

    return Requested;
}

void Player::GetChunk(std::vector<uint8_t> & chunk)
{
    WriteBytes(1);

    const uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();
        return;
    }

    uint32_t Size;
    if (!ReadBytes(&Size, sizeof(Size)))
    {
        StopHost();
        return;
    }

    std::vector<uint8_t> Chunk(Size);
    if (ReadBytes(Chunk.data(), Size))
        chunk.swap(Chunk);
    else
        StopHost();
}

void Player::SetChunk(const void * data, size_t size)
{
    if ((_Chunk.size() == 0) || ((_Chunk.size() == size) && (size != 0) && (data != (const void *) _Chunk.data())))
    {
        _Chunk.resize(size);

        if (size != 0)
            ::memcpy(_Chunk.data(), data, size);
    }

    WriteBytes(2);
    WriteBytes((uint32_t)size);
    WriteBytesOverlapped(data, (uint32_t)size);

    const uint32_t Code = ReadCode(HostSetupTimeout);

    if (Code != 0)
        StopHost();
}

bool Player::HasEditor()
{
    WriteBytes(3);

    uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();

        return false;
    }

    return ReadCode() == 1;
}

void Player::OpenEditor()
{
    if (_hProcess)
        ::AllowSetForegroundWindow(::GetProcessId(_hProcess));

    WriteBytes(4);

    const uint32_t Code = ReadCode(INFINITE);

    if (Code != 0)
        StopHost();
}

#pragma endregion

#pragma region Protected

bool Player::Startup()
{
    if (!_ErrorMessage.empty())
        return false;

    if (_IsStarted)
        return true;

    // A backwards seek can intentionally stop the helper before starting again.
    if (!IsHostRunning() && !StartHost())
    {
        FailHost();
        return false;
    }

    if (!_Chunk.empty())
        SetChunk(_Chunk.data(), _Chunk.size());

    uint32_t EditorPort = 0;

    if (_hEditorRequest)
    {
        // Each bridge instance renders one MIDI port. Open the editor on the first port with a sounding note.
        for (const auto & Message : _Messages)
        {
            if (!Message.IsSysEx() && (Message.Data & 0xF0) == midi::StatusCode::NoteOn &&
                ((Message.Data >> 16) & 0x7F) != 0)
            {
                EditorPort = std::min((Message.Data >> 24) & 0x7FU, 2U);
                break;
            }
        }
    }

    WriteBytes(5);
    WriteBytes(sizeof(_SampleRate) + (EditorPort != 0 ? sizeof(EditorPort) : 0));
    WriteBytes(_SampleRate);
    if (EditorPort != 0)
        WriteBytes(EditorPort);

    if (ReadCode(HostSetupTimeout) != 0)
    {
        FailHost();
        return false;
    }

    _IsStarted = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void Player::Shutdown()
{
    StopHost();
}

void Player::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    WriteBytes(9);
    WriteBytes(dstCount);

    if (ReadCode() != 0 || !_SrcFrames)
    {
        FailHost();
        return;
    }

    while (dstCount != 0)
    {
        const uint32_t ToDo = std::min(dstCount, MaxFrames);

        if (!ReadBytes(_SrcFrames, ToDo * _ChannelCount * sizeof(float)))
        {
            FailHost();
            return;
        }

        // Convert the format of the rendered output.
        for (size_t i = 0; i < ToDo * _ChannelCount; ++i)
            dstFrames[i] = (audio_sample) _SrcFrames[i];

        dstFrames += ToDo * _ChannelCount;
        dstCount -= ToDo;
    }
}

void Player::SendEvent(uint32_t data)
{
    WriteBytes(7);
    WriteBytes(data);

    if (ReadCode() != 0)
        FailHost();
}

void Player::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    const uint32_t SizeAndPort = ((uint32_t) size & 0xFFFFFF) | (portNumber << 24);

    WriteBytes(8);
    WriteBytes(SizeAndPort);
    WriteBytesOverlapped(data, (uint32_t) size);

    if (ReadCode() != 0)
        FailHost();
}

void Player::SendEvent(uint32_t data, uint32_t time)
{
    WriteBytes(10);
    WriteBytes(data);
    WriteBytes(time);

    if (ReadCode() != 0)
        FailHost();
}

void Player::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber, uint32_t time)
{
    const uint32_t SizeAndPort = ((uint32_t) size & 0xFFFFFF) | (portNumber << 24);

    WriteBytes(11);
    WriteBytes(SizeAndPort);
    WriteBytes(time);
    WriteBytesOverlapped(data, (uint32_t) size);

    if (ReadCode() != 0)
        FailHost();
}

void Player::FailHost()
{
    _ErrorMessage = "The VSTi helper stopped responding during MIDI playback.";
    StopHost();
}

#pragma endregion

#pragma region Private

bool Player::StartHost()
{
    std::string CommandLine;

    {
        fs::path HostPath = (const char8_t *) core_api::get_my_full_path();

        HostPath.remove_filename();
        HostPath /= (_ProcessorArchitecture == 64) ? u8"vsthost64.exe" : u8"vsthost32.exe";

        // MS Defender does not like applications that use the operating system... <sigh>
        if (!fs::exists(HostPath))
        {
            Log.AtError().Write(STR_COMPONENT_BASENAME, " can't start VSTi plug-in. Unable to find required host executable \"%s\".", (const char *) HostPath.u8string().c_str());

            return false;
        }

        uint32_t Sum = 0;

        {
            pfc::stringcvt::string_os_from_utf8 plugin_os((const char *) _FilePath.u8string().c_str());

            const TCHAR * ch = plugin_os.get_ptr();

            while (*ch)
            {
                Sum += (TCHAR) (*ch++ * 820109);
            }
        }

        CommandLine = std::string("\"") + (const char *) HostPath.u8string().c_str() + "\" \"" + (const char *) _FilePath.u8string().c_str() + "\" " + pfc::format_int(Sum, 0, 16).c_str();
    }

    if (!_IsCOMInitialized)
    {
        if (FAILED(::CoInitialize(NULL)))
            return false;

        _IsCOMInitialized = true;
    }

    {
        _hReadEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    pfc::string InPipeName, OutPipeName;

    {
        if (!CreatePipeName(InPipeName) || !CreatePipeName(OutPipeName))
        {
            StopHost();

            return false;
        }
    }

    pfc::stringcvt::string_os_from_utf8 InPipeNameOS(InPipeName), OutPipeNameOS(OutPipeName);

    SECURITY_ATTRIBUTES sa =
    {
        sizeof(sa),
        nullptr,
        TRUE,
    };

    {
        HANDLE hPipe = ::CreateNamedPipe(InPipeNameOS, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            StopHost();

            return false;
        }

        _hPipeInRead = ::CreateFile(InPipeNameOS, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeInWrite, 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    {
        HANDLE hPipe = ::CreateNamedPipe(OutPipeNameOS, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            StopHost();

            return false;
        }

        _hPipeOutWrite = ::CreateFile(OutPipeNameOS, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeOutRead, 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    {
        STARTUPINFO si =
        {
            .cb          = sizeof(si),

            .dwFlags     = STARTF_USESTDHANDLES, // | STARTF_USESHOWWINDOW;
        //  .wShowWindow = SW_HIDE,

            .hStdInput   = _hPipeInRead,
            .hStdOutput  = _hPipeOutWrite,
            .hStdError   = ::GetStdHandle(STD_ERROR_HANDLE),
        };

        // Pass editor state and requests independently of the decoder's audio commands.
        const HANDLE EditorEvents[] = { EditorDarkModeEvent(), _hEditorRequest, _hAutoEditorRequest, _EditorClosed.get() };
        HANDLE InheritedEditorEvents[_countof(EditorEvents)] = {};
        bool CanStart = true;
        for (size_t i = 0; i < _countof(EditorEvents); ++i)
        {
            if (!EditorEvents[i])
                continue;

            auto & Inherited = InheritedEditorEvents[i];
            const DWORD Access = EditorEvents[i] == _EditorClosed.get() ? SYNCHRONIZE | EVENT_MODIFY_STATE : SYNCHRONIZE;
            if (!::DuplicateHandle(::GetCurrentProcess(), EditorEvents[i], ::GetCurrentProcess(), &Inherited, Access, TRUE, 0))
            {
                if (i == 0)
                    continue;

                CanStart = false;
                break;
            }

            CommandLine += i == 0 ? " --theme=" : " ";
            CommandLine += pfc::format_int(reinterpret_cast<uintptr_t>(Inherited), 0, 16).c_str();
        }

        Log.AtDebug().Write(STR_COMPONENT_BASENAME " is using \"%s\" to start host process.", CommandLine.c_str());

        PROCESS_INFORMATION pi = { };

        const bool Started = CanStart && ::CreateProcess(NULL, (LPTSTR) (LPCTSTR) pfc::stringcvt::string_os_from_utf8(CommandLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
        for (const auto Inherited : InheritedEditorEvents)
            if (Inherited)
                ::CloseHandle(Inherited);

        if (!Started)
        {
            StopHost();

            return false;
        }

        // Close remote handles so the pipes will break when the process terminates.
        ::CloseHandle(_hPipeOutWrite);
        _hPipeOutWrite = 0;

        ::CloseHandle(_hPipeInRead);
        _hPipeInRead = 0;

        _hProcess = pi.hProcess;
        _hThread = pi.hThread;

        Log.AtDebug().Write(STR_COMPONENT_BASENAME " started DLL host: hProcess 0x%08X, hThread 0x%08X.", _hProcess, _hThread);

    #ifndef _DEBUG
        ::SetPriorityClass(_hProcess, ::GetPriorityClass(::GetCurrentProcess()));
        ::SetThreadPriority(_hThread, ::GetThreadPriority(::GetCurrentThread()));
    #endif
    }

    // The plug-in may load samples or presets before sending its first reply.
    const uint32_t Code = ReadCode(HostSetupTimeout);

    if (Code != 0)
    {
        StopHost();

        return false;
    }

    // Read the complete header before using sizes sent by the helper.
    uint32_t Header[6];
    if (!ReadBytes(Header, sizeof(Header)) || Header[0] > 256 || Header[1] > 256 || Header[2] > 256 ||
        Header[5] == 0 || Header[5] > 2)
    {
        StopHost();
        return false;
    }

    VendorVersion = Header[3];
    Id = Header[4];
    _ChannelCount = Header[5];
    Name.resize(Header[0]);
    VendorName.resize(Header[1]);
    ProductName.resize(Header[2]);
    if (!ReadBytes(Name.data(), Header[0]) || !ReadBytes(VendorName.data(), Header[1]) ||
        !ReadBytes(ProductName.data(), Header[2]))
    {
        StopHost();
        return false;
    }

    if (Name.empty())
        Name = (const char *) _FilePath.stem().u8string().c_str();

    // VST always uses float samples.
    SafeDelete(_SrcFrames);
    _SrcFrames = new float[MaxFrames * _ChannelCount];

    if (_hEditorRequest)
    {
        if (HasEditor())
            SetPlaybackHostProcess(_hEditorRequest, ::GetProcessId(_hProcess));
        else if (!IsHostRunning())
            return false;
    }

    return true;
}

void Player::StopHost() noexcept
{
    if (_IsTerminating || (_hReadEvent == 0))
        return;

    _IsTerminating = true;
    SetPlaybackHostProcess(_hEditorRequest, 0);

    if (_hProcess)
    {
        WriteBytes(0);

        ::WaitForSingleObject(_hProcess, 5000);
        ::TerminateProcess(_hProcess, 0);

        Log.AtDebug().Write(STR_COMPONENT_BASENAME " stopped DLL host: hProcess 0x%08X, hThread 0x%08X.", _hProcess, _hThread);

        ::CloseHandle(_hThread);
        _hThread = NULL;

        ::CloseHandle(_hProcess);
        _hProcess = NULL;
    }

    if (_hPipeInRead)
    {
        ::CloseHandle(_hPipeInRead);
        _hPipeInRead = NULL;
    }

    if (_hPipeInWrite)
    {
        ::CloseHandle(_hPipeInWrite);
        _hPipeInWrite = NULL;
    }

    if (_hPipeOutRead)
    {
        ::CloseHandle(_hPipeOutRead);
        _hPipeOutRead = NULL;
    }

    if (_hPipeOutWrite)
    {
        ::CloseHandle(_hPipeOutWrite);
        _hPipeOutWrite = NULL;
    }

    if (_hReadEvent)
    {
        ::CloseHandle(_hReadEvent);
        _hReadEvent = 0;
    }

    if (_IsCOMInitialized)
    {
        ::CoUninitialize();
        _IsCOMInitialized = false;
    }

    _IsTerminating = false;

    _IsStarted = false;
}

bool Player::IsHostRunning() noexcept
{
    if (_hProcess && ::WaitForSingleObject(_hProcess, 0) == WAIT_TIMEOUT)
        return true;

    return false;
}

uint32_t Player::ReadCode(DWORD timeout) noexcept
{
    uint32_t Code;
    return ReadBytes(&Code, sizeof(Code), timeout) ? Code : UINT32_MAX;
}

bool Player::ReadBytes(void * data, uint32_t size, DWORD timeout) noexcept
{
    auto * Data = static_cast<uint8_t *>(data);
    while (size != 0)
    {
        if (!IsHostRunning())
            return false;

        const uint32_t BytesRead = ReadBytesOverlapped(Data, size, timeout);
        if (BytesRead == 0)
            return false;

        Data += BytesRead;
        size -= BytesRead;
    }

    return true;
}

uint32_t Player::ReadBytesOverlapped(void * data, uint32_t size, DWORD timeout) noexcept
{
    ::ResetEvent(_hReadEvent);

    DWORD BytesRead = 0;
    OVERLAPPED Operation = {};
    Operation.hEvent = _hReadEvent;

    if (::ReadFile(_hPipeOutRead, data, size, &BytesRead, &Operation))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING)
        return 0;

    const HANDLE Handles[] = { _hReadEvent, _hProcess };
    const DWORD State = ::WaitForMultipleObjects(_countof(Handles), Handles, FALSE, timeout);
    if (State == WAIT_OBJECT_0 && ::GetOverlappedResult(_hPipeOutRead, &Operation, &BytesRead, FALSE))
        return BytesRead;

    ::CancelIoEx(_hPipeOutRead, &Operation);
    ::GetOverlappedResult(_hPipeOutRead, &Operation, &BytesRead, TRUE);
    if (State == WAIT_TIMEOUT)
        ::TerminateProcess(_hProcess, 1);

    return 0;
}

void Player::WriteBytes(uint32_t code) noexcept
{
    WriteBytesOverlapped(&code, sizeof(code));
}

void Player::WriteBytesOverlapped(const void * data, uint32_t size) noexcept
{
    if ((size == 0) || !IsHostRunning())
        return;

    DWORD BytesWritten;

    if (!::WriteFile(_hPipeInWrite, data, size, &BytesWritten, nullptr) || (BytesWritten < size))
        StopHost();
}

bool Player::CreatePipeName(pfc::string_base & pipeName)
{
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    pipeName = "\\\\.\\pipe\\";
    pipeName += pfc::print_guid(guid);

    return true;
}

#ifdef MESSAGE_PUMP
static void ProcessPendingMessages()
{
    MSG msg = {};

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#endif

#pragma endregion

}
