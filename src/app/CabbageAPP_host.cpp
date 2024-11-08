/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#include "CabbageAPP_host.h"

#ifdef OS_WIN
#include <sys/stat.h>
#endif

#include "opcodes/CabbageOpcodes.h"
#include "IPlugLogger.h"

using namespace iplug;

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 2048
#endif

#define STRBUFSZ 100


std::unique_ptr<IPlugAPPHost> IPlugAPPHost::sInstance;
UINT gSCROLLMSG;

IPlugAPPHost::IPlugAPPHost(std::string file)
: csdFile(file), mIPlug(MakePlug(InstanceInfo{this}, file))
{
    
    
}

IPlugAPPHost::~IPlugAPPHost()
{
    mExiting = true;
    
    CloseAudio();
    
    
    if(mMidiIn)
        mMidiIn->cancelCallback();
    
    if(mMidiOut)
        mMidiOut->closePort();
}

//static
IPlugAPPHost* IPlugAPPHost::Create(std::string filePath)
{
    sInstance = std::make_unique<IPlugAPPHost>(filePath);
    return sInstance.get();
}

bool IPlugAPPHost::InitProcessor()
{
    cabbageProcessor = dynamic_cast<CabbageProcessor*>(mIPlug.get());
    parameters = cabbageProcessor->getCabbageEngine().getWidgets();

   
    //this callback is triggered from CabbageProcessor.cpp and is responsible for
    //updating the widgets in the VSCode web panel
    auto callback = [&](CabbageOpcodeData data) {
            auto& cabbage = cabbageProcessor->getCabbageEngine();
            auto widgetOpt = cabbage.getWidget(data.channel);
            if (widgetOpt.has_value())
            {
                auto& j = widgetOpt.value().get();
                if(j["type"].get<std::string>() == "gentable")
                {
                    cabbage.updateFunctionTable(data, j);
                    nlohmann::json msg;
                    msg["command"] = "widgetUpdate";
                    msg["channel"] = data.channel;
                    msg["data"] = j.dump();
                    webSocket.send(msg.dump());
                }
                else{
                    if(data.type == CabbageOpcodeData::MessageType::Value)
                    {
                        nlohmann::json json;
                        cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
                        nlohmann::json msg;
                        msg["command"] = "widgetUpdate";
                        msg["channel"] = data.channel;
                        msg["value"] = j["value"].get<float>();
                        webSocket.send(msg.dump());
//                        writeToLog(msg.dump());
                    }
                    else
                    {
//                        nlohmann::json json;
//                        cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
//                        nlohmann::json msg;
//                        msg["command"] = "widgetUpdate";
//                        msg["channel"] = data.channel;
//                        msg["data"] = j.dump();
//                        webSocket.send(msg.dump());
                    }
                }
            }
        };
    
    cabbageProcessor->hostCallback = callback;
}

bool IPlugAPPHost::InitWebSocket()
{
    webSocket.setUrl("ws://localhost:9991");
    
    webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg)
            {
                auto& cabbage = cabbageProcessor->getCabbageEngine();
                if (msg->type == ix::WebSocketMessageType::Message)
                {
                    try{
                        auto json = nlohmann::json::parse(msg->str, nullptr, false);
                        const std::string command = json["command"];
                        auto jsonObj = nlohmann::json::parse(json["obj"].get<std::string>());
                        
                        if(command == "parameterChange")
                        {
                            for(int i = 0 ; i < cabbage.getNumberOfParameters() ; i++)
                            {
                                if(cabbage.getParameterChannel(i).name == jsonObj["channel"].get<std::string>())
                                {
                                    //update underlying JSON object if the value has changed
                                    auto widgetOpt = cabbage.getWidget(jsonObj["channel"]);
                                    if (widgetOpt.has_value())
                                    {
                                        auto& widgetObj = widgetOpt.value().get();
                                        widgetObj["value"] = jsonObj["value"].get<double>();
                                    }
                                    cabbageProcessor->SetParameterValue (i, jsonObj["value"].get<double>());
                                }
                            }
//                            SendParameterValueFromUI(message["paramIdx"], message["value"]);
                        }
                        else if(command == "fileOpenFromVSCode")
                        {
                            if(jsonObj.contains("fileName")){
                                cabbage.setStringChannel(jsonObj["channel"].get<std::string>(), jsonObj["fileName"].get<std::string>());
                            }
                        }
                        else if(command == "widgetStateUpdate")
                        {
                            cabbage.updateWidgetState(jsonObj);
                            
                        }
                        else if(command == "midiMessage")
                        {
                            iplug::IMidiMsg msg {0, jsonObj["statusByte"].get<uint8_t>(),
                                jsonObj["dataByte1"].get<uint8_t>(),
                                jsonObj["dataByte2"].get<uint8_t>()};
                            cabbageProcessor->SendMidiMsgFromUI(msg);
                        }
                        else if(command == "cabbageIsReadyToLoad")
                        {
                        
                        }
                        else if(command == "cabbageSetupComplete")
                        {
                            cabbageProcessor->interfaceHasLoaded();
                        }
                        else if(command == "stopCsound")
                        {
                            std::cout << "stopping Csound" << msg->str << std::endl;
                            cabbageProcessor->stopProcessing();
                        }
                        else
                        {
                            //std::cout << "received message: " << msg->str << std::endl;
                            std::cout << "> " << std::flush;
                        }
                    }
                    catch (nlohmann::json::exception& e) {
                        LOG_VERBOSE("Error:", e.what());
                    }
                }
                else if (msg->type == ix::WebSocketMessageType::Open)
                {
                    LOG_VERBOSE("Connection established");
                    //if connection is ope we need to send all parse jSON objects to VS-Code..
                    for( auto& w : cabbage.getWidgets())
                    {
                        nlohmann::json msg;
                        msg["command"] = "widgetUpdate";
                        msg["channel"] = w["channel"];
                        msg["data"] = w.dump();
                        webSocket.send(msg.dump());
                    }
                    cabbageProcessor->interfaceHasLoaded();
                }
                else if (msg->type == ix::WebSocketMessageType::Close)
                {
                    LOG_VERBOSE("websocket connection closed..");
                }
                else if (msg->type == ix::WebSocketMessageType::Error)
                {
                    // Maybe SSL is not configured properly
                    LOG_VERBOSE("Connection error: ", msg->errorInfo.reason);
                    //std::cout << "> " << std::flush;
                }
            }
    );

    // Now that our callback is setup, we can start our background thread and receive messages
    webSocket.start();
    
}

bool IPlugAPPHost::Init()
{
    mIPlug->SetHost("standalone", mIPlug->GetPluginVersion(false));
    
    if (!InitState())
        return false;
    
    TryToChangeAudioDriverType(); // will init RTAudio with an API type based on gState->mAudioDriverType
    ProbeAudioIO(); // find out what audio IO devs are available and put their IDs in the global variables gAudioInputDevs / gAudioOutputDevs
    InitMidi(); // creates RTMidiIn and RTMidiOut objects
    ProbeMidiIO(); // find out what midi IO devs are available and put their names in the global variables gMidiInputDevs / gMidiOutputDevs
    SelectMIDIDevice(ERoute::kInput, mState.mMidiInDev.Get());
    SelectMIDIDevice(ERoute::kOutput, mState.mMidiOutDev.Get());
    
    mIPlug->OnParamReset(kReset);
    mIPlug->OnActivate(true);

    return true;
}

bool IPlugAPPHost::OpenWindow(HWND pParent)
{
    return mIPlug->OpenWindow(pParent) != nullptr;
}

void IPlugAPPHost::CloseWindow()
{
    mIPlug->CloseWindow();
}

bool IPlugAPPHost::InitState()
{
#if defined OS_WIN
    TCHAR strPath[MAX_PATH_LEN];
    SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath );
    // Use std::string for path concatenation
    mJSONPath = std::string(strPath) + "\\" + BUNDLE_NAME + "\\";
#elif defined OS_MAC
    const char* homePath = getenv("HOME");
    mJSONPath = std::string(homePath) + "/Library/Application Support/" + BUNDLE_NAME + "/";

#else
#error NOT IMPLEMENTED
#endif
    
    /* 
     todo - set default locations for widget src
    if (mState->GetString("jsSrcDir", nullptr) == nullptr)
        mState->SetString("jsSrcDir", "path to JS src dir");
     */

    struct stat st;
    
    if(stat(mJSONPath.c_str(), &st) == 0) // if directory exists
    {
        mJSONPath+=("settings.json"); // add file name to path
        
        
        try{
            std::ifstream file(mJSONPath);
            nlohmann::json settingsJson = {};
            
            if (file.is_open()) {
                // Check if the file is empty
                if (file.peek() == std::ifstream::traits_type::eof()) {
                    std::cout << "File is empty, using default settings." << std::endl;
                    UpdateSettings();
                    return true;
                }
                else {
                    try {
                        // Use the extraction operator to read JSON data from the file
                        file >> settingsJson;
                    }
                    catch (const nlohmann::json::parse_error& e) {
                        std::cerr << "JSON parse error: " << e.what() << std::endl;
                    }
                }
            }
            else {
                std::cerr << "ERROR: Failed to open the settings file. If this is the first time running Cabbage from vscode, you can ignore this error";
            }
                
            file.close();
            
            char buf[STRBUFSZ];
            
            if(stat(mJSONPath.c_str(), &st) == 0) // if settings file exists read values into state
            {
                DBGMSG("Reading json file from %s\n", mJSONPath.c_str());

                mState.mAudioDriverType = settingsJson["currentConfig"]["audio"].value("driver", 0);
                mState.mAudioInDev.Set(settingsJson["currentConfig"]["audio"].value("inputDevice", "Built-in Input").c_str());
                mState.mAudioOutDev.Set(settingsJson["currentConfig"]["audio"].value("outputDevice", "Built-in Output").c_str());
                mState.mAudioInChanL = settingsJson["currentConfig"]["audio"].value("in1", 1);
                mState.mAudioInChanR = settingsJson["currentConfig"]["audio"].value("in2", 2);
                mState.mAudioOutChanL = settingsJson["currentConfig"]["audio"].value("out1", 1);
                mState.mAudioOutChanR = settingsJson["currentConfig"]["audio"].value("out2", 2);
                mState.mBufferSize = settingsJson["currentConfig"]["audio"].value("buffer", 512);
                mState.mAudioSR = settingsJson["currentConfig"]["audio"].value("sr", 44100);
                mState.mMidiInDev.Set(settingsJson["currentConfig"]["midi"].value("inputDevice", "no input").c_str());
                mState.mMidiOutDev.Set(settingsJson["currentConfig"]["midi"].value("outputDvice", "no output").c_str());
                mState.mMidiInChan = settingsJson["currentConfig"]["midi"].value("inChan", 0);
                mState.mMidiOutChan = settingsJson["currentConfig"]["midi"].value("outChan", 0);
                mState.mJsSourceDirectory = settingsJson["currentConfig"].value("jsSourceDir", "add path to JS src directory");
            }

        }
        catch (const nlohmann::json::parse_error& e)
        {
            LOG_VERBOSE("JSON parse error: ", e.what());
            auto t = e.what();
            cabAssert(false, "Can't parse settings file");
        }
        
        // if settings file doesn't exist, populate with default values, otherwise overrwrite
        UpdateSettings();
    }
    else   // folder doesn't exist - make folder and make file
    {
#if defined OS_WIN
        // folder doesn't exist - make folder and make file
        CreateDirectory(mJSONPath.c_str(), NULL);
        mJSONPath+=("\\settings.json");
        UpdateSettings(); // will write file if doesn't exist
#elif defined OS_MAC
        mode_t process_mask = umask(0);
        int result_code = mkdir(mJSONPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        umask(process_mask);
        
        if(!result_code)
        {
            mJSONPath+=("\\settings.json");
            UpdateSettings(); // will write file if doesn't exist
        }
        else
        {
            return false;
        }
#else
#error NOT IMPLEMENTED
#endif
    }
    
    return true;
}

void IPlugAPPHost::addDevicesToSettings( RtAudio& audio, nlohmann::json& settingsJSON)
{
    RtAudio::DeviceInfo info;
    int inputCnt = 0;
    int outputCnt = 0;
    std::vector<unsigned int> devices = audio.getDeviceIds();
    
    for (unsigned int i=0; i<devices.size(); i++)
    {
        info = audio.getDeviceInfo( devices[i] );
        
        if (info.outputChannels > 0)
        {
            const std::string outs = "output" + std::to_string(outputCnt);
            auto outputDevice = cleanDeviceName(info.name);
            nlohmann::json j;
            j["deviceId"] = info.ID;
            j["numChannels"] = info.outputChannels;
            j["sampleRates"] = info.sampleRates;
            settingsJSON["systemAudioMidiIOListing"]["audioOutputDevices"][outputDevice] = j;
            outputCnt++;
        }
        
        // Handle input devices
        if (info.inputChannels > 0)
        {
            const std::string ins = "input" + std::to_string(inputCnt);
            auto inputDevice = cleanDeviceName(info.name);
            nlohmann::json j;
            j["deviceId"] = info.ID;
            j["numChannels"] = info.inputChannels;
            
            settingsJSON["systemAudioMidiIOListing"]["audioInputDevices"][inputDevice] = j;
            inputCnt++;
        }
    }
}
    
void IPlugAPPHost::UpdateSettings()
{
	nlohmann::json settingsJSON;
    settingsJSON["currentConfig"]["audio"]["driver"] = mState.mAudioDriverType;
    settingsJSON["currentConfig"]["audio"]["inputDevice"] = mState.mAudioInDev.Get();
    settingsJSON["currentConfig"]["audio"]["outputDevice"] = mState.mAudioOutDev.Get();
    settingsJSON["currentConfig"]["audio"]["in1"] = mState.mAudioInChanL;
    settingsJSON["currentConfig"]["audio"]["in2"] = mState.mAudioInChanR;
    settingsJSON["currentConfig"]["audio"]["out1"] = mState.mAudioOutChanL;
    settingsJSON["currentConfig"]["audio"]["out2"] = mState.mAudioOutChanR;
    settingsJSON["currentConfig"]["audio"]["buffer"] = mState.mBufferSize;
    settingsJSON["currentConfig"]["audio"]["sr"] = mState.mAudioSR;
    settingsJSON["currentConfig"]["midi"]["inputDevice"] = mState.mMidiInDev.Get();
    settingsJSON["currentConfig"]["midi"]["outputDevice"] = mState.mMidiOutDev.Get();
    settingsJSON["currentConfig"]["midi"]["inChan"] = mState.mMidiInChan;
    settingsJSON["currentConfig"]["midi"]["outChan"] = mState.mMidiOutChan;

    
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    
#ifdef OS_WIN
    settingsJSON["audioDrivers"] = { "DirectSound", "ASIO" };
#elif defined OS_MAC
    settingsJSON["systemAudioMidiIOListing"]["audioDrivers"] = "CoreAudio";
#else
    cabAssert(false, "Not implemented");
#endif
    
    settingsJSON["currentConfig"]["jsSourceDir"] = mState.mJsSourceDirectory;
    
    
    RtAudio audio(apis[0], errorCallback);
    addDevicesToSettings(audio, settingsJSON);

    RtMidiIn midiIn;
    RtMidiOut midiOut;

    int inputCnt = 0;
    int outputCnt = 0;

    // Get the default output device ID
    unsigned int defaultOutputDevice = audio.getDefaultOutputDevice();
    auto info = audio.getDeviceInfo(defaultOutputDevice);
    nlohmann::json json;
    json["numChannels"] = info.outputChannels;
    json["sampleRates"] = info.sampleRates;
    settingsJSON["systemAudioMidiIOListing"]["audioOutputDevices"]["Default Device"] = json;

    // Get the number of input devices
    inputCnt = 0;
    outputCnt = 0;
    unsigned int inputCount = midiIn.getPortCount();
    for (unsigned int i = 0; i < inputCount; ++i) 
    {
        nlohmann::json j;
        j["deviceId"] = i;
        settingsJSON["systemAudioMidiIOListing"]["midiInputDevices"][midiIn.getPortName(i)] = j;
    }

    // Get the number of output devices
    unsigned int outputCount = midiOut.getPortCount();
    for (unsigned int i = 0; i < outputCount; ++i) 
    {
        nlohmann::json j;
        j["deviceId"] = i;
        settingsJSON["systemAudioMidiIOListing"]["midiOutputDevices"][midiOut.getPortName(i)] = j;
    }
        
    std::ofstream settingsFile(mJSONPath);
    if (settingsFile.is_open()) {
        settingsFile << settingsJSON.dump(4);  // Pretty print JSON with 4-space indentation
        settingsFile.close();
//        std::cout << "Settings written to: " << mJSONPath << std::endl;
    } else {
        std::cerr << "Unable to open settings file for writing: " << mJSONPath << std::endl;
    }

}

std::string IPlugAPPHost::GetAudioDeviceName(int idx) const
{
    return mAudioIDDevNames.at(idx);
}

int IPlugAPPHost::GetAudioDeviceIdx(const char* deviceNameToTest) const
{
    for(int i = 0; i < mAudioIDDevNames.size(); ++i)
        LOG_INFO(mAudioIDDevNames.at(i).c_str());
    
    for(int i = 0; i < mAudioIDDevNames.size(); ++i)
    {
        LOG_INFO(mAudioIDDevNames.at(i).c_str());
        if(!strcmp(deviceNameToTest, mAudioIDDevNames.at(i).c_str() ))
            return i;
    }
    
    return -1;
}

int IPlugAPPHost::GetMIDIPortNumber(ERoute direction, const char* nameToTest) const
{
    int start = 1;
    
    if(direction == ERoute::kInput)
    {
        if(!strcmp(nameToTest, OFF_TEXT)) return 0;
        
#ifdef OS_MAC
        start = 2;
        if(!strcmp(nameToTest, "virtual input")) return 1;
#endif
        
        for (int i = 0; i < mMidiIn->getPortCount(); i++)
        {
            if(!strcmp(nameToTest, mMidiIn->getPortName(i).c_str()))
                return (i + start);
        }
    }
    else
    {
        auto pCnt = mMidiOut->getPortCount();
        if(!strcmp(nameToTest, OFF_TEXT)) return 0;
        
#ifdef OS_MAC
        start = 2;
        if(!strcmp(nameToTest, "virtual output")) return 1;
#endif
        
        for (int i = 0; i < pCnt; i++)
        {
            if(!strcmp(nameToTest, mMidiOut->getPortName(i).c_str()))
                return (i + start);
        }
    }
    
    return -1;
}

void IPlugAPPHost::ProbeAudioIO()
{
    std::cout << "\nRtAudio Version " << RtAudio::getVersion() << std::endl;
    
    RtAudio::DeviceInfo info;
    
    mAudioInputDevs.clear();
    mAudioOutputDevs.clear();
    mAudioIDDevNames.clear();
    
    std::vector<unsigned int> devices = mDAC->getDeviceIds();
    
    for (unsigned int i=0; i<devices.size(); i++)
    {
        info = mDAC->getDeviceInfo( devices[i] );
        std::string deviceName = info.name;
        
#ifdef OS_MAC
        size_t colonIdx = deviceName.rfind(": ");
        
        if(colonIdx != std::string::npos && deviceName.length() >= 2)
            deviceName = deviceName.substr(colonIdx + 2, deviceName.length() - colonIdx - 2);
        
#endif
        
        mAudioIDDevNames.push_back(deviceName);

            if(info.inputChannels > 0)
                mAudioInputDevs.push_back(i);
            
            if(info.outputChannels > 0)
                mAudioOutputDevs.push_back(i);
            
            if (info.isDefaultInput)
                mDefaultInputDev = i;
            
            if (info.isDefaultOutput)
                mDefaultOutputDev = i;

    }
}

void IPlugAPPHost::ProbeMidiIO()
{
    if ( !mMidiIn || !mMidiOut )
        return;
    else
    {
        int nInputPorts = mMidiIn->getPortCount();
        
        mMidiInputDevNames.push_back(OFF_TEXT);
        
#ifdef OS_MAC
        mMidiInputDevNames.push_back("virtual input");
#endif
        
        for (int i=0; i<nInputPorts; i++ )
        {
            mMidiInputDevNames.push_back(mMidiIn->getPortName(i));
        }
        
        int nOutputPorts = mMidiOut->getPortCount();
        
        mMidiOutputDevNames.push_back(OFF_TEXT);
        
#ifdef OS_MAC
        mMidiOutputDevNames.push_back("virtual output");
#endif
        
        for (int i=0; i<nOutputPorts; i++ )
        {
            mMidiOutputDevNames.push_back(mMidiOut->getPortName(i));
            //This means the virtual output port wont be added as an input
        }
    }
}

bool IPlugAPPHost::AudioSettingsInStateAreEqual(AppState& os, AppState& ns)
{
    if (os.mAudioDriverType != ns.mAudioDriverType) return false;
    if (strcmp(os.mAudioInDev.Get(), ns.mAudioInDev.Get())) return false;
    if (strcmp(os.mAudioOutDev.Get(), ns.mAudioOutDev.Get())) return false;
    if (os.mAudioSR != ns.mAudioSR) return false;
    if (os.mBufferSize != ns.mBufferSize) return false;
    if (os.mAudioInChanL != ns.mAudioInChanL) return false;
    if (os.mAudioInChanR != ns.mAudioInChanR) return false;
    if (os.mAudioOutChanL != ns.mAudioOutChanL) return false;
    if (os.mAudioOutChanR != ns.mAudioOutChanR) return false;
    //  if (os.mAudioInIsMono != ns.mAudioInIsMono) return false;
    
    return true;
}

bool IPlugAPPHost::MIDISettingsInStateAreEqual(AppState& os, AppState& ns)
{
    if (strcmp(os.mMidiInDev.Get(), ns.mMidiInDev.Get())) return false;
    if (strcmp(os.mMidiOutDev.Get(), ns.mMidiOutDev.Get())) return false;
    if (os.mMidiInChan != ns.mMidiInChan) return false;
    if (os.mMidiOutChan != ns.mMidiOutChan) return false;
    
    return true;
}

bool IPlugAPPHost::TryToChangeAudioDriverType()
{
    CloseAudio();
    LOG_INFO("Closing audio");
    if (mDAC)
    {
        mDAC = nullptr;
    }
    
#if defined OS_WIN
    if(mState.mAudioDriverType == kDeviceASIO)
        mDAC = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO, errorCallback);
    else
        mDAC = std::make_unique<RtAudio>(RtAudio::WINDOWS_DS, errorCallback);
#elif defined OS_MAC
    if(mState.mAudioDriverType == kDeviceCoreAudio)
    {
        LOG_INFO("Setting audio to coreaudio");
        std::vector<RtAudio::Api> apis;
        RtAudio::getCompiledApi(apis);
        RtAudio audio(apis[0], errorCallback);
        mDAC = std::make_unique<RtAudio>(apis[0], errorCallback);
    }
        
    
    //else
    //mDAC = std::make_unique<RtAudio>(RtAudio::UNIX_JACK);
#else
#error NOT IMPLEMENTED
#endif
    LOG_INFO("Audio reopended");

    if(mDAC)
        return true;
    else
        return false;
}

bool IPlugAPPHost::TryToChangeAudio()
{
    auto settingsFile = cabbage::File::getSettingsFile();
    std::ifstream file(settingsFile);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open the file " << settingsFile << std::endl;
        return "";
    }
    
    // Parse the JSON content from the file
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse JSON - " << e.what() << std::endl;
        return "";
    }
    
    int inputID = -1;
    int outputID = -1;
    
#if defined OS_WIN
    cabAssert(false, "Need to handle default IO here...");
    if(mState.mAudioDriverType == kDeviceASIO)
        inputID = GetAudioDeviceIdx(mState.mAudioOutDev.Get());
    else
        inputID = GetAudioDeviceIdx(mState.mAudioInDev.Get());
    
    outputID = jsonData["systemAudioMidiIOListing"]["audioOutputDevices"][mState.mAudioOutDev.Get()]["deviceId"];
#elif defined OS_MAC
    std::string input = mState.mAudioInDev.Get();
    std::string output = mState.mAudioOutDev.Get();
    RtAudio defaultIO;
    if(input == "Built-in Input")
        inputID = defaultIO.getDefaultInputDevice();
    else
        inputID = jsonData["systemAudioMidiIOListing"]["audioInputDevices"][mState.mAudioInDev.Get()]["deviceId"];
    
    if(output == "Built-in Output")
        outputID = defaultIO.getDefaultOutputDevice();
    else
        outputID = jsonData["systemAudioMidiIOListing"]["audioOutputDevices"][mState.mAudioOutDev.Get()]["deviceId"];
  
    
#else
#error NOT IMPLEMENTED
#endif
    
    
    bool failedToFindDevice = false;
    bool resetToDefault = false;
    
    if (inputID == -1)
    {
        if (mDefaultInputDev > -1)
        {
            resetToDefault = true;
            inputID = mDefaultInputDev;
            
            if (mAudioInputDevs.size())
                mState.mAudioInDev.Set(GetAudioDeviceName(inputID).c_str());
        }
        else
            failedToFindDevice = true;
    }
    
    if (outputID == -1)
    {
        if (mDefaultOutputDev > -1)
        {
            resetToDefault = true;
            
            outputID = mDefaultOutputDev;
            
            if (mAudioOutputDevs.size())
                mState.mAudioOutDev.Set(GetAudioDeviceName(outputID).c_str());
        }
        else
            failedToFindDevice = true;
    }
    
    if (resetToDefault)
    {
        DBGMSG("couldn't find previous audio device, reseting to default\n");
        
        UpdateSettings();
    }
    
    if (failedToFindDevice)
        MessageBox(gHWND, "Please check your soundcard settings in Preferences", "Error", MB_OK);
    
    if (inputID != -1 && outputID != -1)
    {
        return InitAudio(inputID, outputID, mState.mAudioSR, mState.mBufferSize);
    }
    
    return false;
}

bool IPlugAPPHost::SelectMIDIDevice(ERoute direction, const char* pPortName)
{
    int port = GetMIDIPortNumber(direction, pPortName);
    
    if(direction == ERoute::kInput)
    {
        if(port == -1)
        {
            mState.mMidiInDev.Set(OFF_TEXT);
            UpdateSettings();
            port = 0;
        }
        
        //TODO: send all notes off?
        if (mMidiIn)
        {
            mMidiIn->closePort();
            
            if (port == 0)
            {
                return true;
            }
#if defined OS_WIN
            else
            {
                mMidiIn->openPort(port-1);
                return true;
            }
#elif defined OS_MAC
            else if(port == 1)
            {
                std::string virtualMidiInputName = "To ";
                virtualMidiInputName += BUNDLE_NAME;
                mMidiIn->openVirtualPort(virtualMidiInputName);
                return true;
            }
            else
            {
                mMidiIn->openPort(port-2);
                return true;
            }
#else
#error NOT IMPLEMENTED
#endif
        }
    }
    else
    {
        if(port == -1)
        {
            mState.mMidiOutDev.Set(OFF_TEXT);
            UpdateSettings();
            port = 0;
        }
        
        if (mMidiOut)
        {
            //TODO: send all notes off?
            mMidiOut->closePort();
            
            if (port == 0)
                return true;
#if defined OS_WIN
            else
            {
                mMidiOut->openPort(port-1);
                return true;
            }
#elif defined OS_MAC
            else if(port == 1)
            {
                std::string virtualMidiOutputName = "From ";
                virtualMidiOutputName += BUNDLE_NAME;
                mMidiOut->openVirtualPort(virtualMidiOutputName);
                return true;
            }
            else
            {
                mMidiOut->openPort(port-2);
                return true;
            }
#else
#error NOT IMPLEMENTED
#endif
        }
    }
    
    return false;
}

void IPlugAPPHost::CloseAudio()
{
    if (mDAC && mDAC->isStreamOpen())
    {
        if (mDAC->isStreamRunning())
        {
            mAudioEnding = true;
            
            while (!mAudioDone)
                Sleep(10);
            
            try
            {
                mDAC->abortStream();
            }
            catch (const std::runtime_error &e)
            {
                LOG_INFO(e.what());
            }
        }
        
        mDAC->closeStream();
    }
}

bool IPlugAPPHost::InitAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs)
{
    CloseAudio();
    
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = inId;
    iParams.nChannels = GetPlug()->MaxNChannels(ERoute::kInput); // TODO: flexible channel count
    iParams.firstChannel = 0; // TODO: flexible channel count
    
    oParams.deviceId = outId;
    oParams.nChannels = GetPlug()->MaxNChannels(ERoute::kOutput); // TODO: flexible channel count
    oParams.firstChannel = 0; // TODO: flexible channel count
    
    mBufferSize = iovs; // mBufferSize may get changed by stream
    
    auto inDevName = cleanDeviceName(mDAC->getDeviceInfo(inId).name);
    auto outDevName = cleanDeviceName(mDAC->getDeviceInfo(outId).name);
    
    LOG_INFO("Attempting to start audio with the following settings:\nSR: ", sr, "\nBuffer Size: ", mBufferSize, "\nInput device: ", inDevName, "\nNumber of channels: ", iParams.nChannels, "\nOutput device: ", outDevName, "\nNumber of channels: ", oParams.nChannels);

    
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_NONINTERLEAVED;
    // options.streamName = BUNDLE_NAME; // JACK stream name, not used on other streams
    
    mBufIndex = 0;
    mSamplesElapsed = 0;
    mSampleRate = (double) sr;
    mVecWait = 0;
    mAudioEnding = false;
    mAudioDone = false;
    
    mIPlug->SetBlockSize(APP_SIGNAL_VECTOR_SIZE);
    mIPlug->SetSampleRate(mSampleRate);
    mIPlug->OnReset();
    
    try
    {
        mDAC->openStream(&oParams, iParams.nChannels > 0 ? &iParams : nullptr, RTAUDIO_FLOAT64, sr, &mBufferSize, &AudioCallback, this, &options);
        
        for (int i = 0; i < iParams.nChannels; i++)
        {
            mInputBufPtrs.Add(nullptr); //will be set in callback
        }
        
        for (int i = 0; i < oParams.nChannels; i++)
        {
            mOutputBufPtrs.Add(nullptr); //will be set in callback
        }
        
        mDAC->startStream();
        
        mActiveState = mState;
    }
    catch (const std::runtime_error &e)
    {
        LOG_INFO("Issue opening audio stream: ", e.what());
        return false;
    }
    
    return true;
}

bool IPlugAPPHost::InitMidi()
{
    try
    {
        mMidiIn = std::make_unique<RtMidiIn>();
    }
    catch (RtMidiError &error)
    {
        mMidiIn = nullptr;
        error.printMessage();
        return false;
    }
    
    try
    {
        mMidiOut = std::make_unique<RtMidiOut>();
    }
    catch (RtMidiError &error)
    {
        mMidiOut = nullptr;
        error.printMessage();
        return false;
    }
    
    mMidiIn->setCallback(&MIDICallback, this);
    mMidiIn->ignoreTypes(false, true, false );
    
    return true;
}

void ApplyFades(double *pBuffer, int nChans, int nFrames, bool down)
{
    for (int i = 0; i < nChans; i++)
    {
        double *pIO = pBuffer + (i * nFrames);
        
        if (down)
        {
            for (int j = 0; j < nFrames; j++)
                pIO[j] *= ((double) (nFrames - (j + 1)) / (double) nFrames);
        }
        else
        {
            for (int j = 0; j < nFrames; j++)
                pIO[j] *= ((double) j / (double) nFrames);
        }
    }
}

// static
int IPlugAPPHost::AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData)
{
    IPlugAPPHost* _this = (IPlugAPPHost*) pUserData;
    
    int nins = _this->GetPlug()->MaxNChannels(ERoute::kInput);
    int nouts = _this->GetPlug()->MaxNChannels(ERoute::kOutput);
    
    double* pInputBufferD = static_cast<double*>(pInputBuffer);
    double* pOutputBufferD = static_cast<double*>(pOutputBuffer);
    
    bool startWait = _this->mVecWait >= APP_N_VECTOR_WAIT; // wait APP_N_VECTOR_WAIT * iovs before processing audio, to avoid clicks
    bool doFade = _this->mVecWait == APP_N_VECTOR_WAIT || _this->mAudioEnding;
    
    if (startWait && !_this->mAudioDone)
    {
        if (doFade)
            ApplyFades(pInputBufferD, nins, nFrames, _this->mAudioEnding);
        
        for (int i = 0; i < nFrames; i++)
        {
            _this->mBufIndex %= APP_SIGNAL_VECTOR_SIZE;
            
            if (_this->mBufIndex == 0)
            {
                for (int c = 0; c < nins; c++)
                {
                    _this->mInputBufPtrs.Set(c, (pInputBufferD + (c * nFrames)) + i);
                }
                
                for (int c = 0; c < nouts; c++)
                {
                    _this->mOutputBufPtrs.Set(c, (pOutputBufferD + (c * nFrames)) + i);
                }
                
                _this->mIPlug->AppProcess(_this->mInputBufPtrs.GetList(), _this->mOutputBufPtrs.GetList(), APP_SIGNAL_VECTOR_SIZE);
                
                _this->mSamplesElapsed += APP_SIGNAL_VECTOR_SIZE;
            }
            
            for (int c = 0; c < nouts; c++)
            {
                pOutputBufferD[c * nFrames + i] *= APP_MULT;
            }
            
            _this->mBufIndex++;
        }
        
        if (doFade)
            ApplyFades(pOutputBufferD, nouts, nFrames, _this->mAudioEnding);
        
        if (_this->mAudioEnding)
            _this->mAudioDone = true;
    }
    else
    {
        memset(pOutputBufferD, 0, nFrames * nouts * sizeof(double));
    }
    
    _this->mVecWait = std::min(_this->mVecWait + 1, uint32_t(APP_N_VECTOR_WAIT + 1));
    
    return 0;
}

// static
void IPlugAPPHost::MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData)
{
    IPlugAPPHost* _this = (IPlugAPPHost*) pUserData;
    
    if (pMsg->size() == 0 || _this->mExiting)
        return;
    
    if (pMsg->size() > 3)
    {
        if(pMsg->size() > MAX_SYSEX_SIZE)
        {
            DBGMSG("SysEx message exceeds MAX_SYSEX_SIZE\n");
            return;
        }
        
        SysExData data { 0, static_cast<int>(pMsg->size()), pMsg->data() };
        
        _this->mIPlug->mSysExMsgsFromCallback.Push(data);
        return;
    }
    else if (pMsg->size())
    {
        IMidiMsg msg;
        msg.mStatus = pMsg->at(0);
        pMsg->size() > 1 ? msg.mData1 = pMsg->at(1) : msg.mData1 = 0;
        pMsg->size() > 2 ? msg.mData2 = pMsg->at(2) : msg.mData2 = 0;
        
        _this->mIPlug->mMidiMsgsFromCallback.Push(msg);
    }
}

// static
void IPlugAPPHost::errorCallback(RtAudioErrorType type, const std::string &errorText )
{
    LOG_INFO(errorText);
}

