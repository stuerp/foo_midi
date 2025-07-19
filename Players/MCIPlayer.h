
/** $VER: MCIPlayer.h (2025.07.09) P. Stuer - Implements a player using the Windows MCI API **/

#pragma once

#include "Player.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class MCIPlayer : public player_t
{
public:
    MCIPlayer() noexcept;
    virtual ~MCIPlayer();

protected:
    #pragma region MIDIPlayer

    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;

    virtual uint8_t GetPortCount() const noexcept override { return 1; };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

    virtual void SendEvent(uint32_t, uint32_t) { };
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t) { };

    #pragma endregion

private:
    void LogMessage(MMRESULT result) const;

private:
    UINT _DeviceId;
    HMIDISTRM _hStream;
    MIDIHDR _Header;

    std::vector<MIDIEVENT> _Events;
};
#pragma warning(default: 4820) // x bytes padding added after data member
