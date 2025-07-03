
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
                PlugIn p;

                p.FilePath = Entry.path();

                std::string VendorName;

                Player.GetVendorName(VendorName);

                std::string ProductName;

                Player.GetProductName(ProductName);

                // Create the plugin name.
                {
                    if (!VendorName.empty() || !ProductName.empty())
                    {
                        p.Name = "";

                        if (VendorName.empty() || ((ProductName.length() >= VendorName.length()) && (::strncmp(VendorName.c_str(), ProductName.c_str(), VendorName.length()) == 0)))
                        {
                            p.Name += ProductName;
                        }
                        else
                        {
                            p.Name += VendorName;

                            if (!ProductName.empty())
                                p.Name += std::string(' ' + ProductName);
                        }
                    }
                    else
                        p.Name = (const char *) Entry.path().stem().u8string().c_str();
                }

                p.Id = Player.GetUniqueID();
                p.HasEditor = Player.HasEditor();

                _PlugIns.push_back(p);
            }
        }
    }
}

}
