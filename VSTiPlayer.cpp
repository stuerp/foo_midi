
/** $VER: VSTiPlayer.cpp (2023.06.01) **/

#include "VSTiPlayer.h"

#define NOMINMAX

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

// #define LOG_EXCHANGE

template <class T> void SafeDelete(T& x) noexcept
{
    if (x)
    {
        delete[] x;
        x = nullptr;
    }
}

#pragma region("Public")
VSTiPlayer::VSTiPlayer() noexcept : MIDIPlayer()
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

    _Name = nullptr;
    _VendorName = nullptr;
    _ProductName = nullptr;

    _ChannelCount = 0;
    _Samples = nullptr;

    _PluginArchitecture = 0;
    _UniqueId = 0;
    _VendorVersion = 0;
}

VSTiPlayer::~VSTiPlayer()
{
    StopHost();

    SafeDelete(_Name);
    SafeDelete(_VendorName);
    SafeDelete(_ProductName);

    SafeDelete(_Samples);
}

bool VSTiPlayer::LoadVST(const char * pathName)
{
    if (!pathName || !pathName[0])
        return false;

    if (pathName != _PluginFilePath)
        _PluginFilePath = pathName;

    _PluginArchitecture = GetProcessorArchitecture(_PluginFilePath);

    if (_PluginArchitecture == 0)
        return false;

    return StartHost();
}

void VSTiPlayer::GetVendorName(pfc::string8 & vendorName) const
{
    vendorName = _VendorName;
}

void VSTiPlayer::GetProductName(pfc::string8 & productName) const 
{
    productName = _ProductName;
}

uint32_t VSTiPlayer::GetVendorVersion() const noexcept
{
    return _VendorVersion;
}

uint32_t VSTiPlayer::GetUniqueID() const noexcept
{
    return _UniqueId;
}

void VSTiPlayer::GetChunk(std::vector<uint8_t> & data)
{
    WriteBytes(1);

    const uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();
        return;
    }

    const uint32_t Size = ReadCode();

    data.resize(Size);

    if (Size != 0)
        ReadBytes(&data[0], Size);
}

void VSTiPlayer::SetChunk(const void * data, size_t size)
{
    if ((_Chunk.size() == 0) || ((_Chunk.size() == size) && (size != 0) && (data != (const void *) &_Chunk[0])))
    {
        _Chunk.resize(size);

        if (size != 0)
            ::memcpy(&_Chunk[0], data, size);
    }

    WriteBytes(2);
    WriteBytes((uint32_t)size);
    WriteBytesOverlapped(data, (uint32_t)size);

    const uint32_t Code = ReadCode();

    if (Code != 0)
        StopHost();
}

bool VSTiPlayer::HasEditor()
{
    WriteBytes(3);

    uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();
        return false;
    }

    Code = ReadCode();

    return Code != 0;
}

void VSTiPlayer::DisplayEditorModal()
{
    WriteBytes(4);

    const uint32_t Code = ReadCode();

    if (Code != 0)
        StopHost();
}
#pragma endregion

#pragma region("Protected")
bool VSTiPlayer::Startup()
{
    if (IsHostRunning())
        return true;

    if (!LoadVST(_PluginFilePath.c_str()))
        return false;

    if (_Chunk.size())
        SetChunk(&_Chunk[0], _Chunk.size());

    WriteBytes(5);
    WriteBytes(sizeof(uint32_t));
    WriteBytes(_SampleRate);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();

    _IsInitialized = true;

    SetFilter(_FilterType, _FilterEffects);

    return true;
}

void VSTiPlayer::Shutdown()
{
    StopHost();
}

void VSTiPlayer::Render(audio_sample * data, unsigned long size)
{
    WriteBytes(9);
    WriteBytes(size);

    const uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();

        ::memset(data, 0, (size_t)size * _ChannelCount * sizeof(audio_sample));

        return;
    }

    if (_Samples == nullptr)
        return;

    while (size)
    {
        size_t ToDo = size > 4096 ? 4096 : size;

        ReadBytes(_Samples, (uint32_t)(ToDo * _ChannelCount * sizeof(float)));

        for (size_t i = 0; i < ToDo * _ChannelCount; i++)
            data[i] = (audio_sample) _Samples[i];

        data += ToDo * _ChannelCount;
        size -= (unsigned long) ToDo;
    }
}

void VSTiPlayer::SendEvent(uint32_t b)
{
    WriteBytes(7);
    WriteBytes(b);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}

void VSTiPlayer::SendSysEx(const uint8_t * event, size_t size, size_t port)
{
    const uint32_t size_plus_port = ((uint32_t)size & 0xFFFFFF) | ((uint32_t)port << 24);

    WriteBytes(8);
    WriteBytes(size_plus_port);
    WriteBytesOverlapped(event, (uint32_t)size);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}

void VSTiPlayer::SendEventWithTime(uint32_t b, unsigned int time)
{
    WriteBytes(10);
    WriteBytes(b);
    WriteBytes(time);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}

void VSTiPlayer::SendSysExWithTime(const uint8_t * event, size_t size, size_t port, unsigned int time)
{
    const uint32_t size_plus_port = ((uint32_t)size & 0xFFFFFF) | ((uint32_t)port << 24);

    WriteBytes(11);
    WriteBytes(size_plus_port);
    WriteBytes(time);
    WriteBytesOverlapped(event, (uint32_t)size);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}
#pragma endregion

#pragma region("Private")
static bool CreatePipeName(pfc::string_base & pipeName)
{
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    pipeName = "\\\\.\\pipe\\";
    pipeName += pfc::print_guid(guid);

    return true;
}

bool VSTiPlayer::StartHost()
{
    if (!_IsCOMInitialized)
    {
        if (FAILED(::CoInitialize(NULL)))
            return false;

        _IsCOMInitialized = true;
    }

    {
        _hReadEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    }

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
            StopHost();

            return false;
        }
    }

    pfc::stringcvt::string_os_from_utf8 InPipeNameOS(InPipeName), OutPipeNameOS(OutPipeName);

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

    std::string CommandLine = "\"";

    {
        CommandLine += core_api::get_my_full_path();

        const size_t SlashPosition = CommandLine.find_last_of('\\');

        if (SlashPosition != std::string::npos)
            CommandLine.erase(CommandLine.begin() + (const __int64)(SlashPosition + 1), CommandLine.end());

        CommandLine += (_PluginArchitecture == 64) ? "vsthost64.exe" : "vsthost32.exe";
        CommandLine += "\" \"";
        CommandLine += _PluginFilePath;
        CommandLine += "\" ";

        uint32_t Sum = 0;

        {
            pfc::stringcvt::string_os_from_utf8 plugin_os(_PluginFilePath.c_str());

            const TCHAR * ch = plugin_os.get_ptr();

            while (*ch)
            {
                Sum += (TCHAR) (*ch++ * 820109);
            }
        }

        CommandLine += pfc::format_int(Sum, 0, 16);
    }

    {
        STARTUPINFO si = { sizeof(si) };

        si.hStdInput = _hPipeInRead;
        si.hStdOutput = _hPipeOutWrite;
        si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    //  si.wShowWindow = SW_HIDE;
        si.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

        PROCESS_INFORMATION pi;

        if (!::CreateProcess(NULL, (LPTSTR) (LPCTSTR) pfc::stringcvt::string_os_from_utf8(CommandLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
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

    #ifdef _DEBUG
        FB2K_console_print("Starting host... (hProcess = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess, 8), ", hThread = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread, 8), ")");
    #endif

    #ifdef NDEBUG
        ::SetPriorityClass(_hProcess, ::GetPriorityClass(::GetCurrentProcess()));
        ::SetThreadPriority(_hThread, ::GetThreadPriority(::GetCurrentThread()));
    #endif
    }

    // Get the startup information.
    const uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();
        return false;
    }

    {
        uint32_t NameLength = ReadCode();
        uint32_t VendorNameLength = ReadCode();
        uint32_t ProductNameLength = ReadCode();

        _VendorVersion = ReadCode();
        _UniqueId = ReadCode();
        _ChannelCount = ReadCode();

        {
            // VST always uses float samples.
            SafeDelete(_Samples);
            _Samples = new float[4096 * _ChannelCount];

            SafeDelete(_Name);
            _Name = new char[NameLength + 1];
            ReadBytes(_Name, NameLength);
            _Name[NameLength] = 0;

            SafeDelete(_VendorName);
            _VendorName = new char[VendorNameLength + 1];
            ReadBytes(_VendorName, VendorNameLength);
            _VendorName[VendorNameLength] = 0;

            SafeDelete(_ProductName);
            _ProductName = new char[ProductNameLength + 1];
            ReadBytes(_ProductName, ProductNameLength);
            _ProductName[ProductNameLength] = 0;
        }
    }

    return true;
}

void VSTiPlayer::StopHost() noexcept
{
    if (_IsTerminating)
        return;

    _IsTerminating = true;

    #ifdef _DEBUG
        FB2K_console_print("Stopping host... (hProcess = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess, 8), ", hThread = 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread, 8), ")");
    #endif

    if (_hProcess)
    {
        WriteBytes(0);

        ::WaitForSingleObject(_hProcess, 5000);
        ::TerminateProcess(_hProcess, 0);

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

    _IsInitialized = false;
}

bool VSTiPlayer::IsHostRunning() noexcept
{
    if (_hProcess && ::WaitForSingleObject(_hProcess, 0) == WAIT_TIMEOUT)
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

uint32_t VSTiPlayer::ReadCode() noexcept
{
    uint32_t Code;

    ReadBytes(&Code, sizeof(Code));

    return Code;
}

void VSTiPlayer::ReadBytes(void * data, uint32_t size) noexcept
{
    if (size == 0)
        return;

    if (IsHostRunning())
    {
        uint8_t * Data = (uint8_t *) data;
        uint32_t BytesTotal = 0;

        while (BytesTotal < size)
        {
            const uint32_t BytesRead = ReadBytesOverlapped(Data + BytesTotal, size - BytesTotal);

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

uint32_t VSTiPlayer::ReadBytesOverlapped(void * data, uint32_t size) noexcept
{
    ::ResetEvent(_hReadEvent);

    ::SetLastError(NO_ERROR);

    DWORD BytesRead;
    OVERLAPPED ol = { 0 };

    ol.hEvent = _hReadEvent;

    if (::ReadFile(_hPipeOutRead, data, size, &BytesRead, &ol))
        return BytesRead;

    if (::GetLastError() != ERROR_IO_PENDING)
        return 0;

    const HANDLE handles[1] = { _hReadEvent };

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
    state = ::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE);
#endif

    if (state == WAIT_OBJECT_0 && ::GetOverlappedResult(_hPipeOutRead, &ol, &BytesRead, TRUE))
        return BytesRead;

    ::CancelIoEx(_hPipeOutRead, &ol);

    return 0;
}

void VSTiPlayer::WriteBytes(uint32_t code) noexcept
{
    WriteBytesOverlapped(&code, sizeof(code));
}

void VSTiPlayer::WriteBytesOverlapped(const void * data, uint32_t size) noexcept
{
    if ((size == 0) || !IsHostRunning())
        return;

    DWORD BytesWritten;

    if (!::WriteFile(_hPipeInWrite, data, size, &BytesWritten, nullptr) || (BytesWritten < size))
        StopHost();
}
#pragma endregion
