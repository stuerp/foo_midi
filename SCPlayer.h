
/** $VER: SCPlayer.h (2023.08.19) Secret Sauce **/

#pragma once

#include "MIDIPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class SCPlayer : public MIDIPlayer
{
public:
    SCPlayer() noexcept;
    virtual ~SCPlayer();

    void SetRootPath(const char * path);

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;

    virtual uint32_t GetSampleBlockSize() const noexcept override { return 0; } // 4096; This doesn't work for some reason.

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

    virtual void SendEvent(uint32_t data, uint32_t time) override;
    virtual void SendSysEx(const uint8_t * data, size_t size , uint32_t portNumber, uint32_t time) override;

private:
    bool LoadCore(const char * filePath);
    void RenderPort(uint32_t port, float * data, uint32_t size) noexcept;

    bool StartHost(uint32_t port);
    void StopHost(uint32_t port) noexcept;
    bool IsHostRunning(uint32_t port) noexcept;

    uint32_t ReadCode(uint32_t port) noexcept;

    void ReadBytes(uint32_t port, void * data, uint32_t size) noexcept;
    uint32_t ReadBytesOverlapped(uint32_t port, void * data, uint32_t size) noexcept;

    void WriteBytes(uint32_t port, uint32_t code) noexcept;
    void WriteBytesOverlapped(uint32_t port, const void * data, uint32_t size) noexcept;

private:
    uint32_t _PluginArchitecture;
    int _COMInitialisationCount;

    std::string _PluginFilePath;

    HANDLE _hReadEvent[3];
    HANDLE _hPipeInRead[3];
    HANDLE _hPipeInWrite[3];
    HANDLE _hPipeOutRead[3];
    HANDLE _hPipeOutWrite[3];
    HANDLE _hProcess[3];
    HANDLE _hThread[3];

    char * _RootPathName;
    float * _Samples;

    bool _IsPortTerminating[3];
};
#pragma warning(default: 4820) // x bytes padding added after data member
