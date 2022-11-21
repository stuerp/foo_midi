#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <foobar2000.h>

extern char _DLLFileName[];

class SCPlayer : public MIDIPlayer
{
public:
    SCPlayer() noexcept;

    virtual ~SCPlayer();

    void set_sccore_path(const char * path);

protected:
    virtual unsigned int send_event_needs_time();
    virtual void send_event(uint32_t b);
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port);
    virtual void render(audio_sample * out, unsigned long count);

    virtual void shutdown();
    virtual bool startup();

    virtual void send_event_time(uint32_t b, unsigned int time);
    virtual void send_sysex_time(const uint8_t * event, size_t size, size_t port, unsigned int time);

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

#endif
