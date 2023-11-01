 
/** $VER: InputDecoder.cpp (2023.11.01) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "InputDecoder.h"

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include <pfc/string-conv-lite.h>

#include <libmidi/MIDIProcessor.h>

#include "KaraokeProcessor.h"
#include "Exceptions.h"

#include <math.h>

volatile int _IsRunning = 0;

critical_section _Lock;
volatile uint32_t _CurrentSampleRate;

const GUID GUIDTagMIDIHash = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };

const char * PlayerTypeNames[] =
{
    "Emu de MIDI",
    "VSTi",
    "FluidSynth",
    "SuperMunt",
    "BASS MIDI",
    "DirectX",
    "LibADLMIDI",
    "LibOPNMIDI",
    "OPL",
    "Nuke",
    "Secret Sauce"
};

#pragma region("input_impl")
/// <summary>
/// Opens the specified file and parses it.
/// </summary>
void InputDecoder::open(service_ptr_t<file> file, const char * filePath, t_input_open_reason, abort_callback & abortHandler)
{
    if (file.is_empty())
        filesystem::g_open(file, filePath, filesystem::open_mode_read, abortHandler);

    _FilePath = filePath;

    {
        _FileStats = file->get_stats(abortHandler);

        if ((_FileStats.m_size == 0) || (_FileStats.m_size > (t_filesize)(1 << 30)))
            throw exception_io_unsupported_format();

        _FileStats2 = file->get_stats2_((uint32_t)stats2_all, abortHandler);

        if ((_FileStats2.m_size == 0) || (_FileStats2.m_size > (t_filesize)(1 << 30)))
            throw exception_io_unsupported_format();
    }

    std::vector<uint8_t> Data;

    Data.resize((size_t) _FileStats.m_size);

    file->read_object(&Data[0], (t_size) _FileStats.m_size, abortHandler);

    {
        _IsSysExFile = IsSysExFileExtension(pfc::string_extension(filePath));

        if (_IsSysExFile)
        {
            if (!MIDIProcessor::Process(Data, nullptr, _Container))
                throw exception_io_data("Invalid SysEx dump");

            return;
        }
    }

    {
        if (!MIDIProcessor::Process(Data, pfc::string_extension(filePath), _Container))
        {
            pfc::string8 Message = "Invalid MIDI file: ";

            switch (MIDIProcessor::GetLastErrorCode())
            {
                case None: Message += "No error"; break;

                case UnknownStatusCode: Message += "Unknown MIDI status code"; break;

                case InsufficientData: Message += "Insufficient data in the stream"; break;

                case InvalidSysExMessage: Message += "Invalid System Exclusive message"; break;
                case InvalidSysExMessageContinuation: Message += "Invalid System Exclusive message"; break;
                case InvalidSysExEndMessage: Message += "Invalid System Exclusive End message"; break;

                case InvalidMetaDataMessage: Message += "Invalid Meta Datamessage"; break;

                // SMF
                case SMFBadHeaderChunkType: Message += "Bad SMF header chunk type"; break;
                case SMFBadHeaderChunkSize: Message += "Bad SMF header chunk size"; break;
                case SMFBadHeaderFormat: Message += "Bad SMF header format"; break;
                case SMFBadHeaderTrackCount: Message += "Bad SMF header track count"; break;
                case SMFBadHeaderTimeDivision: Message += "Bad SMF header time division"; break;

                case SMFUnknownChunkType: Message += "Unknown type specified in SMF chunk"; break;

                case SMFBadFirstMessage: Message += "Bad first message of a track"; break;

                // XMI
                case XMIFORMXDIRNotFound: Message += "FORM XDIR chunk not found"; break;
                case XMICATXMIDNotFound: Message += "CAT XMID chunk not found"; break;
                case XMIFORMXMIDNotFound: Message += "FORM XMID chunk not found"; break;
                case XMIEVNTChunkNotFound: Message += "EVNT chunk not found"; break;

                case XMIInvalidNoteMessage: Message += "Invalid note message"; break;

                default: Message += "Unknown error code"; break;
            }

            throw exception_io_data(Message);
        }

        _TrackCount = _Container.GetTrackCount();

        if (_TrackCount == 0)
            throw exception_io_data("Invalid MIDI file. No tracks found");

        // Check if we read a valid MIDI file.
        {
            bool HasDuration = false;

            for (size_t i = 0; i < _TrackCount; ++i)
            {
                if (_Container.GetDuration(i) != 0)
                {
                    HasDuration = true;
                    break;
                }
            }

            if (!HasDuration)
                throw exception_io_data("Invalid MIDI file");
        }

        _Container.DetectLoops(_DetectXMILoops, _DetectFF7Loops, _DetectRPGMakerLoops, _DetectTouhouLoops);
    }

    // Calculate the hash of the MIDI file.
    {
        Data.resize(0);

        _Container.SerializeAsSMF(Data);

        hasher_md5_state HasherState;
        static_api_ptr_t<hasher_md5> Hasher;

        Hasher->initialize(HasherState);
        Hasher->process(HasherState, &Data[0], Data.size());

        _FileHash = Hasher->get_result(HasherState);
    }

    if (AdvCfgSkipToFirstNote)
        _Container.TrimStart();

//  _LoopInTicks.Clear();
    _LoopRange.Clear();
}
#pragma endregion

#pragma region("input_decoder")
/// <summary>
/// Initializes the decoder before playing the specified subsong.
/// Resets playback position to the beginning of specified subsong. This must be called first, before any other input_decoder methods (other than those inherited from input_info_reader).
/// It is legal to set initialize() more than once, with same or different subsong, to play either the same subsong again or another subsong from same file without full reopen.
/// </summary>
void InputDecoder::decode_initialize(unsigned subSongIndex, unsigned flags, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        throw exception_midi("You cannot play SysEx dumps");

    _IsFirstChunk = true;

    _PlayerType = (PlayerType) (uint32_t) CfgPlayerType;
    _LoopType = (flags & input_flag_playback) ? _LoopTypePlayback : _LoopTypeOther;
    _TimeInSamples = 0;

    InitializeTime(subSongIndex);

    _Container.SetTrackCount(_TrackCount);

    _ExtraPercussionChannel = _Container.GetExtraPercussionChannel();

    // The preset collects a number of settings that will configure the player.
    MIDIPreset Preset;

    // Set the player type based on the content of the container.
    {
        if (_IsXG && CfgUseVSTiWithXG)
        {
            _PlayerType = PlayerType::VSTi;

            pfc::string8 FilePath;

            AdvCfgVSTiXGPlugin.get(FilePath);

            Preset._VSTiFilePath = FilePath;
        }
        else
        if (_IsMT32 && CfgUseSuperMuntWithMT32)
            _PlayerType = PlayerType::SuperMunt;
    }

    // Load the preset from the song if it has one.
    {
        file_info_impl FileInfo;

        get_info(subSongIndex, FileInfo, abortHandler);

        {
            const char * MIDIPresetText = FileInfo.meta_get(TagMIDIPreset, 0);

            if (MIDIPresetText)
            {
                console::print("Using preset \"", MIDIPresetText, "\" from file.");

                Preset.Deserialize(MIDIPresetText);

                // Override the player type with the one specified in the preset.
                _PlayerType = Preset._PlayerType;
            }
        }

        {
            MIDISysExDumps SysExDumps;

            // Load the sysex from the song if it has one.
            {
                const char * MIDISysExDumps = FileInfo.meta_get(TagMIDISysExDumps, 0);

                if (MIDISysExDumps)
                {
                    console::print("Using sysex \"", MIDISysExDumps , "\" from file.");

                    SysExDumps.Deserialize(MIDISysExDumps, _FilePath);
                }
            }

            SysExDumps.Merge(_Container, abortHandler);
        }
    }

    pfc::string8 SoundFontFilePath;

    // Set the player type to BASS MIDI if a SoundFont has been configured for the file.
    {
        pfc::string8 FilePath = _FilePath;
        pfc::string8 TempSoundFontFilePath;

        bool FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);

        if (!FoundSoundFile)
        {
            size_t FileExtensionIndex = FilePath.find_last('.');

            if (FileExtensionIndex > FilePath.scan_filename())
            {
                FilePath.truncate(FileExtensionIndex);

                FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);
            }

            if (!FoundSoundFile)
            {
                FilePath.truncate(FilePath.scan_filename());

                TempSoundFontFilePath = "";
                TempSoundFontFilePath.add_byte(FilePath[FilePath.length() - 1]);
                FilePath.truncate(FilePath.length() - 1);

                size_t FileNameIndex = FilePath.scan_filename();

                if (FileNameIndex != pfc::infinite_size)
                {
                    FilePath += TempSoundFontFilePath;
                    FilePath.add_string(&FilePath[FileNameIndex], FilePath.length() - FileNameIndex - 1);

                    FoundSoundFile = GetSoundFontFilePath(FilePath, TempSoundFontFilePath, abortHandler);
                }
            }
        }

        if (FoundSoundFile)
        {
            console::print("Setting player type to BASS MIDI because matching SoundFont \"", TempSoundFontFilePath, "\".");

            SoundFontFilePath = TempSoundFontFilePath;

            _PlayerType = PlayerType::BASSMIDI;
        }
    }

    // Update sample rate before initializing the fade-out range.
    if (_PlayerType == PlayerType::SuperMunt)
        _SampleRate = (uint32_t) MT32Player::GetSampleRate();

    // Initialize the fade-out range. Case "Never loop", "Never, add 1s decay time", "Loop and fade when detected" or "Always loop and fade",
    MIDIPlayer::LoopMode LoopMode = MIDIPlayer::LoopMode::None;

    _FadeRange.Clear();

    if (!(flags & input_flag_no_looping))
    {
/*
        if (_LoopType < PlayIndefinitelyWhenDetected)
        {
            if ((_LoopType > LoopAndFadeWhenDetected) || _LoopInTicks.IsSet())
            {
                uint32_t Begin =       (uint32_t) ::MulDiv((int)(_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount)), (int) _SampleRate, 1000);
                uint32_t End = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                        (int) _SampleRate, 1000);

                _FadeRange.Set(Begin, End);
                LoopMode = (MIDIPlayer::LoopMode) (MIDIPlayer::LoopMode::Enabled | MIDIPlayer::LoopMode::Forced);
            }
            else
                _FadeRange.Set(_LengthInSamples, _LengthInSamples);
        }
        else
        {
            if ((_LoopType == PlayIndefinitely) || _LoopInTicks.IsSet())
            {
                _FadeRange.Clear();
                LoopMode = (MIDIPlayer::LoopMode) (MIDIPlayer::LoopMode::Enabled | MIDIPlayer::LoopMode::Forced);
            }
            else
                _FadeRange.Set(_LengthInSamples, _LengthInSamples);
        }
*/
        switch (_LoopType)
        {
            case LoopType::NeverLoop:
                LoopMode = MIDIPlayer::LoopMode::None;
                break;

            case LoopType::NeverLoopAddDecayTime:
                LoopMode = MIDIPlayer::LoopMode::None;
                break;

            case LoopType::LoopAndFadeWhenDetected:
            {
                if (_LoopRange.IsSet())
                {
                    LoopMode = MIDIPlayer::LoopMode::Enabled;

                    uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _SampleRate, 1000);
                    uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                          (int) _SampleRate, 1000);

                    _FadeRange.Set(Begin, End);
                }
                else
                {
                    LoopMode = MIDIPlayer::LoopMode::None;

                    _FadeRange.Set(_LengthInSamples, _LengthInSamples);
                }
                break;
            }

            case LoopType::LoopAndFadeAlways:
            {
                LoopMode = (MIDIPlayer::LoopMode) (MIDIPlayer::LoopMode::Enabled | MIDIPlayer::LoopMode::Forced);

                uint32_t Begin =         (uint32_t) ::MulDiv((int)(_LoopRange.Begin() + (_LoopRange.Size() * _LoopCount)), (int) _SampleRate, 1000);
                uint32_t End   = Begin + (uint32_t) ::MulDiv((int) _FadeDuration,                                          (int) _SampleRate, 1000);

                _FadeRange.Set(Begin, End);
                break;
            }

            case LoopType::PlayIndefinitelyWhenDetected:
                _FadeRange.Set(_LengthInSamples, _LengthInSamples);
                break;

            case LoopType::PlayIndefinitely:
                break;
        }
    }

    // Create and initialize the MIDI player.
    delete _Player;
    _Player = nullptr;

    switch (_PlayerType)
    {
        case PlayerType::Unknown:
            break;

        // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413))
        case PlayerType::EmuDeMIDI:
        {
            {
                auto Player = new EdMPlayer;

                _Player = Player;
            }

            {
                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    {
                        insync(_Lock);

                        if (++_IsRunning == 1)
                            _CurrentSampleRate = _SampleRate;
                        else
                        if (_SampleRate != _CurrentSampleRate)
                            _SampleRate = _CurrentSampleRate;
                    }

                    _Player->SetSampleRate(_SampleRate);

                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // VSTi
        case PlayerType::VSTi:
        {
            {
                if (Preset._VSTiFilePath.is_empty())
                    throw exception_midi("No VSTi specified in preset");

                auto Player = new VSTiPlayer;

                if (!Player->LoadVST(Preset._VSTiFilePath))
                    throw exception_midi(pfc::string8("Unable to load VSTi from \"") + Preset._VSTiFilePath + "\"");
            
                if (Preset._VSTiConfig.size())
                    Player->SetChunk(&Preset._VSTiConfig[0], Preset._VSTiConfig.size());

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
                else
                {
                    std::string ErrorMessage;

                    if (_Player->GetErrorMessage(ErrorMessage))
                        throw exception_io_data(ErrorMessage.c_str());
                }
            }
            break;
        }

        // FluidSynth
        case PlayerType::FluidSynth:
        {
            {
                _ActiveVoiceCount = 0;
                _PeakVoiceCount = 0;

                if (Preset._SoundFontFilePath.is_empty() && SoundFontFilePath.is_empty())
                    throw exception_midi("No SoundFont defined in preset and no SoundFont file or directory found");
            }

            pfc::string8 FluidSynthDirectoryPath = CfgFluidSynthDirectoryPath;

            if (FluidSynthDirectoryPath.is_empty())
            {
                console::warning("FluidSynth path configured. Attempting to load libraries from plugin install path");

                FluidSynthDirectoryPath = core_api::get_my_full_path();
                FluidSynthDirectoryPath.truncate(FluidSynthDirectoryPath.scan_filename());
            }

            {
                auto Player = new FSPlayer;

                Player->SetAbortHandler(&abortHandler);

                if (!Player->Initialize(pfc::wideFromUTF8(FluidSynthDirectoryPath)))
                    throw exception_midi("FluidSynth path not configured");

                Player->SetSoundFontFile(Preset._SoundFontFilePath);

                if (SoundFontFilePath.length())
                    Player->SetSoundFontFile(SoundFontFilePath);

                Player->SetInterpolationMode(_FluidSynthInterpolationMethod);
                Player->EnableEffects(Preset._EffectsEnabled);
                Player->SetVoiceCount(Preset._VoiceCount);
                Player->EnableDynamicLoading(AdvCfgLoadSoundFontDynamically.get());

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
                else
                {
                    std::string ErrorMessage;

                    if (_Player->GetErrorMessage(ErrorMessage))
                        throw exception_io_data(ErrorMessage.c_str());
                }
            }
            break;
        }

        // Munt (MT-32)
        case PlayerType::SuperMunt:
        {
            {
                auto Player = new MT32Player(!_IsMT32, Preset._MuntGMSet);

                pfc::string8 BasePath = CfgMT32ROMDirectoryPath;

                if (BasePath.is_empty())
                {
                    console::warning("No Munt base path configured, attempting to load ROMs from plugin install path.");

                    BasePath = core_api::get_my_full_path();
                    BasePath.truncate(BasePath.scan_filename());
                }

                Player->setBasePath(BasePath);
                Player->SetAbortHandler(&abortHandler);

                if (!Player->isConfigValid())
                    throw exception_midi("The Munt driver needs to be configured with a valid MT-32 or CM32L ROM set.");

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // BASS MIDI
        case PlayerType::BASSMIDI:
        {
            {
                {
                    _ActiveVoiceCount = 0;
                    _PeakVoiceCount = 0;

                    if (Preset._SoundFontFilePath.is_empty() && SoundFontFilePath.is_empty())
                        throw exception_midi("No SoundFont defined in preset and no SoundFont file or directory found");
                }

                auto Player = new BMPlayer;

                Player->SetSoundFontDirectory(Preset._SoundFontFilePath);

                if (SoundFontFilePath.length())
                    Player->SetSoundFontFile(SoundFontFilePath);

                Player->SetInterpolationMode(_BASSMIDIInterpolationMode);
                Player->EnableEffects(Preset._EffectsEnabled);
                Player->SetVoiceCount(Preset._VoiceCount);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
                else
                {
                    std::string ErrorMessage;

                    if (_Player->GetErrorMessage(ErrorMessage))
                        throw exception_io_data(ErrorMessage.c_str());
                }
            }
            break;
        }

        // DirectX
        case PlayerType::DirectX:
        {
    #ifdef DXISUPPORT
            pfc::array_t<t_uint8> serialized_midi_file;
            midi_file.SerializeAsSMF(serialized_midi_file);

            delete dxiProxy;
            dxiProxy = nullptr;

            dxiProxy = new DXiProxy;
            if (SUCCEEDED(dxiProxy->initialize()))
            {
                dxiProxy->SetSampleRate(srate);
                if (SUCCEEDED(dxiProxy->setSequence(serialized_midi_file.get_ptr(), serialized_midi_file.get_count())))
                {
                    if (SUCCEEDED(dxiProxy->setPlugin(thePreset.dxi_plugin)))
                    {
                        dxiProxy->Stop();
                        dxiProxy->Play(TRUE);

                        eof = false;
                        _DontLoop = true;

                        if (doing_loop)
                        {
                            set_loop();
                        }

                        return;
                    }
                }
            }
    #endif
            break;
        }

        // ADL
        case PlayerType::ADL:
        {
            {
                auto Player = new ADLPlayer;

                Player->SetBank(Preset._ADLBankNumber);
                Player->SetBankFilePath(Preset._ADLBankFilePath);
                Player->SetChipCount(Preset._ADLChipCount);
                Player->SetFullPanning(Preset._ADLUsePanning);
                Player->Set4OpCount(Preset._ADLChipCount * 4 /*cfg_adl_4op*/);
                Player->SetCore(Preset._ADLEmulatorCore);
                Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // OPN
        case PlayerType::OPN:
        {
            {
                auto Player = new OPNPlayer;

                Player->setBank(Preset._OPNBankNumber);
                Player->setChipCount(Preset._ADLChipCount);
                Player->setFullPanning(Preset._ADLUsePanning);
                Player->setCore(Preset._OPNEmulatorCore);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // OPL
        case PlayerType::OPL:
        {
            break;
        }

        // Nuke
        case PlayerType::Nuke:
        {
            {
                auto Player = new NukePlayer();

                Player->SetSynth(Preset._NukeSynth);
                Player->SetBank(Preset._NukeBank);
                Player->SetExtp(Preset._NukeUsePanning);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }

        // Secret Sauce
        case PlayerType::SecretSauce:
        {
            {
                auto Player = new SCPlayer();

                pfc::string8 PathName;

                AdvCfgSecretSauceDirectoryPath.get(PathName);

                if (PathName.is_empty())
                {
                    console::warning("Secret Sauce path not configured, yet somehow enabled; trying plugin directory...");

                    PathName = core_api::get_my_full_path();
                    PathName.truncate(PathName.scan_filename());
                }

                Player->SetRootPath(PathName);

                _Player = Player;
            }

            {
                _Player->Configure(Preset._MIDIFlavor, !Preset._UseMIDIEffects);
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, _LoopType, _CleanFlags))
                {
                    _IsEndOfContainer = false;

                    return;
                }
            }
            break;
        }
    }

    throw exception_midi("No MIDI player specified");
}

/// <summary>
/// Reads/decodes one chunk of audio data.
/// Use false return value to signal end of file (no more data to return). Before calling decode_run(), decoding must be initialized by a decode_initialize() call.
/// </summary>
bool InputDecoder::decode_run(audio_chunk & audioChunk, abort_callback & abortHandler)
{
    abortHandler.check();

    if (_IsEndOfContainer)
        return false;

    bool Success = false;

#ifdef DXISUPPORT
    if (_PlayerType == PlayerType::DirectX)
    {
        unsigned todo = 4096;

        if (_DontLoop)
        {
            if (length_samples && samples_done + todo > length_samples)
            {
                todo = length_samples - samples_done;
                if (!todo) return false;
            }
        }

    #if audio_sample_size != 32
        sample_buffer.grow_size(todo * 2);

        float * ptr = sample_buffer.get_ptr();

        thePlayer->FillBuffer(ptr, todo);

        audioChunk.set_data_32(ptr, todo, 2, srate);
    #else
        audioChunk.set_data_size(todo * 2);

        float * ptr = audioChunk.get_data();

        dxiProxy->fillBuffer(ptr, todo);

        audioChunk.set_srate(srate);
        audioChunk.set_channels(2);
        audioChunk.set_sample_count(todo);
    #endif

        samples_done += todo;

        rv = true;
    }
    else
#endif

    if (_Player)
    {
        const uint32_t SamplesToDo = 4096;
        const uint32_t ChannelCount = _Player->GetChannelCount();

        audioChunk.set_data_size((t_size) SamplesToDo * ChannelCount);

        audio_sample * Samples = audioChunk.get_data();

        _Player->SetAbortHandler(&abortHandler);

        uint32_t SamplesDone = _Player->Play(Samples, SamplesToDo);

        if (SamplesDone == 0)
        {
            std::string ErrorMessage;

            if (_Player->GetErrorMessage(ErrorMessage) != 0)
                throw exception_io_data(ErrorMessage.c_str());

            return false;
        }

        audioChunk.set_srate(_SampleRate);
        audioChunk.set_channels(ChannelCount);
        audioChunk.set_sample_count(SamplesDone);

        Success = true;
    }

    // Scale the samples if fading was requested.
    if (Success && _FadeRange.IsSet())
    {
        uint32_t BeginOfChunk = _TimeInSamples;
        uint32_t EndOfChunk   = _TimeInSamples + (uint32_t) audioChunk.get_sample_count();

        _TimeInSamples = EndOfChunk;

        if (EndOfChunk >= _FadeRange.Begin())
        {
            for (size_t i = std::max(_FadeRange.Begin(), BeginOfChunk), j = std::min(EndOfChunk, _FadeRange.End()); i < j; ++i)
            {
                audio_sample * Sample = audioChunk.get_data() + (i - BeginOfChunk) * 2;

                audio_sample Scale = (audio_sample) (_FadeRange.End() - i) / (audio_sample) _FadeRange.Size();

                Sample[0] *= Scale;
                Sample[1] *= Scale;
            }

            if (EndOfChunk > _FadeRange.End())
            {
                uint32_t SamplesRemaining = 0;

                if (_FadeRange.End() > BeginOfChunk)
                    SamplesRemaining = _FadeRange.End() - BeginOfChunk;

                audioChunk.set_sample_count(SamplesRemaining);

                _IsEndOfContainer = true;

                if (SamplesRemaining == 0)
                    return false;
            }
        }
    }

    _AudioChunkDuration = audioChunk.get_duration();

    return Success;
}

/// <summary>
/// Seeks to the specified time offset. Before seeking or other decoding calls, decoding must be initialized with decode_initialize() call.
/// </summary>
void InputDecoder::decode_seek(double timeInSeconds, abort_callback &)
{
    _TimeInSamples = (uint32_t) (timeInSeconds * _SampleRate);
    _IsFirstChunk = true;
    _IsEndOfContainer = false;

    uint32_t OffsetInMs = (uint32_t) (timeInSeconds * 1000.);

    if (OffsetInMs > _LoopRange.End())
        OffsetInMs = _LoopRange.Begin() + (OffsetInMs - _LoopRange.Begin()) % _LoopRange.Size();

    uint32_t TimeInSamples = (uint32_t) ::MulDiv((int) OffsetInMs, (int) _SampleRate, 1000);

    if ((_LengthInSamples != 0U) && (TimeInSamples >= (_LengthInSamples - _SampleRate)))
    {
        _IsEndOfContainer = true;
        return;
    }

#ifdef DXISUPPORT
    if (_PlayerType == PlayerType::DirectX)
    {
        dxiProxy->setPosition(seek_msec);

        samples_done = done;

        return;
    }
    else
#endif
    if (_Player)
    {
        _Player->Seek(TimeInSamples);
        return;
    }
}

/// <summary>
/// Signals dynamic VBR bitrate, etc. Called after each decode_run() (or not called at all if caller doesn't care about dynamic info).
/// </summary>
bool InputDecoder::decode_get_dynamic_info(file_info & fileInfo, double & timestampDelta)
{
    bool Success = false;

    if (_IsFirstChunk)
    {
        {
            fileInfo.info_set_int(TagSampleRate, _SampleRate);
        }

        {
            assert(_countof(PlayerTypeNames) == ((size_t) PlayerType::Max + 1));

            const char * PlayerName = "Unknown";

            if ((_PlayerType >= PlayerType::Min) && (_PlayerType <= PlayerType::Max))
                PlayerName = PlayerTypeNames[(size_t) _PlayerType];
            else
                PlayerName = "VSTi";

            fileInfo.info_set(TagMIDIPlayer, PlayerName);
        }

        {
            if (_ExtraPercussionChannel != ~0L)
            {
                char Text[4];

                ::_itoa_s((int)(_ExtraPercussionChannel + 1), Text, 10);
                fileInfo.info_set(TagExtraPercusionChannel, Text);
            }
        }

        _IsFirstChunk = false;

        Success = true;
        timestampDelta = 0.;
    }

    if ((_PlayerType == PlayerType::FluidSynth) || (_PlayerType == PlayerType::BASSMIDI))
    {
        uint32_t VoiceCount = 0;

        if (_PlayerType == PlayerType::FluidSynth)
        {
            auto Player = (FSPlayer *) _Player;

            VoiceCount = Player->GetActiveVoiceCount();
        }
        else
        if (_PlayerType == PlayerType::BASSMIDI)
        {
            auto Player = (BMPlayer *) _Player;

            VoiceCount = Player->GetActiveVoiceCount();
        }

        if (VoiceCount != _ActiveVoiceCount)
        {
            fileInfo.info_set_int(TagMIDIActiveVoices, (t_int64) VoiceCount);

            _ActiveVoiceCount = VoiceCount;

            Success = true;
        }

        if (VoiceCount > _PeakVoiceCount)
        {
            fileInfo.info_set_int(TagMIDIPeakVoices, (t_int64) VoiceCount);

            _PeakVoiceCount = VoiceCount;

            Success = true;
        }

        if (Success)
            timestampDelta = _AudioChunkDuration;
    }

    return Success;
}
#pragma endregion

#pragma region("input_info_reader")
/// <summary>
/// Retrieves information about the specified subsong.
/// </summary>
void InputDecoder::get_info(t_uint32 subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        return;

    ConvertMetaDataToTags(subSongIndex, fileInfo, abortHandler);

    fileInfo.set_length(_Container.GetDuration(subSongIndex, true) * 0.001);
}
#pragma endregion

#pragma region("input_info_writer")
/// <summary>
/// Set the tags for the specified file.
/// </summary>
void InputDecoder::retag_set_info(t_uint32, const file_info & fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        throw exception_io_data("You cannot tag SysEx dumps");

    file_info_impl fi(fileInfo);

    {
        fi.meta_remove_field(TagMIDISysExDumps);

        const char * SysExDumps = fi.info_get(TagMIDISysExDumps);

        if (SysExDumps)
            fi.meta_set(TagMIDISysExDumps, SysExDumps);
    }

    {
        file::ptr TagFile;

        filesystem::g_open_tempmem(TagFile, abortHandler);

        tag_processor::write_apev2(TagFile, fi, abortHandler);

        {
            pfc::array_t<t_uint8> Tag;

            TagFile->seek(0, abortHandler);

            Tag.set_count((t_size)TagFile->get_size_ex(abortHandler));

            TagFile->read_object(Tag.get_ptr(), Tag.get_count(), abortHandler);

            static_api_ptr_t<metadb_index_manager>()->set_user_data(GUIDTagMIDIHash, _Hash, Tag.get_ptr(), Tag.get_count());
        }
    }
}
#pragma endregion

/// <summary>
/// Initializes the Index Manager.
/// </summary>
void InputDecoder::InitializeIndexManager()
{
    try
    {       
        static_api_ptr_t<metadb_index_manager>()->add(new service_impl_t<FileHasher>, GUIDTagMIDIHash, system_time_periods::week * 4);
    }
    catch (...) { }
}

/// <summary>
/// Initializes the time parameters.
/// </summary>
void InputDecoder::InitializeTime(size_t subSongIndex)
{
//  _LoopInTicks.Set(_Container.GetLoopBeginTimestamp(subSongIndex), _Container.GetLoopEndTimestamp(subSongIndex));
    _LoopRange.Set(_Container.GetLoopBeginTimestamp(subSongIndex, true), _Container.GetLoopEndTimestamp(subSongIndex, true));

    _LengthInSamples = (uint32_t) ::MulDiv((int) GetPlaybackTime(subSongIndex), (int) _SampleRate, 1000);
}

/// <summary>
/// Gets the total play back time taking into account any looping and decay time, in ms.
/// </summary>
uint32_t InputDecoder::GetPlaybackTime(size_t subSongIndex)
{
    uint32_t LengthInMs = _Container.GetDuration(subSongIndex, true);

    if (_LoopType == LoopType::NeverLoopAddDecayTime)
        LengthInMs += (uint32_t) CfgDecayTime;
    else
    if (_LoopType > LoopType::LoopAndFadeWhenDetected)
    {
        if (!_LoopRange.HasBegin())
            _LoopRange.SetBegin(0);

        if (!_LoopRange.HasEnd())
            _LoopRange.SetEnd(LengthInMs);

        LengthInMs = _LoopRange.Begin() + (_LoopRange.Size() * _LoopCount) + _FadeDuration;
    }

    return LengthInMs;
}

/// <summary>
/// Gets the path name of the matching SoundFont file for the specified file, if any.
/// </summary>
bool InputDecoder::GetSoundFontFilePath(const pfc::string8 filePath, pfc::string8 & soundFontPath, abort_callback & abortHandler) noexcept
{
    static const char * Extensions[] =
    {
        "json",
        "sflist",
        "sf2pack",
        "sfogg",
        "sf2",
        "sf3"
    };

    soundFontPath = filePath;

    size_t length = soundFontPath.length();

    for (size_t i = 0; i < _countof(Extensions); ++i)
    {
        soundFontPath.truncate(length);
        soundFontPath += ".";
        soundFontPath += Extensions[i];

        if (filesystem::g_exists(soundFontPath, abortHandler))
            return true;
    }

    return false;
}

/// <summary>
/// Converts the MIDI metadata to tags.
/// </summary>
void InputDecoder::ConvertMetaDataToTags(size_t subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    _IsMT32 = false;
    _IsXG = false;

    KaraokeProcessor kp;

    {
        MIDIMetaData MetaData;

        _Container.GetMetaData(subSongIndex, MetaData);

        MIDIMetaDataItem Item;

        {
            bool HasTitle = MetaData.GetItem("title", Item);

            for (size_t i = 0; i < MetaData.GetCount(); ++i)
            {
                const MIDIMetaDataItem & mdi = MetaData[i];

            #ifdef _DEBUG
//              console::print(mdi.Name.c_str(), ":", mdi.Value.c_str());
            #endif

                if (pfc::stricmp_ascii(mdi.Name.c_str(), "type") == 0)
                {
                    fileInfo.info_set(TagMIDIType, mdi.Value.c_str());

                    _IsMT32 = (mdi.Value == "MT-32");
                    _IsXG = (mdi.Value == "XG");
                }
                else
                if (pfc::stricmp_ascii(mdi.Name.c_str(), "lyrics_type") == 0)
                {
                    fileInfo.info_set(TagMIDILyricsType, mdi.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(mdi.Name.c_str(), "lyrics") == 0)
                {
                    kp.AddUnsyncedLyrics(mdi.Timestamp, mdi.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(mdi.Name.c_str(), "soft_karaoke_lyrics") == 0)
                {
                    kp.AddSyncedLyrics(mdi.Timestamp, mdi.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(mdi.Name.c_str(), "soft_karaoke_text") == 0)
                {
                    kp.AddSyncedText(mdi.Value.c_str());
                }
                else
                {
                    std::string Name = mdi.Name;

                    if (!HasTitle && (pfc::stricmp_ascii(Name.c_str(), "display_name") == 0))
                        Name = "title";

                    AddTag(fileInfo, Name.c_str(), mdi.Value.c_str(), 0);
                }
            }
        }
    }

    if (!kp.GetUnsyncedLyrics().is_empty())
    {
        auto Lyrics = kp.GetUnsyncedLyrics();
        pfc::string8 UTF8;

        KaraokeProcessor::UTF8Encode(Lyrics, UTF8);

        fileInfo.meta_set("lyrics", UTF8);
    }

    if (!kp.GetSyncedLyrics().is_empty())
    {
        auto Lyrics = kp.GetSyncedLyrics();
        pfc::string8 UTF8;

        KaraokeProcessor::UTF8Encode(Lyrics, UTF8);

        fileInfo.meta_set("syncedlyrics", UTF8);
    }

    // General info
    fileInfo.info_set_int(TagChannels, 2);
    fileInfo.info_set(TagEncoding, "Synthesized");

    // Specific info
    fileInfo.info_set_int(TagMIDIFormat,       _Container.GetFormat());
    fileInfo.info_set_int(TagMIDITrackCount,   _Container.GetFormat() == 2 ? 1 : _Container.GetTrackCount());
    fileInfo.info_set_int(TagMIDIChannelCount, _Container.GetChannelCount(subSongIndex));
    fileInfo.info_set_int(TagMIDITicks,        _Container.GetDuration(subSongIndex));

    {
        uint32_t LoopBegin = _Container.GetLoopBeginTimestamp(subSongIndex);
        uint32_t LoopEnd = _Container.GetLoopEndTimestamp(subSongIndex);
        uint32_t LoopBeginInMS = _Container.GetLoopBeginTimestamp(subSongIndex, true);
        uint32_t LoopEndInMS = _Container.GetLoopEndTimestamp(subSongIndex, true);

        if (LoopBegin != ~0U) fileInfo.info_set_int(TagMIDILoopStart, LoopBegin);
        if (LoopEnd != ~0U) fileInfo.info_set_int(TagMIDILoopEnd, LoopEnd);
        if (LoopBeginInMS != ~0U) fileInfo.info_set_int(TagMIDILoopStartInMs, LoopBeginInMS);
        if (LoopEndInMS != ~0U) fileInfo.info_set_int(TagMIDILoopEndInMs, LoopEndInMS);
    }

    {
        pfc::string8 FileHashString;

        for (size_t i = 0U; i < 16; ++i)
            FileHashString += pfc::format_uint((t_uint8)_FileHash.m_data[i], 2, 16);

        fileInfo.info_set(TagMIDIHash, FileHashString);
    }

    {
        service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<FileHasher>;

        _Hash = IndexClient->transform(fileInfo, playable_location_impl(_FilePath, (t_uint32) subSongIndex));

        pfc::array_t<t_uint8> Tags;

        static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTagMIDIHash, _Hash, Tags);

        t_size TagCount = Tags.get_count();

        if (TagCount > 0)
        {
            file::ptr File;

            filesystem::g_open_tempmem(File, abortHandler);

            File->write_object(Tags.get_ptr(), TagCount, abortHandler);

            fileInfo.meta_remove_all();

            tag_processor::read_trailing(File, fileInfo, abortHandler);

            fileInfo.info_set("tagtype", "apev2 db");
        }
    }
}
/*
/// <summary>
/// Returns the number of bytes required to represent a Shift-JIS character.
/// </summary>
static size_t DecodeShiftJISChar(const uint8_t * data, size_t size)
{
    if (size == 0)
        return 0;

    if (size > 2)
        size = 2;

    if (data[0] < 0x80)
        return (size_t)((data[0] > 0) ? 1 : 0);

    if (size >= 1)
    {
        // TODO: definitely weak
        if (unsigned(data[0] - 0xA1) < 0x3F)
            return 1;
    }

    if (size >= 2)
    {
        // TODO: probably very weak
        if ((unsigned(data[0] - 0x81) < 0x1F || unsigned(data[0] - 0xE0) < 0x10) && unsigned(data[1] - 0x40) < 0xBD)
            return 2;
    }

    return 0;
}

/// <summary>
/// Is the data a Shift-JIS string?
/// </summary>
static bool IsValidShiftJISOld(const char * data, size_t size)
{
    for (size_t i = 0; (i < size) && (data[i] != 0);)
    {
        size_t n = ::DecodeShiftJISChar((const uint8_t *)data + i, size - i);

        if (n == 0)
            return false;

        i += n;

        if (i > size)
            return false;
    }

    return true;
}
*/
/// <summary>
/// Is the data a Shift-JIS string?
/// char ShiftJIS[] = { 0x82, 0xA0, 0x82, 0xA2, 0x82, 0xA4 }; / char UTF8[] = { 0xE3, 0x81, 0x82, 0xE3, 0x81, 0x84, 0xE3, 0x81, 0x86 };
/// </summary>
static bool IsValidShiftJIS(const char * data)
{
    bool Result = false;

    uint8_t d1 = (uint8_t) data[0];
    uint8_t d2 = (uint8_t) data[1];

    int j1 = 0;
    int j2 = 0;

    // Check if the first byte is valid Shift-JIS.
    if ((d1 >= 0x81 && d1 <= 0x84) || (d1 >= 0x87 && d1 <= 0x9F))
    {
        j1 = 2 * (d1 - 0x70) - 1;

        if (d2 >= 0x40 && d2 <= 0x9E)
        {
            j2 = d2 - 31;

            if (j2 > 95)
                j2 -= 1;

            Result = true;
        }
        else
        if (d2 >= 0x9F && d2 <= 0xFC)
        {
            j2 = d2 - 126;
            j1 += 1;

            Result = true;
        }
    }
    else
    if (d1 >= 0xE0 && d1 <= 0xEF)
    {
        j1 = 2 * (d1 - 0xB0) - 1;

        if (d2 >= 0x40 && d2 <= 0x9E)
        {
            j2 = d2 - 31;

            if (j2 > 95)
                j2 -= 1;

            Result = true;
        }
        else
        if (d2 >= 0x9F && d2 <= 0xFC)
        {
            j2 = d2 - 126;
            j1 += 1;

            Result = true;
        }
    }

    return Result;
}

/// <summary>
/// Adds the specified tag.
/// </summary>
void InputDecoder::AddTag(file_info & fileInfo, const char * name, const char * value, t_size maxLength)
{
    if ((name == nullptr) || (value == nullptr))
        return;

    if (value[0] == '\0')
        return;

    pfc::string8 NewValue;

    if ((maxLength != 0) && value[maxLength - 1])
    {
        NewValue.set_string(value, maxLength);
        value = NewValue;
    }
    else
        maxLength = ::strlen(value);

    if (pfc::is_lower_ascii(value) || pfc::is_valid_utf8(value, maxLength))
        fileInfo.meta_add(name, value);
    else
    if (IsValidShiftJIS(value))
        fileInfo.meta_add(name, pfc::stringcvt::string_utf8_from_codepage(932, value)); // Shift-JIS
    else
        fileInfo.meta_add(name, pfc::stringcvt::string_utf8_from_ansi(value));
}

/*
#define XMIDI_CONTROLLER_FOR_LOOP 0x74 // For Loop
#define XMIDI_CONTROLLER_NEXT_BREAK 0x75 // Next/Break

#define EMIDI_CONTROLLER_TRACK_DESIGNATION 110 // Track Designation
#define EMIDI_CONTROLLER_TRACK_EXCLUSION 111 // Track Exclusion
#define EMIDI_CONTROLLER_LOOP_BEGIN XMIDI_CONTROLLER_FOR_LOOP
#define EMIDI_CONTROLLER_LOOP_END XMIDI_CONTROLLER_NEXT_BREAK
*/

static input_factory_t<InputDecoder> InputDecoderFactory;
