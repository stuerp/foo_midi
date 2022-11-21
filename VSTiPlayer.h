#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

typedef void * HANDLE;

class VSTiPlayer : public MIDIPlayer
{
public:
    // zero variables
    VSTiPlayer() noexcept;

    // close, unload
    virtual ~VSTiPlayer();

    // load, open, verify type
    bool LoadVST(const char * path);

    // must be loaded for the following to work
    void getVendorString(std::string & out) const;
    void getProductString(std::string & out) const;
    long getVendorVersion() const noexcept;
    long getUniqueID() const noexcept;

    // configuration
    void getChunk(std::vector<uint8_t> & out);
    void setChunk(const void * in, unsigned long size);

    // editor
    bool hasEditor();
    void displayEditorModal();

    // setup
    unsigned getChannelCount() noexcept;

protected:
    virtual unsigned int send_event_needs_time() noexcept override;
    virtual void send_event(uint32_t);
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port);
    virtual void render(audio_sample *, unsigned long);

    virtual void shutdown();
    virtual bool startup();

    virtual void send_event_time(uint32_t b, unsigned int time);
    virtual void send_sysex_time(const uint8_t * event, size_t size, size_t port, unsigned int time);

private:
    unsigned test_plugin_platform();

    bool process_create();
    void process_terminate() noexcept;
    bool process_running() noexcept;

    uint32_t process_read_code() noexcept;
    void process_read_bytes(void * buffer, uint32_t size) noexcept;
    uint32_t process_read_bytes_pass(void * buffer, uint32_t size) noexcept;

    void process_write_code(uint32_t code) noexcept;
    void process_write_bytes(const void * buffer, uint32_t size) noexcept;

    std::string _PluginPathName;

    HANDLE hProcess;
    HANDLE hThread;
    HANDLE hReadEvent;
    HANDLE hChildStd_IN_Rd;
    HANDLE hChildStd_IN_Wr;
    HANDLE hChildStd_OUT_Rd;
    HANDLE hChildStd_OUT_Wr;

    char * _Name;
    char * _Vendor;
    char * _Product;

    uint32_t _VendorVersion;
    uint32_t _UniqueId;

    unsigned _PluginPlatform;
    unsigned _ChannelCount;

    std::vector<uint8_t> _Chunk;

    float * _VSTBuffer;

    bool bInitialized;
    bool bTerminating;
    char _Padding[6];
};

#endif
