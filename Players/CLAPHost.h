
/** $VER: CLAPHost.h (2025.06.27) P. Stuer **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <filesystem>

#include <clap/clap.h>

namespace fs = std::filesystem;

namespace CLAP
{

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Represents a CLAP plug-in.
/// </summary>
struct PlugIn
{
    std::string Name;
    uint32_t Index;
    fs::path FilePath;
    bool HasGUI;
};

/// <summary>
/// Implements a host for CLAP plug-ins.
/// </summary>
class Host
{
public:
    static void GetPlugIns(const fs::path & directoryPath) noexcept;

public:
    static std::vector<PlugIn> PlugIns;
    static const clap_host_t Parameters;

private:
    static void GetPlugIns(const fs::path & filePath, const std::function<void (const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept;

    static bool VerifyNotePorts(const clap_plugin_t * plugIn) noexcept;
    static bool VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept;
    static bool HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept;
};

}
