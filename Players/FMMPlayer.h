
/** $VER: FMMPlayer.h (2025.06.19) - Wrapper for yuno's fmmidi **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

namespace midisynth
{
    namespace opn
    {
        class fm_note_factory;
    }
 
   class synthesizer;
}

class FMMPlayer : public player_t
{
public:
    FMMPlayer();

    virtual ~FMMPlayer();

    void SetProgramsFilePath(const std::wstring & programPath);

    static const std::string DefaultProgramsFileName;

private:
    virtual bool Startup();
    virtual void Shutdown();
    virtual void Render(audio_sample *, uint32_t);
    virtual bool Reset() { return false; }

    virtual void SendEvent(uint32_t);
    virtual void SendEvent(uint32_t, uint32_t time) { };

    std::wstring _ProgramsFilePath;

    midisynth::opn::fm_note_factory * _Factory;

    midisynth::synthesizer * _Synthesizers[4];
};
