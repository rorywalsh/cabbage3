#pragma once


#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#endif


#include "IPlug_include_in_plug_hdr.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <string>
#include <vector>
#include <regex>
#include "APP/IPlugAPP.h"
#include "IPlugPaths.h"
#include "CabbageParser.h"
#include "Cabbage.h"
#include <future> // For std::async and std::future
#include <mutex>  // For std::mutex and std::lock_guard

#ifndef CabbageApp
#include "CabbageServer.h"
#endif

#include <iostream>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

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
    
#ifdef CabbageApp
    CabbageProcessor(const iplug::InstanceInfo& info, std::string csdFile);
#else
    CabbageProcessor(const iplug::InstanceInfo& info);
    CabbageServer server;
#endif
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
    

    
    void updateUI()
    {
        
    }
    
    void stopProcessing()
    {
        cabbage.stopProcessing();
    }
    
    Cabbage& getCabbage()
    {
        return cabbage;
    }
    
    void setupCallbacks();
    void updateWidgetsOnUIOpen();
    
#ifdef CabbageApp
    std::function<void(CabbageOpcodeData)> hostCallback = nullptr;
#endif
    
private:
    
    TimerThread timer;
    std::mutex mMutex;
    void timerCallback();
    std::string host = {"127.0.0.1"};
    Cabbage cabbage;
    std::function<void(std::string)> triggerScript;
    int csndIndex = 0;
    int csdKsmps = 0;
    int pos = 0;
    bool uiIsOpen = false;
    bool firstTime = true;
    
    
};
