
/** $VER: SCPlayer.h (2025.07.03) Secret Sauce **/

#pragma once

#include "Player.h"

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

class SCPlayer : public player_t
{
public:
    SCPlayer() noexcept;
    virtual ~SCPlayer();

    void SetRootPath(const fs::path & directoryPath);

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual bool Reset() override;

    virtual uint32_t GetSampleBlockSize() const noexcept override { return 0; } // 4096; This doesn't work for some reason.

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

    virtual void SendEvent(uint32_t data, uint32_t time) override;
    virtual void SendSysEx(const uint8_t * data, size_t size , uint32_t portNumber, uint32_t time) override;

private:
    bool StartHosts(const fs::path & filePath);
    void RenderPort(uint32_t port, float * data, uint32_t size) noexcept;

    bool StartHost(uint32_t port);
    void StopHost(uint32_t port) noexcept;
    bool IsHostRunning(uint32_t port) noexcept;

    static bool CreatePipeName(pfc::string_base & pipeName);

    uint32_t ReadCode(uint32_t port) noexcept;

    void ReadBytes(uint32_t port, void * data, uint32_t size) noexcept;
    uint32_t ReadBytesOverlapped(uint32_t port, void * data, uint32_t size) noexcept;

    void WriteBytes(uint32_t port, uint32_t code) noexcept;
    void WriteBytesOverlapped(uint32_t port, const void * data, uint32_t size) noexcept;

private:
    uint32_t _ProcessorArchitecture;
    int _COMInitialisationCount;

    fs::path _RootPath;
    fs::path _FilePath;

    static const size_t MaxPorts = 3;

    HANDLE _hReadEvent[MaxPorts];
    HANDLE _hPipeInRead[MaxPorts];
    HANDLE _hPipeInWrite[MaxPorts];
    HANDLE _hPipeOutRead[MaxPorts];
    HANDLE _hPipeOutWrite[MaxPorts];
    HANDLE _hProcess[MaxPorts];
    HANDLE _hThread[MaxPorts];

    bool _IsPortTerminating[MaxPorts];

    float * _Samples;
};

#pragma warning(default: 4820) // x bytes padding added after data member
