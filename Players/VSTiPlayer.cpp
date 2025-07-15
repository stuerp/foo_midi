
/** $VER: VSTiPlayer.cpp (2025.07.13) **/

#include "pch.h"

#include "VSTiPlayer.h"
#include "Resource.h"
#include "Log.h"

#define NOMINMAX

// #define LOG_EXCHANGE

namespace VSTi
{

template <class T> void SafeDelete(T& x) noexcept
{
    if (x)
    {
        delete[] x;
        x = nullptr;
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
    StopHost();

    SafeDelete(_SrcFrames);
}

bool Player::LoadVST(const fs::path & filePath)
{
    if (filePath.empty())
        return false;

    _FilePath = filePath;
    _ProcessorArchitecture = GetProcessorArchitecture(_FilePath);

    if (_ProcessorArchitecture == 0)
        return false;

    return StartHost();
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

    const uint32_t Size = ReadCode();

    chunk.resize(Size);

    if (Size != 0)
        ReadBytes(chunk.data(), Size);
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

    const uint32_t Code = ReadCode();

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

    Code = ReadCode();

    return Code != 0;
}

void Player::DisplayEditorModal()
{
    WriteBytes(4);

    const uint32_t Code = ReadCode();

    if (Code != 0)
        StopHost();
}

#pragma endregion

#pragma region Protected

bool Player::Startup()
{
    if (_IsStarted)
        return true;

    if (_Chunk.size() != 0)
        SetChunk(_Chunk.data(), _Chunk.size());

    WriteBytes(5);
    WriteBytes(sizeof(uint32_t));
    WriteBytes(_SampleRate);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();

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

    const uint32_t Code = ReadCode();

    if (Code != 0)
    {
        StopHost();

        ::memset(dstFrames, 0, (size_t) dstCount * _ChannelCount * sizeof(audio_sample));

        return;
    }

    if (_SrcFrames == nullptr)
        return;

    while (dstCount != 0)
    {
        uint32_t ToDo = std::min(dstCount, MaxFrames);

        ReadBytes(_SrcFrames, (uint32_t) (ToDo * _ChannelCount * sizeof(float)));

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

    const uint32_t Code = ReadCode();

    if (Code != 0)
        StopHost();
}

void Player::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    const uint32_t SizeAndPort = ((uint32_t) size & 0xFFFFFF) | (portNumber << 24);

    WriteBytes(8);
    WriteBytes(SizeAndPort);
    WriteBytesOverlapped(data, (uint32_t) size);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}

void Player::SendEvent(uint32_t data, uint32_t time)
{
    WriteBytes(10);
    WriteBytes(data);
    WriteBytes(time);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}

void Player::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber, uint32_t time)
{
    const uint32_t SizeAndPort = ((uint32_t) size & 0xFFFFFF) | (portNumber << 24);

    WriteBytes(11);
    WriteBytes(SizeAndPort);
    WriteBytes(time);
    WriteBytesOverlapped(data, (uint32_t) size);

    const uint32_t code = ReadCode();

    if (code != 0)
        StopHost();
}
#pragma endregion

#pragma region Private

static bool CreatePipeName(pfc::string_base & pipeName)
{
    GUID guid;

    if (FAILED(::CoCreateGuid(&guid)))
        return false;

    pipeName = "\\\\.\\pipe\\";
    pipeName += pfc::print_guid(guid);

    return true;
}

bool Player::StartHost()
{
    std::string CommandLine;;

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

    {
        STARTUPINFO si =
        {
            .cb =  sizeof(si),

            .dwFlags = STARTF_USESTDHANDLES, // | STARTF_USESHOWWINDOW;
        //  .wShowWindow = SW_HIDE,

            .hStdInput = _hPipeInRead,
            .hStdOutput = _hPipeOutWrite,
            .hStdError = ::GetStdHandle(STD_ERROR_HANDLE),
        };

    #ifdef _DEBUG
        FB2K_console_print(STR_COMPONENT_BASENAME, " is using \"", CommandLine.c_str(), "\" to start host process.");
    #endif

        PROCESS_INFORMATION pi = { };

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
        FB2K_console_print(STR_COMPONENT_BASENAME, " started DLL host hProcess 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess, 8), " / hThread 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread, 8), ".");
    #else
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

    // Get the name, vendor name and product name.
    uint32_t NameLength        = ReadCode();
    uint32_t VendorNameLength  = ReadCode();
    uint32_t ProductNameLength = ReadCode();

    VendorVersion = ReadCode();
    Id            = ReadCode();
    _ChannelCount = ReadCode();

    Name.resize(NameLength);
    ReadBytes(Name.data(), NameLength);

    VendorName.resize(VendorNameLength);
    ReadBytes(VendorName.data(), VendorNameLength);

    ProductName.resize(ProductNameLength);
    ReadBytes(ProductName.data(), ProductNameLength);

    if (Name.empty())
        Name = (const char *) _FilePath.stem().u8string().c_str();

    // VST always uses float samples.
    SafeDelete(_SrcFrames);
    _SrcFrames = new float[MaxFrames * _ChannelCount];

    return true;
}

void Player::StopHost() noexcept
{
    if (_IsTerminating || (_hReadEvent == 0))
        return;

    _IsTerminating = true;

    #ifdef _DEBUG
        FB2K_console_print(STR_COMPONENT_BASENAME, " stopped DLL host hProcess 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hProcess, 8), " / hThread 0x", pfc::format_hex_lowercase((t_uint64)(size_t)_hThread, 8), ".");
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

    _IsStarted = false;
}

bool Player::IsHostRunning() noexcept
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

uint32_t Player::ReadCode() noexcept
{
    uint32_t Code;

    ReadBytes(&Code, sizeof(Code));

    return Code;
}

void Player::ReadBytes(void * data, uint32_t size) noexcept
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

uint32_t Player::ReadBytesOverlapped(void * data, uint32_t size) noexcept
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

    DWORD State;

#ifdef MESSAGE_PUMP
    for (;;)
    {
        State = ::MsgWaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE, QS_ALLEVENTS);

        if (state == WAIT_OBJECT_0 + _countof(handles))
            ProcessPendingMessages();
        else
            break;
    }
#else
    State = ::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE);
#endif

    if (State == WAIT_OBJECT_0 && ::GetOverlappedResult(_hPipeOutRead, &ol, &BytesRead, TRUE))
        return BytesRead;

    ::CancelIoEx(_hPipeOutRead, &ol);

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

#pragma endregion

}
