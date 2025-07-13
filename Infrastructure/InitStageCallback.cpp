
/** $VER: InitStageCallback.cpp (2025.07.13) **/

#include "pch.h"

#include <sdk/foobar2000-lite.h>
#include <sdk/initquit.h>

#include "InputDecoder.h"
#include "Log.h"

#include "PreferencesProcessing.h"

#pragma warning(disable: 26409)

class InitStageCallback : public init_stage_callback
{
public:
    InitStageCallback() noexcept { };

    InitStageCallback(const InitStageCallback&) = delete;
    InitStageCallback(const InitStageCallback&&) = delete;
    InitStageCallback& operator=(const InitStageCallback&) = delete;
    InitStageCallback& operator=(InitStageCallback&&) = delete;

    virtual ~InitStageCallback() { };

    void on_init_stage(t_uint32 stage) noexcept override
    {
        if (stage == init_stages::after_config_read)
        {
            Log.SetLevel((LogLevel) CfgLogLevel.get());
        }
        else
        if (stage == init_stages::after_ui_init)
        {
            Log.AtDebug().Format(STR_COMPONENT_BASENAME " is initializing the metadb index manager after the user interface has been initialized.");

            InputDecoder::InitializeIndexManager();
        }
    }
};

static service_factory_single_t<InitStageCallback> _Factory;
