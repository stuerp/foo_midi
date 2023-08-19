
/** $VER: FSPlayer.h (2023.08.19) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include "FluidSynth.h"

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member
/// <summary>
/// Implements a MIDI player using FluidSynth.
/// </summary>
class FSPlayer : public MIDIPlayer
{
public:
    FSPlayer();
    virtual ~FSPlayer();

    bool Initialize(const WCHAR * basePath);

    void SetSoundFontDirectory(const char * directoryPath);
    void SetSoundFontFile(const char * filePath);

    void EnableDynamicLoading(bool enabled = true);
    void EnableEffects(bool enabled = true);
    void SetVoiceCount(uint32_t voices);

    void SetInterpolationMode(uint32_t mode);

    uint32_t GetActiveVoiceCount() const noexcept;

private:
    #pragma region("MIDIPlayer")
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;
    virtual bool Reset() override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;
    #pragma endregion

    fluid_sfloader_t * GetSoundFontLoader(fluid_settings_t * settings);

private:
    std::string _ErrorMessage;

    fluid_settings_t * _Settings[3];
    fluid_synth_t * _Synth[3];

    pfc::string8 _SoundFontDirectoryPath;
    std::string _SoundFontFilePath;

    bool _DoDynamicLoading;
    bool _DoReverbAndChorusProcessing;
    uint32_t _VoiceCount;

    uint32_t _InterpolationMode;

    FluidSynth _FluidSynth;
};
#pragma warning(default: 4820) // x bytes padding added after data member
