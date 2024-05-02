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
#include "CabbageParser.h"
#include "Cabbage.h"


class TimerThread {
public:
    TimerThread() : mStop(false) {}

    // Start the thread with a member function callback and a timer interval
        template <typename T>
        void Start(T* obj, void (T::*memberFunc)(), int intervalMillis) {
            mThread = std::thread([=]() {
                while (!mStop) {
                    // Call the member function on the object instance
                    (obj->*memberFunc)();
                    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMillis)); // Sleep for the specified interval
                    //std::cout << "Timer tick" << std::endl; // Debug output
                }
                //std::cout << "Timer stopped" << std::endl; // Debug output
            });
        }


    // Stop the timer thread
    void Stop() {
        mStop = true;
        if (mThread.joinable()) {
            mThread.join();
        }
    }

private:
    std::thread mThread;
    std::atomic<bool> mStop;
};


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
    
    std::function<void(CabbageOpcodeData)> hostCallback = nullptr;
private:
    
    TimerThread timer;
    void timerCallback();
    std::string host = {"127.0.0.1"};

    Cabbage cabbage;
    int csndIndex = 0;
    int csdKsmps = 0;
    int pos = 0;
    
    
};
