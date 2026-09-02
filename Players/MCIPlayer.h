
/** $VER: MCIPlayer.h (2026.09.02) P. Stuer - Implements a player that sends to a Windows MIDI output port **/

#pragma once

#include "Player.h"

#include <atomic>
#include <memory>
#include <thread>

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Plays a MIDI stream on an external Windows MIDI output port (a hardware module, a virtual port or a
/// user-mode synthesizer driver such as VST MIDI Synth). The player renders silence because the audio
/// is produced by the device instead of by foobar2000.
/// </summary>
/// <remarks>
/// Events are sent with midiOutShortMsg() and the decoder is paced to real time, rather than handing a
/// scheduled stream to the driver with midiStreamOut(). Streaming would let the driver do the timing,
/// but not every driver implements it.
///
/// All instances share one connection per device. foobar2000 decodes the next track before the current
/// one has finished, so two players exist at every track boundary. A hardware port refuses the second
/// open outright, and version 2.6.0 of the VST MIDI Synth driver corrupts its heap when one thread opens
/// the port while another is sending to it. The connection is opened once, only one player sends to it
/// at a time, and it is closed a few seconds after the last player has let go of it.
/// </remarks>
class MCIPlayer : public player_t
{
public:
    MCIPlayer() noexcept;

    virtual ~MCIPlayer();

    /// <summary>
    /// Sets the identifier of the MIDI output device to send to, as used by midiOutGetDevCaps().
    /// </summary>
    void SetDeviceId(uint32_t deviceId) noexcept { _DeviceId = (UINT) deviceId; }

    /// <summary>
    /// Gets the number of MIDI output devices.
    /// </summary>
    static uint32_t GetDeviceCount() noexcept;

    /// <summary>
    /// Gets the name of a MIDI output device. Returns an empty string when the device does not exist.
    /// </summary>
    static std::string GetDeviceName(uint32_t deviceId) noexcept;

    /// <summary>
    /// Closes every device connection. Called when foobar2000 quits.
    /// </summary>
    static void CloseAllConnections() noexcept;

    virtual void SetAbortHandler(foobar2000_io::abort_callback * abortHandler) noexcept override { _AbortHandler = abortHandler; }

protected:
    #pragma region player_t

    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual bool Reset() override;

    virtual uint8_t GetPortCount() const noexcept override { return 1; };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

    virtual void SendEvent(uint32_t, uint32_t) override { };
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t) override { };

    #pragma endregion

private:
    class Connection;

    uint32_t GetPlayingTime() const noexcept;
    void WaitForPlayingTime() noexcept;

    void StartWatchdog();
    void StopWatchdog() noexcept;

private:
    static const uint32_t MaxDriftInMS = 500;           // Resynchronises instead of racing to catch up after a pause.
    static const uint32_t MaxWaitInMS = 10'000;         // Never blocks the decoder thread for longer than this while pacing.
    static const uint32_t OwnershipTimeoutInMS = 180'000; // How long a player waits for the previous track to let go of the device.

    static const uint32_t WatchdogTimeoutInMS = 750;    // Silences the device when the decoder stops feeding us (paused, stalled).

    UINT _DeviceId;
    std::shared_ptr<Connection> _Connection;
    foobar2000_io::abort_callback * _AbortHandler;

    uint64_t _FrameIndex;                               // Number of frames rendered since playback started.
    uint32_t _StartTime;                                // Result of ::timeGetTime() when playback started.

    std::thread _Watchdog;
    std::atomic<bool> _IsWatchdogRunning;
    std::atomic<uint32_t> _LastRenderTime;              // Result of ::timeGetTime() during the most recent Render() call.
    std::atomic<bool> _IsSilenced;
};

#pragma warning(default: 4820) // x bytes padding added after data member
