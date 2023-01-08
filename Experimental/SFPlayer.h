#ifndef __SFPlayer_h__
#define __SFPlayer_h__

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
    virtual bool startup() override;
    virtual void shutdown() override;
    virtual void render(audio_sample *, unsigned long) override;
    virtual bool reset() override;

    virtual bool get_last_error(std::string & p_out) override;

    virtual void send_event(uint32_t b) override;
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port) override;

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

#endif
