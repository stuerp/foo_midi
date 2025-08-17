
/** $VER: SCPlayer.cpp (2025.07.03) Secret Sauce **/

#include "pch.h"

#include "SCPlayer.h"

#include <sdk/foobar2000-lite.h>

#include <vector>

#include "SecretSauce.h"

#pragma region Public

SCPlayer::SCPlayer() noexcept : player_t(),  _Samples()
{
    _IsStarted = false;
    _COMInitialisationCount = 0;

    for (size_t i = 0; i < _countof(_hProcess); ++i)
    {
        _hProcess[i] = NULL;
        _hThread[i] = NULL;
        _hReadEvent[i] = NULL;
        _hPipeInRead[i] = NULL;
        _hPipeInWrite[i] = NULL;
        _hPipeOutRead[i] = NULL;
        _hPipeOutWrite[i] = NULL;

        _IsPortTerminating[i] = false;
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

    _RootPath.clear();
}

/// <summary>
/// Sets the root path name of Secret Sauce.
/// </summary>
void SCPlayer::SetRootPath(const fs::path & directoryPath)
{
    _RootPath = directoryPath;
}

#pragma endregion

#pragma region Protected

/// <summary>
/// Starts the player.
/// </summary>
bool SCPlayer::Startup()
{
    if (_IsStarted)
        return true;

    if (_RootPath.empty())
        return false;

    fs::path FilePath = _RootPath / _DLLFileName;

    if (!StartHosts(FilePath))
        return false;

    for (uint32_t i = 0; i < _countof(_hProcess); ++i)
    {
        WriteBytes(i, 1);
        WriteBytes(i, sizeof(uint32_t));
        WriteBytes(i, _SampleRate);

        if (ReadCode(i) != 0)
            return false;
    }

    _IsStarted = true;

    Reset();

    return true;
}

/// <summary>
/// Stops the player.
/// </summary>
void SCPlayer::Shutdown()
{
    for (uint32_t i = 0; i < _countof(_hProcess); ++i)
        StopHost(i);

    _IsStarted = false;
}

/// <summary>
/// Renders a block of samples.
/// </summary>
void SCPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    ::memset(sampleData, 0, sampleCount * 2 * sizeof(audio_sample));

    while (sampleCount != 0)
    {
        unsigned long ToDo = (sampleCount > 4096) ? 4096 : sampleCount;

        for (size_t i = 0; i < _countof(_hProcess); ++i)
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

/// <summary>
/// Resets the player.
/// </summary>
bool SCPlayer::Reset()
{
    for (uint8_t i = 0; i < MaxPorts; ++i)
        ResetPort(i, 0);

    return true;
}

/// <summary>
/// Sends the specified MIDI event.
/// </summary>
void SCPlayer::SendEvent(uint32_t event)
{
    uint32_t PortNumber = (event >> 24) & 0xFF;

    if (PortNumber > (_countof(_hProcess) - 1))
        PortNumber = 0;

    WriteBytes(PortNumber, 2);
    WriteBytes(PortNumber, event & 0xFFFFFF);

    if (ReadCode(PortNumber) != 0)
        StopHost(PortNumber);
}

/// <summary>
/// Sends the specified MIDI event with a timestamp.
/// </summary>
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

/// <summary>
/// Sends the specified SysEx event.
/// </summary>
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

/// <summary>
/// Sends the specified SysEx event with a timestamp.
/// </summary>
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

#pragma region Private

/// <summary>
/// Starts the host processes.
/// </summary>
bool SCPlayer::StartHosts(const fs::path & filePath)
{
    if (filePath.empty())
        return false;

    _FilePath = filePath;
    _ProcessorArchitecture = GetProcessorArchitecture(_FilePath);

    if (_ProcessorArchitecture == 0)
        return false;

    for (uint32_t i = 0; i < _countof(_hProcess); ++i)
        if (!StartHost(i))
            return false;

    return true;
}

/// <summary>
/// Renders samples on the specified port.
/// </summary>
void SCPlayer::RenderPort(uint32_t portNumber, float * data, uint32_t size) noexcept
{
    if (portNumber >= _countof(_hProcess))
        return;

    WriteBytes(portNumber, 4);
    WriteBytes(portNumber, size);

    if (ReadCode(portNumber) != 0)
    {
        StopHost(portNumber);

        ::memset(data, 0, size * sizeof(float) * 2);
        return;
    }

    ReadBytes(portNumber, data, size * sizeof(float) * 2);
}

/// <summary>
/// Starts the host process of the specified port.
/// </summary>
bool SCPlayer::StartHost(uint32_t portNumber)
{
    if (portNumber >= _countof(_hProcess))
        return false;

    if (_COMInitialisationCount == 0)
    {
        if (FAILED(::CoInitialize(NULL)))
            return false;
    }

    ++_COMInitialisationCount;

    _hReadEvent[portNumber] = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    SECURITY_ATTRIBUTES sa =
    {
        sizeof(sa),
        nullptr,
        TRUE,
    };

    pfc::string InPipeName, OutPipeName;

    {
        if (!CreatePipeName(InPipeName) || !CreatePipeName(OutPipeName))
        {
            StopHost(portNumber);

            return false;
        }
    }

    pfc::stringcvt::string_os_from_utf8 InPipeNameOS(InPipeName), OutPipeNameOS(OutPipeName);

    {
        HANDLE hPipe = ::CreateNamedPipe(InPipeNameOS, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            StopHost(portNumber);

            return false;
        }

        _hPipeInRead[portNumber] = ::CreateFile(InPipeNameOS, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeInWrite[portNumber], 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    {
        HANDLE hPipe = ::CreateNamedPipe(OutPipeNameOS, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            StopHost(portNumber);

            return false;
        }

        _hPipeOutWrite[portNumber] = ::CreateFile(OutPipeNameOS, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

        ::DuplicateHandle(GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &_hPipeOutRead[portNumber], 0, FALSE, DUPLICATE_SAME_ACCESS);

        ::CloseHandle(hPipe);
    }

    std::string CommandLine = "\"";

    {
        CommandLine += core_api::get_my_full_path();

        const size_t SlashPosition = CommandLine.find_last_of('\\');

        if (SlashPosition != std::string::npos)
            CommandLine.erase(CommandLine.begin() + (const __int64)(SlashPosition + 1), CommandLine.end());

        CommandLine += (_ProcessorArchitecture == 64) ? "scpipe64.exe" : "scpipe32.exe";
        CommandLine += "\" \"";
        CommandLine += (const char *) _FilePath.u8string().c_str();
        CommandLine += "\"";
    }

    {
        STARTUPINFO si =
        {
            .cb = sizeof(si),

            .dwFlags = STARTF_USESTDHANDLES, // | STARTF_USESHOWWINDOW;
        //  .wShowWindow = SW_HIDE,

            .hStdInput = _hPipeInRead[portNumber],
            .hStdOutput = _hPipeOutWrite[portNumber],
            .hStdError = ::GetStdHandle(STD_ERROR_HANDLE),
        };

        PROCESS_INFORMATION pi = { };

        if (!::CreateProcess(NULL, (LPTSTR) (LPCTSTR) pfc::stringcvt::string_os_from_utf8(CommandLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            StopHost(portNumber);

            return false;
        }

        // Close remote handles so pipes will break when process terminates.
        ::CloseHandle(_hPipeOutWrite[portNumber]);
        _hPipeOutWrite[portNumber] = nullptr;

        ::CloseHandle(_hPipeInRead[portNumber]);
        _hPipeInRead[portNumber] = nullptr;

        _hProcess[portNumber] = pi.hProcess;
        _hThread[portNumber] = pi.hThread;

    #ifdef _DEBUG
        FB2K_console_print("Starting host... (hProcess = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess[portNumber], 8), ", hThread = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread[portNumber], 8), ")");
    #else
        ::SetPriorityClass(_hProcess[portNumber], ::GetPriorityClass(::GetCurrentProcess()));
        ::SetThreadPriority(_hThread[portNumber], ::GetThreadPriority(::GetCurrentThread()));
    #endif
    }

    // Get the startup information.
    const uint32_t Code = ReadCode(portNumber);

    if (Code != 0)
    {
        StopHost(portNumber);

        return false;
    }

    return true;
}

/// <summary>
/// Starts the host process of the specified port.
/// </summary>
void SCPlayer::StopHost(uint32_t portNumber) noexcept
{
    if (portNumber >= _countof(_hProcess))
        return;

    if (_IsPortTerminating[portNumber])
        return;

    _IsPortTerminating[portNumber] = true;

    #ifdef _DEBUG
        FB2K_console_print("Stopping host... (hProcess = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess[portNumber], 8), ", hThread = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread[portNumber], 8), ")");
    #endif

    if (_hProcess[portNumber])
    {
        WriteBytes(portNumber, 0);

        ::WaitForSingleObject(_hProcess[portNumber], 5000);
        ::TerminateProcess(_hProcess[portNumber], 0);

        ::CloseHandle(_hThread[portNumber]);
        _hThread[portNumber] = NULL;

        ::CloseHandle(_hProcess[portNumber]);
        _hProcess[portNumber] = NULL;
    }

    if (_hPipeInRead[portNumber])
    {
        ::CloseHandle(_hPipeInRead[portNumber]);
        _hPipeInRead[portNumber] = NULL;
    }

    if (_hPipeInWrite[portNumber])
    {
        ::CloseHandle(_hPipeInWrite[portNumber]);
        _hPipeInWrite[portNumber] = NULL;
    }

    if (_hPipeOutRead[portNumber])
    {
        ::CloseHandle(_hPipeOutRead[portNumber]);
        _hPipeOutRead[portNumber] = NULL;
    }

    if (_hPipeOutWrite[portNumber])
    {
        ::CloseHandle(_hPipeOutWrite[portNumber]);
        _hPipeOutWrite[portNumber] = NULL;
    }

    if (_hReadEvent[portNumber])
    {
        ::CloseHandle(_hReadEvent[portNumber]);
        _hReadEvent[portNumber] = NULL;
    }

    if (--_COMInitialisationCount == 0)
        ::CoUninitialize();

    _IsPortTerminating[portNumber] = false;
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

/// <summary>
/// Creates a unique pipe name.
/// </summary>
bool SCPlayer::CreatePipeName(pfc::string_base & pipeName) noexcept
{
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    pipeName = "\\\\.\\pipe\\";
    pipeName += pfc::print_guid(guid);

    return true;
}

/// <summary>
/// Reads a code from a port.
/// </summary>
uint32_t SCPlayer::ReadCode(uint32_t portNumber) noexcept
{
    uint32_t Code;

    ReadBytes(portNumber, &Code, sizeof(Code));

    return Code;
}

/// <summary>
/// Reads a number of bytes from a port.
/// </summary>
void SCPlayer::ReadBytes(uint32_t portNumber, void * data, uint32_t size) noexcept
{
    if ((portNumber >= _countof(_hProcess)) || (size == 0))
        return;

    if (IsHostRunning(portNumber))
    {
        uint8_t * Data = (uint8_t *) data;
        uint32_t BytesTotal = 0;

        while (BytesTotal < size)
        {
            const uint32_t BytesRead = ReadBytesOverlapped(portNumber, Data + BytesTotal, size - BytesTotal);

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

/// <summary>
/// Reads a number of bytes from a port using overlapped I/O.
/// </summary>
uint32_t SCPlayer::ReadBytesOverlapped(uint32_t portNumber, void * out, uint32_t size) noexcept
{
    if ((portNumber >= _countof(_hProcess)) || (size == 0))
        return 0;

    ::ResetEvent(_hReadEvent);

    ::SetLastError(NO_ERROR);

    DWORD BytesRead;
    OVERLAPPED ol = { 0 };

    ol.hEvent = _hReadEvent[portNumber];

    if (::ReadFile(_hPipeOutRead[portNumber], out, size, &BytesRead, &ol))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING)
        return 0;

    const HANDLE WaitHandles[1] = { _hReadEvent[portNumber] };

    ::SetLastError(NO_ERROR);

    DWORD Result;

#ifdef MESSAGE_PUMP
    for (;;)
    {
        Result = ::MsgWaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE, QS_ALLEVENTS);

        if (Result == WAIT_OBJECT_0 + _countof(WaitHandles))
            ProcessPendingMessages();
        else
            break;
    }
#else
    Result = ::WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE);
#endif

    if ((Result == WAIT_OBJECT_0) && ::GetOverlappedResult(_hPipeOutRead[portNumber], &ol, &BytesRead, TRUE))
        return BytesRead;

    ::CancelIoEx(_hPipeOutRead, &ol);

    return 0;
}

/// <summary>
/// Writes a number of bytes to a port.
/// </summary>
void SCPlayer::WriteBytes(uint32_t portNumber, uint32_t code) noexcept
{
    WriteBytesOverlapped(portNumber, &code, sizeof(code));
}

/// <summary>
/// Writes a number of bytes to a port using overlapped I/O.
/// </summary>
void SCPlayer::WriteBytesOverlapped(uint32_t portNumber, const void * data, uint32_t size) noexcept
{
    if ((portNumber >= _countof(_hProcess)) || (size == 0) || !IsHostRunning(portNumber))
        return;

    DWORD BytesWritten;

    if (!::WriteFile(_hPipeInWrite[portNumber], data, size, &BytesWritten, NULL) || (BytesWritten < size))
        StopHost(portNumber);
}

/// <summary>
/// Returns true if the host process of the specified port is running.
/// </summary>
bool SCPlayer::IsHostRunning(uint32_t portNumber) noexcept
{
    if (portNumber >= _countof(_hProcess))
        return false;

    if ((_hProcess[portNumber] != NULL) && (::WaitForSingleObject(_hProcess[portNumber], 0) == WAIT_TIMEOUT))
        return true;

    return false;
}

#pragma endregion
