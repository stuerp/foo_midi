
/** $VER: MCIPlayer.h (2026.09.02) P. Stuer - Implements a player that streams to a Windows MIDI output port **/

#pragma once

#include "Player.h"

#include <atomic>
#include <thread>
#include <vector>

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Plays a MIDI stream on an external Windows MIDI output port (a hardware module, a virtual port or a
/// user-mode synthesizer driver such as VST MIDI Synth). The player renders silence because the audio
/// is produced by the device instead of by foobar2000.
/// </summary>
class MCIPlayer : public player_t
{
public:
    MCIPlayer() noexcept;

    virtual ~MCIPlayer();

    /// <summary>
    /// Sets the identifier of the MIDI output device to stream to, as used by midiOutGetDevCaps().
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
    struct buffer_t
    {
        std::vector<uint8_t> Data;
        MIDIHDR Header;
        bool IsPrepared;

        buffer_t() noexcept : Header(), IsPrepared(false) { }
    };

    void AppendShortMessage(uint32_t deltaTime, uint32_t message);
    void AppendLongMessage(uint32_t deltaTime, const uint8_t * data, size_t size);

    uint32_t GetCurrentTick() const noexcept;
    uint32_t GetDeltaTime() noexcept;

    bool Flush() noexcept;
    buffer_t * GetFreeBuffer() noexcept;
    void ReleaseBuffers(bool force) noexcept;

    uint32_t GetStreamPositionInMS() const noexcept;
    void ThrottleToDevice() noexcept;

    void SilenceDevice() noexcept;

    void StartWatchdog();
    void StopWatchdog() noexcept;

    void LogMessage(MMRESULT result, const char * functionName) const noexcept;

private:
    // 1 tick equals 1 ms: MIDIPROP_TEMPO (us per quarter note) divided by MIDIPROP_TIMEDIV (ticks per quarter note).
    static const uint32_t TicksPerQuarterNote = 1'000;
    static const uint32_t MicroSecondsPerQuarterNote = 1'000'000;

    static const size_t BufferCount = 8;                // Number of stream buffers that can be in flight at the same time.
    static const size_t MaxBufferSize = 16 * 1'024;     // in bytes

    static const uint32_t FlushIntervalInMS = 50;       // Queue at least this much playing time before sending a buffer to the device.
    static const uint32_t TargetLeadInMS = 250;         // How far the queued stream may run ahead of what the device is playing.
    static const uint32_t WatchdogTimeoutInMS = 750;    // Silences the device when the decoder stops feeding us (paused, stalled).

    UINT _DeviceId;
    HMIDISTRM _hStream;

    std::vector<uint8_t> _Pending;                      // Events that have not been sent to the device yet.
    uint32_t _PendingTick;                              // Tick of the first event in _Pending.
    buffer_t _Buffers[BufferCount];

    uint64_t _FrameIndex;                               // Number of frames rendered since startup.
    uint32_t _LastEventTick;                            // Tick of the most recently queued event.

    std::thread _Watchdog;
    std::atomic<bool> _IsWatchdogRunning;
    std::atomic<uint32_t> _LastRenderTime;              // Result of ::timeGetTime() during the most recent Render() call.
    std::atomic<bool> _IsSilenced;
};

#pragma warning(default: 4820) // x bytes padding added after data member
