
/** $VER: SCPlayer.cpp (2022.12.31) Secret Sauce **/

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include "SCPlayer.h"

#include <sdk/foobar2000-lite.h>

#include <vector>

// It's a secret to everybody!
char _DLLFileName[] = { 'F', 'P', 'P', 'b', 'e', 'r', '.', 'q', 'y', 'y', 0 };

static struct init_sc_name
{
    init_sc_name() noexcept
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
} _InitSCName;

SCPlayer::SCPlayer() noexcept : MIDIPlayer(), _SCCorePathName(0)
{
    _IsInitialized = false;
    _COMInitialisationCount = 0;

    for (unsigned int i = 0; i < 3; ++i)
    {
        _IsPortTerminating[i] = false;

        hProcess[i] = nullptr;
        hThread[i] = nullptr;
        hReadEvent[i] = nullptr;
        hChildStd_IN_Rd[i] = nullptr;
        hChildStd_IN_Wr[i] = nullptr;
        hChildStd_OUT_Rd[i] = nullptr;
        hChildStd_OUT_Wr[i] = nullptr;
    }

    _Buffer = new(std::nothrow) float[4096 * 2];
}

SCPlayer::~SCPlayer()
{
    shutdown();

    if (_Buffer)
    {
        delete[] _Buffer;
        _Buffer = nullptr;
    }

    if (_SCCorePathName)
    {
        ::free(_SCCorePathName);
        _SCCorePathName = nullptr;
    }
}

bool SCPlayer::startup()
{
    pfc::string8 path;

    if (_IsInitialized)
        return true;

    if (!_SCCorePathName)
        return false;

    path = _SCCorePathName;
    path += "\\";
    path += _DLLFileName;

    if (!LoadCore(path))
        return false;

    for (uint32_t i = 0; i < 3; ++i)
    {
        process_write_code(i, 1);
        process_write_code(i, sizeof(uint32_t));
        process_write_code(i, _SampleRate);

        if (process_read_code(i) != 0)
            return false;
    }

    _IsInitialized = true;

    setFilterMode(_FilterMode, _IsReverbChorusDisabled);

    return true;
}

void SCPlayer::shutdown()
{
    for (uint32_t i = 0; i < 3; ++i)
        process_terminate(i);

    _IsInitialized = false;
}

static uint16_t getwordle(uint8_t * data)
{
    return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

static uint32_t getdwordle(uint8_t * data)
{
    return data[0] | (static_cast<uint32_t>(data[1]) << 8) | (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

unsigned SCPlayer::test_plugin_platform()
{
#define iMZHeaderSize (0x40)
#define iPEHeaderSize (4 + 20 + 224)

    uint8_t peheader[iPEHeaderSize];
    uint32_t dwOffsetPE;

    std::string URI = "file://";

    URI += PluginFilePath;

    file::ptr f;
    abort_callback_dummy m_abort;

    try
    {
        filesystem::g_open(f, URI.c_str(), filesystem::open_mode_read, m_abort);

        f->read_object(peheader, iMZHeaderSize, m_abort);

        if (getwordle(peheader) != 0x5A4D)
            return 0;

        dwOffsetPE = getdwordle(peheader + 0x3c);

        f->seek(dwOffsetPE, m_abort);
        f->read_object(peheader, iPEHeaderSize, m_abort);

        if (getdwordle(peheader) != 0x00004550)
            return 0;

        switch (getwordle(peheader + 4))
        {
            case 0x014C:
                return 32;

            case 0x8664:
                return 64;

            default:
                return 0;
        }
    }
    catch (...)
    {
    }

    return 0;
}

bool SCPlayer::LoadCore(const char * filePath)
{
    if (!filePath || !filePath[0])
        return false;

    if (filePath != PluginFilePath)
        PluginFilePath = filePath;

    _PluginArchitecture = test_plugin_platform();

    if (!_PluginArchitecture)
        return false;

    bool Success = process_create(0);

    if (Success)
        Success = process_create(1);

    if (Success)
        Success = process_create(2);

    return Success;
}

static bool create_pipe_name(pfc::string_base & out)
{
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    out = "\\\\.\\pipe\\";
    out += pfc::print_guid(guid);

    return true;
}

bool SCPlayer::process_create(uint32_t port)
{
    SECURITY_ATTRIBUTES sa =
    {
        sizeof(sa),
        nullptr,
        TRUE,
    };

    if (_COMInitialisationCount == 0)
    {
        if (FAILED(::CoInitialize(NULL)))
            return false;
    }

    ++_COMInitialisationCount;

    hReadEvent[port] = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    pfc::string8 pipe_name_in, pipe_name_out;

    if (!create_pipe_name(pipe_name_in) || !create_pipe_name(pipe_name_out))
    {
        process_terminate(port);
        return false;
    }

    pfc::stringcvt::string_os_from_utf8 pipe_name_in_os(pipe_name_in), pipe_name_out_os(pipe_name_out);

    HANDLE hPipe = ::CreateNamedPipe(pipe_name_in_os, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        process_terminate(port);
        return false;
    }

    hChildStd_IN_Rd[port] = ::CreateFile(pipe_name_in_os, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);

    ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &hChildStd_IN_Wr[port], 0, FALSE, DUPLICATE_SAME_ACCESS);

    ::CloseHandle(hPipe);

    hPipe = ::CreateNamedPipe(pipe_name_out_os, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &sa);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        process_terminate(port);
        return false;
    }

    hChildStd_OUT_Wr[port] = ::CreateFile(pipe_name_out_os, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    ::DuplicateHandle(GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &hChildStd_OUT_Rd[port], 0, FALSE, DUPLICATE_SAME_ACCESS);
    ::CloseHandle(hPipe);

    std::string CommandLine = "\"";

    CommandLine += core_api::get_my_full_path();

    const size_t slash = CommandLine.find_last_of('\\');

    if (slash != std::string::npos)
        CommandLine.erase(CommandLine.begin() + (const __int64)(slash + 1), CommandLine.end());

    CommandLine += (_PluginArchitecture == 64) ? "scpipe64.exe" : "scpipe32.exe";
    CommandLine += "\" \"";
    CommandLine += PluginFilePath;
    CommandLine += "\"";

    STARTUPINFO si = { sizeof(si) };

    si.hStdInput = hChildStd_IN_Rd[port];
    si.hStdOutput = hChildStd_OUT_Wr[port];
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
//  si.wShowWindow = SW_HIDE;
    si.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

    PROCESS_INFORMATION pi;

    if (!::CreateProcess(NULL, (LPTSTR) (LPCTSTR) pfc::stringcvt::string_os_from_utf8(CommandLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        process_terminate(port);
        return false;
    }

    // Close remote handles so pipes will break when process terminates.
    ::CloseHandle(hChildStd_OUT_Wr[port]);
    hChildStd_OUT_Wr[port] = nullptr;

    ::CloseHandle(hChildStd_IN_Rd[port]);
    hChildStd_IN_Rd[port] = nullptr;

    hProcess[port] = pi.hProcess;
    hThread[port] = pi.hThread;

#ifdef NDEBUG
    SetPriorityClass(hProcess[port], GetPriorityClass(GetCurrentProcess()));
    SetThreadPriority(hThread[port], GetThreadPriority(GetCurrentThread()));
#endif

    uint32_t code = process_read_code(port);

    if (code != 0)
    {
        process_terminate(port);
        return false;
    }

    return true;
}

void SCPlayer::process_terminate(uint32_t port)
{
    if (_IsPortTerminating[port])
        return;

    _IsPortTerminating[port] = true;

    if (hProcess[port])
    {
        process_write_code(port, 0);

        ::WaitForSingleObject(hProcess[port], 5000);
        ::TerminateProcess(hProcess[port], 0);

        ::CloseHandle(hThread[port]);
        ::CloseHandle(hProcess[port]);
    }

    if (hChildStd_IN_Rd[port])
    {
        ::CloseHandle(hChildStd_IN_Rd[port]);
        hChildStd_IN_Rd[port] = nullptr;
    }

    if (hChildStd_IN_Wr[port])
    {
        ::CloseHandle(hChildStd_IN_Wr[port]);
        hChildStd_IN_Wr[port] = nullptr;
    }

    if (hChildStd_OUT_Rd[port])
    {
        ::CloseHandle(hChildStd_OUT_Rd[port]);
        hChildStd_OUT_Rd[port] = nullptr;
    }

    if (hChildStd_OUT_Wr[port])
    {
        ::CloseHandle(hChildStd_OUT_Wr[port]);
        hChildStd_OUT_Wr[port] = nullptr;
    }

    if (hReadEvent[port])
    {
        ::CloseHandle(hReadEvent[port]);
        hReadEvent[port] = nullptr;
    }

    if (--_COMInitialisationCount == 0)
        ::CoUninitialize();

    _COMInitialisationCount = 0;

    _IsPortTerminating[port] = false;

    hProcess[port] = nullptr;
    hThread[port] = nullptr;

}

bool SCPlayer::process_running(uint32_t port)
{
    if (hProcess[port] && ::WaitForSingleObject(hProcess[port], 0) == WAIT_TIMEOUT)
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

uint32_t SCPlayer::process_read_bytes_pass(uint32_t port, void * out, uint32_t size)
{
    OVERLAPPED ol = {};

    ol.hEvent = hReadEvent[port];

    ::ResetEvent(hReadEvent);

    DWORD BytesRead;

    ::SetLastError(NO_ERROR);

    if (::ReadFile(hChildStd_OUT_Rd[port], out, size, &BytesRead, &ol))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING)
        return 0;

    const HANDLE handles[1] = { hReadEvent[port] };

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

    if (state == WAIT_OBJECT_0 && GetOverlappedResult(hChildStd_OUT_Rd[port], &ol, &BytesRead, TRUE)) return BytesRead;

#if _WIN32_WINNT >= 0x600
    ::CancelIoEx(hChildStd_OUT_Rd, &ol);
#else
    ::CancelIo(hChildStd_OUT_Rd[port]);
#endif

    return 0;
}

void SCPlayer::process_read_bytes(uint32_t port, void * out, uint32_t size)
{
    if (process_running(port) && size)
    {
        uint8_t * ptr = static_cast<uint8_t *>(out);
        uint32_t done = 0;

        while (done < size)
        {
            uint32_t delta = process_read_bytes_pass(port, ptr + done, size - done);

            if (delta == 0)
            {
                memset(out, 0xFF, size);
                break;
            }

            done += delta;
        }
    }
    else
        memset(out, 0xFF, size);
}

uint32_t SCPlayer::process_read_code(uint32_t port)
{
    uint32_t code;

    process_read_bytes(port, &code, sizeof(code));

    return code;
}

void SCPlayer::process_write_bytes(uint32_t port, const void * in, uint32_t size)
{
    if (process_running(port))
    {
        if (size == 0)
            return;

        DWORD BytesWritten;

        if (!::WriteFile(hChildStd_IN_Wr[port], in, size, &BytesWritten, NULL) || BytesWritten < size)
            process_terminate(port);
    }
}

void SCPlayer::process_write_code(uint32_t port, uint32_t code)
{
    process_write_bytes(port, &code, sizeof(code));
}

void SCPlayer::set_sccore_path(const char * path)
{
    size_t len;

    if (_SCCorePathName)
        ::free(_SCCorePathName);

    len = strlen(path);
    _SCCorePathName = (char *) ::malloc(len + 1);

    if (_SCCorePathName)
        ::memcpy(_SCCorePathName, path, len + 1);
}

void SCPlayer::send_event(uint32_t b)
{
    uint32_t port = (b >> 24) & 0xFF;

    if (port > 2)
        port = 0;

    process_write_code(port, 2);
    process_write_code(port, b & 0xFFFFFF);

    if (process_read_code(port) != 0)
        process_terminate(port);
}

void SCPlayer::send_sysex(const uint8_t * event, size_t size, size_t port)
{
    process_write_code((uint32_t)port, 3);
    process_write_code((uint32_t)port, (uint32_t)size);
    process_write_bytes((uint32_t)port, event, (uint32_t)size);

    if (process_read_code((uint32_t)port) != 0)
        process_terminate((uint32_t)port);

    if (port == 0)
    {
        send_sysex(event, size, 1);
        send_sysex(event, size, 2);
    }
}

void SCPlayer::send_event_time(uint32_t b, unsigned int time)
{
    uint32_t port = (b >> 24) & 0xFF;

    if (port > 2)
        port = 0;

    process_write_code(port, 6);
    process_write_code(port, b & 0xFFFFFF);
    process_write_code(port, time);

    if (process_read_code(port) != 0)
        process_terminate(port);
}

unsigned int SCPlayer::send_event_needs_time()
{
    return 0; // 4096; This doesn't work for some reason
}

void SCPlayer::send_sysex_time(const uint8_t * event, size_t size, size_t port, unsigned int time)
{
    process_write_code((uint32_t)port, 7);
    process_write_code((uint32_t)port, (uint32_t)size);
    process_write_code((uint32_t)port, (uint32_t)time);
    process_write_bytes((uint32_t)port, event, (uint32_t)size);

    if (process_read_code((uint32_t)port) != 0)
        process_terminate((uint32_t)port);

    if (port == 0)
    {
        send_sysex_time(event, size, 1, time);
        send_sysex_time(event, size, 2, time);
    }
}

void SCPlayer::render_port(uint32_t port, float * out, uint32_t count)
{
    process_write_code(port, 4);
    process_write_code(port, count);

    if (process_read_code(port) != 0)
    {
        process_terminate(port);

        ::memset(out, 0, count * sizeof(float) * 2);
        return;
    }

    process_read_bytes(port, out, count * sizeof(float) * 2);
}

void SCPlayer::render(audio_sample * out, unsigned long count)
{
    ::memset(out, 0, count * sizeof(audio_sample) * 2);

    while (count)
    {
        unsigned long todo = count > 4096 ? 4096 : count;

        for (unsigned long i = 0; i < 3; ++i)
        {
            render_port(i, _Buffer, todo);

            for (unsigned long j = 0; j < todo; ++j)
            {
                out[j * 2 + 0] += _Buffer[j * 2 + 0];
                out[j * 2 + 1] += _Buffer[j * 2 + 1];
            }
        }

        out   += todo * 2;
        count -= todo;
    }
}
