 
/** $VER: InputDecoder.cpp (2023.07.22) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "InputDecoder.h"

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include <libmidi/MIDIProcessor.h>

#include "KaraokeProcessor.h"
#include "Exceptions.h"

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

    Data.resize((size_t)_FileStats.m_size);

    file->read_object(&Data[0], (t_size)_FileStats.m_size, abortHandler);

    {
        _IsSysExFile = IsSysExFileExtension(pfc::string_extension(filePath));

        if (_IsSysExFile)
        {
            if (!MIDIProcessor::ProcessSysEx(Data, _Container))
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

                default: Message += "Unknown error code"; break;
            }

            throw exception_io_data(Message);
        }

        _TrackCount = (size_t)_Container.GetTrackCount();

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

    _LoopInTicks.Clear();
    _LoopInMs.Clear();
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

    _PlayerType = (uint32_t) CfgPlayerType;
    _IsFirstChunk = true;
    _LoopType = (flags & input_flag_playback) ? _LoopTypePlayback : _LoopTypeOther;
    _SamplesPlayed = 0;

    InitializeTime(subSongIndex);

    _Container.SetTrackCount((uint32_t)_TrackCount);

    _ExtraPercussionChannel = _Container.GetExtraPercussionChannel();

    MIDIPreset Preset;

    {
        file_info_impl FileInfo;

        get_info(subSongIndex, FileInfo, abortHandler);

        {
            const char * MIDIPresetText = FileInfo.meta_get(TagMIDIPreset, 0);

            if (MIDIPresetText)
            {
                console::print("Using preset ", MIDIPresetText, " from track.");

                Preset.Deserialize(MIDIPresetText);

                // Override the player type with the one specified in the preset.
                _PlayerType = Preset._PlayerType;
            }
        }

        MIDISysExDumps SysExDumps;

        {
            const char * MIDISysExDumps = FileInfo.meta_get(TagMIDISysExDumps, 0);

            if (MIDISysExDumps)
            {
                console::print("Using sysex ", MIDISysExDumps , " from track.");

                SysExDumps.Deserialize(MIDISysExDumps, _FilePath);
            }
        }

        SysExDumps.Merge(_Container, abortHandler);
    }

    if (_IsMT32)
    {
        console::print("Setting player type to SuperMunt because MIDI file contains MT-32 messages.");

        _PlayerType = PlayerTypeSuperMunt;
    }

    // Gets a SoundFont file that has been configured for the file, if any.
    pfc::string8 SoundFontFilePath;

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

            _PlayerType = PlayerTypeBASSMIDI;
        }
    }

    // Update sample rate before initializing the fade-out range.
    if (_PlayerType == PlayerTypeSuperMunt)
        _SampleRate = (uint32_t)MT32Player::GetSampleRate();

    // Initialize the fade-out range. Case "Never loop", "Never, add 1s decay time", "Loop and fade when detected" or "Always loop and fade",
    if ((flags & input_flag_no_looping) || (_LoopType < PlayIndefinitelyWhenDetected))
    {
        if ((_LoopType > LoopAndFadeWhenDetected) || !_LoopInTicks.IsEmpty())
        {
            uint32_t Begin = (uint32_t)::MulDiv((int)(_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount)), (int)_SampleRate, 1000);
            uint32_t End = Begin + _SampleRate * _FadeDuration / 1000;

            _FadeRange.Set(Begin, End);
            _IsLooping = true;
        }
        else
        {
            _FadeRange.Set(_LengthInSamples, _LengthInSamples);
            _IsLooping = false;
        }
    }
    else
    {
        if ((_LoopType == PlayIndefinitely) || !_LoopInTicks.IsEmpty())
        {
            _FadeRange.Clear();
            _IsLooping = true;
        }
        else
        {
            _FadeRange.Set(_LengthInSamples, _LengthInSamples);
            _IsLooping = false;
        }
    }

    // Create and initialize the MIDI player.
    delete _Player;
    _Player = nullptr;

    MIDIPlayer::LoopMode LoopModes = MIDIPlayer::LoopMode((int) MIDIPlayer::LoopModeEnabled | (_IsLooping ? (int) MIDIPlayer::LoopModeForced : 0));

    switch (_PlayerType)
    {
        // Emu de MIDI (Sega PSG, Konami SCC and OPLL (Yamaha YM2413))
        case PlayerTypeEmuDeMIDI:
        {
            {
                auto Player = new EdMPlayer;

                _Player = Player;
            }

            {
                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    {
                        insync(_Lock);

                        if (++_IsRunning == 1)
                            _CurrentSampleRate = _SampleRate;
                        else
                        if (_SampleRate != _CurrentSampleRate)
                            _SampleRate = _CurrentSampleRate;

                        _IsEmuDeMIDI = true;
                    }

                    _Player->SetSampleRate(_SampleRate);

                    _IsEndOfContainer = false;
                    _DontLoop = true;

                    return;
                }
            }
            break;
        }

        // VSTi
        case PlayerTypeVSTi:
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
                _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

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
        case PlayerTypeFluidSynth:
        {
    #ifdef FLUIDSYNTHSUPPORT
            /*HMODULE fsmod = LoadLibraryEx( FLUIDSYNTH_DLL, nullptr, LOAD_LIBRARY_AS_DATAFILE );
                if ( !fsmod )
                {
                    throw exception_io_data("Failed to load FluidSynth.dll");
                }
                FreeLibrary( fsmod );*/

            auto sfPlayer = new SFPlayer;

            midiPlayer = sfPlayer;

            sfPlayer->SetSoundFontDirectory(thePreset.soundfont_path);

            if (file_soundfont.length())
                sfPlayer->SetSoundFontFile(file_soundfont);

            sfPlayer->SetSampleRate(srate);
            sfPlayer->setInterpolationMethod(_FluidSynthInterpolationMethod);
            sfPlayer->setDynamicLoading(cfg_soundfont_dynamic.get());
            sfPlayer->EnableEffects(thePreset.effects);
            sfPlayer->setVoiceCount(thePreset.voices);

            sfPlayer->SetFilter((MIDIPlayer::filter_mode) thePreset.MIDIStandard, !thePreset._UseMIDIEffects);

            uint32 LoopMode = MIDIPlayer::LoopModeEnabled;

            if (_IsLooping)
                LoopMode |= MIDIPlayer::loop_mode_force;

            if (sfPlayer->Load(_Container, subSongIndex, LoopMode, _CleanFlags))
            {
                _IsEndOfContainer = false;
                _DontLoop = true;

                return;
            }
    #endif
        }

        // Munt (MT-32)
        case PlayerTypeSuperMunt:
        {
            {
                auto Player = new MT32Player(!_IsMT32, Preset._MuntGMSet);

                pfc::string8 BasePath = CfgMuntDirectoryPath;

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

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

                    return;
                }
            }
            break;
        }

        // BASS MIDI
        case PlayerTypeBASSMIDI:
        {
            {
                {
                    _BASSMIDIVoiceCount = 0;
                    _BASSMIDIVoiceMax = 0;

                    if (Preset._SoundFontFilePath.is_empty() && SoundFontFilePath.is_empty())
                        throw exception_midi("No SoundFont defined in preset and no SoundFont file or directory found");
                }

                auto Player = new BMPlayer;

                Player->SetSoundFontDirectory(Preset._SoundFontFilePath);

                if (SoundFontFilePath.length())
                    Player->SetSoundFontFile(SoundFontFilePath);

                Player->SetInterpolationMode((int)_BASSMIDIInterpolationMode);
                Player->EnableEffects(Preset._BASSMIDIEffectsEnabled);
                Player->SetVoiceCount((int)Preset._BASSMIDIVoices);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);
                _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

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
        case PlayerTypeDirectX:
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
        case PlayerTypeADL:
        {
            {
                auto Player = new ADLPlayer;

                Player->setBank(Preset._ADLBankNumber);
                Player->setChipCount(Preset._ADLChipCount);
                Player->setFullPanning(Preset._ADLUsePanning);
                Player->set4OpCount(Preset._ADLChipCount * 4 /*cfg_adl_4op*/);
                Player->setCore(Preset._ADLEmulatorCore);
                Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                _Player = Player;
            }

            {
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

                    return;
                }
            }
            break;
        }

        // OPN
        case PlayerTypeOPN:
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
                _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

                    return;
                }
            }
            break;
        }

        // OPL
        case PlayerTypeOPL:
        {
            break;
        }

        // Nuke
        case PlayerTypeNuke:
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

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

                    return;
                }
            }
            break;
        }

        // Secret Sauce
        case PlayerTypeSecretSauce:
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
                _Player->SetFilter((MIDIPlayer::FilterType) Preset._MIDIStandard, !Preset._UseMIDIEffects);
                _Player->SetSampleRate(_SampleRate);

                if (_Player->Load(_Container, subSongIndex, LoopModes, _CleanFlags))
                {
                    _IsEndOfContainer = false;
                    _DontLoop = true;

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
    if (_PlayerType == PlayerTypeDirectX)
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
        const size_t SamplesToDo = 4096;
        const size_t ChannelCount = _Player->GetChannelCount();

        audioChunk.set_data_size(SamplesToDo * ChannelCount);

        audio_sample * Samples = audioChunk.get_data();

        _Player->SetAbortHandler(&abortHandler);

        size_t SamplesDone = _Player->Play(Samples, SamplesToDo);

        if (SamplesDone == 0)
        {
            std::string ErrorMessage;

            if (_Player->GetErrorMessage(ErrorMessage) != 0)
                throw exception_io_data(ErrorMessage.c_str());

            return false;
        }

        audioChunk.set_srate(_SampleRate);
        audioChunk.set_channels((uint32_t)ChannelCount);
        audioChunk.set_sample_count(SamplesDone);

        Success = true;
    }

    // Scale the samples if fading was requested.
    if (Success && _FadeRange.IsSet())
    {
        uint32_t BeginOfChunk = _SamplesPlayed;
        uint32_t EndOfChunk   = _SamplesPlayed + (uint32_t)audioChunk.get_sample_count();

        _SamplesPlayed = EndOfChunk;

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
void InputDecoder::decode_seek(double timeInSeconds, abort_callback&)
{
    uint32_t OffsetInSamples = (uint32_t)audio_math::time_to_samples(timeInSeconds, 1000U);

    _SamplesPlayed = (uint32_t)((t_int64(OffsetInSamples) * t_int64(_SampleRate)) / 1000U);
    _IsFirstChunk = true;
    _IsEndOfContainer = false;

    if (OffsetInSamples > _LoopInMs.End())
        OffsetInSamples = (OffsetInSamples - _LoopInMs.Begin()) % _LoopInMs.Size() + _LoopInMs.Begin();

    uint32_t SeekTime = (uint32_t)((t_int64(OffsetInSamples) * t_int64(_SampleRate)) / 1000U);

    if ((_LengthInSamples != 0U) && (SeekTime >= (_LengthInSamples - _SampleRate)))
    {
        _IsEndOfContainer = true;
        return;
    }

#ifdef DXISUPPORT
    if (_PlayerType == PlayerTypeDirectX)
    {
        dxiProxy->setPosition(seek_msec);

        samples_done = done;

        return;
    }
    else
#endif
    if (_Player)
    {
        _Player->Seek(SeekTime);
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
            assert(_countof(PlayerTypeNames) == (PlayerTypeMax + 1));

            const char * PlayerName = "Unknown";

            if (_PlayerType <= PlayerTypeMax)
                PlayerName = PlayerTypeNames[_PlayerType];
            else
                PlayerName = "VSTi";

            fileInfo.info_set(TagMIDIPlayer, PlayerName);
        }

        {
            if (_ExtraPercussionChannel != ~0L)
            {
                char Text[4];

                _itoa_s((int)(_ExtraPercussionChannel + 1), Text, 10);
                fileInfo.info_set(TagExtraPercusionChannel, Text);
            }
        }

        _IsFirstChunk = false;

        Success = true;
        timestampDelta = 0.;
    }

    if (_PlayerType == PlayerTypeBASSMIDI)
    {
        auto Player = (BMPlayer *)_Player;

        size_t VoiceCount = Player->GetActiveVoiceCount();

        if (VoiceCount != _BASSMIDIVoiceCount)
        {
            fileInfo.info_set_int(TagBASSMIDIVoiceCount, (t_int64) VoiceCount);

            _BASSMIDIVoiceCount = VoiceCount;

            Success = true;
        }

        if (VoiceCount > _BASSMIDIVoiceMax)
        {
            fileInfo.info_set_int(TagBASSMIDIVoicesMax, (t_int64) VoiceCount);

            _BASSMIDIVoiceMax = VoiceCount;

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

    // Calculate the total length of play back taking into account any looping and fading.
    {
        uint32_t LengthInMs = _Container.GetDuration(subSongIndex, true);

        double LengthInSeconds = (double) LengthInMs * 0.001;

        if (_LoopTypeOther == NeverLoopAdd1sDecay)
            LengthInSeconds += 1.;

        if ((_LoopTypeOther > LoopAndFadeWhenDetected) || !_LoopInTicks.IsEmpty())
        {
            if (!_LoopInMs.HasBegin())
                _LoopInMs.SetBegin(0);

            if (!_LoopInMs.HasEnd())
                _LoopInMs.SetEnd(LengthInMs);

            LengthInSeconds = (double) (_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount) + _FadeDuration) * 0.001;
        }

        fileInfo.set_length(LengthInSeconds);
    }
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
    _LoopInTicks.Set(_Container.GetLoopBeginTimestamp(subSongIndex), _Container.GetLoopEndTimestamp(subSongIndex));
    _LoopInMs.Set(_Container.GetLoopBeginTimestamp(subSongIndex, true), _Container.GetLoopEndTimestamp(subSongIndex, true));

    // Calculate the total length of play back taking into account any looping and fading.
    {
        uint32_t LengthInMs = _Container.GetDuration(subSongIndex, true);

        _LengthInSamples = (uint32_t) (((__int64) LengthInMs * (__int64) _SampleRate) / 1000U);

        if (_LoopType == NeverLoopAdd1sDecay)
            _LengthInSamples += _SampleRate;

        if ((_LoopType > LoopAndFadeWhenDetected) || !_LoopInTicks.IsEmpty())
        {
            if (!_LoopInMs.HasBegin())
                _LoopInMs.SetBegin(0);

            if (!_LoopInMs.HasEnd())
                _LoopInMs.SetEnd(LengthInMs);

//          _LengthInSamples = ((_LoopInMs.Begin() + (_LoopInMs.Size() * _LoopCount) + _FadeDuration) * _SampleRate) / 1000U;
        }
    }
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
#ifdef SF2PACK
        "sf2pack",
        "sfogg",
#endif
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

    KaraokeProcessor kp;

    {
        MIDIMetaData MetaData;

        _Container.GetMetaData(subSongIndex, MetaData);

        MIDIMetaDataItem Item;

        {
            bool HasTitle = MetaData.GetItem("title", Item);

            for (size_t i = 0; i < MetaData.GetCount(); ++i)
            {
                const MIDIMetaDataItem& mdi = MetaData[i];

            #ifdef _DEBUG
//              console::print(mdi.Name.c_str(), ":", mdi.Value.c_str());
            #endif

                if (pfc::stricmp_ascii(mdi.Name.c_str(), "type") == 0)
                {
                    fileInfo.info_set(TagMIDIType, mdi.Value.c_str());

                    _IsMT32 = (Item.Value == "MT-32");
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

    unsigned char d1 = (unsigned char)data[0];
    unsigned char d2 = (unsigned char)data[1];

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
