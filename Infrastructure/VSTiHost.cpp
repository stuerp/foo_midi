
/** $VER: VSTiHost.cpp (2025.07.12) P. Stuer **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "VSTiHost.h"
#include "VSTiPlayer.h"
#include "Log.h"

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
    try
    {
        for (const auto & Entry : fs::directory_iterator(directoryPath))
        {
            if (Entry.is_directory())
            {
                GetPlugIns_(Entry.path());
            }
            else
            if ((Entry.path().extension() == ".dll") || (Entry.path().extension() == ".vst2") || (Entry.path().extension() == ".vst3"))
            {
                Log.AtDebug().Format(STR_COMPONENT_BASENAME " is examining \"%s\"...", (const char *) Entry.path().u8string().c_str());

                Player Player;

                if (Player.LoadVST(Entry.path()))
                {
                    std::string Name;

                    // Create the plug-in name.
                    {
                        if (!Player.ProductName.empty())
                        {
                            if (Player.ProductName.starts_with(Player.VendorName))
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

                        if (Name.empty())
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
    catch (std::exception e)
    {
        Log.AtError().Format(STR_COMPONENT_BASENAME " failed to get VSTi plug-ins: %s", e.what());
    }
}

}
