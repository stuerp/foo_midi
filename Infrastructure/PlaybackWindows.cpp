#include "pch.h"

#include <sdk/play_callback.h>
#include <sdk/threadsLite.h>

#include "CLAPPlayer.h"
#include "Configuration.h"
#include "VSTiPlayer.h"

namespace
{
    class PlaybackWindows : public play_callback_static
    {
    public:
        unsigned get_flags() override
        {
            return flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_dynamic_info | flag_on_playback_time;
        }

        void on_playback_new_track(metadb_handle_ptr track) override
        {
            Reset();
            _Track = track;
        }

        void on_playback_stop(play_control::t_stop_reason) override
        {
            Reset();
        }

        void on_playback_dynamic_info(const file_info & info) override
        {
            if (_Track.is_empty())
                return;

            // Use the decoder's player, including per-track presets and MIDI overrides.
            const char * Player = info.info_get(InfoMIDIPlayer);
            if (!Player)
                return;

            _PlayerType = PlayerType::Unknown;
            if (std::strcmp(Player, "VSTi") == 0)
                _PlayerType = PlayerType::VSTi;
            else if (std::strcmp(Player, "CLAP") == 0)
                _PlayerType = PlayerType::CLAP;
            else
                return;

            fb2k::inMainThread([this, Generation = _Generation]
            {
                if (Generation == _Generation)
                    ShowPlaybackEditor();
            });
        }

        void on_playback_starting(play_control::t_track_command, bool) override { }
        void on_playback_seek(double) override { }
        void on_playback_pause(bool) override { }
        void on_playback_edited(metadb_handle_ptr) override { }
        void on_playback_dynamic_info_track(const file_info &) override { }
        void on_playback_time(double) override
        {
            // A device change can replace the decoder while foobar2000 keeps the current track.
            ShowPlaybackEditor();
        }
        void on_volume_change(float) override { }

    private:
        void ShowPlaybackEditor()
        {
            const bool ShowVSTi = _PlayerType == PlayerType::VSTi && CfgShowVSTiDuringPlayback;
            const bool ShowCLAP = _PlayerType == PlayerType::CLAP && CfgShowCLAPDuringPlayback;
            if ((!ShowVSTi && !ShowCLAP) || core_api::is_shutting_down())
                return;

            metadb_handle_ptr Current;
            if (!playback_control::get()->get_now_playing(Current) || Current != _Track)
                return;

            if (ShowVSTi)
                VSTi::Player::OpenPlaybackEditor({}, false);
            else
                CLAPPlayer::OpenPlaybackEditor({}, 0, false);
        }

        void Reset()
        {
            ++_Generation;
            _PlayerType = PlayerType::Unknown;
            _Track.release();
            CLAPPlayer::ClosePlaybackEditor();
        }

        uint32_t _Generation = 0;
        PlayerType _PlayerType = PlayerType::Unknown;
        metadb_handle_ptr _Track;
    };

    play_callback_static_factory_t<PlaybackWindows> PlaybackWindowsFactory;
}
