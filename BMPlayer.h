#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#pragma warning(disable: 4625 4626 5045)

#include "MIDIPlayer.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & total_sample_size, uint64_t & sample_loaded_size); // Called by foo_midi

typedef struct sflist_presets sflist_presets;

#pragma warning(disable: 4820) // x bytes padding added after data member
class BMPlayer : public MIDIPlayer
{
public:
    BMPlayer();
    virtual ~BMPlayer();

    void setSoundFont(const char * in);
    void setFileSoundFont(const char * in);
    void setInterpolation(int level);
    void setEffects(bool enable = true);
    void setVoices(int voices);

    unsigned int getVoicesActive();

private:
    virtual bool startup() override;
    virtual void shutdown() override;
    virtual void render(audio_sample * out, unsigned long count) override;

    virtual void send_event(uint32_t b) override;
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port) override;

    virtual bool get_last_error(std::string & p_out) override;

    void compound_presets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels);
    void reset_parameters();
    bool load_font_item(std::vector<BASS_MIDI_FONTEX> & presetList, std::string path);

    std::string sLastError;

    std::vector<HSOUNDFONT> _soundFonts;
    sflist_presets * _presetList[2];

    std::string _SoundFontDirectoryPath;
    std::string _SoundFontFilePath;

    HSTREAM _stream[3];

    int _InterpolationLevel;
    int _VoiceCount;

    uint8_t bank_lsb_override[48];

    bool _AreEffectsEnabled;
    bool bank_lsb_overridden;
};
#pragma warning(default: 4820) // x bytes padding added after data member
