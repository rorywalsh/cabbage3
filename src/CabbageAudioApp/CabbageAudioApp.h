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
#include "platform/choc_ReenableAllWarnings.h"
#include <ixwebsocket/IXWebSocketServer.h>


#include "CabbageProcessor.h"
#include "WebSocketTestServer.h"
#include <readerwriterqueue.h>

class CabbageAudioApp {
public:
    
    enum class CommandType {
        KillProcessor,
        InitCabbage,
        StopAudio
    };
    
    struct AudioConfig {
        int audioDriverType = 0;
        std::string audioInDev = "";
        std::string audioOutDev = "";
        int audioInChanL = 1;
        int audioInChanR = 2;
        int audioOutChanL = 1;
        int audioOutChanR = 2;
        int bufferSize = 512;
        int audioSR = 44100;
        std::string midiInDev = "";
        std::string midiOutDev = "";
        int midiInChan = 0;
        int midiOutChan = 0;
        std::string jsSourceDirectory = "";

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
    AudioConfig audioConfig; // Audio configuration
    

    void onIdle();
    void sendWidgetDataToVscode();
    void addMessageToQueue(CabbageAudioApp::CommandType command){   messageQueue.enqueue(command);  }
    void setCsoundFile(std::string file){   csdFileAndPath = file;  }
    void scanAudioDevices(); // Scan and populate settings with available audio/MIDI devices
    void initialiseCabbage(); // Initialize Cabbage if CSD file exists
    
    // Test-related methods
    size_t getMessageQueueSize() const { return messageQueue.size_approx(); }
    void startWebSocketServerForTesting();
    void stopWebSocketServerForTesting();
    bool initialiseWebSocketConnection();
    
    std::unique_ptr<WebSocketTestServer> testServer;
private:
    void hostCallback(CabbageOpcodeData data);
    ix::WebSocket webSocket;
    bool createCabbageProcessor();
    void initialiseAudio(bool startStream);
    void initialiseMidi();
    void deinitAudioAndMidi();
    
    // Websocket server - for communication with vscode
    ix::WebSocketServer webSocketServer;
    
    
    // Functions for running test server - for tests without vscode
    bool shouldStartTestServer = false;
    moodycamel::ReaderWriterQueue<CabbageAudioApp::CommandType> messageQueue;
    
    // Return a valid device ID for a given device name
    int getAudioDeviceId(const std::string& deviceName) const;
    
    // Settings functions
    void addDevicesToSettings(const std::string& settingsFile);
    
    std::unique_ptr<RtAudio> audioDevice = nullptr;
    std::unique_ptr<RtMidiIn> midiInDevice = nullptr;
    std::unique_ptr<RtMidiOut> midiOutDevice = nullptr;
    int midiOutChannel = -1;
    int midiInChannel = -1;

    unsigned int numOutputChannels = 2; // Number of audio channels
    unsigned int numInputChannels = 1;
    std::atomic<bool> canProcessAudio{false}; // Flag to track stream state
    std::atomic<bool> canDestroyProcessor{false}; // Flag to track stream state
    float** emptyInputBuffer; // Preallocated empty input buffer
    bool emptyInputBufferInitialized = false; // Flag to check if the buffer is initialized
    unsigned int bufferSize; // Size of the audio buffer (in frames)

    float** getEmptyInputBuffer() const;
    unsigned int getNumInputChannels() const;
    unsigned int getNumOutputChannels() const;

    // Callbacks for audio and midi
    static int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                            double streamTime, RtAudioStreamStatus status, void* userData);
    
    static void midiCallback(double deltatime, std::vector<uint8_t> *pMsg, void *userData);
    
    int portNumber = 0;
    std::string csdFileAndPath = "";

};

#endif // CABBAGEAUDIOAPP_H
