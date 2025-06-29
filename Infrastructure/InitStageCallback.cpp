
/** $VER: InitStageCallback.cpp (2025.06.29) **/

#include "pch.h"

#include <sdk/foobar2000-lite.h>
#include <sdk/initquit.h>

#include "InputDecoder.h"
#include "CLAPHost.h"

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
        if (stage == init_stages::after_ui_init)
        {
            InputDecoder::InitializeIndexManager();

            if ((PlayerTypes) (uint32_t) CfgPlayerType == PlayerTypes::CLAP)
                CLAP::Host::GetInstance().Load(CfgPlugInFilePath.get().c_str(), (uint32_t) CfgCLAPIndex);
        }
    }
};

static service_factory_single_t<InitStageCallback> _InitStageCallbackFactory;
