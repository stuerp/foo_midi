#include "stdafx.h"

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

// #define LOG_EXCHANGE

enum
{
    BUFFER_SIZE = 4096
};

static HANDLE _hNUL = nullptr;
static HANDLE _hPipeIn = nullptr;
static HANDLE _hPipeOut = nullptr;

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
//{ WCHAR Text[1024]; ::_swprintf(Text, TEXT("size: %ld, MsgSize: %ld"), size, MsgSize); ::MessageBox(::GetDesktopWindow(), TEXT("Code 3"), TEXT("SCPipe"), MB_OK); }
#endif

void WriteBytes(const void * out, uint32_t size)
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

void WriteCode(uint32_t code)
{
    WriteBytes(&code, sizeof(code));
}

void ReadBytes(void * in, uint32_t size)
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

uint32_t ReadCode()
{
    uint32_t code;

    ReadBytes(&code, sizeof(code));

    return code;
}

LONG __stdcall FilterUnhandledExceptions(LPEXCEPTION_POINTERS param)
{
    if (::IsDebuggerPresent())
        return ::UnhandledExceptionFilter(param);

    ::TerminateProcess(::GetCurrentProcess(), 0);

    return 0; // Never reached
}

int WINAPI _tWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    int argc = 0;

    LPWSTR * argv = ::CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv == nullptr || argc != 2)
        return 1;

    unsigned code = 0;

    SCCore * Sampler = nullptr;
    uint32_t SampleRate = 44100;
    std::vector<float> SampleBuffer;

    uint8_t * MsgData = nullptr;
    size_t MsgSize = 0;

    _hNUL = ::CreateFile(_T("NUL"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    _hPipeIn = ::GetStdHandle(STD_INPUT_HANDLE);
    ::SetStdHandle(STD_INPUT_HANDLE, _hNUL);

    _hPipeOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::SetStdHandle(STD_OUTPUT_HANDLE, _hNUL);

    if (FAILED(::CoInitialize(NULL)))
    {
        code = 5;
        goto exit;
    }

#ifndef _DEBUG
    ::SetUnhandledExceptionFilter(FilterUnhandledExceptions);
#endif

    Sampler = new SCCore;

    if (!Sampler->Load(argv[1]))
    {
        code = 1;
        goto exit;
    }

    if (Sampler->TG_initialize(0) < 0)
    {
        code = 2;
        goto exit;
    }

    SampleBuffer.resize((size_t) (BUFFER_SIZE * 4));

    WriteCode(0);

    for (;;)
    {
        uint32_t Command = ReadCode();

        if (!Command)
            break;

        switch (Command)
        {
            case 1: // Set Sample Rate
            {
                uint32_t Size = ReadCode();

                if (Size != sizeof(SampleRate))
                {
                    code = 10;
                    goto exit;
                }

                SampleRate = ReadCode();

                Sampler->TG_activate(44100.0, 1024);
                Sampler->TG_setMaxBlockSize(256);
                Sampler->TG_setSampleRate((float) SampleRate);
                Sampler->TG_setSampleRate((float) SampleRate);
                Sampler->TG_setMaxBlockSize(BUFFER_SIZE);

                WriteCode(0);
                break;
            }

            case 2: // Send MIDI Event
            {
                uint32_t Code = ReadCode();

                Sampler->TG_ShortMidiIn(Code, 0);

                WriteCode(0);
                break;
            }

            case 3: // Send System Exclusive Event
            {
                uint32_t Size = ReadCode();

                if ((size_t)(Size + 1) > MsgSize)
                {
                    MsgSize = (size_t)(Size + 1024) & ~1023;
                    MsgData = (uint8_t *) ::realloc(MsgData, MsgSize);
                }

                if (MsgData == nullptr)
                {
                    code = 3;
                    goto exit;
                }

                ReadBytes(MsgData, Size);

                if (MsgData[Size - 1] != 0xF7)
                    MsgData[Size] = 0xF7;

                Sampler->TG_LongMidiIn(MsgData, 0);

                WriteCode(0);
                break;
            }

            case 4: // Render Samples
            {
                uint32_t count = ReadCode();

                WriteCode(0);

                while (count)
                {
                    unsigned count_to_do = min(count, BUFFER_SIZE);

                    ::memset(&SampleBuffer[0], 0, count_to_do * sizeof(float));
                    ::memset(&SampleBuffer[BUFFER_SIZE], 0, count_to_do * sizeof(float));

                    Sampler->TG_setInterruptThreadIdAtThisTime();
                    Sampler->TG_Process(&SampleBuffer[0], &SampleBuffer[BUFFER_SIZE], count_to_do);

                    float * out = &SampleBuffer[(size_t) (BUFFER_SIZE * 2)];

                    for (unsigned i = 0; i < count_to_do; ++i)
                    {
                        float sample = SampleBuffer[i];

                        out[0] = sample;
                        sample = SampleBuffer[(size_t) BUFFER_SIZE + i];
                        out[1] = sample;

                        out += 2;
                    }

                    WriteBytes(&SampleBuffer[(size_t) (BUFFER_SIZE * 2)], count_to_do * sizeof(float) * 2);

                    count -= count_to_do;
                }
                break;
            }

            case 5: // Junk Samples
            {
                uint32_t count = ReadCode();

                while (count)
                {
                    unsigned count_to_do = min(count, BUFFER_SIZE);

                    Sampler->TG_setInterruptThreadIdAtThisTime();
                    Sampler->TG_Process(&SampleBuffer[0], &SampleBuffer[BUFFER_SIZE], count_to_do);

                    count -= count_to_do;
                }

                WriteCode(0);
                break;
            }

            case 6: // Send event, with timestamp
            {
                uint32_t Code = ReadCode();
                uint32_t Timestamp = ReadCode();

                Sampler->TG_ShortMidiIn(Code, Timestamp);

                WriteCode(0);
                break;
            }

            case 7: // Send System Exclusive, with timestamp
            {
                uint32_t Size = ReadCode();
                uint32_t Timestamp = ReadCode();

                if ((size_t)(Size + 1) > MsgSize)
                {
                    MsgSize = (size_t)(Size + 1024) & ~1023;
                    MsgData = (uint8_t *) ::realloc(MsgData, MsgSize);
                }

                if (MsgData == nullptr)
                {
                    code = 3;
                    goto exit;
                }

                ReadBytes(MsgData, Size);

                if (MsgData[Size - 1] != 0xF7)
                    MsgData[Size] = 0xF7;

                Sampler->TG_LongMidiIn(MsgData, Timestamp);

                WriteCode(0);
                break;
            }

            default:
                code = 4;
                goto exit;
        }
    }

exit:
    delete Sampler;

    ::CoUninitialize();

    if (argv)
        ::LocalFree(argv);

    WriteCode(code);

    if (_hNUL)
    {
        ::CloseHandle(_hNUL);

        ::SetStdHandle(STD_INPUT_HANDLE, _hPipeIn);
        ::SetStdHandle(STD_OUTPUT_HANDLE, _hPipeOut);
    }

    return (int) code;
}
