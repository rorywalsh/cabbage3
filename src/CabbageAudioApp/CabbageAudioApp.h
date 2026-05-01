/*
 * Copyright (c) 2024 Rory Walsh
 *
 * This file is part of Cabbage3
 *
 * Cabbage3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cabbage3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cabbage3.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CABBAGEAUDIOAPP_H
#define CABBAGEAUDIOAPP_H

#include <RtAudio.h>
#include <RtMidi.h>

#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

#include "choc/platform/choc_DisableAllWarnings.h"
#include "choc/platform/choc_ReenableAllWarnings.h"

#include "CabbageProcessor.h"
#include "CabbageAudioRecorder.h"
#include <concurrentqueue.h>

class CabbageAudioApp
{
  public:
    enum class CommandType
    {
        KillProcessor,
        InitCabbage,
        StopAudio
    };

    struct AudioConfig
    {
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

        bool loadFromJson(const std::string &settingsPath)
        {
            std::ifstream file(settingsPath);
            if (!file)
            {
                std::cerr << "Error: Could not open settings file: " << settingsPath << std::endl;
                return false;
            }

            try
            {
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

                // Handle jsSourceDir as array or string for backward compatibility
                if (settingsJson["currentConfig"].contains("jsSourceDir"))
                {
                    const auto &jsSourceDirValue = settingsJson["currentConfig"]["jsSourceDir"];
                    if (jsSourceDirValue.is_array() && !jsSourceDirValue.empty())
                    {
                        jsSourceDirectory = jsSourceDirValue[0].get<std::string>(); // Use first directory
                    }
                    else if (jsSourceDirValue.is_string())
                    {
                        jsSourceDirectory = jsSourceDirValue.get<std::string>();
                    }
                    else
                    {
                        jsSourceDirectory = "add path to JS src directory";
                    }
                }
                else
                {
                    jsSourceDirectory = "add path to JS src directory";
                }
            }
            catch (nlohmann::json::exception &e)
            {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                return false;
            }

            return true;
        }
    };

    CabbageAudioApp(int argc, char *argv[]);
    ~CabbageAudioApp();
    void closeAudioDevice();
    bool parseComandLineArgs(int argc, char *argv[]);

    bool isStreamRunning() const;
    static void errorCallback(RtAudioErrorType type, const std::string &errorText);

    std::unique_ptr<CabbageProcessor> processor; // Main processor
    AudioConfig audioConfig;                     // Audio configuration

    void onIdle();
    void sendWidgetDataToVscode();
    void addMessageToQueue(CabbageAudioApp::CommandType command) { messageQueue.enqueue(command); }
    void setCsoundFile(std::string file) { csdFileAndPath = file; }
    void scanAudioDevices();  // Scan and populate settings with available audio/MIDI devices
    void initialiseCabbage(); // Initialize Cabbage if CSD file exists

    // Test-related methods
    size_t getMessageQueueSize() const { return messageQueue.size_approx(); }
    bool initialiseStdioConnection();

    bool getCanDestroyProcessor() const { return canDestroyProcessor.load(); }
    bool getAudioShutdownComplete() const { return audioShutdownComplete.load(); }

    void hostCallback(CabbageOpcodeData data);

    // Flag set after Csound initialization to signal that signal handlers need re-registration
    // (Csound installs its own handlers that override ours)
    std::atomic<bool> needsSignalHandlerReset{false};

  private:
    void sendJsonMessage(const nlohmann::json &msg);
    void processIncomingMessage(const std::string &message);
    bool createCabbageProcessor();
    void initialiseAudio(bool startStream);
    void initialiseMidi();
    void deinitAudioAndMidi();

    // stdin/stdout communication thread
    std::thread stdinThread;
    std::atomic<bool> shouldStopStdinThread{false};
    std::mutex stdoutMutex; // Protect stdout writes

    moodycamel::ConcurrentQueue<CabbageAudioApp::CommandType> messageQueue;

    // Return a valid device ID for a given device name
    int getAudioDeviceId(const std::string &deviceName) const;

    // Settings functions
    void addDevicesToSettings(const std::string &settingsFile);

    std::unique_ptr<RtAudio> audioDevice = nullptr;
    std::unique_ptr<RtMidiIn> midiInDevice = nullptr;
    std::unique_ptr<RtMidiOut> midiOutDevice = nullptr;
    int midiOutChannel = -1;
    int midiInChannel = -1;

    // Audio recording
    std::unique_ptr<cabbage::AudioRecorder> recorder = nullptr;

    unsigned int numOutputChannels = 2; // Number of audio channels
    unsigned int numInputChannels = 1;
    std::atomic<bool> canProcessAudio{false};     // Flag to track stream state
    std::atomic<bool> canDestroyProcessor{false}; // Flag to track stream state
    std::atomic<bool> audioShutdownComplete{false};
    float **emptyInputBuffer; // Preallocated empty input buffer
    float **getEmptyInputBuffer() const;
    bool emptyInputBufferInitialised = false; // Flag to check if the buffer is initialised
    unsigned int bufferSize;                  // Size of the audio buffer (in frames)

    unsigned int getNumInputChannels() const;
    unsigned int getNumOutputChannels() const;

    // Callbacks for audio and midi
    static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
                             RtAudioStreamStatus status, void *userData);

    static void midiCallback(double deltatime, std::vector<uint8_t> *pMsg, void *userData);

    // VU meter: per-output-channel levels updated by the audio thread,
    // read and reset by the idle thread. Sized to numOutputChannels in initialiseAudio().
    std::unique_ptr<std::atomic<float>[]> vuPeakLevels; // atomic max per buffer
    std::unique_ptr<std::atomic<float>[]> vuRmsLevels;  // latest RMS per buffer
    int vuIdleCounter = 0;

    int portNumber = 0;
    std::string csdFileAndPath = "";
};

#endif // CABBAGEAUDIOAPP_H
