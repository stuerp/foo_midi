
/** $VER: MCIPlayer.h (2026.09.02) P. Stuer - Implements a player that sends to a Windows MIDI output port **/

#pragma once

#include "Player.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Plays a MIDI stream on an external Windows MIDI output port (a hardware module, a virtual port or a
/// user-mode synthesizer driver such as VST MIDI Synth). The player renders silence because the audio
/// is produced by the device instead of by foobar2000.
/// </summary>
/// <remarks>
/// Events are sent with midiOutShortMsg() and the decoder is paced to real time, rather than handing a
/// scheduled stream to the driver with midiStreamOut(). Streaming would let the driver do the timing,
/// but not every driver implements it: version 2.6.0 of the VST MIDI Synth driver crashes the host.
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
    uint32_t GetPlayingTime() const noexcept;
    void Send(uint32_t message) noexcept;
    void WaitForPlayingTime() noexcept;

    void SilenceDevice() noexcept;

    void StartWatchdog();
    void StopWatchdog() noexcept;

    void LogMessage(MMRESULT result, const char * functionName) const noexcept;

private:
    static const uint32_t OpenAttempts = 12;            // Retries while a driver is still releasing a previous client.
    static const uint32_t OpenRetryIntervalInMS = 250;

    static const uint32_t MaxDriftInMS = 500;           // Resynchronises instead of racing to catch up after a pause.
    static const uint32_t MaxWaitInMS = 10'000;         // Never blocks the decoder thread for longer than this.

    static const uint32_t WatchdogTimeoutInMS = 750;    // Silences the device when the decoder stops feeding us (paused, stalled).

    UINT _DeviceId;
    HMIDIOUT _hDevice;

    // The watchdog thread sends to the device too. Not every driver tolerates being called from two
    // threads at once: version 2.6.0 of the VST MIDI Synth driver faults when that happens.
    mutable std::mutex _DeviceMutex;

    bool _IsTimerResolutionSet;

    uint64_t _FrameIndex;                               // Number of frames rendered since playback started.
    uint32_t _StartTime;                                // Result of ::timeGetTime() when playback started.

    std::vector<uint8_t> _SysEx;                        // Holds a system exclusive message while the device sends it.

    std::thread _Watchdog;
    std::atomic<bool> _IsWatchdogRunning;
    std::atomic<uint32_t> _LastRenderTime;              // Result of ::timeGetTime() during the most recent Render() call.
    std::atomic<bool> _IsSilenced;
};

#pragma warning(default: 4820) // x bytes padding added after data member
