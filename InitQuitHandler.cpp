
/** $VER: InitQuit.cpp (2023.05.24) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>
#include <sdk/initquit.h>

#include "Configuration.h"
#include "FileHasher.h"
#include "InputDecoder.h"

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
        InputDecoder::InitializeIndexManager();
    }

    void on_quit() noexcept override
    {
    }
};

static initquit_factory_t<InitQuitHandler> _InitQuitHandlerFactory;
