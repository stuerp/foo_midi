
/** $VER: VSTi.h (2024.05.20) **/

#pragma once

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Supports the VSTi player.
/// </summary>
class VSTi
{
public:
    static void Enumerate(const char * pathName = nullptr, uFindFile * findFile = nullptr);

public:
    struct plugin_t
    {
        std::string Name;
        std::string PathName;
        uint32_t Id;
        bool HasEditor;
    };

    static std::vector<plugin_t> PlugIns;
    static std::vector<uint8_t> Config;
};
