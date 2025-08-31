#include "stdafx.h"

core_t::core_t()
{
    _hModule = 0;

    Dispose();
}

core_t::~core_t()
{
    Unload();
}

bool core_t::Load(const wchar_t * pathName) noexcept
{
    _hModule = (void *) ::LoadLibraryW(pathName);

    if (_hModule == NULL)
        return false;

    *(void **) &TG_initialize                     = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_initialize");
//  *(void **) &TG_terminate                      = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_terminate");
    *(void **) &TG_activate                       = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_activate");
//  *(void **) &TG_deactivate                     = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_deactivate");
    *(void **) &TG_setSampleRate                  = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_setSampleRate");
    *(void **) &TG_setMaxBlockSize                = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_setMaxBlockSize");
    *(void **) &TG_flushMidi                      = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_flushMidi");
    *(void **) &TG_setInterruptThreadIdAtThisTime = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_setInterruptThreadIdAtThisTime");
//  *(void **) &TG_PMidiIn                        = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_PMidiIn");
    *(void **) &TG_ShortMidiIn                    = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_ShortMidiIn");
    *(void **) &TG_LongMidiIn                     = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_LongMidiIn");
//  *(void **) &TG_isFatalError                   = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_isFatalError");
    *(void **) &TG_getErrorStrings                = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_getErrorStrings");
    *(void **) &TG_XPgetCurTotalRunningVoices     = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_XPgetCurTotalRunningVoices");
    *(void **) &TG_XPsetSystemConfig              = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_XPsetSystemConfig");
    *(void **) &TG_XPgetCurSystemConfig           = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_XPgetCurSystemConfig");
    *(void **) &TG_Process                        = (void *) ::GetProcAddress((HMODULE) _hModule, "TG_Process");

    if (TG_initialize && TG_activate && TG_setSampleRate && TG_setMaxBlockSize && TG_flushMidi && TG_setInterruptThreadIdAtThisTime && TG_ShortMidiIn && TG_LongMidiIn && TG_XPgetCurTotalRunningVoices && TG_Process)
        return true;

    Dispose(); 

    return false;
}

void core_t::Unload() noexcept
{
    Dispose();
}

void core_t::Dispose() noexcept
{
    TG_initialize = nullptr;
    TG_terminate = nullptr;
    TG_activate = nullptr;
    TG_deactivate = nullptr;
    TG_setSampleRate = nullptr;
    TG_setMaxBlockSize = nullptr;
    TG_flushMidi = nullptr;
    TG_setInterruptThreadIdAtThisTime = nullptr;
    TG_PMidiIn = nullptr;
    TG_ShortMidiIn = nullptr;
    TG_LongMidiIn = nullptr;
    TG_isFatalError = nullptr;
    TG_getErrorStrings = nullptr;
    TG_XPgetCurTotalRunningVoices = nullptr;
    TG_XPsetSystemConfig = nullptr;
    TG_XPgetCurSystemConfig = nullptr;
    TG_Process = nullptr;

    if (_hModule)
    {
        ::FreeLibrary((HMODULE) _hModule);
        _hModule = nullptr;
    }
}
