
/** $VER: rcpmain.cpp (2025.09.06) P. Stuer **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "RCP\RCP.h"

using namespace rcp;

/// <summary>
/// 
/// </summary>
int rcpmain(int argc, wchar_t * argv[])
{
    int Result = 0;

    if (argc < 3)
    {
        ::printf("Usage: rcpdump.exe [options] input.bin output.mid\n");
        ::printf("Input file formats:\n");
        ::printf("    RCP/R36/G36 Recomposer sequence file\n");
        ::printf("    CM6         Recomposer MT-32/CM-64 control file\n");
        ::printf("    GSD         Recomposer SC-55 control file\n");
        ::printf("Output file formats:\n");
        ::printf("    Sequence files are converted into MIDIs.\n");
        ::printf("    Control files can be converted to raw SYX or MIDI.\n");
        ::printf("        The file extension of the output file specifies the format.");
        ::printf("\n");
        ::printf("Options:\n");
        ::printf("    -Loops n            Loop each track at least n times. (default: 2)\n");
        ::printf("    -NoLoopExtension    No Loop Extension. Do not fill short tracks to the length of longer ones.\n");
        ::printf("    -WolfteamLoop       Wolfteam Loop mode (loop from measure 2 on)\n");
        ::printf("    -KeepMutedChannels  Convert data with MIDI channel set to -1. Some RCPs use it for muting.\n");

        return 0;
    }

    converter_t RCPConverter;

    converter_options_t & Options = RCPConverter._Options;

    int argbase = 1;

    {
        while (argbase < argc && argv[argbase][0] == '-')
        {
            if (!::_wcsicmp(argv[argbase] + 1, L"Loops"))
            {
                argbase++;

                if (argbase < argc)
                {
                    Options._RCPLoopCount = (uint16_t) ::wcstoul(argv[argbase], nullptr, 0);

                    if (Options._RCPLoopCount == 0)
                        Options._RCPLoopCount = 2;
                }
            }
            else
            if (!::_wcsicmp(argv[argbase] + 1, L"NoLoopExtension"))
                Options._ExtendLoops = true;
            else
            if (!::_wcsicmp(argv[argbase] + 1, L"WolfteamLoop"))
                Options._WolfteamLoopMode = true;
            else
            if (!::_wcsicmp(argv[argbase] + 1, L"KeepMutedChannels"))
                Options._KeepMutedChannels = true;
            else
                break;

            argbase++;
        }

        if (argc < argbase + 1)
        {
            ::printf("Not enough arguments.\n");
            return 0;
        }
    }

    time_t Time;
	
    ::time(&Time);

    tm TM;

    ::localtime_s(&TM, &Time);

    char TimeText[32]; ::strftime(TimeText, _countof(TimeText), "%Y-%m-%d %H:%M:%S", &TM);

    const std::wstring FilePath(argv[argbase]);

    RCPConverter.SetFilePath(FilePath);

    ::printf("\n%s, \"%S\"\n", TimeText, FilePath.c_str());

    buffer_t SrcData;

    try
    {
        SrcData.ReadFile(FilePath.c_str());

        buffer_t DstData;

        std::wstring FileExtension;

        if (argv[argbase + 1] != nullptr)
        {
            fs::path Path(argv[argbase + 1]);

            FileExtension = Path.extension().wstring();

            if (!FileExtension.empty() && FileExtension[0] == L'.')
                FileExtension = FileExtension.substr(1);
        }

        RCPConverter.ToSMF(SrcData, DstData, FileExtension);

        if (argc >= argbase + 2)
        {
            DstData.WriteFile(argv[argbase + 1]);

            #ifdef _RCP_VERBOSE
            {
                ::printf("Src: %d bytes, Dst: %d bytes\n", SrcData.Size, DstData.Size);

                fs::path RefFilePath(FilePath);

                RefFilePath.replace_extension(L".mid");

                {
                    buffer_t RefFile;

                    try
                    {
                        RefFile.ReadFile(RefFilePath.c_str());
                    }
                    catch (std::runtime_error & e)
                    {
                        std::string Text(msc::FormatText("Reference file \"%s\" not found: ", msc::WideToUTF8(RefFilePath).c_str()));

                        throw std::runtime_error(Text + e.what());
                    }
    
                    if (RefFile.Size != DstData.Size)
                        throw std::runtime_error(msc::FormatText("Conversion error in \"%s\". File size mismatch (%u != %u)", msc::WideToUTF8(FilePath).c_str(), RefFile.Size, DstData.Size));

                    if (::memcmp(RefFile.Data, DstData.Data, RefFile.Size) != 0)
                        throw std::runtime_error(msc::FormatText("Conversion error in \"%s\". File content mismatch", msc::WideToUTF8(FilePath).c_str()));
                }

                midi::container_t Container;

                std::vector<uint8_t> Data;

                Data.insert(Data.end(), DstData.Data, DstData.Data + DstData.Size);

                if (!midi::processor_t::Process(Data, FilePath.c_str(), Container))
                    throw std::runtime_error(msc::FormatText("MIDIProcesser failed: \"%s\"", msc::WideToUTF8(FilePath).c_str()));
            }
            #endif
        }
    }
    catch (std::exception & e)
    {
        ::printf("Exception: %s\n", e.what());
    }

    ::putwchar('\n');

    return Result;
}
