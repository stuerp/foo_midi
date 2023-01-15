
/** $VER: Secret Sauce (2023.01.04) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class SCPlayer : public MIDIPlayer
{
public:
    SCPlayer() noexcept;
    virtual ~SCPlayer();

    void set_sccore_path(const char * path);

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;

    virtual unsigned int GetSampleBlockSize() override;

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t *, size_t, size_t) override;

    virtual void SendEventWithTime(uint32_t, unsigned int) override;
    virtual void SendSysExWithTime(const uint8_t *, size_t, size_t, unsigned int) override;

private:
    bool LoadCore(const char * path);

    void send_command(uint32_t port, uint32_t command);

    void render_port(uint32_t port, float * out, uint32_t count);

    void junk(uint32_t port, unsigned long count);

    unsigned test_plugin_platform();

    bool process_create(uint32_t port);
    void process_terminate(uint32_t port);
    bool process_running(uint32_t port);
    uint32_t process_read_code(uint32_t port);
    void process_read_bytes(uint32_t port, void * buffer, uint32_t size);
    uint32_t process_read_bytes_pass(uint32_t port, void * buffer, uint32_t size);
    void process_write_code(uint32_t port, uint32_t code);
    void process_write_bytes(uint32_t port, const void * buffer, uint32_t size);

private:
    unsigned _PluginArchitecture;
    int _COMInitialisationCount;

    std::string PluginFilePath;

    HANDLE hProcess[3];
    HANDLE hThread[3];
    HANDLE hReadEvent[3];
    HANDLE hChildStd_IN_Rd[3];
    HANDLE hChildStd_IN_Wr[3];
    HANDLE hChildStd_OUT_Rd[3];
    HANDLE hChildStd_OUT_Wr[3];

    char * _SCCorePathName;
    float * _Buffer;

    bool _IsPortTerminating[3];
};
#pragma warning(default: 4820) // x bytes padding added after data member
