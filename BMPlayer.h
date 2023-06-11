
/** $VER: BMPlayer.h (2023.06.11) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded);

struct sflist_presets;

#pragma warning(disable: 4820) // x bytes padding added after data member
class BMPlayer : public MIDIPlayer
{
public:
    BMPlayer();
    virtual ~BMPlayer();

    void SetSoundFontDirectory(const char * directoryPath);
    void SetSoundFontFile(const char * filePath);
    void SetInterpolationMode(int interpolationMode);
    void EnableEffects(bool enable = true);
    void SetVoiceCount(int voices);

    size_t GetActiveVoiceCount() const;

private:
    #pragma region("MIDIPlayer")
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * samples, unsigned long samplesSize) override;

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, size_t port) override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;
    #pragma endregion

    void ResetParameters();
    bool LoadSoundFontConfiguration(std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations, std::string pathName);

    void CompoundPresets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels);

private:
    std::string _ErrorMessage;

    std::vector<HSOUNDFONT> _SoundFonts;
    sflist_presets * _Presets[2];

    std::string _SoundFontDirectoryPath;
    std::string _SoundFontFilePath;

    HSTREAM _Stream[3];

    int _InterpolationMode;
    int _VoiceCount;

    uint8_t _BankLSBOverride[48];

    bool _DoReverbAndChorusProcessing;
    bool _IsBankLSBOverridden;
};
#pragma warning(default: 4820) // x bytes padding added after data member
