
/** $VER: VSTiHost.cpp (2025.07.03) P. Stuer **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "VSTiHost.h"
#include "VSTiPlayer.h"

namespace VSTi
{

/// <summary>
/// Gets the VSTi plug-ins.
/// </summary>
std::vector<PlugIn> Host::GetPlugIns(const fs::path & directoryPath) noexcept
{
    _PlugIns.clear();

    if (directoryPath.empty())
        return _PlugIns;

    GetPlugIns_(directoryPath);

    std::sort(_PlugIns.begin(), _PlugIns.end(), [](PlugIn a, PlugIn b) { return a.Name < b.Name; });

    return _PlugIns;
}

/// <summary>
/// Gets the VSTi plug-ins.
/// </summary>
void Host::GetPlugIns_(const fs::path & directoryPath) noexcept
{
    for (const auto & Entry : fs::directory_iterator(directoryPath))
    {
        if (Entry.is_directory())
        {
            GetPlugIns_(Entry.path());
        }
        else
        if (Entry.path().extension() == ".dll")
        {
//          console::print(STR_COMPONENT_BASENAME " is examining \"", (const char *) Entry.path().u8string().c_str(), "\"...");

            Player Player;

            if (Player.LoadVST(Entry.path()))
            {
                std::string Name;

                {
                    // Create the plugin name.
                    if (!Player.VendorName.empty() || !Player.ProductName.empty())
                    {
                        if (Player.VendorName.empty() || ((Player.ProductName.length() >= Player.VendorName.length()) && (::strncmp(Player.VendorName.c_str(), Player.ProductName.c_str(), Player.VendorName.length()) == 0)))
                        {
                            Name = Player.ProductName;
                        }
                        else
                        {
                            Name = Player.VendorName;

                            if (!Player.ProductName.empty())
                                Name += std::string(' ' + Player.ProductName);
                        }
                    }
                    else
                        Name = (const char *) Entry.path().stem().u8string().c_str();
                }

                PlugIn p =
                {
                    .Name = Name,
                    .FilePath = Entry.path(),
                    .Id = Player.Id,
                    .HasEditor = Player.HasEditor()
                };

                _PlugIns.push_back(p);
            }
        }
    }
}

}
