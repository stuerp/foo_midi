
/** $VER: MCIPlayer.h (2023.12.23) Implements a player using the Windows MCI API **/

#pragma once

#include "MIDIPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class MCIPlayer : public MIDIPlayer
{
public:
    MCIPlayer() noexcept;
    virtual ~MCIPlayer();

protected:
    #pragma region MIDIPlayer
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

    virtual void SendEvent(uint32_t, uint32_t) { };
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t) { };
    #pragma endregion

private:
    void LogMessage(MMRESULT result) const;

private:
    HMIDIOUT _hDevice;
    MIDIHDR _Header;
};
#pragma warning(default: 4820) // x bytes padding added after data member
