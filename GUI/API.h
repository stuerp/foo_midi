
/** $VER: API.h (2025.07.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4514 5045 5262 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>

#pragma region IKeyboard

constexpr uint32_t InterfaceVersion = 1;

class NOVTABLE IMusicKeyboard : public service_base
{
public:
    /**
     * \brief Registers the calling producer and negotiates the interface version. Pass `InterfaceVersion`.
     */
    virtual void Initialize(uint32_t interfaceVersion) = 0;
    /**
     * \brief Delivers a MIDI message from the producer to the visualizer.
     */
    virtual void ProcessMessage(uint32_t message, uint32_t timestamp) = 0;
    /**
     * \brief Reports the sample index of the first sample of the producer's current chunk.
     */
    virtual void SetPosition(uint32_t position) = 0;

    FB2K_MAKE_SERVICE_INTERFACE(IMusicKeyboard, service_base);
};

DECLARE_CLASS_GUID(IMusicKeyboard, 0x527da15c, 0x5125, 0x4ba3, 0x99, 0x5d, 0xb8, 0x22, 0x98, 0x08, 0x41, 0x50);

class NOVTABLE IMusicKeyboard_v2 : public IMusicKeyboard
{
public:
    /**
     * \brief Reports the sample index of the first sample of the producer's current chunk and the sample rate (Hz) needed to convert it to time.
     *        Supersedes `SetPosition()`, which does not carry the sample rate.
     */
    virtual void SetTimelinePosition(uint32_t position, uint32_t sampleRate) = 0;

    FB2K_MAKE_SERVICE_INTERFACE(IMusicKeyboard_v2, IMusicKeyboard);
};

DECLARE_CLASS_GUID(IMusicKeyboard_v2, 0x8bdf606d, 0xd3cf, 0x4442, 0xa0, 0x96, 0xeb, 0xb4, 0xe4, 0x59, 0x24, 0x7d);

#pragma endregion

#pragma region IAPI

class NOVTABLE IAPI : public service_base
{
public:
    /**
     * \brief Gets a `MusicKeyboard` object.
     */
    virtual IMusicKeyboard::ptr GetMusicKeyboard() const = 0;

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(IAPI)
};

DECLARE_CLASS_GUID(IAPI, 0x32166e94, 0xdb9f, 0x4a63, 0xa9, 0x37, 0x85, 0x36, 0xb2, 0x06, 0x51, 0x70);

#pragma endregion

PFC_DECLARE_EXCEPTION(foo_vis_midi_exception, pfc::exception, "foo_vis_midi error");
