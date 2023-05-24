#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

typedef void * HANDLE;

#pragma warning(disable: 4820) // x bytes padding added after data member
class VSTiPlayer : public MIDIPlayer
{
public:
    VSTiPlayer() noexcept;
    virtual ~VSTiPlayer();

    bool LoadVST(const char * path);

    void getVendorString(pfc::string8 & out) const;
    void getProductString(pfc::string8 & out) const;
    long getVendorVersion() const noexcept;
    long getUniqueID() const noexcept;

    // Configuration
    void getChunk(std::vector<uint8_t> & out);
    void setChunk(const void * in, unsigned long size);

    // Editor
    bool hasEditor();
    void displayEditorModal();

    // Setup
    virtual unsigned GetChannelCount() const noexcept override { return _ChannelCount; }

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;

    virtual unsigned int GetSampleBlockSize() noexcept override;

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t *, size_t, size_t) override;

    virtual void SendEventWithTime(uint32_t, unsigned int) override;
    virtual void SendSysExWithTime(const uint8_t *, size_t, size_t, unsigned int) override;

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
};
#pragma warning(default: 4820) // x bytes padding added after data member
