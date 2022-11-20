#include "stdafx.h"

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

// #define LOG_EXCHANGE

enum
{
    BUFFER_SIZE = 4096
};

static HANDLE hNUL = NULL;
static HANDLE hPipeIn = NULL;
static HANDLE hPipeOut = NULL;

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

void put_bytes(const void * out, uint32_t size)
{
    DWORD BytesWritten;

    ::WriteFile(hPipeOut, out, size, &BytesWritten, NULL);

#ifdef LOG_EXCHANGE
    TCHAR logfile[MAX_PATH];
    _stprintf_s(logfile, _T("bytes_%08u.out"), exchange_count++);
    FILE * f = _tfopen(logfile, _T("wb"));
    fwrite(out, 1, size, f);
    fclose(f);
#endif
}

void put_code(uint32_t code)
{
    put_bytes(&code, sizeof(code));
}

void get_bytes(void * in, uint32_t size)
{
    DWORD BytesRead;

    if (!::ReadFile(hPipeIn, in, size, &BytesRead, NULL) || BytesRead < size)
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

uint32_t get_code()
{
    uint32_t code;

    get_bytes(&code, sizeof(code));

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

#ifdef _DEBUG
    MessageBox(GetDesktopWindow(), _T("Boop"), _T("meh"), 0);
#endif

    unsigned code = 0;

    SCCore * Sampler = nullptr;

    uint32_t SampleRate = 44100;

    std::vector<float> SampleBuffer;

    uint8_t * msgbuf = NULL;
    size_t msgsize = 0;

    hNUL = ::CreateFile(_T("NUL"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    hPipeIn = ::GetStdHandle(STD_INPUT_HANDLE);

    ::SetStdHandle(STD_INPUT_HANDLE, hNUL);

    hPipeOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    ::SetStdHandle(STD_OUTPUT_HANDLE, hNUL);

    if (FAILED(::CoInitialize(NULL)))
        return 5;

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

    put_code(0);

    for (;;)
    {
        uint32_t Command = get_code();

        if (!Command)
            break;

        switch (Command)
        {
            case 1: // Set Sample Rate
            {
                uint32_t size = get_code();

                if (size != sizeof(SampleRate))
                {
                    code = 10;
                    goto exit;
                }

                SampleRate = get_code();

                Sampler->TG_activate(44100.0, 1024);
                Sampler->TG_setMaxBlockSize(256);
                Sampler->TG_setSampleRate((float) SampleRate);
                Sampler->TG_setSampleRate((float) SampleRate);
                Sampler->TG_setMaxBlockSize(BUFFER_SIZE);

                put_code(0);
            }
            break;

            case 2: // Send MIDI Event
            {
                uint32_t b = get_code();

                Sampler->TG_ShortMidiIn(b, 0);

                put_code(0);
            }
            break;

            case 3: // Send System Exclusive Event
            {
                uint32_t size = get_code();

                if ((size_t) size + 1 > msgsize)
                {
                    msgsize = (size_t) size + 1024 & ~1023;

                    uint8_t * msgbufnew = (uint8_t *) ::realloc(msgbuf, msgsize);

                    if (msgbufnew == NULL)
                        goto exit;
                }

                if (!msgbuf)
                {
                    code = 3;
                    goto exit;
                }

                get_bytes(msgbuf, size);

                if (msgbuf[size - 1] != 0xF7)
                    msgbuf[size] = 0xF7;

                Sampler->TG_LongMidiIn(msgbuf, 0);

                put_code(0);
            }
            break;

            case 4: // Render Samples
            {
                uint32_t count = get_code();

                put_code(0);

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

                    put_bytes(&SampleBuffer[(size_t) (BUFFER_SIZE * 2)], count_to_do * sizeof(float) * 2);

                    count -= count_to_do;
                }
                break;
            }

            case 5: // Junk Samples
            {
                uint32_t count = get_code();

                while (count)
                {
                    unsigned count_to_do = min(count, BUFFER_SIZE);

                    Sampler->TG_setInterruptThreadIdAtThisTime();
                    Sampler->TG_Process(&SampleBuffer[0], &SampleBuffer[BUFFER_SIZE], count_to_do);

                    count -= count_to_do;
                }

                put_code(0);
                break;
            }

            case 6: // Send event, with timestamp
            {
                uint32_t b = get_code();
                uint32_t timestamp = get_code();

                Sampler->TG_ShortMidiIn(b, timestamp);

                put_code(0);
                break;
            }

            case 7: // Send System Exclusive, with timestamp
            {
                uint32_t size = get_code();
                uint32_t timestamp = get_code();

                if ((size_t) size + 1 > msgsize)
                {
                    msgsize = (size_t) size + 1024 & ~1023;

                    uint8_t * msgbufnew = (uint8_t *) ::realloc(msgbuf, msgsize);

                    if (msgbufnew == NULL)
                        goto exit;
                }

                if (!msgbuf)
                {
                    code = 3;
                    goto exit;
                }

                get_bytes(msgbuf, size);

                if (msgbuf[size - 1] != 0xF7)
                    msgbuf[size] = 0xF7;

                Sampler->TG_LongMidiIn(msgbuf, timestamp);

                put_code(0);
            }
            break;

            default:
                code = 4;
                goto exit;
                break;
        }
    }

exit:
    delete Sampler;

    ::CoUninitialize();

    if (argv)
        ::LocalFree(argv);

    put_code(code);

    if (hNUL)
    {
        ::CloseHandle(hNUL);

        ::SetStdHandle(STD_INPUT_HANDLE, hPipeIn);
        ::SetStdHandle(STD_OUTPUT_HANDLE, hPipeOut);
    }

    return (int) code;
}
