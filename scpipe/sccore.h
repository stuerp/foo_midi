#ifndef _SCCore_h_
#define _SCCore_h_

// Static single instance - duplicate library to temp path for unique instance

class SCCore
{
public:
    int (*TG_initialize)(int i); // i = 0, returns negative on failure

//  void (* TG_terminate)(); // Unused - terminates process

    void (*TG_activate)(float sampleRate, int blockSize);

//  void (* TG_deactivate)(); // Unused

    void (*TG_setSampleRate)(float sampleRate);

    void (*TG_setMaxBlockSize)(unsigned int blockSize);

    void (*TG_flushMidi)(); // Called after applying presets

    void (*TG_setInterruptThreadIdAtThisTime)();

//  void (*TG_PMidiIn)(MpPacket *, int count); // Unknown

    void (*TG_ShortMidiIn)(unsigned int eventCode, unsigned int deltaFrames);

    void (*TG_LongMidiIn)(const unsigned char * sysEx, unsigned int deltaFrames);

//  void (*TG_isFatalError)(int errCode); // Unused

//  void (*TG_getErrorStrings)(int errCode); // Unused

    unsigned int (*TG_XPgetCurTotalRunningVoices)(); // Unused

//  void (*TG_XPsetSystemConfig)();

//  void (*TG_XPgetCurSystemConfig)();

    void (*TG_Process)(float * left, float * right, unsigned int count);

    SCCore();
    ~SCCore();

    bool Load(const wchar_t * path) noexcept;
    void Unload() noexcept;

private:
    void Dispose() noexcept;

private:
    void * _hModule;
};

#endif
