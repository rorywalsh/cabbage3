#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"

#include <iostream>
#include "APP/IPlugAPP.h"

#include <cassert>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

#include "CabbageParser.h"
#include "Cabbage.h"
// Use (void) to silence unused warnings.



enum EParams
{
    kGain = 0,
    kNumParams
};

enum EMsgTags
{
    kMsgTagButton1 = 0,
    kMsgTagButton2 = 1,
    kMsgTagButton3 = 2,
    kMsgTagBinaryTest = 3
};

enum EControlTags
{
    kCtrlTagMeter = 0,
};

class CabbageProcessor final : public iplug::Plugin
{
public:
    
    CabbageProcessor(const iplug::InstanceInfo& info, std::string csdFile);
    ~CabbageProcessor();
    void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) override;
    void ProcessMidiMsg(const iplug::IMidiMsg& msg) override;
    void OnReset() override;
    void OnIdle() override;
    bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
    void OnParamChange(int paramIdx) override;
    
    //Csound API functions for deailing with midi input
    static int OpenMidiInputDevice (CSOUND* csnd, void** userData, const char* devName);
    static int OpenMidiOutputDevice (CSOUND* csnd, void** userData, const char* devName);
    static int ReadMidiData (CSOUND* csound, void* userData, unsigned char* mbuf, int nbytes);
    static int WriteMidiData (CSOUND* csound, void* userData, const unsigned char* mbuf, int nbytes);
      
    void stopProcessing()
    {
        cabbage.stopProcessing();
    }
    
    Cabbage& getCabbage()
    {
        return cabbage;
    }
    
private:
    
    
    std::string host = {"127.0.0.1"};

    Cabbage cabbage;
    int csndIndex = 0;
    int csdKsmps = 0;
    int pos = 0;
    
    
};
