
/** $VER: BMPlayer.h (2024.05.05) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded);

struct sflist_presets;

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
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
    virtual void Render(audio_sample * sampleData, uint32_t samplesCount) override;

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;
    #pragma endregion

    void ResetParameters();
    bool LoadSoundFontConfiguration(std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations, std::string pathName);

    void CompoundPresets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels);

private:
    static const uint32_t MaxSamples = 512;
    static const uint32_t ChannelCount = 2;

    float _Buffer[MaxSamples * ChannelCount];

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
