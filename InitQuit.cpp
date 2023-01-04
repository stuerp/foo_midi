
/** $VER: InitQuit.cpp (2023.01.04) **/

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>

#include <sdk/initquit.h>

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include "Configuration.h"
#include "FileHasher.h"

#pragma warning(disable: 26409)

class InitQuitHandler : public initquit
{
public:
    InitQuitHandler() noexcept { };
    InitQuitHandler(const InitQuitHandler&) = delete;
    InitQuitHandler(const InitQuitHandler&&) = delete;
    InitQuitHandler& operator=(const InitQuitHandler&) = delete;
    InitQuitHandler& operator=(InitQuitHandler&&) = delete;
    virtual ~InitQuitHandler() { };

    void on_init() noexcept override
    {
        try
        {       
            static_api_ptr_t<metadb_index_manager>()->add(new service_impl_t<FileHasher>, GUIDTagMIDIHash, system_time_periods::week * 4);
        }
        catch (...) { }
    }

    void on_quit() noexcept override
    {
    }
};

static initquit_factory_t<InitQuitHandler> g_initquit_midi_factory;
