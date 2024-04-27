#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include <iostream>
#include "APP/IPlugAPP.h"
#include "csound.hpp"
#include <cassert>
#include <filesystem>
 
// Use (void) to silence unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))

using namespace iplug;

const int kNumPresets = 3;

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

class ICabbage final : public Plugin
{
public:
    ICabbage(const InstanceInfo& info);
    ~ICabbage();
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    void ProcessMidiMsg(const IMidiMsg& msg) override;
    void OnReset() override;
    void OnIdle() override;
    bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
    void OnParamChange(int paramIdx) override;
    
    //Csound API functions for deailing with midi input
    static int OpenMidiInputDevice (CSOUND* csnd, void** userData, const char* devName);
    static int OpenMidiOutputDevice (CSOUND* csnd, void** userData, const char* devName);
    static int ReadMidiData (CSOUND* csound, void* userData, unsigned char* mbuf, int nbytes);
    static int WriteMidiData (CSOUND* csound, void* userData, const unsigned char* mbuf, int nbytes);
    
    bool setupAndStartCsound();
    
    void setCsdFile(std::string file)
    {
        csdFile = file;
    }
    
    void compileCsdFile (std::string csoundFile)
    {
        csCompileResult = csound->Compile (csoundFile.c_str());
    }
    
    bool csdCompiledWithoutError()
    {
        return csCompileResult == 0 ? true : false;
    }
    
private:
    std::string host = {"127.0.0.1"};
    ix::WebSocketServer server;
    int samplePosForMidi = 0;
    std::string csoundOutput = {};
    std::unique_ptr<CSOUND_PARAMS> csoundParams;
    int csCompileResult = -1;
    int numCsoundOutputChannels = 0;
    int numCsoundInputChannels = 0;
    int pos = 0;
    MYFLT csScale = 0.0;
    MYFLT *csSpin = nullptr;
    MYFLT *csSpout = nullptr;
    int samplingRate = 44100;
    int csndIndex = 0;
    int csdKsmps = 0;
    std::string csdFile = {}, csdFilePath = {};
    std::unique_ptr<Csound> csound;
    
};
