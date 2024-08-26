
/** $VER: BASSInitializer.h (2024.08.25) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "SoundFontCache.h"

#include <bassmidi.h>

#pragma warning(disable: 4820) // x bytes padding added after data member
class BASSInitializer
{
public:
    BASSInitializer() : _IsInitialized(false)
    {
    }

    ~BASSInitializer()
    {
        if (_IsInitialized)
            CacheDispose();
    }

    bool IsInitialized()
    {
        insync(_Lock);

        return _IsInitialized;
    }

    bool Initialize()
    {
        insync(_Lock);

        if (!_IsInitialized)
        {
            SetBasePath();

            LoadPlugIn("bassflac.dll");
            LoadPlugIn("basswv.dll");
            LoadPlugIn("bassopus.dll");
            LoadPlugIn("bass_mpc.dll");

            ::BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
            ::BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

            _IsInitialized = BASS_Init(0, 44100, 0, 0, NULL);

            if (!_IsInitialized)
                _IsInitialized = (::BASS_ErrorGetCode() == BASS_ERROR_ALREADY);

            if (_IsInitialized)
            {
                ::BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, (const void *) 0);
                ::BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, 256);

                CacheInit();
            }
        }

        return _IsInitialized;
    }

    /// <summary>
    /// Sets the base path of the BASS library files.
    /// </summary>
    void SetBasePath()
    {
        _BasePath = core_api::get_my_full_path();

        size_t slash = _BasePath.find_last_of('\\');

        _BasePath.erase(_BasePath.begin() + (const __int64)(slash + 1), _BasePath.end());
    }

    /// <summary>
    /// Loads a BASS plugin.
    /// </summary>
    void LoadPlugIn(const char * fileName)
    {
        std::string PathName = _BasePath;

        PathName += fileName;

        ::BASS_PluginLoad((const char *) pfc::stringcvt::string_os_from_utf8(PathName.c_str()).get_ptr(), BASS_UNICODE);
    }

private:
    critical_section _Lock;

    std::string _BasePath;

    bool _IsInitialized;
};
#pragma warning(default: 4820) // x bytes padding added after data member
