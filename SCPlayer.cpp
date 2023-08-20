
/** $VER: SCPlayer.cpp (2023.08.19) Secret Sauce **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "SCPlayer.h"

#include <sdk/foobar2000-lite.h>
#include <pfc/pathUtils.h>

#include <vector>

// It's a secret to everybody!
char _DLLFileName[] = { 'F', 'P', 'P', 'b', 'e', 'r', '.', 'q', 'y', 'y', 0 };

static struct FileNameDecrypter
{
    FileNameDecrypter() noexcept
    {
        char * p;

        for (p = _DLLFileName; *p; ++p)
        {
            if (*p >= 'A' && *p <= 'Z')
                *p = (((*p - 'A') + 13) % 26) + 'A';
            else
            if (*p >= 'a' && *p <= 'z')
                *p = (((*p - 'a') + 13) % 26) + 'a';
        }
    }
} _FileNameDecrypter;

#pragma region("Public")
SCPlayer::SCPlayer() noexcept : MIDIPlayer(), _RootPathName(0)
{
    _IsInitialized = false;
    _COMInitialisationCount = 0;

    for (size_t i = 0; i < 3; ++i)
    {
        _IsPortTerminating[i] = false;

        _hProcess[i] = NULL;
        _hThread[i] = NULL;
        _hReadEvent[i] = NULL;
        _hPipeInRead[i] = NULL;
        _hPipeInWrite[i] = NULL;
        _hPipeOutRead[i] = NULL;
        _hPipeOutWrite[i] = NULL;
    }

    _Samples = new(std::nothrow) float[4096 * 2];
}

SCPlayer::~SCPlayer()
{
    Shutdown();

    if (_Samples)
    {
        delete[] _Samples;
        _Samples = nullptr;
    }

    if (_RootPathName)
    {
        ::free(_RootPathName);
        _RootPathName = nullptr;
    }
}

void SCPlayer::SetRootPath(const char * rootPathName)
{
    if (_RootPathName)
        ::free(_RootPathName);

    size_t Length = ::strlen(rootPathName);

    _RootPathName = (char *) ::malloc(Length + 1);

    if (_RootPathName)
        ::memcpy(_RootPathName, rootPathName, Length + 1);
}
#pragma endregion

#pragma region("Protected")
bool SCPlayer::Startup()
{
    pfc::string8 path;

    if (_IsInitialized)
        return true;

    if (!_RootPathName)
        return false;

    path = pfc::io::path::combine(_RootPathName, _DLLFileName);

    if (!LoadCore(path))
        return false;

    for (uint32_t i = 0; i < 3; ++i)
    {
        WriteBytes(i, 1);
        WriteBytes(i, sizeof(uint32_t));
        WriteBytes(i, _SampleRate);

        if (ReadCode(i) != 0)
            return false;
    }

    _IsInitialized = true;

    Configure(_ConfigurationType, _FilterEffects);

    return true;
}

void SCPlayer::Shutdown()
{
    for (uint32_t i = 0; i < 3; ++i)
        StopHost(i);

    _IsInitialized = false;
}

void SCPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    ::memset(sampleData, 0, sampleCount * 2 * sizeof(audio_sample));

    while (sampleCount != 0)
    {
        unsigned long ToDo = (sampleCount > 4096) ? 4096 : sampleCount;

        for (size_t i = 0; i < 3; ++i)
        {
            RenderPort((uint32_t) i, _Samples, (uint32_t) ToDo);

            // Convert the format of the rendered output.
            for (size_t j = 0; j < ToDo; ++j)
            {
                sampleData[j * 2 + 0] += _Samples[j * 2 + 0];
                sampleData[j * 2 + 1] += _Samples[j * 2 + 1];
            }
        }

        sampleData += ToDo * 2;
        sampleCount -= ToDo;
    }
}

void SCPlayer::SendEvent(uint32_t event)
{
    uint32_t PorNumber = (event >> 24) & 0xFF;

    if (PorNumber > 2)
        PorNumber = 0;

    WriteBytes(PorNumber, 2);
    WriteBytes(PorNumber, event & 0xFFFFFF);

    if (ReadCode(PorNumber) != 0)
        StopHost(PorNumber);
}

void SCPlayer::SendEvent(uint32_t data, uint32_t time)
{
    uint32_t PortNumber = (data >> 24) & 0xFF;

    if (PortNumber > 2)
        PortNumber = 0;

    WriteBytes(PortNumber, 6);
    WriteBytes(PortNumber, data & 0xFFFFFF);
    WriteBytes(PortNumber, time);

    if (ReadCode(PortNumber) != 0)
        StopHost(PortNumber);
}

void SCPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    WriteBytes(portNumber, 3);
    WriteBytes(portNumber, (uint32_t) size);
    WriteBytesOverlapped(portNumber, data, (uint32_t) size);

    if (ReadCode(portNumber) != 0)
        StopHost(portNumber);

    if (portNumber == 0)
    {
        SendSysEx(data, size, 1);
        SendSysEx(data, size, 2);
    }
}

void SCPlayer::SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber, uint32_t time)
{
    WriteBytes(portNumber, 7);
    WriteBytes(portNumber, (uint32_t) size);
    WriteBytes(portNumber, (uint32_t) time);
    WriteBytesOverlapped(portNumber, event, (uint32_t) size);

    if (ReadCode(portNumber) != 0)
        StopHost(portNumber);

    if (portNumber == 0)
    {
        SendSysEx(event, size, 1, time);
        SendSysEx(event, size, 2, time);
    }
}
#pragma endregion

#pragma region("Private")
bool SCPlayer::LoadCore(const char * filePath)
{
    if (!filePath || !filePath[0])
        return false;

    if (filePath != _PluginFilePath)
        _PluginFilePath = filePath;

    _PluginArchitecture = GetProcessorArchitecture(_PluginFilePath);

    if (_PluginArchitecture == 0)
        return false;

    bool Success = StartHost(0);

    if (Success)
        Success = StartHost(1);

    if (Success)
        Success = StartHost(2);

    return Success;
}

void SCPlayer::RenderPort(uint32_t port, float * data, uint32_t size) noexcept
{
    WriteBytes(port, 4);
    WriteBytes(port, size);

    if (ReadCode(port) != 0)
    {
        StopHost(port);

        ::memset(data, 0, size * sizeof(float) * 2);
        return;
    }

    ReadBytes(port, data, size * sizeof(float) * 2);
}

static bool CreatePipeName(pfc::string_base & pipeName)
{
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    pipeName = "\\\\.\\pipe\\";
    pipeName += pfc::print_guid(guid);

    return true;
}

bool SCPlayer::StartHost(uint32_t port)
{
    if (_COMInitialisationCount == 0)
    {
        if (FAILED(::CoInitialize(NULL)))
            return false;
    }

    ++_COMInitialisationCount;

    _hReadEvent[port] = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    SECURITY_ATTRIBUTES sa =
    {
        sizeof(sa),
        nullptr,
        TRUE,
    };

    pfc::string8 InPipeName, OutPipeName;

    {
        if (!CreatePipeName(InPipeName) || !CreatePipeName(OutPipeName))
        {
            StopHost(port);

            return false;
        }
    }

    pfc::stringcvt::string_os_from_utf8 InPipeNameOS(InPipeName), OutPipeNameOS(OutPipeName);

    {
        HANDLE hPipe = ::CreateNamedPipe(InPipeNameOS, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            StopHost(port);

            return false;
        }

        _hPipeInRead[port] = ::CreateFile(InPipeNameOS, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeInWrite[port], 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    {
        HANDLE hPipe = ::CreateNamedPipe(OutPipeNameOS, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            StopHost(port);

            return false;
        }

        _hPipeOutWrite[port] = ::CreateFile(OutPipeNameOS, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeOutRead[port], 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    std::string CommandLine = "\"";

    {
        CommandLine += core_api::get_my_full_path();

        const size_t SlashPosition = CommandLine.find_last_of('\\');

        if (SlashPosition != std::string::npos)
            CommandLine.erase(CommandLine.begin() + (const __int64)(SlashPosition + 1), CommandLine.end());

        CommandLine += (_PluginArchitecture == 64) ? "scpipe64.exe" : "scpipe32.exe";
        CommandLine += "\" \"";
        CommandLine += _PluginFilePath;
        CommandLine += "\"";
    }

    {
        STARTUPINFO si = { sizeof(si) };

        si.hStdInput = _hPipeInRead[port];
        si.hStdOutput = _hPipeOutWrite[port];
        si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    //  si.wShowWindow = SW_HIDE;
        si.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

        PROCESS_INFORMATION pi;

        if (!::CreateProcess(NULL, (LPTSTR) (LPCTSTR) pfc::stringcvt::string_os_from_utf8(CommandLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            StopHost(port);

            return false;
        }

        // Close remote handles so pipes will break when process terminates.
        ::CloseHandle(_hPipeOutWrite[port]);
        _hPipeOutWrite[port] = nullptr;

        ::CloseHandle(_hPipeInRead[port]);
        _hPipeInRead[port] = nullptr;

        _hProcess[port] = pi.hProcess;
        _hThread[port] = pi.hThread;

    #ifdef _DEBUG
        FB2K_console_print("Starting host... (hProcess = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess[port], 8), ", hThread = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread[port], 8), ")");
    #endif

    #ifdef NDEBUG
        ::SetPriorityClass(_hProcess[port], ::GetPriorityClass(::GetCurrentProcess()));
        ::SetThreadPriority(_hThread[port], ::GetThreadPriority(::GetCurrentThread()));
    #endif
    }

    // Get the startup information.
    const uint32_t Code = ReadCode(port);

    if (Code != 0)
    {
        StopHost(port);
        return false;
    }

    return true;
}

void SCPlayer::StopHost(uint32_t port) noexcept
{
    if (_IsPortTerminating[port])
        return;

    _IsPortTerminating[port] = true;

    #ifdef _DEBUG
        FB2K_console_print("Stopping host... (hProcess = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess[port], 8), ", hThread = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread[port], 8), ")");
    #endif

    if (_hProcess[port])
    {
        WriteBytes(port, 0);

        ::WaitForSingleObject(_hProcess[port], 5000);
        ::TerminateProcess(_hProcess[port], 0);

        ::CloseHandle(_hThread[port]);
        _hThread[port] = NULL;

        ::CloseHandle(_hProcess[port]);
        _hProcess[port] = NULL;
    }

    if (_hPipeInRead[port])
    {
        ::CloseHandle(_hPipeInRead[port]);
        _hPipeInRead[port] = NULL;
    }

    if (_hPipeInWrite[port])
    {
        ::CloseHandle(_hPipeInWrite[port]);
        _hPipeInWrite[port] = NULL;
    }

    if (_hPipeOutRead[port])
    {
        ::CloseHandle(_hPipeOutRead[port]);
        _hPipeOutRead[port] = NULL;
    }

    if (_hPipeOutWrite[port])
    {
        ::CloseHandle(_hPipeOutWrite[port]);
        _hPipeOutWrite[port] = NULL;
    }

    if (_hReadEvent[port])
    {
        ::CloseHandle(_hReadEvent[port]);
        _hReadEvent[port] = NULL;
    }

    if (--_COMInitialisationCount == 0)
        ::CoUninitialize();

    _IsPortTerminating[port] = false;
}

bool SCPlayer::IsHostRunning(uint32_t port) noexcept
{
    if (_hProcess[port] && ::WaitForSingleObject(_hProcess[port], 0) == WAIT_TIMEOUT)
        return true;

    return false;
}

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

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

uint32_t SCPlayer::ReadCode(uint32_t port) noexcept
{
    uint32_t Code;

    ReadBytes(port, &Code, sizeof(Code));

    return Code;
}

void SCPlayer::ReadBytes(uint32_t port, void * data, uint32_t size) noexcept
{
    if (size == 0)
        return;

    if (IsHostRunning(port))
    {
        uint8_t * Data = (uint8_t *) data;
        uint32_t BytesTotal = 0;

        while (BytesTotal < size)
        {
            const uint32_t BytesRead = ReadBytesOverlapped(port, Data + BytesTotal, size - BytesTotal);

            if (BytesRead == 0)
            {
                ::memset(data, 0xFF, size);
                break;
            }

            BytesTotal += BytesRead;
        }
    }
    else
        ::memset(data, 0xFF, size);
}

uint32_t SCPlayer::ReadBytesOverlapped(uint32_t port, void * out, uint32_t size) noexcept
{
    ::ResetEvent(_hReadEvent);

    ::SetLastError(NO_ERROR);

    DWORD BytesRead;
    OVERLAPPED ol = { 0 };

    ol.hEvent = _hReadEvent[port];

    if (::ReadFile(_hPipeOutRead[port], out, size, &BytesRead, &ol))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING)
        return 0;

    const HANDLE handles[1] = { _hReadEvent[port] };

    ::SetLastError(NO_ERROR);

    DWORD state;

#ifdef MESSAGE_PUMP
    for (;;)
    {
        state = ::MsgWaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE, QS_ALLEVENTS);

        if (state == WAIT_OBJECT_0 + _countof(handles))
            ProcessPendingMessages();
        else
            break;
    }
#else
    state = ::WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
#endif

    if (state == WAIT_OBJECT_0 && ::GetOverlappedResult(_hPipeOutRead[port], &ol, &BytesRead, TRUE))
        return BytesRead;

    ::CancelIoEx(_hPipeOutRead, &ol);

    return 0;
}

void SCPlayer::WriteBytes(uint32_t port, uint32_t code) noexcept
{
    WriteBytesOverlapped(port, &code, sizeof(code));
}

void SCPlayer::WriteBytesOverlapped(uint32_t port, const void * data, uint32_t size) noexcept
{
    if ((size == 0) || !IsHostRunning(port))
        return;

    DWORD BytesWritten;

    if (!::WriteFile(_hPipeInWrite[port], data, size, &BytesWritten, NULL) || BytesWritten < size)
        StopHost(port);
}
