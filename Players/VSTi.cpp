
/** $VER: VSTi.cpp (2025.02.24) P. Stuer **/

#include "pch.h"

#include <sdk/foobar2000-lite.h>

#include "Configuration.h"
#include "resource.h"
#include "VSTi.h"

#include "VSTiPlayer.h"

std::vector<VSTi::plugin_t> VSTi::PlugIns;
std::vector<uint8_t> VSTi::Config;

/// <summary>
/// Gets all VTSi from the specified path.
/// </summary>
void VSTi::Enumerate(const char * pathName, uFindFile * findFile)
{
    pfc::string DirectoryPath;

    if (findFile == nullptr)
    {
        PlugIns.clear();

        AdvCfgVSTiPluginDirectoryPath.get(DirectoryPath);

        if (DirectoryPath.is_empty())
            return;

        console::print(STR_COMPONENT_BASENAME " is enumerating VST instruments...");

        DirectoryPath = pfc::io::path::combine(DirectoryPath, "*.*");

        pathName = DirectoryPath;

        findFile = ::uFindFirstFile(DirectoryPath);
    }

    if (findFile == nullptr)
        return;

    do
    {
        pfc::string PathName(pathName);

        PathName.truncate(PathName.length() - 3);
        PathName += findFile->GetFileName();

        // Enter all subdirectories to look voor plug-ins.
        if (findFile->IsDirectory() && ::strcmp(findFile->GetFileName(), ".") && ::strcmp(findFile->GetFileName(), ".."))
        {
            PathName = pfc::io::path::combine(PathName, "*.*");

            uFindFile * FindFile = ::uFindFirstFile(PathName);

            if (FindFile)
                Enumerate(PathName, FindFile);
        }
        else
        {
            if ((PathName.length() < 5) || (pfc::stricmp_ascii(PathName.get_ptr() + PathName.length() - 4, ".dll") != 0))
                continue;

            // Examine all DLL files.
            if (findFile->GetFileSize() != 0)
            {
                console::print(STR_COMPONENT_BASENAME " is examining \"", PathName, "\"...");

                VSTiPlayer Player;

                if (Player.LoadVST(PathName))
                {
                    plugin_t Plugin;

                    Plugin.PathName = PathName;

                    std::string VendorName;

                    Player.GetVendorName(VendorName);

                    std::string ProductName;

                    Player.GetProductName(ProductName);

                    // Create the plugin name.
                    {
                        Plugin.Name = "VSTi ";

                        if ((VendorName.length() != 0) || (ProductName.length() != 0))
                        {
                            if ((VendorName.length() == 0) || ((ProductName.length() >= VendorName.length()) && (::strncmp(VendorName.c_str(), ProductName.c_str(), VendorName.length()) == 0)))
                            {
                                Plugin.Name += ProductName;
                            }
                            else
                            {
                                Plugin.Name += VendorName;

                                if (ProductName.length() != 0)
                                    Plugin.Name += std::string(' ' + ProductName);
                            }
                        }
                        else
                            Plugin.Name = findFile->GetFileName();
                    }

                    Plugin.Id = Player.GetUniqueID();
                    Plugin.HasEditor = Player.HasEditor();

                    PlugIns.push_back(Plugin);
                }
            }
        }
    }
    while (findFile->FindNext());

    delete findFile;
}
