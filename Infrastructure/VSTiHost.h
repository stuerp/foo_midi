
/** $VER: VSTiHost.h (2025.07.03) **/

#pragma once

namespace VSTi
{

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Represents a VSTi plug-in.
/// </summary>
struct PlugIn
{
    std::string Name;
    fs::path FilePath;
    uint32_t Id;
    bool HasEditor;
};

/// <summary>
/// Supports the VSTi player.
/// </summary>
class Host
{
public:
    std::vector<PlugIn> GetPlugIns(const fs::path & directoryPath) noexcept;

private:
    void GetPlugIns_(const fs::path & directoryPath) noexcept;

public:
    std::vector<uint8_t> Config;

private:
    std::vector<PlugIn> _PlugIns;
};

}
