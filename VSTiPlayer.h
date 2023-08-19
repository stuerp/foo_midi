
/** $VER: VSTiPlayer.h (2023.06.12) **/

#pragma once

#include "MIDIPlayer.h"

typedef void * HANDLE;

#pragma warning(disable: 4820) // x bytes padding added after data member
class VSTiPlayer : public MIDIPlayer
{
public:
    VSTiPlayer() noexcept;
    virtual ~VSTiPlayer();

    bool LoadVST(const char * path);

    void GetVendorName(pfc::string8 & out) const;
    void GetProductName(pfc::string8 & out) const;
    uint32_t GetVendorVersion() const noexcept;
    uint32_t GetUniqueID() const noexcept;

    // Configuration
    void GetChunk(std::vector<uint8_t> & data);
    void SetChunk(const void * data, size_t size);

    // Editor
    bool HasEditor();
    void DisplayEditorModal();

    // Setup
    virtual uint32_t GetChannelCount() const noexcept override { return _ChannelCount; }

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;

    virtual uint32_t GetSampleBlockSize() const noexcept override { return 4096; }

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

    virtual void SendEvent(uint32_t data, uint32_t time) override;
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t portNumber, uint32_t time) override;

private:
    bool StartHost();
    void StopHost() noexcept;
    bool IsHostRunning() noexcept;

    uint32_t ReadCode() noexcept;

    void ReadBytes(void * data, uint32_t size) noexcept;
    uint32_t ReadBytesOverlapped(void * data, uint32_t size) noexcept;

    void WriteBytes(uint32_t code) noexcept;
    void WriteBytesOverlapped(const void * data, uint32_t size) noexcept;

private:
    uint32_t _PluginArchitecture;
    bool _IsCOMInitialized;

    std::string _PluginFilePath;

    HANDLE _hReadEvent;
    HANDLE _hPipeInRead;
    HANDLE _hPipeInWrite;
    HANDLE _hPipeOutRead;
    HANDLE _hPipeOutWrite;
    HANDLE _hProcess;
    HANDLE _hThread;

    char * _Name;
    char * _VendorName;
    char * _ProductName;

    uint32_t _VendorVersion;
    uint32_t _UniqueId;

    uint32_t _ChannelCount;

    std::vector<uint8_t> _Chunk;
    float * _Samples;

    bool _IsTerminating;
};
#pragma warning(default: 4820) // x bytes padding added after data member
