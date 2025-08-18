#include "stdafx.h"

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

// #define LOG_EXCHANGE

const size_t BlockSize = 4096;

static HANDLE _hNUL = nullptr;
static HANDLE _hPipeIn = nullptr;
static HANDLE _hPipeOut = nullptr;

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
//{ WCHAR Text[1024]; ::_swprintf(Text, TEXT("size: %ld, MsgSize: %ld"), size, MsgSize); ::MessageBox(::GetDesktopWindow(), TEXT("Code 3"), TEXT("SCPipe"), MB_OK); }
#endif

static LONG __stdcall FilterUnhandledExceptions(LPEXCEPTION_POINTERS param);

static uint32_t Read();
static void ReadBytes(void * in, uint32_t size);

static void Write(uint32_t code);
static void WriteBytes(const void * out, uint32_t size);

int WINAPI _tWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    int argc = 0;

    LPWSTR * argv = ::CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv == nullptr || argc != 2)
        return 1;

    uint32_t Result = 0;

    SCCore * Sampler = nullptr;
    uint32_t SampleRate = 44100;
    std::vector<float> SrcFrames;

    uint8_t * MsgData = nullptr;
    size_t MsgSize = 0;

//  std::vector<uint8_t> Config;

    _hNUL = ::CreateFile(_T("NUL"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    _hPipeIn = ::GetStdHandle(STD_INPUT_HANDLE);
    ::SetStdHandle(STD_INPUT_HANDLE, _hNUL);

    _hPipeOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::SetStdHandle(STD_OUTPUT_HANDLE, _hNUL);

    if (FAILED(::CoInitialize(NULL)))
    {
        Result = 5;
        goto exit;
    }

#ifndef _DEBUG
    ::SetUnhandledExceptionFilter(FilterUnhandledExceptions);
#endif

    Sampler = new SCCore;

    if (!Sampler->Load(argv[1]))
    {
        Result = 1;
        goto exit;
    }

    if (Sampler->TG_initialize(0) < 0)
    {
        Result = 2;
        goto exit;
    }
/*
    Config.resize(256); std::fill(Config.begin(), Config.end(), (uint8_t) 0xFF);

    Sampler->TG_XPgetCurSystemConfig(Config.data());

    Config[0] = 3;
    Config[4] = 3;

    Sampler->TG_XPsetSystemConfig(Config.data());
*/
    SrcFrames.resize((size_t) (BlockSize * 4));

    Write(0);

MessageBoxW(HWND_DESKTOP, L"Hello", L"Hello", MB_OK);

    for (;;)
    {
        uint32_t Command = Read();

        if (Command == 0)
            break;

        switch (Command)
        {
            case 1: // Set Sample Rate
            {
                uint32_t Size = Read();

                if (Size != sizeof(SampleRate))
                {
                    Result = 10;
                    goto exit;
                }

                SampleRate = Read();

                Sampler->TG_activate(44100.0, 1024);
                Sampler->TG_setMaxBlockSize(256);
                Sampler->TG_setSampleRate((float) SampleRate);
//              Sampler->TG_setSampleRate((float) SampleRate);
                Sampler->TG_setMaxBlockSize(BlockSize);

                Write(0);
                break;
            }

            case 2: // Send MIDI Event
            {
                uint32_t Code = Read();

                Sampler->TG_ShortMidiIn(Code, 0);

                Write(0);
                break;
            }

            case 3: // Send System Exclusive Event
            {
                uint32_t Size = Read();

                if ((size_t) (Size + 1) > MsgSize)
                {
                    auto NewSize = (size_t) (Size + 1024) & ~1023;
                    auto NewData = (uint8_t *) ::realloc(MsgData, MsgSize);

                    if (NewData == nullptr)
                    {
                        Result = 3;
                        goto exit;
                    }

                    MsgSize = NewSize;
                    MsgData = NewData;
                }

                if (MsgData == nullptr)
                {
                    Result = 3;
                    goto exit;
                }

                ReadBytes(MsgData, Size);

                if (MsgData[Size - 1] != 0xF7)
                    MsgData[Size] = 0xF7;

                Sampler->TG_LongMidiIn(MsgData, 0);

                Write(0);
                break;
            }

            case 4: // Render Samples
            {
                uint32_t DstCount = Read();

                Write(0);

                while (DstCount != 0)
                {
                    uint32_t SrcCount = min(DstCount, BlockSize);

                    ::memset(&SrcFrames[0],          0, SrcCount * sizeof(float));
                    ::memset(&SrcFrames[BlockSize], 0, SrcCount * sizeof(float));

                    Sampler->TG_setInterruptThreadIdAtThisTime();
                    Sampler->TG_Process(&SrcFrames[0], &SrcFrames[BlockSize], SrcCount);

                    float * DstFrames = &SrcFrames[(size_t) (BlockSize * 2)];

                    for (unsigned i = 0; i < SrcCount; ++i)
                    {
                        float Sample = SrcFrames[i];
                        DstFrames[0] = Sample;

                        Sample = SrcFrames[(size_t) BlockSize + i];
                        DstFrames[1] = Sample;

                        DstFrames += 2;
                    }

                    WriteBytes(&SrcFrames[(size_t) (BlockSize * 2)], SrcCount * sizeof(float) * 2);

                    DstCount -= SrcCount;
                }
                break;
            }

            case 5: // Junk Samples
            {
                uint32_t DstCount = Read();

                while (DstCount != 0)
                {
                    uint32_t SrcCount = min(DstCount, BlockSize);

                    Sampler->TG_setInterruptThreadIdAtThisTime();
                    Sampler->TG_Process(&SrcFrames[0], &SrcFrames[BlockSize], SrcCount);

                    DstCount -= SrcCount;
                }

                Write(0);
                break;
            }

            case 6: // Send event, with timestamp
            {
                uint32_t Code = Read();
                uint32_t Timestamp = Read();

                Sampler->TG_ShortMidiIn(Code, Timestamp);

                Write(0);
                break;
            }

            case 7: // Send System Exclusive, with timestamp
            {
                uint32_t Size = Read();
                uint32_t Timestamp = Read();

                if ((size_t)(Size + 1) > MsgSize)
                {
                    auto NewSize = (size_t)(Size + 1024) & ~1023;
                    auto NewData = (uint8_t *) ::realloc(MsgData, MsgSize);

                    if (NewData == nullptr)
                    {
                        Result = 3;
                        goto exit;
                    }

                    MsgSize = NewSize;
                    MsgData = NewData;
                }

                if (MsgData == nullptr)
                {
                    Result = 3;
                    goto exit;
                }

                ReadBytes(MsgData, Size);

                if (MsgData[Size - 1] != 0xF7)
                    MsgData[Size] = 0xF7;

                Sampler->TG_LongMidiIn(MsgData, Timestamp);

                Write(0);
                break;
            }

            default:
                Result = 4;
                goto exit;
        }
    }

exit:
    delete Sampler;

    ::CoUninitialize();

    if (argv)
        ::LocalFree(argv);

    Write(Result);

    if (_hNUL)
    {
        ::CloseHandle(_hNUL);

        ::SetStdHandle(STD_INPUT_HANDLE, _hPipeIn);
        ::SetStdHandle(STD_OUTPUT_HANDLE, _hPipeOut);
    }

    return (int) Result;
}

#ifndef _DEBUG

static LONG __stdcall FilterUnhandledExceptions(LPEXCEPTION_POINTERS param)
{
    if (::IsDebuggerPresent())
        return ::UnhandledExceptionFilter(param);

    ::TerminateProcess(::GetCurrentProcess(), 0);

    return 0; // Never reached
}

#endif

static uint32_t Read()
{
    uint32_t Code;

    ReadBytes(&Code, sizeof(Code));

    return Code;
}

static void ReadBytes(void * in, uint32_t size)
{
    DWORD BytesRead;

    if (!::ReadFile(_hPipeIn, in, size, &BytesRead, NULL) || BytesRead < size)
    {
        ::memset(in, 0, size);
    #ifdef LOG_EXCHANGE
        TCHAR logfile[MAX_PATH];
        _stprintf_s(logfile, _T("bytes_%08u.err"), exchange_count++);
        FILE * f = _tfopen(logfile, _T("wb"));
        _ftprintf(f, _T("Wanted %u bytes, got %u"), size, dwRead);
        fclose(f);
    #endif
    }
    else
    {
    #ifdef LOG_EXCHANGE
        TCHAR logfile[MAX_PATH];
        _stprintf_s(logfile, _T("bytes_%08u.in"), exchange_count++);
        FILE * f = _tfopen(logfile, _T("wb"));
        fwrite(in, 1, size, f);
        fclose(f);
    #endif
    }
}

void Write(uint32_t code)
{
    WriteBytes(&code, sizeof(code));
}

static void WriteBytes(const void * out, uint32_t size)
{
    DWORD BytesWritten;

    ::WriteFile(_hPipeOut, out, size, &BytesWritten, NULL);

#ifdef LOG_EXCHANGE
    TCHAR logfile[MAX_PATH];
    _stprintf_s(logfile, _T("bytes_%08u.out"), exchange_count++);
    FILE * f = _tfopen(logfile, _T("wb"));
    fwrite(out, 1, size, f);
    fclose(f);
#endif
}
