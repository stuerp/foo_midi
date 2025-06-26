
/** $VER: CLAPHost.h (2025.06.25) P. Stuer **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace foo_midi
{

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Implements a host for CLAP plug-ins.
/// </summary>
class clap_host_t
{
public:
    static void GetPlugIns(const fs::path & directoryPath) noexcept;

public:
    struct plugin_t
    {
        std::string Name;
        uint32_t Index;
        std::string PathName;
    };

    static std::vector<plugin_t> PlugIns;

private:
    static void GetPlugIns(const fs::path & filePath, const std::function<void (const std::string & name, uint32_t id)> & callback) noexcept;
};

}
