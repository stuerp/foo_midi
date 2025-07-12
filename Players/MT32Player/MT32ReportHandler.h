
/** $VER: MT32ReportHandler.h (2025.07.12) **/

#pragma once

#include "pch.h"

#include <mt32emu.h>

#include "Resource.h"
#include "Log.h"

class ReportHandler : public MT32Emu::IReportHandlerV1
{
public:
    virtual ~ReportHandler() { }

    // IReportHandler
    virtual void printDebug(const char * format, va_list args) override
    {
        char Line[1024] = { };

        (void) ::vsnprintf(Line, sizeof(Line) - 1, format, args);

        Log.AtDebug().Format(STR_COMPONENT_BASENAME " MT-32 Player says %s.", Line);
    };

    virtual void onErrorControlROM() override { };
    virtual void onErrorPCMROM() override { };

    virtual void showLCDMessage(const char * message) override
    {
        Log.AtInfo().Format(STR_COMPONENT_BASENAME " MT-32 Player says %s.", message);
    }

    virtual void onMIDIMessagePlayed() override { };
    virtual bool onMIDIQueueOverflow() override { return false; };
    virtual void onMIDISystemRealtime(MT32Emu::Bit8u system_realtime) override { };
    virtual void onDeviceReset() override { };
    virtual void onDeviceReconfig() override { };
    virtual void onNewReverbMode(MT32Emu::Bit8u mode) override { };
    virtual void onNewReverbTime(MT32Emu::Bit8u time) override { };
    virtual void onNewReverbLevel(MT32Emu::Bit8u level) override { };
    virtual void onPolyStateChanged(MT32Emu::Bit8u part_num) override { };

    virtual void onProgramChanged(MT32Emu::Bit8u partNumber, const char * soundGroupName, const char * patchName) override
    {
        Log.AtTrace().Format(STR_COMPONENT_BASENAME " MT-32 Player says sound group %s, patch %s, part %d", soundGroupName, patchName, partNumber);
    };

    // IReportHandlerV1
    virtual void onLCDStateUpdated() override { };
    virtual void onMidiMessageLEDStateUpdated(bool ledState) override { };
};
