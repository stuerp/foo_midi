
/** $VER: BASSInitializer.h (2025.03.19) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "SoundFontCache.h"
#include "resource.h"

#include <bassmidi.h>

#include <pfc/pathUtils.h>

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
            _BasePath = pfc::io::path::getParent( core_api::get_my_full_path());

            LoadPlugIn("bassflac.dll");
            LoadPlugIn("basswv.dll");
            LoadPlugIn("bassopus.dll");
            LoadPlugIn("bass_mpc.dll");

            ::BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
            ::BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

            _IsInitialized = ::BASS_Init(0, 44100, 0, 0, NULL);

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
    /// Loads a BASS plugin.
    /// </summary>
    void LoadPlugIn(const char * fileName)
    {
        pfc::string PathName = pfc::io::path::combine(_BasePath, fileName);

        if (::BASS_PluginLoad((const char *) pfc::stringcvt::string_os_from_utf8(PathName).get_ptr(), BASS_UNICODE) == 0)
            console::print(STR_COMPONENT_BASENAME, " failed to load plugin \"", fileName, "\": Error %d.", ::BASS_ErrorGetCode());
    }

private:
    critical_section _Lock;

    pfc::string _BasePath;

    bool _IsInitialized;
};
#pragma warning(default: 4820) // x bytes padding added after data member
