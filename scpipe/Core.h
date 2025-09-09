#pragma once

class core_t
{
public:
    void (*TG_Process)(float * left, float * right, unsigned int count);

    void (*TG_LongMidiIn)(const unsigned char * sysEx, unsigned int deltaFrames);

    void (*TG_PMidiIn)(int msg, int count);

    void (*TG_ShortMidiIn)(unsigned int eventCode, unsigned int deltaFrames);

    void (*TG_XPgetCurSystemConfig)(void * config);

    int (*TG_XPgetCurTotalRunningVoices)();

    void (*TG_XPsetSystemConfig)(void * config);

    void (*TG_activate)(float sampleRate, int blockSize);

    void (* TG_deactivate)();

    void (*TG_flushMidi)(); // Called after applying presets

    char * (*TG_getErrorStrings)(int errorCode);

    int (*TG_initialize)(int mode); // i = 0, returns negative on failure

    void (* TG_terminate)();

    void (*TG_isFatalError)(int errorCode);

    int (*TG_setMaxBlockSize)(unsigned int blockSize);

    int (*TG_setSampleRate)(float sampleRate);

    void (*TG_setInterruptThreadIdAtThisTime)();

    core_t();
    ~core_t();

    bool Load(const wchar_t * path) noexcept;
    void Unload() noexcept;

private:
    void Dispose() noexcept;

private:
    void * _hModule;
};
