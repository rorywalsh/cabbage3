#ifndef CABBAGEAUDIOAPP_H
#define CABBAGEAUDIOAPP_H

#include <RtAudio.h>
#include <RtMidi.h>

#include <atomic>
#include <memory>
#include <vector>

// Undefined these in ixWebsocket
#undef logInfo
#undef logDebug
#undef logWarning
#undef logError
#include "platform/choc_DisableAllWarnings.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include "platform/choc_ReenableAllWarnings.h"

#include "CabbageProcessor.h"
#include "WebSocketTestServer.h"


class CabbageAudioApp {
public:
    
    struct AudioConfig {
        int audioDriverType;
        std::string audioInDev;
        std::string audioOutDev;
        int audioInChanL;
        int audioInChanR;
        int audioOutChanL;
        int audioOutChanR;
        int bufferSize;
        int audioSR;
        std::string midiInDev;
        std::string midiOutDev;
        int midiInChan;
        int midiOutChan;
        std::string jsSourceDirectory;

        bool loadFromJson(const std::string& settingsPath) 
        {
            std::ifstream file(settingsPath);
            if (!file) {
                std::cerr << "Error: Could not open settings file: " << settingsPath << std::endl;
                return false;
            }

            try {
                nlohmann::json settingsJson;
                file >> settingsJson;

                audioDriverType = settingsJson["currentConfig"]["audio"].value("driver", 0);
                audioInDev = settingsJson["currentConfig"]["audio"].value("inputDevice", "Built-in Input");
                audioOutDev = settingsJson["currentConfig"]["audio"].value("outputDevice", "Built-in Output");
                audioInChanL = settingsJson["currentConfig"]["audio"].value("in1", 1);
                audioInChanR = settingsJson["currentConfig"]["audio"].value("in2", 2);
                audioOutChanL = settingsJson["currentConfig"]["audio"].value("out1", 1);
                audioOutChanR = settingsJson["currentConfig"]["audio"].value("out2", 2);
                bufferSize = settingsJson["currentConfig"]["audio"].value("bufferSize", 512);
                audioSR = settingsJson["currentConfig"]["audio"].value("sr", 44100);

                midiInDev = settingsJson["currentConfig"]["midi"].value("inputDevice", "no input");
                midiOutDev = settingsJson["currentConfig"]["midi"].value("outputDevice", "no output");
                midiInChan = settingsJson["currentConfig"]["midi"].value("inChan", 0);
                midiOutChan = settingsJson["currentConfig"]["midi"].value("outChan", 0);

                jsSourceDirectory = settingsJson["currentConfig"].value("jsSourceDir", "add path to JS src directory");

            } catch (nlohmann::json::exception& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                return false;
            }

            return true;
        }
    };
    
    CabbageAudioApp(int argc, char* argv[]);
    ~CabbageAudioApp();
    void closeAudioDevice();
    bool parseComandLineArgs(int argc, char* argv[]);
    
    bool isStreamRunning() const;
    static void errorCallback(RtAudioErrorType type, const std::string& errorText);

    std::unique_ptr<CabbageProcessor> processor; // Main processor
    
    bool isRunningInDebugMode() { return debugMode; };

    void sendWidgetDataToVscode();

private:
    void hostCallback(CabbageOpcodeData data);
    ix::WebSocket webSocket;
    AudioConfig audioConfig;
    void initCabbage();
    void initialiseAudio(bool startStream);
    void initialiseMidi();
    void deinitAudioAndMidi();
    
    // Websocket server - for communication with vscode
    ix::WebSocketServer webSocketServer;
    bool initialiseWebSocketConnection();
    
    
    // Functions for running test server - for tests without vscode
    bool shouldStartTestServer = false;
    std::unique_ptr<WebSocketTestServer> testServer;
    void startWebSocketServerForTesting();
    void stopWebSocketServerForTesting();
    
    
    // Return a valid device ID for a given device name
    int getAudioDeviceId(const std::string& deviceName) const;
    
    // Settings functions
    void addDevicesToSettings(const std::string& settingsFile);
    
    std::unique_ptr<RtAudio> audioDevice = nullptr;
    std::unique_ptr<RtMidiIn> midiInDevice = nullptr;
    std::unique_ptr<RtMidiOut> midiOutDevice = nullptr;
    int midiOutChannel = -1;
    int midiInChannel = -1;

    unsigned int numChannels; // Number of audio channels
    std::atomic<bool> isRunning; // Flag to track stream state
    float** emptyInputBuffer; // Preallocated empty input buffer
    bool emptyInputBufferInitialized = false; // Flag to check if the buffer is initialized
    unsigned int bufferSize; // Size of the audio buffer (in frames)

    float** getEmptyInputBuffer() const;
    unsigned int getNumChannels() const;

    // Callbacks for audio and midi
    static int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                            double streamTime, RtAudioStreamStatus status, void* userData);
    
    static void midiCallback(double deltatime, std::vector<uint8_t> *pMsg, void *userData);
    
    int portNumber = 0;
    std::string csdFileAndPath = "";
    bool debugMode = false; // Debug mode flag to run the app without a file
    

};

#endif // CABBAGEAUDIOAPP_H
