
/** $VER: MT32Player.h (2025.07.12) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <Player.h>

#include <mt32emu.h>

#include <set>

#include "MT32ReportHandler.h"

namespace foobar2000_io
{
    class abort_callback;
};

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

class MT32Player : public player_t
{
public:
    MT32Player(bool isMT32, uint32_t gmSet);
    virtual ~MT32Player();

    virtual void SetAbortHandler(foobar2000_io::abort_callback * abortHandler) noexcept override
    {
        _AbortCallback = abortHandler;
    }

    void SetROMDirectory(const fs::path & directoryPath) noexcept;

//  bool IsConfigValid() noexcept;

    uint32_t GetSampleRate() noexcept;

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual bool Reset() override;

    virtual uint8_t GetPortCount() const noexcept override { return 1; };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

private:
    bool LoadROMs(const std::string & machineID) noexcept;
    bool LoadROMs(const std::vector<std::string> & machineIDs) noexcept;
    bool LoadMachineROMs(const std::string & machineID);

    std::vector<std::string> GetMatchingMachineIDs(const std::string & machineID);
    std::set<std::string> IdentifyControlROMs();

private:
    fs::path _ROMDirectory;

    ReportHandler _ReportHandler;
    MT32Emu::Service _Service;

    bool _IsMT32;
    uint32_t _GMSet;

    foobar2000_io::abort_callback * _AbortCallback;
};

#pragma warning(default: 4820) // x bytes padding added after data member
