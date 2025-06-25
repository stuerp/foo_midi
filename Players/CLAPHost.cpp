
/** $VER: CLAPHost.cpp (2025.06.25) P. Stuer **/

#include "pch.h"

#include <sdk/foobar2000-lite.h>

#include "Configuration.h"
#include "resource.h"
#include "CLAPHost.h"
#include "CLAPPlayer.h"

#include <clap/clap.h>

namespace foo_midi
{

/// <summary>
/// Gets the CLAP plug-ins.
/// </summary>
void clap_host_t::GetPlugIns(const fs::path & directoryPath)
{
    if (directoryPath.empty())
        return;

    for (const auto & Entry : std::filesystem::directory_iterator(directoryPath))
    {
        if (Entry.is_directory())
        {
            GetPlugIns(Entry.path());
        }
        else
        if ((Entry.path().extension() == ".clap") || (Entry.path().extension() == ".dll"))
        {
            console::print(STR_COMPONENT_BASENAME " is examining \"", Entry.path().string().c_str(), "\"...");

            GetPlugIns(Entry.path(), [Entry](const std::string & name, uint32_t index)
            {
                plugin_t PlugIn = { "CLAP " + name, index, Entry.path().string() };

                PlugIns.push_back(PlugIn);
            });
        }
    }
}

/// <summary>
/// Gets the CLAP plug-ins in plug-in file.
/// </summary>
void clap_host_t::GetPlugIns(const fs::path & filePath, const std::function<void(const std::string & name, uint32_t index)> & callback) noexcept
{
    HMODULE hPlugIn = ::LoadLibraryA(filePath.string().c_str());

    if (hPlugIn == NULL)
        return;

    PlugIns.clear();

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(hPlugIn, "clap_entry");

    if ((Entry != nullptr) && (Entry->init != nullptr) && Entry->init(filePath.string().c_str()))
    {
        const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if (Factory != nullptr)
        {
            uint32_t PlugInCount = Factory->get_plugin_count(Factory);

            for (uint32_t i = 0; i < PlugInCount; ++i)
            {
                const auto * Descriptor = Factory->get_plugin_descriptor(Factory, i);

                callback(Descriptor->name, i);
            }
        }

        Entry->deinit();
    }

    ::FreeLibrary(hPlugIn);
}

std::vector<clap_host_t::plugin_t> clap_host_t::PlugIns;
}
