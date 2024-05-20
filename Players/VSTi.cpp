
/** $VER: VSTi.cpp (2024.05.20) P. Stuer **/

#include "framework.h"

#include <sdk/foobar2000-lite.h>

#include "Configuration.h"
#include "VSTi.h"

#include "VSTiPlayer.h"

pfc::array_t<VSTi::plugin_t> VSTi::PlugIns;
std::vector<uint8_t> VSTi::Config;

/// <summary>
/// Gets all VTSi from the specified path.
/// </summary>
void VSTi::Enumerate(const char * pathName, uFindFile * findFile)
{
    pfc::string8 DirectoryPath;

    if (findFile == nullptr)
    {
        PlugIns.set_size(0);

        AdvCfgVSTiPluginDirectoryPath.get(DirectoryPath);

        if (DirectoryPath.is_empty())
            return;

        console::print("Enumerating VST instruments...");

        DirectoryPath = pfc::io::path::combine(DirectoryPath, "*.*");

        pathName = DirectoryPath;

        findFile = ::uFindFirstFile(DirectoryPath);
    }

    if (findFile == nullptr)
        return;

    do
    {
        pfc::string8 PathName(pathName);

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
                console::print("Examining \"", PathName, "\"...");

                VSTiPlayer Player;

                if (Player.LoadVST(PathName))
                {
                    plugin_t Plugin;

                    Plugin.PathName = PathName;

                    pfc::string8 VendorName;

                    Player.GetVendorName(VendorName);

                    pfc::string8 ProductName;

                    Player.GetProductName(ProductName);

                    // Get the plugin name.
                    {
                        Plugin.Name = "VSTi ";

                        if (VendorName.length() || ProductName.length())
                        {
                            if ((VendorName.length() == 0) || ((ProductName.length() >= VendorName.length()) && (::strncmp(VendorName.c_str(), ProductName.c_str(), VendorName.length()) == 0)))
                            {
                                Plugin.Name += ProductName;
                            }
                            else
                            {
                                Plugin.Name += VendorName;

                                if (ProductName.length())
                                    Plugin.Name += std::string(' ' + ProductName);
                            }
                        }
                        else
                            Plugin.Name = findFile->GetFileName();
                    }

                    Plugin.Id = Player.GetUniqueID();
                    Plugin.HasEditor = Player.HasEditor();

                    PlugIns.append_single(Plugin);
                }
            }
        }
    }
    while (findFile->FindNext());

    delete findFile;
}
