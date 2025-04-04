
#include "CabbageProcessor.h"
#include <iostream>

//========================================================================================
pluginType* LatticeProcessorPluginFactory::createPlugin(const clap_host* host)
{
    //create a new instance of CabbageProcessor 
    auto* processor = new CabbageProcessor();
    return new pluginType(host, *processor);
}
//========================================================================================

CabbageProcessor::CabbageProcessor(std::string csdFile)
    : Processor(), cabbage(*this, csdFile)
{

    if (!cabbage.setupCsound())
    {
        lattice::logInfo << "Csound could not be compiled";
        return;
    }

    // All message to webview will be wrapped in window.postMessage()
    setWebViewSendFunctionName("window.postMessage");
    
    addParameters();
    addChannels();

    auto rootPath = cabbage::File::getParentDirectory(cabbage::File::getCsdFileAndPath(cabbage.getCsdFile()));
    
    setMountPoint(rootPath);

    if (auto json = cabbage::File::parseCabbageSection(cabbage::File::getCsdFileAndPath(cabbage.getCsdFile())))
    {
        auto w = cabbage::Utils::findPropertyInForm<int>(*json, "size.width");
        auto h = cabbage::Utils::findPropertyInForm<int>(*json, "size.height");
        setEditorSize(w.value(), h.value());
    }
      
    startOnIdle();
  
}


//========================================================================================
// Add channels based on channelConfig property
//========================================================================================
void CabbageProcessor::addChannels()
{
    auto file = cabbage::File::getCsdFileAndPath(cabbage.getCsdFile());
    lattice::logInfo << file;
    
    cabbage::Utils::check(lattice::File::exists(file), "Can't find csd file");
    
    
    auto channelConfig = cabbage::Engine::getIOChannalConfig(file);
    auto [inputBuses, outputBuses] = cabbage.parseBusConfiguration(channelConfig);
    
    matchingNumInputsOutputs = inputBuses.size() == outputBuses.size();
    
    int inputBusIndex = 1;
    for (int bus : inputBuses)
    {
        addInputBus("Input Bus" + std::to_string(inputBusIndex), bus, lattice::ChannelLayout(bus));
        inputBusIndex++;
    }

    int outputBusIndex = 1;
    for (int bus : outputBuses)
    {
        addOutputBus("Output Bus" + std::to_string(outputBusIndex), bus, lattice::ChannelLayout(bus));
        outputBusIndex++;
    }
    
    auto config = getChannelConfig();
    totalNumInputs = config.getTotalNumInputChannels();
    totalNumOutputs = config.getTotalNumOutputChannels();
    
    
}

//========================================================================================
// Add parameters based on widget declarations
//========================================================================================
void CabbageProcessor::addParameters()
{
    std::vector<std::string> rangeTypes = cabbage.getRangeWidgetTypes(cabbage.getWidgets());
    for (auto &w : cabbage.getWidgets())
    {
        if (w.contains("automatable") && w["automatable"] == 1 &&
            (!w.contains("channelType") || w["channelType"] == "number"))
        {
            const std::string widgetType = w["type"].get<std::string>();

            try
            {
                // check if widget has a range - range widget parameters are initialised differently to other
                // widgets
                if (std::any_of(rangeTypes.begin(), rangeTypes.end(),
                                [&](const std::string &type) { return widgetType == type; }))
                {
                    addParameter({w["channel"].get<std::string>(), 
                        w["range"]["min"].get<float>(),
                        w["range"]["max"].get<float>(), 
                        w["range"]["defaultValue"].get<float>(),
                        w["range"]["increment"].get<float>(), 
                        w["range"]["skew"].get<float>()});
                }
                else
                {
                    addParameter({w["channel"].get<std::string>(), 
                        w["min"].get<float>(),
                        w["max"].get<float>(), 
                        w["defaultValue"].get<float>()});
                }

                cabbage.initParameter(w);
            }
            catch (nlohmann::json::exception &e)
            {
                lattice::logInfo << "JSON error: " << e.what() << "\n" << w.dump(4);
                cabbage::Utils::check(false, "");
            }
        }
    }
}

//========================================================================================
// Main processing function
//========================================================================================
void CabbageProcessor::process(float** inputs, float** outputs, std::size_t blockSize)
{
    // only process audio if Csound has compiled successfully.
    if (cabbage.csdCompiledWithoutError())
    {
        for (int i = 0; i < static_cast<int>(blockSize); i++, ++csndIndex)
        {
            if (csndIndex >= cabbage.getKsmps())
            {
                cabbage.performKsmps();
                csndIndex = 0;
            }

            // In cases where we have the same number of inputs/outputs we can read
            // and write in the same loop. In cases where we have a different number
            // of inputs/outputs we first iterate over the inputs, and then the outputs.
            // This adds a little overhead, hence this is only done when needed.
            if (matchingNumInputsOutputs)
            {
                for (int channel = 0; channel < totalNumOutputs; channel++)
                {
                    pos = csndIndex * totalNumOutputs;
                    cabbage.setSpIn(channel + pos, inputs[channel][i]);
                    // outputs[channel][i] = inputs[channel][i] + cabbage.getSpOut(channel + pos);
                    outputs[channel][i] = cabbage.getSpOut(channel + pos);
                }
            }
            else
            {
                // Process inputs first
                for (int inputChannel = 0; inputChannel < totalNumInputs; inputChannel++)
                {
                    pos = csndIndex * totalNumInputs; // Position in interleaved array
                    cabbage.setSpIn(inputChannel + pos, inputs[inputChannel][i]);
                }

                // Process outputs
                for (int outputChannel = 0; outputChannel < totalNumOutputs; outputChannel++)
                {
                    pos = csndIndex * totalNumOutputs; // Position in interleaved array
                    // Fill output buffer from Csound's processed output
                    outputs[outputChannel][i] = cabbage.getSpOut(outputChannel + pos);
                }
            }
        }
    }
    else
    {
        // calling this once here in case errors are missed in vscode logger
        cabbage.displayAndClearCompileErrors();

        // zero outputs so we don't get unwanted signal when csound fails
        for (int i = 0; i < static_cast<int>(blockSize); i++)
            for (int channel = 0; channel < totalNumOutputs; channel++)
                outputs[channel][i] = 0;
    }
}

//========================================================================================
// onIdle function
//========================================================================================
void CabbageProcessor::onIdle()
{
#ifndef CabbageApp
    if (uiIsOpen)
    {
#endif
        cabbage.processCsoundMessages();

#if defined(LINUX) && !defined(CabbageApp)
        nlohmann::json message;
        while (memoryQueue.receiveFromChild(message))
        {
            OnMessageFromWebView(message.dump(4).c_str());
        }
#endif

#ifndef CabbageApp
    }
#endif

    CabbageOpcodeData data;
    
    if (allowDequeuing)
    {
        while (cabbage.opcodeData.try_dequeue(data))
        {
            for (auto &widget : cabbage.getWidgets())
            {
                if (data.channel == cabbage::Parser::removeQuotes(widget["channel"]))
                {
                    cabbage::Parser::updateJson(widget, data.cabbageJson, widget.size());
                }
            }
            
#ifdef CabbageApp
            hostCallback(data);
#else
            cabbage.processCsoundMessages();
            updateWidgetData(data);
#endif
        }
    }
}

//========================================================================================
// this function will be called from the onIdle function if Csound has sent update messages
//========================================================================================
void CabbageProcessor::updateWidgetData(const CabbageOpcodeData &data)
{
    std::string updatedWidgetJson;

    if (data.type == CabbageOpcodeData::MessageType::Value)
    {
        updatedWidgetJson = cabbage.getUpdatedWidgetJsonStr(data.channel, data.cabbageJson["value"].get<float>());
    }
    else
    {
        auto widgetOpt = cabbage.getWidget(data.channel);
        if (widgetOpt.has_value())
        {
            auto &j = widgetOpt.value().get();
            if (j["type"].get<std::string>() == "genTable")
            {
                cabbage.updateFunctionTable(data, j);
            }
            cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
            updatedWidgetJson = cabbage.getUpdatedWidgetJsonStr(data.channel, j.dump());
        }
    }

    if (!updatedWidgetJson.empty())
        sendWebViewMessage(updatedWidgetJson);
}


void CabbageProcessor::onIdleScheduler()
{
    while (isIdleRunning)
    {
        idleCounter++;
        if (idleCounter % 100 == 0)
        {
            onIdle();
        }
    }
}

// Start idle thread in a separate non-realtime thread
void CabbageProcessor::startOnIdle()
{
    isIdleRunning = true;
    idleThread = std::thread(&CabbageProcessor::onIdleScheduler, this);
}

// Stop the idle background thread
void CabbageProcessor::stopOnIdle()
{
    isIdleRunning = false;
    
    if (idleThread.joinable())
    {
        idleThread.join();
    }
}

//========================================================================================
// Triggered when Cabbage is ready to start processing messages
//========================================================================================
void CabbageProcessor::setCabbageIsReady()
{
    lattice::logDebug << "Cabbage is now ready";
    uiIsOpen = true;
    allowDequeuing = true;
}

//========================================================================================
// Callback function - triggered when a message is sent from the webview
//========================================================================================
void CabbageProcessor::onMesssgeFromWebView(const nlohmann::json& j)
{
    // Incoming JSON message is always wrapped in []
    auto incomingMessage = j.at(0);

    if (incomingMessage["command"] == "cabbageIsReadyToLoad")
    {
        setCabbageIsReady();
        updateUI();
    }
    else if (incomingMessage["command"] == "parameterChange")
    {
        try
        {
            // Parse the JSON string contained in "obj"
            auto obj = nlohmann::json::parse(incomingMessage["obj"].get<std::string>());
            
            // Extract values
            float value = obj.value("value", 0.f);
            auto paramIdx = obj.value("paramIdx", -1);

            // Update Csound channel
            cabbage.setControlChannel(obj.value("channel", ""), value);
            sendParameterUpdateToHost(paramIdx, value);
        }
        catch (const nlohmann::json::exception& e)
        {
            lattice::logError << "Failed to parse 'obj': " << e.what();
        }
    }
}


//========================================================================================
// Update UI - we typically call this when we want to update widgets in the UI
//========================================================================================
void CabbageProcessor::updateUI()
{
    // iterate over all widget objects and send to webview
    for (auto &w : cabbage.getWidgets())
    {
        if (w.contains("channel")) // only let valid object through.
        {
            auto updatedWidget = cabbage.getUpdatedWidgetJsonStr(w["channel"].get<std::string>(), w.dump());
            sendWebViewMessage(updatedWidget);
        }
    }
}

//========================================================================================
// This can be called from the host - if so update the
// corresponding parameter value using updateParameter() function
//========================================================================================
void CabbageProcessor::setParameter(int paramId, double value)
{
    getParameters()[paramId].value = value;
    cabbage.setControlChannel(getParameters()[paramId].name, value);
}

void CabbageProcessor::prepareToPlay(double sr, uint32_t /*minFrameCount*/, uint32_t /*maxFrameCount*/)
{
    sampleRate = sr;
}

//========================================================================================
//=============================== CSOUND MIDI FUNCTIONS ==================================
//========================================================================================
// Opens MIDI input device
//========================================================================================
int CabbageProcessor::OpenMidiInputDevice(CSOUND *csound, void **userData, const char * /*devName*/)
{
    *userData = csoundGetHostData(csound);
    return 0;
}

//========================================================================================
// Reads MIDI input data from host, gets called every time there is MIDI input to our plugin
//========================================================================================
int CabbageProcessor::ReadMidiData(CSOUND * /*csound*/, void *userData, unsigned char *mbuf, int nbytes)
{
    auto *pluginData = static_cast<cabbage::Engine *>(userData);

    if (!userData)
    {
        cabbage::Utils::check(userData, "\nInvalid");
        return 0;
    }

    int cnt = 0;

    // Access the note event queue
    auto noteEvents = pluginData->getProcessor().getNoteEvents();
    while (!noteEvents.empty())
    {
        // Get the front event
        auto event = noteEvents.front();

        // Prevent overflow
        if (cnt + 3 > nbytes) break;

        // Determine the MIDI status byte
        uint8_t statusByte = 0;
        uint8_t velocity = static_cast<uint8_t>(event.velocity * 127); // Normalize velocity to MIDI range

        if (event.type == lattice::NoteEvent::Type::noteOn)
            statusByte = 0x90; // Note On, channel 1
        else if (event.type == lattice::NoteEvent::Type::noteOff)
            statusByte = 0x80; // Note Off, channel 1
        else
        {
            noteEvents.pop_front(); // Move to the next event
            continue; // Skip unsupported types
        }

        // Fill the MIDI buffer
        *mbuf++ = statusByte;
        *mbuf++ = static_cast<uint8_t>(event.key); // MIDI note number (0-127)
        *mbuf++ = velocity;

        cnt += 3;

        // Remove the processed event from the queue
        noteEvents.pop_front();
    }

    return cnt;
}


//========================================================================================
// Opens MIDI output device, adding -QN to your CsOptions will causes this method to be called
// as soon as your plugin loads
//========================================================================================
int CabbageProcessor::OpenMidiOutputDevice(CSOUND *csound, void **userData, const char * /*devName*/)
{
    *userData = csoundGetHostData(csound);
    return 0;
}

//========================================================================================
// Write MIDI data to plugin's MIDI output. Each time Csound outputs a midi message this
// method should be called. Note: you must have -Q set in your CsOptions
//========================================================================================
int CabbageProcessor::WriteMidiData(CSOUND * /*csound*/, void *_userData, const unsigned char* /*mbuf*/, int nbytes)
{
    auto *userData = static_cast<CabbageProcessor *>(_userData);

    if (!userData)
    {
        cabbage::Utils::check(userData, "\n\nInvalid");
        return 0;
    }

    return nbytes;
}
