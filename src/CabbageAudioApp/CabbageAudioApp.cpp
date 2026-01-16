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

#include "CabbageAudioApp.h"
#include <iostream>
#include <algorithm>
#include "argparse.hpp"
#include <filesystem>

//==============================================================================
// Constructor - responsible for creating processor and initialising audio/midi
// and stdin/stdout connection to vscode
//==============================================================================
CabbageAudioApp::CabbageAudioApp(int argc, char *argv[]) : bufferSize(512)
{
    // Parse command line flags. If not valid file is passed, we wait
    // for stdin message to load a file instead. In this way we can debug
    // the app without having to pass a file from vscode on startup.
    parseComandLineArgs(argc, argv);
}
//==============================================================================
CabbageAudioApp::~CabbageAudioApp()
{
    closeAudioDevice();

    // Stop stdin thread
    shouldStopStdinThread = true;
    if (stdinThread.joinable())
    {
        stdinThread.join();
    }
}

void CabbageAudioApp::closeAudioDevice()
{
    // Check if audioDevice exists before trying to use it
    if (!audioDevice)
    {
        return;
    }

    try
    {
        if (audioDevice->isStreamOpen())
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

    if (emptyInputBufferInitialised)
    {
        // Clean up the preallocated empty input buffer
        for (unsigned int ch = 0; ch < numInputChannels; ++ch)
        {
            delete[] emptyInputBuffer[ch];
        }

        delete[] emptyInputBuffer;
    }
}

//==============================================================================
// Parse command line arguments
//==============================================================================
bool CabbageAudioApp::parseComandLineArgs(int argc, char *argv[])
{
    argparse::ArgumentParser program("CabbageApp");

    // Define the --file argument (required string)
    program.add_argument("--file")
        .help("Path to the CSD file")
        .default_value(std::string("null")); // Default to empty string if not provided

    // Define the --portNumber argument (integer)
    program.add_argument("--portNumber")
        .help("Port number for the server")
        .default_value(9991)
        .action(
            [](const std::string &value)
            {
                return std::stoi(value); // Convert the string to an integer
            });

    try
    {
        // Parse command-line arguments
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        // If an error occurs, print it and exit
        lattice::logDebug << err.what();
    }

    // Retrieve the parsed arguments. If no file is given, launch anyway and listen for a file
    // to be sent via stdin from VS Code extension
    if (program.get<std::string>("--file") != "null")
        csdFileAndPath = std::filesystem::absolute(program.get<std::string>("--file")).string();

    portNumber = program.get<int>("--portNumber");
    return true;
}

//==============================================================================
// Scan available audio/MIDI devices and populate settings file
//==============================================================================
void CabbageAudioApp::scanAudioDevices()
{
    // quickly init/deinit audio in order to query devices..
    initialiseAudio(false);
    initialiseMidi();
    deinitAudioAndMidi();
}

//==============================================================================
// Initialize Cabbage if CSD file exists - for debug purposes
//==============================================================================
void CabbageAudioApp::initialiseCabbage()
{
    if (lattice::File::exists(csdFileAndPath))
    {
        createCabbageProcessor();
    }
}

//==============================================================================
// This method gets call via the processor - whenever Csound updates some widgets
// through calls to cabbageSet opcodes it will send the data to the vscode frontend
//==============================================================================
void CabbageAudioApp::hostCallback(CabbageOpcodeData data)
{
    auto updatedOpt = processor->processOpcodeData(data);
    if (updatedOpt.has_value())
    {
        auto &j = updatedOpt.value();
        nlohmann::json msg;
        msg["command"] = "widgetUpdate";
        msg["id"] = data.channel;

        if (data.type == CabbageOpcodeData::MessageType::Value)
        {
            msg["value"] = j["value"].get<float>();
            sendJsonMessage(msg);
        }
        else if (data.type == CabbageOpcodeData::MessageType::Widget)
        {
            msg["widgetJson"] = j.dump();
            sendJsonMessage(msg);
            // Small delay only for widget creation to prevent stdout buffer overflow
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        else
        {
            msg["widgetJson"] = j.dump();
            sendJsonMessage(msg);
        }
    }
}

//==============================================================================
// Send JSON message to stdout (to be read by VS Code extension)
//==============================================================================
void CabbageAudioApp::sendJsonMessage(const nlohmann::json &msg)
{
    std::lock_guard<std::mutex> lock(stdoutMutex);

    // Ensure stdout is unbuffered for immediate delivery
    std::cout << "CABBAGE_JSON:" << msg.dump() << std::endl;
    std::cout.flush(); // Explicit flush to ensure message is sent immediately
}

//==============================================================================
// Sets up stdin/stdout communication with VS Code extension
//==============================================================================
bool CabbageAudioApp::initialiseStdioConnection()
{
    lattice::logDebug << "Initializing stdin/stdout communication with VS Code";

    // Start a thread to read from stdin
    stdinThread = std::thread(
        [this]()
        {
            std::string line;
            while (!shouldStopStdinThread && std::getline(std::cin, line))
            {
                if (!line.empty())
                {
                    processIncomingMessage(line);
                }
            }
        });

    // If we have a CSD file at startup, send widget data
    if (!csdFileAndPath.empty())
    {
        sendWidgetDataToVscode();
    }

    return true;
}

//==============================================================================
// Process incoming JSON message from stdin
//==============================================================================
void CabbageAudioApp::processIncomingMessage(const std::string &message)
{
    try
    {
        auto json = nlohmann::json::parse(message, nullptr, false);
        const std::string command = json["command"];
        nlohmann::json jsonObj;

        // Handle both old obj wrapper format and new direct properties format
        if (json.contains("obj"))
        {
            //"obj" can be a string when coming from vscode - but will always be
            // an object when testing outside vscode
            if (json["obj"].is_string())
                jsonObj = nlohmann::json::parse(json["obj"].get<std::string>());
            else
                jsonObj = json["obj"];
        }
        else
        {
            // New format: properties are directly on the message
            jsonObj = json;
            jsonObj.erase("command"); // Remove command field from the object
        }

        // Add command back to jsonObj for Engine processing
        jsonObj["command"] = command;

        // Try to handle with Engine first (only if processor exists)
        if (processor && processor->getCabbageEngine().processWebViewCommand(jsonObj))
        {
            lattice::logDebug << "Command handled by Engine: " << command;
            return;
        }

        // Handle environment-specific commands
        if (command == "cabbageIsReadyToLoad")
        {
            if (!processor)
            {
                // Webview loaded before processor was created - this is normal on initial load.
                // Webview will send cabbageIsReadyToLoad again after widgets are received.
                lattice::logDebug << "Received cabbageIsReadyToLoad but processor not ready yet - ignoring";
                return;
            }
            lattice::logDebug << "Received cabbageIsReadyToLoad - enabling message dequeuing";
            // Signal that webview is ready - this enables message dequeuing
            // Now queued table data and other updates will be processed and sent
            processor->setCabbageIsReady();
        }

        else if (command == "parameterChange")
        {
            if (!processor)
            {
                lattice::logInfo << "Processor is null! Cannot process parameterChange.";
                return;
            }
            auto &cabbage = processor->getCabbageEngine();
            cabbage.handleParameterUpdate(jsonObj);
            // Engine handles everything for standalone (no gesture handling needed)
        }

        else if (command == "fileOpenFromVSCode")
        {
            if (!processor)
            {
                lattice::logInfo << "Processor is null! Cannot process fileOpenFromVSCode.";
                return;
            }

            auto &cabbage = processor->getCabbageEngine();
            if (jsonObj.contains("fileName"))
            {
                cabbage.setStringChannel(jsonObj["channel"].get<std::string>(), jsonObj["fileName"].get<std::string>());
            }
        }

        else if (command == "onFileChanged")
        {
            lattice::logDebug << "Received onFileChanged message for file: "
                              << json["lastSavedFileName"].get<std::string>();
            csdFileAndPath = json["lastSavedFileName"].get<std::string>();
            if (lattice::File::exists(csdFileAndPath))
            {
                lattice::logDebug << "File exists, adding InitCabbage to queue";
                // push this to FIFO queue on main thread..
                addMessageToQueue(CabbageAudioApp::CommandType::InitCabbage);
            }
            else
            {
                lattice::logDebug << "File does not exist";
            }
        }

        else if (command == "midiMessage")
        {
            if (!processor)
            {
                lattice::logInfo << "Processor is null! Cannot process midiMessage.";
                return;
            }

            processor->addNoteEventFromJson(jsonObj);
        }

        else if (command == "stopAudio")
        {
            // when VS Code tries to end the process, it first send a stopAudio message..
            // push this to FIFO queue on main thread..
            addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
            addMessageToQueue(CabbageAudioApp::CommandType::KillProcessor);
        }

        else
        {
            // lattice::logDebug << "received message: " << message;
        }
    }
    catch (nlohmann::json::exception &e)
    {
        lattice::logDebug << "Error:" << e.what() << " - ";
        lattice::logDebug << message;
    }
}

//==============================================================================
void CabbageAudioApp::sendWidgetDataToVscode()
{
    if (!processor)
        return;

    auto &cabbage = processor->getCabbageEngine();

    for (auto &w : cabbage.getWidgets())
    {
        // Skip genTable widgets - they will be sent via opcode queue with samples
        if (w.contains("type") && w["type"] == "genTable")
        {
            continue;
        }
        
        nlohmann::json msg;
        msg["command"] = "widgetUpdate";
        // Use id if available, otherwise fallback to channel
        if (w.contains("id") && w["id"].is_string())
        {
            msg["id"] = w["id"];
        }
        else if (w.contains("channels") && w["channels"].is_array() && !w["channels"].empty() &&
                 w["channels"][0].contains("id"))
        {
            msg["id"] = w["channels"][0]["id"];
        }
        else
        {
            msg["id"] = w["channel"];
        }
        msg["widgetJson"] = w.dump();
        sendJsonMessage(msg);
    }

    // Note: Don't call setCabbageIsReady() here. 
    // It should only be called when webview explicitly sends cabbageIsReadyToLoad,
    // otherwise queued messages (like table data) will be sent before webview is connected.
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

int CabbageAudioApp::getAudioDeviceId(const std::string &deviceName) const
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
bool CabbageAudioApp::createCabbageProcessor()
{
    canProcessAudio.store(false);
    canDestroyProcessor.store(false);
    numInputChannels = cabbage::File::getNumberOfInputChannels(csdFileAndPath);
    numOutputChannels = cabbage::File::getNumberOfOutputChannels(csdFileAndPath);

    // Init audio and MIDI
    initialiseAudio(true);
    initialiseMidi();

    std::stringstream config;
    config << std::to_string(getNumInputChannels()) << "-" << std::to_string(getNumOutputChannels());

    processor = std::make_unique<CabbageProcessor>(csdFileAndPath, config.str());

    if (!processor->getCabbageEngine().csdCompiledWithoutError())
    {
        lattice::logDebug << "Coudn't compile Csound...";
        return false;
    }

    lattice::logDebug << "Num widgets : " << processor->getCabbageEngine().getWidgets().size();

    // Preallocate the empty input buffer in case of no input device
    emptyInputBuffer = new float *[numInputChannels];
    for (unsigned int ch = 0; ch < numInputChannels; ++ch)
    {
        emptyInputBuffer[ch] = new float[bufferSize];
        std::fill(emptyInputBuffer[ch], emptyInputBuffer[ch] + bufferSize, 0.0f); // Initialize with zeros
    }

    emptyInputBufferInitialised = true;

    // Register callback - will be triggered from CabbageProcessor
    processor->hostCallback = [&](CabbageOpcodeData data) { hostCallback(data); };

    canProcessAudio.store(true);

    return processor->getCabbageEngine().csdCompiledWithoutError();
}

//==============================================================================
// Initialise rtaudio - set up divers, etc
//==============================================================================
void CabbageAudioApp::initialiseAudio(bool startStream)
{
    // Create an instance of RtAudio
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    canProcessAudio.store(false);

    if (!audioDevice)
    {
#if defined LATTICE_WINDOWS
        audioDevice = std::make_unique<RtAudio>(
            (audioConfig.audioDriverType == RtAudio::Api::WINDOWS_ASIO) ? RtAudio::WINDOWS_ASIO : RtAudio::WINDOWS_DS,
            errorCallback);
#elif defined LATTICE_MACOS
        audioDevice = std::make_unique<RtAudio>(RtAudio::Api::MACOSX_CORE, errorCallback);
#else
        audioDevice = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
#endif
    }
    else
    {
        // Optionally stop/close existing stream before reusing
        //        if (audioDevice->isStreamRunning())
        //            audioDevice->stopStream();
        //        if (audioDevice->isStreamOpen())
        //            audioDevice->closeStream();
    }

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
    const int outputDeviceId = getAudioDeviceId(audioConfig.audioOutDev);
    ;
    outputParameters.deviceId = outputDeviceId != -1 ? outputDeviceId : audioDevice->getDefaultOutputDevice();

    // the outputs are set by the Csound header..
    const int availableOutputs = audioDevice->getDeviceInfo(outputParameters.deviceId).outputChannels;
    outputParameters.nChannels = getNumOutputChannels() > availableOutputs ? availableOutputs : getNumOutputChannels();
    outputParameters.firstChannel = 0;
    numOutputChannels = outputParameters.nChannels;

    // Set up input stream parameters
    RtAudio::StreamParameters inputParameters;
    const int inputDeviceId = getAudioDeviceId(audioConfig.audioInDev);
    ;
    inputParameters.deviceId = inputDeviceId != -1 ? inputDeviceId : audioDevice->getDefaultInputDevice();

    // the inputs are set by the Csound header..
    const int availableInputs = audioDevice->getDeviceInfo(inputParameters.deviceId).inputChannels;
    inputParameters.nChannels = getNumInputChannels() > availableInputs ? availableInputs : getNumInputChannels();
    inputParameters.firstChannel = 0;
    numInputChannels = inputParameters.nChannels;

    unsigned int sampleRate = audioConfig.audioSR;
    unsigned int bufferFrames = audioConfig.bufferSize;

    if (startStream)
    {
        lattice::logDebug << "Attempting to start audio with the following settings:\nSR: " << audioConfig.audioSR
                          << "\nBuffer Size: " << audioConfig.bufferSize << "\nInput device: " << audioConfig.audioInDev
                          << "\nNumber of input channels: " << inputParameters.nChannels
                          << "\nOutput device: " << audioConfig.audioOutDev
                          << "\nNumber of output channels: " << outputParameters.nChannels;

        try
        {
            // Open the audio stream. If the selected audio input device has no
            // channels, i.e., it's not valid, pass nullptr for input stream
            audioDevice->openStream(&outputParameters, inputParameters.nChannels == 0 ? nullptr : &inputParameters,
                                    RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &CabbageAudioApp::audioCallback,
                                    this); // Pass 'this' as userData

            audioDevice->startStream();
        }
        catch (const std::runtime_error &e)
        {
            lattice::logDebug << "Error: " << e.what();
            return;
        }
    }
}

//==============================================================================
// Clean up audio and MIDI resources
//==============================================================================
void CabbageAudioApp::deinitAudioAndMidi()
{
    // Stop and close audio stream if running
    try
    {
        audioDevice->stopStream();
    }
    catch (const std::runtime_error &e)
    {
        lattice::logDebug << "Error stopping audio stream: " << e.what();
    }

    if (audioDevice->isStreamOpen())
    {
        lattice::logInfo << "Closing audio stream...";
        audioDevice->closeStream();
    }

    // Clean up MIDI devices
    if (midiInDevice)
    {
        midiInDevice->cancelCallback();
        midiInDevice->closePort();
        midiInDevice = nullptr;
    }

    if (midiOutDevice)
    {
        midiOutDevice->closePort();
        midiOutDevice = nullptr;
    }

    // Clean up empty input buffer if it was initialised
    if (emptyInputBufferInitialised)
    {
        lattice::logInfo << "Cleaning up empty input buffer...";
        for (unsigned int ch = 0; ch < numInputChannels; ++ch)
        {
            delete[] emptyInputBuffer[ch];
        }
        delete[] emptyInputBuffer;
        emptyInputBufferInitialised = false;
    }

    // Reset audio device
    audioDevice = nullptr;

    lattice::logDebug << "Audio and MIDI devices successfully deinitialised";
}

//============================================================================
void CabbageAudioApp::errorCallback(RtAudioErrorType type, const std::string &errorText)
{
    lattice::logDebug << errorText;
}

float **CabbageAudioApp::getEmptyInputBuffer() const
{
    return emptyInputBuffer;
}

unsigned int CabbageAudioApp::getNumInputChannels() const
{
    return numInputChannels;
}

unsigned int CabbageAudioApp::getNumOutputChannels() const
{
    return numOutputChannels;
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
    int16_t key = -1;          // MIDI note number
    double velocity = 0.0;     // MIDI velocity (normalized to 0.0-1.0)
    int32_t noteId = -1;       // Unique note ID (you can generate this if needed)
    uint32_t sampleOffset = 0; // Sample offset (you can calculate this if needed)

    // Handle Note On and Note Off messages
    if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) // Note On (0x90) or Note Off (0x80)
    {
        if (msg->size() >= 2)
        {
            key = static_cast<int16_t>((*msg)[1]);             // MIDI note number
            velocity = static_cast<double>((*msg)[2]) / 127.0; // Normalize velocity to 0.0-1.0

            // For Note Off messages with velocity 0, treat them as Note On with velocity 0
            if ((status & 0xF0) == 0x80 || velocity == 0.0)
            {
                type = lattice::NoteEvent::Type::noteOff; // Mark as Note Off
                velocity = 0.0;                           // Force velocity to 0 for Note Off
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

// The stdin thread picks up messages, some of which require action
// on the main thread, i.e, resetting Csound. Hence the onIdle() callback
void CabbageAudioApp::onIdle()
{
    CabbageAudioApp::CommandType command;

    while (messageQueue.try_dequeue(command))
    {
        switch (command)
        {
        case CommandType::KillProcessor:
            if (processor.get())
            {
                processor.reset();
            }
            break;

        case CommandType::InitCabbage:
            lattice::logDebug << "Processing InitCabbage command";
            if (createCabbageProcessor())
            {
                lattice::logDebug << "Cabbage processor created successfully";
                // If this is a recompile (UI already open), enable dequeuing immediately
                // so that queued genTable updates are sent
                processor->setCabbageIsReady();
                sendWidgetDataToVscode();
            }
            else
            {
                lattice::logDebug << "Failed to create Cabbage processor";
                nlohmann::json msg;
                msg["command"] = "failedToCompile";
                sendJsonMessage(msg);
            }
            break;

        case CommandType::StopAudio:
            canProcessAudio.store(false);

            // If no audio device is running, set canDestroyProcessor to true immediately
            // This prevents deadlock in test environments where audio callback never runs
            if (!audioDevice || !audioDevice->isStreamRunning())
            {
                canDestroyProcessor.store(true);
            }
            else
            {
                // Wait for canDestroyProcessor with timeout to prevent deadlock
                auto startWait = std::chrono::steady_clock::now();
                auto timeout = std::chrono::milliseconds(1000); // 1 second timeout
                while (!canDestroyProcessor.load() && (std::chrono::steady_clock::now() - startWait) < timeout)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            // ensure idle thread has stopped..
            if (processor)
                processor->stopIdleThread();

            if (audioDevice)
            {
                audioDevice->stopStream();
                audioDevice->closeStream();
            }
            break;
        }
    }
}

int CabbageAudioApp::audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                   double /*streamTime*/, RtAudioStreamStatus /*status*/, void *userData)
{
    // Cast userData to CabbageAudioApp*
    CabbageAudioApp *app = static_cast<CabbageAudioApp *>(userData);

    // Cast buffers to float*
    float *myfltInputBuffer = static_cast<float *>(inputBuffer);
    float *myfltOutputBuffer = static_cast<float *>(outputBuffer);

    // Get the number of input and output channels
    unsigned int numInputChannels = app->getNumInputChannels();
    unsigned int numOutputChannels = app->getNumOutputChannels();

    // Deinterleave the input buffer into separate channels
    float **deinterleavedInput = nullptr;
    if (myfltInputBuffer && numInputChannels > 0)
    {
        deinterleavedInput = new float *[numInputChannels];
        for (unsigned int ch = 0; ch < numInputChannels; ++ch)
        {
            deinterleavedInput[ch] = new float[nBufferFrames];
            for (unsigned int i = 0; i < nBufferFrames; ++i)
            {
                deinterleavedInput[ch][i] = myfltInputBuffer[i * numInputChannels + ch];
            }
        }
    }
    else
    {
        // Use the preallocated empty input buffer
        deinterleavedInput = app->getEmptyInputBuffer();
    }

    // Deinterleave the output buffer into separate channels
    float **deinterleavedOutput = new float *[numOutputChannels];
    for (unsigned int ch = 0; ch < numOutputChannels; ++ch)
    {
        deinterleavedOutput[ch] = new float[nBufferFrames];
        // Initialize to silence
        memset(deinterleavedOutput[ch], 0, nBufferFrames * sizeof(float));
    }

    // Pass the deinterleaved buffers to the process method
    if (app->canProcessAudio.load())
    {
        app->canDestroyProcessor.store(true);
        app->processor->process(deinterleavedInput, deinterleavedOutput, nBufferFrames);
    }

    // Interleave the processed output back into the RtAudio buffer
    for (unsigned int ch = 0; ch < numOutputChannels; ++ch)
    {
        for (unsigned int i = 0; i < nBufferFrames; ++i)
        {
            myfltOutputBuffer[i * numOutputChannels + ch] = deinterleavedOutput[ch][i];
        }
    }

    // Clean up the deinterleaved buffers (except the preallocated empty input buffer)
    if (myfltInputBuffer && numInputChannels > 0)
    {
        for (unsigned int ch = 0; ch < numInputChannels; ++ch)
        {
            delete[] deinterleavedInput[ch];
        }
        delete[] deinterleavedInput;
    }
    for (unsigned int ch = 0; ch < numOutputChannels; ++ch)
    {
        delete[] deinterleavedOutput[ch];
    }
    delete[] deinterleavedOutput;

    return 0;
}

//============================================================================================
void CabbageAudioApp::addDevicesToSettings(const std::string &settingsPath)
{
    std::ifstream file(settingsPath);
    if (!file)
    {
        lattice::logDebug << "Error: Could not open settings file: " << settingsPath;
        return;
    }

    nlohmann::json settingsJson;

    // Check if file is empty before parsing
    if (file.peek() != std::ifstream::traits_type::eof())
    {
        try
        {
            file >> settingsJson; // Parse existing JSON
        }
        catch (nlohmann::json::exception &e)
        {
            lattice::logDebug << "Error parsing JSON: " << e.what();
            file.close();
            return;
        }
    }
    else
    {
        lattice::logDebug << "Settings file is empty. Starting with a blank JSON object.";
    }

    file.close(); // Close read mode

    try
    {
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

#ifdef LATTICE_WINDOWS
        settingsJson["systemAudioMidiIOListing"]["audioDrivers"] = {"DirectSound", "ASIO"};
#elif defined LATTICE_MACOS
        settingsJson["systemAudioMidiIOListing"]["audioDrivers"] = "CoreAudio";
#else
        settingsJson["systemAudioMidiIOListing"]["audioDrivers"] = {"Pulse", "Alsa", "Jack"};
#endif

        // Write updated JSON back to file
        std::ofstream outFile(settingsPath);
        if (!outFile)
        {
            lattice::logDebug << "Error: Could not open settings file for writing: " << settingsPath;
            return;
        }
        outFile << std::setw(4) << settingsJson;
        outFile.close();

        lattice::logDebug << "Devices successfully added to settings file.";
    }
    catch (nlohmann::json::exception &e)
    {
        lattice::logDebug << "Error processing JSON: " << e.what();
    }
}
