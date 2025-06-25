
/** $VER: CLAPHost.h (2025.06.25) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace foo_midi
{

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Supports the CLAP player.
/// </summary>
class clap_host_t
{
public:
    struct plugin_t
    {
        std::string Name;
        uint32_t Index;
        std::string PathName;
    };

    static void GetPlugIns(const fs::path & directoryPath);

public:
    static std::vector<plugin_t> PlugIns;

private:
    static void GetPlugIns(const fs::path & filePath, const std::function<void (const std::string & name, uint32_t id)> & callback) noexcept;
};

}
