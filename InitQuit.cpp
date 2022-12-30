
/** $VER: InitQuit.cpp (2022.12.30) **/

#include <sdk/foobar2000-lite.h>

#include <sdk/initquit.h>

#include <sdk/hasher_md5.h>
#include <sdk/metadb_index.h>
#include <sdk/system_time_keeper.h>

#include "TrackHasher.h"

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

    void on_init() override
    {
        static_api_ptr_t<metadb_index_manager>()->add(new service_impl_t<TrackHasher>, guid_midi_index, system_time_periods::week * 4);
    }

    void on_quit() noexcept override
    {
    }
};

static initquit_factory_t<InitQuitHandler> g_initquit_midi_factory;
