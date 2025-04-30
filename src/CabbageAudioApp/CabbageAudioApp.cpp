#include "CabbageAudioApp.h"
#include <iostream>
#include <algorithm>
#include "argparse.hpp"
#include <filesystem>

//==============================================================================
// Constructor - responsible for creating processor and initialising audio/midi
// and websocket connection to vscode
//==============================================================================
CabbageAudioApp::CabbageAudioApp(int argc, char* argv[])
    : numChannels(2), bufferSize(512), isRunning(false)
{
    // Parse command line flags. If not valid file is passed, we wait 
    // for websocket message to load a file instead. In this way we can debug
    // the app without having to pass a file from vscode on startup.
    parseComandLineArgs(argc, argv);
     
    // Optionally start test server for development purposes
    if (shouldStartTestServer)
    {
        startWebSocketServerForTesting();
        testServer->setUpdateInterval(500);
        // Wait for the server to start (adjust delay if needed)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Init websocket client connection
    initialiseWebSocketConnection();
}
//==============================================================================
CabbageAudioApp::~CabbageAudioApp()
{
    if (isRunning)
    {
        try
        {
            lattice::logInfo << "Stopping rtaudio stream";
            audioDevice->stopStream();
        }
        catch (const std::runtime_error &e)
        {
            lattice::logDebug << "Error: " << e.what();
        }
        if (audioDevice->isStreamOpen())
        {
            lattice::logInfo << "Closing rtaudio stream";
            audioDevice->closeStream();
        }
    }
    
    if (emptyInputBufferInitialized)
    {
        // Clean up the preallocated empty input buffer
        for (unsigned int ch = 0; ch < numChannels; ++ch)
        {
            delete[] emptyInputBuffer[ch];
        }

        delete[] emptyInputBuffer;
    }
   
    
    // Stop test server if its running
    if(testServer)
        testServer->stop();
}

//==============================================================================
// Parse command line arguments
//==============================================================================
bool CabbageAudioApp::parseComandLineArgs(int argc, char* argv[])
{
    argparse::ArgumentParser program("CabbageApp");

    // Define the --file argument (required string)
    program.add_argument("--file")
        .help("Path to the CSD file")
        .required()  // Make it a required argument
        .default_value(std::string("")); // Default to empty string if not provided

    // Define the --portNumber argument (integer)
    program.add_argument("--portNumber")
        .help("Port number for the server")
        .required()  // Make it a required argument
        .action([](const std::string& value) {
            return std::stoi(value);  // Convert the string to an integer
        });

    // Define the --startTestServer argument (boolean)
    program.add_argument("--startTestServer")
        .help("Whether to start the test server (true/false)")
        .default_value(false)  // Default to false if not provided
        .action([](const std::string& value) {
            // Convert to lowercase first for case-insensitive comparison
            std::string lowerValue;
            lowerValue.resize(value.size());
            std::transform(value.begin(), value.end(), lowerValue.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            return lowerValue == "true";
        });

    try {
        // Parse command-line arguments
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        // If an error occurs, print it and exit
        lattice::logDebug << err.what();
    }

    // Retrieve the parsed arguments. If not file is given launch anyway, and listen for a file
    // to be sent over websocket connection
    csdFileAndPath = std::filesystem::absolute(program.get<std::string>("--file")).string();
    if (lattice::File::exists(csdFileAndPath))
    {
        initCabbage();
    }
    else
    {
        debugMode = true;
    }
        
    portNumber = program.get<int>("--portNumber");
    shouldStartTestServer = program.get<bool>("--startTestServer");
    return true;
    
}

//==============================================================================
// This method gets call via the processor - whenever Csound updates some widgets
// through calls to cabbageSet opcodes....
//==============================================================================
void CabbageAudioApp::hostCallback(CabbageOpcodeData data)
{
    auto &cabbage = processor->getCabbageEngine();
    auto widgetOpt = cabbage.getWidget(data.channel);
    
    if (widgetOpt.has_value())
    {
        auto &j = widgetOpt.value().get();

        // this will update a genTable
        if (j["type"].get<std::string>() == "genTable")
        {
            cabbage.updateFunctionTable(data, j);
            nlohmann::json msg;
            msg["command"] = "widgetUpdate";
            msg["channel"] = data.channel;
            msg["data"] = j.dump();
            webSocket.send(msg.dump());
        }
        else
        {
            if (data.type == CabbageOpcodeData::MessageType::Value)
            {
                nlohmann::json json;
                cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
                nlohmann::json msg;
                msg["command"] = "widgetUpdate";
                msg["channel"] = data.channel;
                msg["value"] = j["value"].get<float>();
                webSocket.send(msg.dump());
            }
            else
            {
                nlohmann::json json;
                cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
                nlohmann::json msg;
                msg["command"] = "widgetUpdate";
                msg["channel"] = data.channel;
                msg["data"] = j.dump();
                webSocket.send(msg.dump());
            }
        }
    }
}

//==============================================================================
// Simple test server for development and testing
//==============================================================================
void CabbageAudioApp::startWebSocketServerForTesting()
{
    auto csOptionsText = cabbage::File::getCsOptions(csdFileAndPath);
    
    std::regex rtMidiPattern(R"(-\+rtmidi\s*=\s*NULL)");
    bool testMidi = std::regex_search(csOptionsText, rtMidiPattern);
    
    if (!testServer)
    {
        // Setting repeatable to true - this ensure each test is the same
        testServer = std::make_unique<WebSocketTestServer>(*processor, portNumber, true);
        testServer->testMidi(testMidi);
    }
    
    testServer->initialise();
    testServer->start();

}

void CabbageAudioApp::stopWebSocketServerForTesting()
{
    if (testServer)
    {
        testServer->stop();
    }
}
//==============================================================================
// Sets upo websocket client and waits for connection from vscode - or test server
//==============================================================================
bool CabbageAudioApp::initialiseWebSocketConnection()
{
    std::string address("ws://localhost:");
    address.append(std::to_string(portNumber).c_str());
    lattice::logDebug << "Attempting to connect to WebSocket at " << address;
    webSocket.setUrl(address);

    webSocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr &msg)
        {
            auto &cabbage = processor->getCabbageEngine();
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                try
                {
                    auto json = nlohmann::json::parse(msg->str, nullptr, false);
                    const std::string command = json["command"];
                    nlohmann::json jsonObj;
                    if (json.contains("obj"))
                    {
                        //"obj" can be a string when coming from vscode - but will always be
                        // and object when testing outside vscode
                        if(json["obj"].is_string())
                            jsonObj = nlohmann::json::parse(json["obj"].get<std::string>());
                        else
                            jsonObj = json["obj"];
                    }
                    if (command == "parameterChange")
                    {
                        for (int i = 0; i < cabbage.getNumberOfParameters(); i++)
                        {
                            if (cabbage.getParameterChannel(i).name == jsonObj["channel"].get<std::string>())
                            {
                                // update underlying JSON object if the value has changed
                                auto widgetOpt = cabbage.getWidget(jsonObj["channel"]);
                                if (widgetOpt.has_value())
                                {
                                    auto &widgetObj = widgetOpt.value().get();
                                    widgetObj["value"] = jsonObj["value"].get<double>();
                                }
                                processor->setParameter(i, jsonObj["value"].get<double>());
                            }
                        }
                        //                            SendParameterValueFromUI(message["paramIdx"], message["value"]);
                    }
                    else if (command == "fileOpenFromVSCode")
                    {
                        if (jsonObj.contains("fileName"))
                        {
                            cabbage.setStringChannel(jsonObj["channel"].get<std::string>(),
                                                     jsonObj["fileName"].get<std::string>());
                        }
                    }
                    else if (command == "onFileChanged")
                    {
                        csdFileAndPath = json["lastSavedFileName"].get<std::string>();
                        if (lattice::File::exists(csdFileAndPath))
                        {
                            processor = nullptr;
                            initCabbage();
                            sendWidgetDataToVscode();
                        }
                    }

                    else if (command == "widgetStateUpdate")
                    {
                        cabbage.updateWidgetState(jsonObj);
                    }
                    else if (command == "midiMessage")
                    {
                        cabbage::Utils::check(false, "need to add this");
                    }
                    else if (command == "cabbageIsReadyToLoad")
                    {
                        nlohmann::json msg;
                        msg["command"] = "cabbageIsReadyToLoad";
                        msg["data"] = "";
                        webSocket.send(msg.dump());
                    }
                    else if (command == "stopCsound")
                    {
                        lattice::logDebug << "stopping Csound" << msg->str;
                        //                        processor->stopProcessing();
                    }
                    else if (command == "stopAudio")
                    {
                        //when VS Code tries to end the process, it first send a stopAudio message..
                        lattice::logDebug << "Closing audio and MIDI devices....";
                        if (audioDevice)
                            audioDevice->closeStream();
                    }
                    else
                    {
                        //lattice::logDebug << "received message: " << msg->str;
                    }
                }
                catch (nlohmann::json::exception &e)
                {
                    lattice::logDebug << "Error:" << e.what() << " - ";
                    lattice::logDebug << msg->str;
                    return false;
                }
            }
            else if (msg->type == ix::WebSocketMessageType::Open)
            {
                lattice::logDebug << "Connection established";

                if (csdFileAndPath.empty())
                {
                    lattice::logDebug << "Waiting for file to be sent from VS-Code";
                }
                else
                {
                    sendWidgetDataToVscode();
                }                
                
            }
            else if (msg->type == ix::WebSocketMessageType::Close)
            {
                lattice::logDebug << "websocket connection closed..";
            }
            else if (msg->type == ix::WebSocketMessageType::Error)
            {
                // Maybe SSL is not configured properly
                lattice::logDebug << "Connection error: " << msg->errorInfo.reason;
                // std::cout << "> " << std::flush;
            }

            return true;
        });

    // Now that our callback is setup, we can start our background thread and receive messages
    webSocket.start();

    return true;
}
//==============================================================================
void CabbageAudioApp::sendWidgetDataToVscode()
{
    // if connection is open we need to send all parse jSON objects to VS-Code..
    nlohmann::json msg;
    msg["command"] = "cabbageIsReadyToLoad";
    msg["data"] = "";
    webSocket.send(msg.dump());

    auto &cabbage = processor->getCabbageEngine();

    for (auto &w : cabbage.getWidgets())
    {
        nlohmann::json msg;
        msg["command"] = "widgetUpdate";
        msg["channel"] = w["channel"];
        msg["data"] = w.dump();
        webSocket.send(msg.dump());
    }

    processor->setCabbageIsReady();
}

//==============================================================================
// This method is called from the RtMidiIn callback
//==============================================================================
void CabbageAudioApp::initialiseMidi()
{
    try
    {
        midiInDevice = std::make_unique<RtMidiIn>();
    }
    catch (RtMidiError &error)
    {
        midiInDevice = nullptr;
        error.printMessage();
        return;
    }

    try
    {
        midiOutDevice = std::make_unique<RtMidiOut>();
    }
    catch (RtMidiError &error)
    {
        midiOutDevice = nullptr;
        error.printMessage();
        return;
    }

    midiInDevice->setCallback(&midiCallback, this);
    midiInDevice->ignoreTypes(false, true, false);

}

int CabbageAudioApp::getAudioDeviceId(const std::string& deviceName) const
{
    const auto ids = audioDevice->getDeviceIds();
    for (const auto &id : ids)
    {
        const auto name = audioDevice->getDeviceInfo(id).name;
        if (deviceName == name)
            return id;
    }

    return -1;
}

//==============================================================================
// Initialise Cabbage - create processor and set up audio and midi
//==============================================================================
void CabbageAudioApp::initCabbage()
{
    processor = std::make_unique<CabbageProcessor>(csdFileAndPath);

    // Preallocate the empty input buffer in case of no input device
    emptyInputBuffer = new float *[numChannels];
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        emptyInputBuffer[ch] = new float[bufferSize];
        std::fill(emptyInputBuffer[ch], emptyInputBuffer[ch] + bufferSize, 0.0f); // Initialize with zeros
    }

    emptyInputBufferInitialized = true;

    // Init audio and MIDI
    initialiseAudio();
    initialiseMidi();


    // Register callback - will be triggered from CabbageProcessor
    processor->hostCallback = [&](CabbageOpcodeData data) { hostCallback(data); };
}


//==============================================================================
// Initialise rtaudio - set up divers, etc
//==============================================================================
void CabbageAudioApp::initialiseAudio()
{
    // Create an instance of RtAudio
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    
    
#if defined LATTICE_WINDOWS
    if (audioConfig.audioDriverType == RtAudio::Api::WINDOWS_ASIO)
        audioDevice = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO, errorCallback);
    else
        audioDevice = std::make_unique<RtAudio>(RtAudio::WINDOWS_DS, errorCallback);
#elif defined LATTICE_MACOS
    // RtAudio::Api::MACOSX_CORE is default on MacOS
    audio = std::make_unique<RtAudio>(RtAudio::Api::MACOSX_CORE, errorCallback);
#else
    audio = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
#endif
    
    auto settingsFilePath = cabbage::File::getSettingsFile();
    addDevicesToSettings(settingsFilePath);
    audioConfig.loadFromJson(settingsFilePath);
    
    // Check if audio devices are available
    if (audioDevice->getDeviceCount() < 1)
    {
        lattice::logError << "No audio devices found!";
        return;
    }

    // Set up output stream parameters
    

    RtAudio::StreamParameters outputParameters;
    const int outputDeviceId = getAudioDeviceId(audioConfig.audioOutDev);;
    outputParameters.deviceId = outputDeviceId != -1 ? outputDeviceId : audioDevice->getDefaultOutputDevice();
    outputParameters.nChannels = audioDevice->getDeviceInfo(outputParameters.deviceId).outputChannels; 
    outputParameters.firstChannel = 0;

    
    // Set up input stream parameters
    RtAudio::StreamParameters inputParameters;
    const int inputDeviceId = getAudioDeviceId(audioConfig.audioInDev);;
    inputParameters.deviceId = inputDeviceId != -1 ? inputDeviceId : audioDevice->getDefaultInputDevice();
    inputParameters.nChannels = audioDevice->getDeviceInfo(inputParameters.deviceId).inputChannels; 
    unsigned int sampleRate = audioConfig.audioSR;
    unsigned int bufferFrames = audioConfig.bufferSize;

    lattice::logDebug << "Attempting to start audio with the following settings:\nSR: " << audioConfig.audioSR
                      << "\nBuffer Size: " << audioConfig.bufferSize << "\nInput device: " << audioConfig.audioInDev
                      << "\nNumber of input channels: " << inputParameters.nChannels << "\nOutput device: " << audioConfig.audioOutDev
                      << "\nNumber of output channels: " << outputParameters.nChannels;
    
    try
    {
        // Open the audio stream. If the selected audio input device has no
        // channels, i.e., it's not valid, pass nullptr for input stream
        audioDevice->openStream(&outputParameters,
                          inputParameters.nChannels == 0 ? nullptr : &inputParameters,
                          RTAUDIO_FLOAT32,
                          sampleRate,
                          &bufferFrames, 
                          &CabbageAudioApp::audioCallback,
                          this); // Pass 'this' as userData
        
        audioDevice->startStream();
        isRunning = true; // Mark the stream as running
    }
    catch (const std::runtime_error &e)
    {
        lattice::logDebug << "Error: " << e.what();
        return;
    }
}

void CabbageAudioApp::errorCallback(RtAudioErrorType type, const std::string &errorText)
{
    lattice::logDebug << errorText;
}

bool CabbageAudioApp::isStreamRunning() const
{
    return isRunning;
}

float **CabbageAudioApp::getEmptyInputBuffer() const
{
    return emptyInputBuffer;
}

unsigned int CabbageAudioApp::getNumChannels() const
{
    return numChannels;
}

void CabbageAudioApp::midiCallback(double deltatime, std::vector<uint8_t> *msg, void *userData)
{
    // Cast userData to CabbageAudioApp* (or whatever your app class is)
    CabbageAudioApp *app = static_cast<CabbageAudioApp *>(userData);

    // Ensure the MIDI message is not empty
    if (msg->empty())
    {
        lattice::logDebug << "Empty MIDI message received!";
        return;
    }

    // Extract the MIDI status byte (first byte)
    uint8_t status = (*msg)[0];

    // Determine the MIDI message type (note on, note off, etc.)
    lattice::NoteEvent::Type type = lattice::NoteEvent::Type::undefined;
    int16_t key = -1;              // MIDI note number
    double velocity = 0.0;         // MIDI velocity (normalized to 0.0-1.0)
    int32_t noteId = -1;           // Unique note ID (you can generate this if needed)
    uint32_t sampleOffset = 0;     // Sample offset (you can calculate this if needed)

    // Handle Note On and Note Off messages
    if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) // Note On (0x90) or Note Off (0x80)
    {
        if (msg->size() >= 2)
        {
            key = static_cast<int16_t>((*msg)[1]); // MIDI note number
            velocity = static_cast<double>((*msg)[2]) / 127.0; // Normalize velocity to 0.0-1.0

            // For Note Off messages with velocity 0, treat them as Note On with velocity 0
            if ((status & 0xF0) == 0x80 || velocity == 0.0)
            {
                type = lattice::NoteEvent::Type::noteOff; // Mark as Note Off
                velocity = 0.0; // Force velocity to 0 for Note Off
            }
            else
            {
                type = lattice::NoteEvent::Type::noteOn; // Mark as Note On
            }

            // Generate a unique note ID (you can use a counter or another method)
            static int32_t nextNoteId = 0;
            noteId = nextNoteId++;

            // Calculate sample offset (if needed)
            // For now, we'll set it to 0, but you can calculate it based on deltatime
            sampleOffset = static_cast<uint32_t>(deltatime * 44100); // Assuming 44100 Hz sample rate
        }
    }

    // Add the NoteEvent to the processor
    app->processor->addNoteEvent({type, key, velocity, noteId, sampleOffset});

}


int CabbageAudioApp::audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                   double streamTime, RtAudioStreamStatus status, void *userData)
{
    // Cast userData to CabbageAudioApp*
    CabbageAudioApp *app = static_cast<CabbageAudioApp *>(userData);

    // Cast buffers to float*
    float *floatInputBuffer = static_cast<float *>(inputBuffer);
    float *floatOutputBuffer = static_cast<float *>(outputBuffer);

    // Get the number of channels
    unsigned int numChannels = app->getNumChannels();

    // Deinterleave the input buffer into separate channels
    float **deinterleavedInput = nullptr;
    if (floatInputBuffer)
    {
        deinterleavedInput = new float *[numChannels];
        for (unsigned int ch = 0; ch < numChannels; ++ch)
        {
            deinterleavedInput[ch] = new float[nBufferFrames];
            for (unsigned int i = 0; i < nBufferFrames; ++i)
            {
                deinterleavedInput[ch][i] = floatInputBuffer[i * numChannels + ch];
            }
        }
    }
    else
    {
        // Use the preallocated empty input buffer
        deinterleavedInput = app->getEmptyInputBuffer();
    }

    // Deinterleave the output buffer into separate channels
    float **deinterleavedOutput = new float *[numChannels];
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        deinterleavedOutput[ch] = new float[nBufferFrames];
    }

    // Pass the deinterleaved buffers to the process method
    app->processor->process(deinterleavedInput, deinterleavedOutput, nBufferFrames);

    // Interleave the processed output back into the RtAudio buffer
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        for (unsigned int i = 0; i < nBufferFrames; ++i)
        {
            floatOutputBuffer[i * numChannels + ch] = deinterleavedOutput[ch][i];
        }
    }

    // Clean up the deinterleaved buffers (except the preallocated empty input buffer)
    if (floatInputBuffer)
    {
        for (unsigned int ch = 0; ch < numChannels; ++ch)
        {
            delete[] deinterleavedInput[ch];
        }
        delete[] deinterleavedInput;
    }
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        delete[] deinterleavedOutput[ch];
    }
    delete[] deinterleavedOutput;

    return 0;
}


//============================================================================================
void CabbageAudioApp::addDevicesToSettings(const std::string& settingsPath)
{
    std::ifstream file(settingsPath);
    if (!file) {
        lattice::logDebug << "Error: Could not open settings file: " << settingsPath;
        return;
    }

    nlohmann::json settingsJson;

    // Check if file is empty before parsing
    if (file.peek() != std::ifstream::traits_type::eof()) {
        try {
            file >> settingsJson; // Parse existing JSON
        } catch (nlohmann::json::exception& e) {
            lattice::logDebug << "Error parsing JSON: " << e.what();
            file.close();
            return;
        }
    } else {
        lattice::logDebug << "Settings file is empty. Starting with a blank JSON object.";
    }

    file.close(); // Close read mode

    try {
        RtAudio::DeviceInfo info;
        int inputCnt = 0;
        int outputCnt = 0;
        
        auto listOfCurrentDevices = audioDevice->getDeviceIds();

        for (unsigned int i = 0; i < listOfCurrentDevices.size(); i++)
        {
            info = audioDevice->getDeviceInfo(listOfCurrentDevices[i]);
            
            if (info.outputChannels > 0)
            {
                const std::string outputDevice = info.name;
                nlohmann::json j;
                j["deviceId"] = info.ID;
                j["numChannels"] = info.outputChannels;
                j["sampleRates"] = info.sampleRates;

                settingsJson["systemAudioMidiIOListing"]["audioOutputDevices"][outputDevice] = j;
                outputCnt++;
            }
            
            if (info.inputChannels > 0)
            {
                const std::string inputDevice = info.name;
                nlohmann::json j;
                j["deviceId"] = info.ID;
                j["numChannels"] = info.inputChannels;

                settingsJson["systemAudioMidiIOListing"]["audioInputDevices"][inputDevice] = j;
                inputCnt++;
            }
        }

        // Write updated JSON back to file
        std::ofstream outFile(settingsPath);
        if (!outFile) {
            lattice::logDebug << "Error: Could not open settings file for writing: " << settingsPath;
            return;
        }
        outFile << std::setw(4) << settingsJson;
        outFile.close();

        lattice::logDebug << "Devices successfully added to settings file.";

    } catch (nlohmann::json::exception& e) {
        lattice::logDebug << "Error processing JSON: " << e.what();
    }
}


