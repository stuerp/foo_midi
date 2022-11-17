#include "VSTiPlayer.h"

#define NOMINMAX

#include <foobar2000.h>

// #define LOG_EXCHANGE

template <class T> void SafeRelease(T& x) noexcept
{
    if (x)
    {
        delete[] x;
        x = nullptr;
    }
}

VSTiPlayer::VSTiPlayer() noexcept : MIDIPlayer()
{
    bInitialized = false;
    bTerminating = false;
    hProcess = NULL;
    hThread = NULL;
    hReadEvent = NULL;
    hChildStd_IN_Rd = NULL;
    hChildStd_IN_Wr = NULL;
    hChildStd_OUT_Rd = NULL;
    hChildStd_OUT_Wr = NULL;

    _Name = nullptr;
    _Vendor = nullptr;
    _Product = nullptr;

    _ChannelCount = 0;
    _VSTBuffer = nullptr;

    _PluginPlatform = 0;
    _UniqueId = 0;
    _VendorVersion = 0;
}

VSTiPlayer::~VSTiPlayer()
{
    process_terminate();

    SafeRelease(_Name);
    SafeRelease(_Vendor);
    SafeRelease(_Product);

    SafeRelease(_VSTBuffer);
}

static uint16_t getwordle(const uint8_t * data) noexcept
{
    return (uint16_t) (data[0] | (((uint16_t) data[1]) << 8));
}

static uint32_t getdwordle(const uint8_t * data) noexcept
{
    return data[0] | (((uint32_t) data[1]) << 8) | (((uint32_t) data[2]) << 16) | (((uint32_t) data[3]) << 24);
}

unsigned VSTiPlayer::test_plugin_platform()
{
    constexpr size_t iMZHeaderSize = 0x40;
    constexpr size_t iPEHeaderSize = 4 + 20 + 224;

    uint8_t peheader[iPEHeaderSize];
    uint32_t dwOffsetPE;

    std::string plugin = "file://";
    plugin += _PluginPathName;

    file::ptr f;
    abort_callback_dummy m_abort;

    try
    {
        filesystem::g_open(f, plugin.c_str(), filesystem::open_mode_read, m_abort);

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

bool VSTiPlayer::LoadVST(const char * pathName)
{
    if (!pathName || !pathName[0])
        return false;

    if (pathName != _PluginPathName)
        _PluginPathName = pathName;

    _PluginPlatform = test_plugin_platform();

    if (!_PluginPlatform)
        return false;

    return process_create();
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

bool VSTiPlayer::process_create()
{
    SECURITY_ATTRIBUTES saAttr = { };

    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!bInitialized)
    {
        if (FAILED(::CoInitialize(NULL)))
            return false;

        bInitialized = true;
    }

    hReadEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    pfc::string8 pipe_name_in, pipe_name_out;

    if (!create_pipe_name(pipe_name_in) || !create_pipe_name(pipe_name_out))
    {
        process_terminate();

        return false;
    }

    pfc::stringcvt::string_os_from_utf8 pipe_name_in_os(pipe_name_in), pipe_name_out_os(pipe_name_out);

    HANDLE hPipe = ::CreateNamedPipe(pipe_name_in_os, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        process_terminate();

        return false;
    }

    hChildStd_IN_Rd = ::CreateFile(pipe_name_in_os, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);

    ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &hChildStd_IN_Wr, 0, FALSE, DUPLICATE_SAME_ACCESS);

    ::CloseHandle(hPipe);

    hPipe = CreateNamedPipe(pipe_name_out_os, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        process_terminate();

        return false;
    }

    hChildStd_OUT_Wr = CreateFile(pipe_name_out_os, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);

    ::DuplicateHandle(::GetCurrentProcess(), hPipe, ::GetCurrentProcess(), &hChildStd_OUT_Rd, 0, FALSE, DUPLICATE_SAME_ACCESS);

    ::CloseHandle(hPipe);

    std::string CommandLine = "\"";

    {
        CommandLine += core_api::get_my_full_path();

        const size_t slash = CommandLine.find_last_of('\\');

        if (slash != std::string::npos)
            CommandLine.erase(CommandLine.begin() + slash + 1, CommandLine.end());

        CommandLine += (_PluginPlatform == 64) ? "vsthost64.exe" : "vsthost32.exe";
        CommandLine += "\" \"";
        CommandLine += _PluginPathName;
        CommandLine += "\" ";

        unsigned sum = 0;

        pfc::stringcvt::string_os_from_utf8 plugin_os(_PluginPathName.c_str());

        const TCHAR * ch = plugin_os.get_ptr();

        while (*ch)
        {
            sum += (TCHAR) (*ch++ * 820109);
        }

        CommandLine += pfc::format_int(sum, 0, 16);
    }

    PROCESS_INFORMATION pi;

    STARTUPINFO si = { sizeof(si) };

    si.hStdInput = hChildStd_IN_Rd;
    si.hStdOutput = hChildStd_OUT_Wr;
    si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
//  si.wShowWindow = SW_HIDE;
    si.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

    if (!::CreateProcess(NULL, (LPTSTR) (LPCTSTR) pfc::stringcvt::string_os_from_utf8(CommandLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        process_terminate();

        return false;
    }

    // Close remote handles so pipes will break when process terminates
    ::CloseHandle(hChildStd_OUT_Wr);
    hChildStd_OUT_Wr = NULL;

    ::CloseHandle(hChildStd_IN_Rd);
    hChildStd_IN_Rd = NULL;

    hProcess = pi.hProcess;
    hThread = pi.hThread;

#ifdef NDEBUG
    ::SetPriorityClass(hProcess, ::GetPriorityClass(::GetCurrentProcess()));
    ::SetThreadPriority(hThread, ::GetThreadPriority(::GetCurrentThread()));
#endif

    const uint32_t code = process_read_code();

    if (code != 0)
    {
        process_terminate();
        return false;
    }

    {
        uint32_t name_string_length = process_read_code();
        uint32_t vendor_string_length = process_read_code();
        uint32_t product_string_length = process_read_code();

        _VendorVersion = process_read_code();
        _UniqueId = process_read_code();
        _ChannelCount = process_read_code();

        SafeRelease(_VSTBuffer);

        // VST always uses float samples.
        _VSTBuffer = new float[4096 * _ChannelCount];

        SafeRelease(_Name);
        SafeRelease(_Vendor);
        SafeRelease(_Product);

        _Name = new char[name_string_length + 1];
        _Vendor = new char[vendor_string_length + 1];
        _Product = new char[product_string_length + 1];

        process_read_bytes(_Name, name_string_length);
        process_read_bytes(_Vendor, vendor_string_length);
        process_read_bytes(_Product, product_string_length);

        _Name[name_string_length] = 0;
        _Vendor[vendor_string_length] = 0;
        _Product[product_string_length] = 0;
    }

    return true;
}

void VSTiPlayer::process_terminate() noexcept
{
    if (bTerminating)
        return;

    bTerminating = true;

    if (hProcess)
    {
        process_write_code(0);

        ::WaitForSingleObject(hProcess, 5000);
        ::TerminateProcess(hProcess, 0);

        ::CloseHandle(hThread);
        ::CloseHandle(hProcess);
    }

    if (hChildStd_IN_Rd)
        ::CloseHandle(hChildStd_IN_Rd);

    if (hChildStd_IN_Wr)
        ::CloseHandle(hChildStd_IN_Wr);

    if (hChildStd_OUT_Rd)
        ::CloseHandle(hChildStd_OUT_Rd);

    if (hChildStd_OUT_Wr)
        ::CloseHandle(hChildStd_OUT_Wr);

    if (hReadEvent)
        ::CloseHandle(hReadEvent);

    if (bInitialized)
        ::CoUninitialize();

    bInitialized = false;
    bTerminating = false;

    hProcess = NULL;
    hThread = NULL;
    hReadEvent = NULL;
    hChildStd_IN_Rd = NULL;
    hChildStd_IN_Wr = NULL;
    hChildStd_OUT_Rd = NULL;
    hChildStd_OUT_Wr = NULL;

    initialized = false;
}

bool VSTiPlayer::process_running() noexcept
{
    if (hProcess && ::WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT)
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

uint32_t VSTiPlayer::process_read_bytes_pass(void * out, uint32_t size) noexcept
{
    OVERLAPPED ol = { 0 };

    ol.hEvent = hReadEvent;

    ::ResetEvent(hReadEvent);

    DWORD BytesRead;

    ::SetLastError(NO_ERROR);

    if (::ReadFile(hChildStd_OUT_Rd, out, size, &BytesRead, &ol))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING)
        return 0;

    const HANDLE handles[1] = { hReadEvent };

    ::SetLastError(NO_ERROR);

    DWORD state;

#ifdef MESSAGE_PUMP
    for (;;)
    {
        state = MsgWaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE, QS_ALLEVENTS);

        if (state == WAIT_OBJECT_0 + _countof(handles))
            ProcessPendingMessages();
        else
            break;
    }
#else
    state = ::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE);
#endif

    if (state == WAIT_OBJECT_0 && ::GetOverlappedResult(hChildStd_OUT_Rd, &ol, &BytesRead, TRUE))
        return BytesRead;

    ::CancelIoEx(hChildStd_OUT_Rd, &ol);

    return 0;
}

void VSTiPlayer::process_read_bytes(void * out, uint32_t size) noexcept
{
    if (size == 0)
        return;

    if (process_running())
    {
        uint8_t * ptr = (uint8_t *) out;
        uint32_t done = 0;

        while (done < size)
        {
            const uint32_t delta = process_read_bytes_pass(ptr + done, size - done);

            if (delta == 0)
            {
                ::memset(out, 0xFF, size);
                break;
            }

            done += delta;
        }
    }
    else
        ::memset(out, 0xFF, size);
}

uint32_t VSTiPlayer::process_read_code() noexcept
{
    uint32_t code;

    process_read_bytes(&code, sizeof(code));

    return code;
}

void VSTiPlayer::process_write_bytes(const void * in, uint32_t size) noexcept
{
    if (size == 0)
        return;

    if (!process_running())
        return;

    DWORD BytesWritten;

    if (!::WriteFile(hChildStd_IN_Wr, in, size, &BytesWritten, nullptr) || (BytesWritten < size))
        process_terminate();
}

void VSTiPlayer::process_write_code(uint32_t code) noexcept
{
    process_write_bytes(&code, sizeof(code));
}

void VSTiPlayer::getVendorString(std::string & out) const
{
    out = _Vendor;
}

void VSTiPlayer::getProductString(std::string & out) const 
{
    out = _Product;
}

long VSTiPlayer::getVendorVersion() const noexcept
{
    return _VendorVersion;
}

long VSTiPlayer::getUniqueID() const noexcept
{
    return _UniqueId;
}

void VSTiPlayer::getChunk(std::vector<uint8_t> & out)
{
    process_write_code(1);

    const uint32_t code = process_read_code();

    if (code == 0)
    {
        const uint32_t size = process_read_code();

        out.resize(size);

        if (size)
            process_read_bytes(&out[0], size);
    }
    else
        process_terminate();
}

void VSTiPlayer::setChunk(const void * in, unsigned long size)
{
    if (_Chunk.size() == 0 || (_Chunk.size() == size && size != 0 && in != (const void *) &_Chunk[0]))
    {
        _Chunk.resize(size);

        if (size)
            ::memcpy(&_Chunk[0], in, size);
    }

    process_write_code(2);
    process_write_code(size);
    process_write_bytes(in, size);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();
}

bool VSTiPlayer::hasEditor()
{
    process_write_code(3);

    uint32_t code = process_read_code();

    if (code != 0)
    {
        process_terminate();

        return false;
    }

    code = process_read_code();

    return code != 0;
}

void VSTiPlayer::displayEditorModal()
{
    process_write_code(4);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();
}

void VSTiPlayer::shutdown()
{
    process_terminate();
}

bool VSTiPlayer::startup()
{
    if (process_running())
        return true;

    if (!LoadVST(_PluginPathName.c_str()))
        return false;

    if (_Chunk.size())
        setChunk(&_Chunk[0], _Chunk.size());

    process_write_code(5);
    process_write_code(sizeof(uint32_t));
    process_write_code(uSampleRate);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();

    initialized = true;

    setFilterMode(mode, reverb_chorus_disabled);

    return true;
}

unsigned VSTiPlayer::getChannelCount() noexcept
{
    return _ChannelCount;
}

void VSTiPlayer::send_event(uint32_t b)
{
    process_write_code(7);
    process_write_code(b);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();
}

void VSTiPlayer::send_sysex(const uint8_t * event, size_t size, size_t port)
{
    const uint32_t size_plus_port = (size & 0xFFFFFF) | (port << 24);

    process_write_code(8);
    process_write_code(size_plus_port);
    process_write_bytes(event, size);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();
}

void VSTiPlayer::send_event_time(uint32_t b, unsigned int time)
{
    process_write_code(10);
    process_write_code(b);
    process_write_code(time);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();
}

void VSTiPlayer::send_sysex_time(const uint8_t * event, size_t size, size_t port, unsigned int time)
{
    const uint32_t size_plus_port = (size & 0xFFFFFF) | (port << 24);

    process_write_code(11);
    process_write_code(size_plus_port);
    process_write_code(time);
    process_write_bytes(event, size);

    const uint32_t code = process_read_code();

    if (code != 0)
        process_terminate();
}

void VSTiPlayer::render(audio_sample * out, unsigned long count)
{
    process_write_code(9);
    process_write_code(count);

    const uint32_t code = process_read_code();

    if (code != 0)
    {
        process_terminate();

        ::memset(out, 0, count * _ChannelCount * sizeof(audio_sample));

        return;
    }

    while (count)
    {
        unsigned ToDo = 4096 * _ChannelCount;

        if (ToDo > count)
            ToDo = count;

        if (_VSTBuffer != nullptr)
        {
            process_read_bytes(_VSTBuffer, ToDo * _ChannelCount * sizeof(float));

            for (int i = 0; i < ToDo * _ChannelCount; i++)
                out[i] = (audio_sample) _VSTBuffer[i];
        }

        out += ToDo * _ChannelCount;
        count -= ToDo;
    }
}

unsigned int VSTiPlayer::send_event_needs_time() noexcept
{
    return 4096;
}
