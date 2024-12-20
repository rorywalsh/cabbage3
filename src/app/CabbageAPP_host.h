/*
 * Copyright (C) the iPlug 2 developers, Rory Walsh (c) 2024
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 * 
 * Modifications made by Rory Walsh in 2024.
 * 
 * This file is based on the iPlug 2 library, which is licensed under the
 * [iPlug 2 License Information]. The original copyright notice and license
 * must remain intact in the portions of the code that have not been modified.
 */

#pragma once

#undef OK

#include <cstdlib>
#include <string>
#include <vector>
#include <limits>
#include <memory>

#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"

#include "config.h"
#if defined(CabbageApp)
    #include <ixwebsocket/IXWebSocketServer.h>
#endif

#ifdef OS_WIN
#include <WindowsX.h>
#include <commctrl.h>
#include <shlobj.h>
#define DEFAULT_INPUT_DEV "Default Device"
#define DEFAULT_OUTPUT_DEV "Default Device"
#elif defined(OS_MAC)
#import <Cocoa/Cocoa.h>
#include "IPlugSWELL.h"
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#define DEFAULT_INPUT_DEV "Built-in Input"
#define DEFAULT_OUTPUT_DEV "Built-in Output"
#elif defined(OS_LINUX)
#include "IPlugSWELL.h"
#endif

#include "../choc_DisableAllWarnings.h"
#include "RtAudio.h"
#include "RtMidi.h"
#include "../choc_ReenableAllWarnings.h"

#include "CabbageAPP.h"
#include "CabbageProcessor.h"


#define OFF_TEXT "off"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;

BEGIN_IPLUG_NAMESPACE

const int kNumBufferSizeOptions = 11;
const std::string kBufferSizeOptions[kNumBufferSizeOptions] = {"32", "64", "96", "128", "192", "256", "512", "1024", "2048", "4096", "8192" };
const int kDeviceDS = 0; const int kDeviceCoreAudio = 0; const int kDeviceAlsa = 0;
const int kDeviceASIO = 1; const int kDeviceJack = 1;
extern UINT gSCROLLMSG;

class IPlugAPP;

/** A class that hosts an IPlug as a standalone app and provides Audio/Midi I/O */
class IPlugAPPHost
{
public:
    
    /** Used to manage changes to app i/o */
    struct AppState
    {
        WDL_String mAudioInDev;
        WDL_String mAudioOutDev;
        WDL_String mMidiInDev;
        WDL_String mMidiOutDev;
        uint32_t mAudioDriverType;
        uint32_t mAudioSR;
        uint32_t mBufferSize;
        uint32_t mMidiInChan;
        uint32_t mMidiOutChan;
        
        uint32_t mAudioInChanL;
        uint32_t mAudioInChanR;
        uint32_t mAudioOutChanL;
        uint32_t mAudioOutChanR;
        
        //custom app state fields
        WDL_String mJsSourceDirectory;
        
        AppState()
        : mAudioInDev(DEFAULT_INPUT_DEV)
        , mAudioOutDev(DEFAULT_OUTPUT_DEV)
        , mMidiInDev(OFF_TEXT)
        , mMidiOutDev(OFF_TEXT)
        , mAudioDriverType(0) // DirectSound / CoreAudio by default
        , mBufferSize(512)
        , mAudioSR(44100)
        , mMidiInChan(0)
        , mMidiOutChan(0)
        
        , mAudioInChanL(1)
        , mAudioInChanR(2)
        , mAudioOutChanL(1)
        , mAudioOutChanR(2)
        {
        }
        
        AppState (const AppState& obj)
        : mAudioInDev(obj.mAudioInDev.Get())
        , mAudioOutDev(obj.mAudioOutDev.Get())
        , mMidiInDev(obj.mMidiInDev.Get())
        , mMidiOutDev(obj.mMidiOutDev.Get())
        , mAudioDriverType(obj.mAudioDriverType)
        , mBufferSize(obj.mBufferSize)
        , mAudioSR(obj.mAudioSR)
        , mMidiInChan(obj.mMidiInChan)
        , mMidiOutChan(obj.mMidiOutChan)
        
        , mAudioInChanL(obj.mAudioInChanL)
        , mAudioInChanR(obj.mAudioInChanR)
        , mAudioOutChanL(obj.mAudioInChanL)
        , mAudioOutChanR(obj.mAudioInChanR)
        {
        }
        
        bool operator==(const AppState& rhs) const {
            return (rhs.mAudioDriverType == mAudioDriverType &&
                    rhs.mBufferSize == mBufferSize &&
                    rhs.mAudioSR == mAudioSR &&
                    rhs.mMidiInChan == mMidiInChan &&
                    rhs.mMidiOutChan == mMidiOutChan &&
                    (strcmp(rhs.mAudioInDev.Get(), mAudioInDev.Get()) == 0) &&
                    (strcmp(rhs.mAudioOutDev.Get(), mAudioOutDev.Get()) == 0) &&
                    (strcmp(rhs.mMidiInDev.Get(), mMidiInDev.Get()) == 0) &&
                    (strcmp(rhs.mMidiOutDev.Get(), mMidiOutDev.Get()) == 0) &&
                    
                    rhs.mAudioInChanL == mAudioInChanL &&
                    rhs.mAudioInChanR == mAudioInChanR &&
                    rhs.mAudioOutChanL == mAudioOutChanL &&
                    rhs.mAudioOutChanR == mAudioOutChanR
                    
                    );
        }
        bool operator!=(const AppState& rhs) const { return !operator==(rhs); }
    };
    
#ifdef CabbageApp
    static IPlugAPPHost* Create(std::string filePath);
#else
    static IPlugAPPHost* Create();
#endif
    static std::unique_ptr<IPlugAPPHost> sInstance;
    
    void PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* pInputDevInfo, RtAudio::DeviceInfo* pOutputDevInfo);
    void PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
    void PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
    void PopulateDriverSpecificControls(HWND hwndDlg);
    void PopulateAudioDialogs(HWND hwndDlg);
    bool PopulateMidiDialogs(HWND hwndDlg);
    void PopulatePreferencesDialog(HWND hwndDlg);
    
#ifdef CabbageApp
    IPlugAPPHost(std::string csdFile);
#else
    IPlugAPPHost();
#endif
    ~IPlugAPPHost();
    
    bool OpenWindow(HWND pParent);
    void CloseWindow();
    
    bool Init();
    bool InitProcessor();
    bool InitWebSocket();
    bool InitState();
    void UpdateSettings();
    
    /** Returns the name of the audio device at idx
     * @param idx The index RTAudio has given the audio device
     * @return The device name. Core Audio device names are truncated. */
    WDL_String GetAudioDeviceName(int idx) const;
    // returns the rtaudio device ID, based on the (truncated) device name
    
    /** Returns the audio device index linked to a particular name
     * @param name The name of the audio device to test
     * @return The integer index RTAudio has given the audio device */
    int GetAudioDeviceIdx(const char* name) const;
    
    /** @param direction Either kInput or kOutput
     * @param name The name of the midi device
     * @return An integer specifying the output port number, where 0 means any */
    int GetMIDIPortNumber(ERoute direction, const char* name) const;
    
    /** find out which devices have input channels & which have output channels, add their ids to the lists */
    void ProbeAudioIO();
    void ProbeMidiIO();
    bool InitMidi();
    void CloseAudio();
    bool InitAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs);
    bool AudioSettingsInStateAreEqual(AppState& os, AppState& ns);
    bool MIDISettingsInStateAreEqual(AppState& os, AppState& ns);
    
    bool TryToChangeAudioDriverType();
    bool TryToChangeAudio();
    bool SelectMIDIDevice(ERoute direction, const char* portName);
    
    static int AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData);
    static void MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData);
    static void errorCallback(RtAudioErrorType type, const std::string& errorText);
    
#include <cstring>
#include <cctype>

    static const char* cleanDeviceName(const char* deviceName) {
        if (!deviceName) {
            return ""; // Handle null input gracefully
        }

        // Allocate a static buffer for the result
        static char cleanName[256]; // Adjust size as needed
        std::strncpy(cleanName, deviceName, sizeof(cleanName) - 1);
        cleanName[sizeof(cleanName) - 1] = '\0'; // Ensure null-termination

        // Find the colon in the string
        char* colonPos = std::strchr(cleanName, ':');
        if (colonPos) {
            // Skip the colon and point to the part after it
            std::memmove(cleanName, colonPos + 1, std::strlen(colonPos + 1) + 1);
        }

        // Remove leading spaces
        char* firstNonSpace = cleanName;
        while (*firstNonSpace && std::isspace(static_cast<unsigned char>(*firstNonSpace))) {
            ++firstNonSpace;
        }

        // Shift the string to remove leading spaces
        if (firstNonSpace != cleanName) {
            std::memmove(cleanName, firstNonSpace, std::strlen(firstNonSpace) + 1);
        }

        return cleanName;
    }

    void addDevicesToSettings( RtAudio& audio, nlohmann::json& settings);
    
    static WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    IPlugAPP* GetPlug() { return mIPlug.get(); }
    
    std::string getCsdFile(){   return csdFile; }
    
private:
    std::vector<nlohmann::json> parameters;
    std::unique_ptr<IPlugAPP> mIPlug = nullptr;
    std::unique_ptr<RtAudio> mDAC = nullptr;
    std::unique_ptr<RtMidiIn> mMidiIn = nullptr;
    std::unique_ptr<RtMidiOut> mMidiOut = nullptr;
    std::function<void(std::string, std::string)> updateStringChannelCallback = nullptr;
    int mMidiOutChannel = -1;
    int mMidiInChannel = -1;
    std::string csdFile;
#if defined CabbageApp
    void updateHost(CabbageOpcodeData data);
    ix::WebSocket webSocket;
#endif
    CabbageProcessor* cabbageProcessor;
    /**  */
    AppState mState;
    /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
    AppState mTempState;
    /** When the audio driver is started the current state is copied here so that if OK is pressed after APPLY nothing is changed */
    AppState mActiveState;
    
    double mSampleRate = 44100.;
    uint32_t mSamplesElapsed = 0;
    uint32_t mVecWait = 0;
    uint32_t mBufferSize = 512;
    uint32_t mBufIndex = 0; // index for signal vector, loops from 0 to mSigVS
    bool mExiting = false;
    bool mAudioEnding = false;
    bool mAudioDone = false;
    
    /** The index of the operating systems default input device, -1 if not detected */
    int32_t mDefaultInputDev = -1;
    /** The index of the operating systems default output device, -1 if not detected */
    int32_t mDefaultOutputDev = -1;

    WDL_String mJSONPath;
    
    std::vector<uint32_t> mAudioInputDevs;
    std::vector<uint32_t> mAudioOutputDevs;
    std::vector<WDL_String> mAudioIDDevNames;
    std::vector<WDL_String> mMidiInputDevNames;
    std::vector<WDL_String> mMidiOutputDevNames;
    
    WDL_PtrList<double> mInputBufPtrs;
    WDL_PtrList<double> mOutputBufPtrs;
    
    friend class IPlugAPP;
};

END_IPLUG_NAMESPACE
