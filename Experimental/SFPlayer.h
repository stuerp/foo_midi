#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <fluidsynth.h>

class SFPlayer : public MIDIPlayer
{
public:
    // zero variables
    SFPlayer();

    // close, unload
    virtual ~SFPlayer();

    // configuration
    void setSoundFont(const char * in);
    void setFileSoundFont(const char * in);
    void setInterpolationMethod(unsigned method);
    void setDynamicLoading(bool enabled);
    void setEffects(bool enabled);
    void setVoiceCount(unsigned int voices);

private:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;
    virtual bool Reset() override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, size_t port) override;

    std::string _last_error;

    fluid_settings_t * _settings[3];
    fluid_synth_t * _synth[3];
    std::string sSoundFontName;
    std::string _SoundFontFilePath;

    unsigned uInterpolationMethod;
    bool bDynamicLoading;
    bool bEffects;
    unsigned int uVoices;
};
