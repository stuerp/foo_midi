
/** $VER: BMPlayer.h (2023.01.04) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & total_sample_size, uint64_t & sample_loaded_size);

struct sflist_presets;

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
    #pragma region("MIDIPlayer")
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * samples, unsigned long samplesSize) override;

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, size_t port) override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;
    #pragma endregion

    void compound_presets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels);
    void reset_parameters();
    bool load_font_item(std::vector<BASS_MIDI_FONTEX> & presetList, std::string path);

    std::string _ErrorMessage;

    std::vector<HSOUNDFONT> _SoundFonts;
    sflist_presets * _Presets[2];

    std::string _SoundFontDirectoryPath;
    std::string _SoundFontFilePath;

    HSTREAM _Stream[3];

    int _InterpolationLevel;
    int _VoiceCount;

    uint8_t _BankLSBOverride[48];

    bool _AreEffectsEnabled;
    bool _IsBankLSBOverridden;
};
#pragma warning(default: 4820) // x bytes padding added after data member
