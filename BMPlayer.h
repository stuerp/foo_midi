
/** $VER: BMPlayer.h (2023.07.24) **/

#pragma once

#include "MIDIPlayer.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded);

struct sflist_presets;

#pragma warning(disable: 4820) // x bytes padding added after data member
/// <summary>
/// Implements a MIDI player using BASS MIDI.
/// </summary>
class BMPlayer : public MIDIPlayer
{
public:
    BMPlayer();
    virtual ~BMPlayer();

    void SetSoundFontDirectory(const char * directoryPath);
    void SetSoundFontFile(const char * filePath);

    void SetInterpolationMode(uint32_t interpolationMode);
    void EnableEffects(bool enable = true);
    void SetVoiceCount(uint32_t voices);

    uint32_t GetActiveVoiceCount() const noexcept;

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

    HSTREAM _Stream[3];

    std::string _SoundFontDirectoryPath;
    std::string _SoundFontFilePath;

    uint32_t _InterpolationMode;
    uint32_t _VoiceCount;

    uint8_t _BankLSBOverride[48];

    bool _DoReverbAndChorusProcessing;
    bool _IsBankLSBOverridden;
};
#pragma warning(default: 4820) // x bytes padding added after data member
