
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

CabbageProcessor::CabbageProcessor(std::string csdFile, std::string config)
    : Processor(), cabbage(*this, csdFile)
{

    
    if (!cabbage.setupCsound())
    {
        suspendProcessing();
        return;
    }

    // All message to webview will be wrapped in window.postMessage()
    setWebViewSendFunctionName("window.postMessage");
    
    addParameters();
    addChannels(config);

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

CabbageProcessor::~CabbageProcessor()
{
    stopIdleThread();
}


//========================================================================================
// Add channels based on channelConfig property
//========================================================================================
void CabbageProcessor::addChannels(const std::string& config)
{
    auto file = cabbage::File::getCsdFileAndPath(cabbage.getCsdFile());
    
    cabbage::Utils::check(lattice::File::exists(file), "Can't find csd file");
    
    
    auto channelConfig = config.empty() ? cabbage::Engine::getIOChannalConfig(file) : config;
    auto [inputBuses, outputBuses] = cabbage.parseBusConfiguration(channelConfig);
    
   
    
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
    
    auto ioConfig = getChannelConfig();
    totalNumInputs = ioConfig.getTotalNumInputChannels();
    totalNumOutputs = ioConfig.getTotalNumOutputChannels();
    
    matchingNumInputsOutputs = totalNumInputs == totalNumOutputs;
    
    
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
                
                w["parameterIndex"] = cabbage.getCurrentParameterCount();
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
// Main processing function - this is called by the CLAP process function
//========================================================================================
void CabbageProcessor::process(float** inputs, float** outputs, std::size_t blockSize)
{
    if (!processingEnabled.load(std::memory_order_relaxed))
    {
        return;
    }

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
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sleep to avoid busy-waiting
        onIdle();
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
void CabbageProcessor::onWebViewIsReady()
{

}

void CabbageProcessor::setCabbageIsReady()
{
    uiIsOpen = true;
    allowDequeuing = true;

#ifndef CabbageApp
    //We update the UI each time the plugin window is shown
    updateUI();
#endif
}

//========================================================================================
// Callback function - triggered when a message is sent from the webview
//========================================================================================
void CabbageProcessor::onMessageFromWebView(const nlohmann::json& j)
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
            auto gesture = obj.value("gesture", "complete");

            // Update Csound channel
            cabbage.setControlChannel(obj.value("channel", ""), value);
            getParameters()[paramIdx].value = value;

            if (gesture == "begin") {
                addParameterChange({paramIdx, getParameter(paramIdx).toNormalised(value), lattice::ParamChangeType::GestureBegin});
            } else if (gesture == "value") {
                addParameterChange({paramIdx, getParameter(paramIdx).toNormalised(value), lattice::ParamChangeType::Value});
            } else if (gesture == "end") {
                addParameterChange({paramIdx, getParameter(paramIdx).toNormalised(value), lattice::ParamChangeType::GestureEnd});
            }
            else{
                addParameterChange({paramIdx, getParameter(paramIdx).toNormalised(value), lattice::ParamChangeType::Complete});
            }
            
            auto widgetOpt = cabbage.getWidget(obj.value("channel", ""));
            if (widgetOpt.has_value())
            {
                auto &j = widgetOpt.value().get();
                j["value"] = value;
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            lattice::logError << "Failed to parse 'obj': " << e.what();
        }
    }
    else if (incomingMessage["command"] == "midiMessage")
    {
        addNoteEventFromJson(nlohmann::json::parse(incomingMessage["obj"].get<std::string>()));
        //"{\"statusByte\":144,\"dataByte1\":77,\"dataByte2\":127}"
    }
}

//========================================================================================
// Adds a note event from a JSON message
//========================================================================================
void CabbageProcessor::addNoteEventFromJson(const nlohmann::json& j)
{
    uint8_t statusByte = j["statusByte"].get<uint8_t>();
    uint8_t dataByte1 = j["dataByte1"].get<uint8_t>();
    uint8_t dataByte2 = j["dataByte2"].get<uint8_t>();
    
    // Determine the event type based on MIDI status byte
    lattice::NoteEvent::Type eventType;
    if ((statusByte & 0xF0) == 0x90)
    {
        // MIDI note-on with velocity 0 is treated as note-off
        eventType = (dataByte2 == 0) ? lattice::NoteEvent::Type::noteOff : lattice::NoteEvent::Type::noteOn;
    }
    else if ((statusByte & 0xF0) == 0x80)
    {  // Note-off message
        eventType = lattice::NoteEvent::Type::noteOff;
    }
    else
    {
        // Handle other cases or throw an error
        throw std::runtime_error("Unsupported MIDI message type");
    }
    
    // Convert MIDI velocity (0-127) to normalized velocity (0.0-1.0)
    double velocity = static_cast<double>(dataByte2) / 127.0;
    
    // Create the NoteEvent

    auto noteEvent = lattice::NoteEvent(eventType, static_cast<int16_t>(dataByte1), velocity, -1, 0);
    addNoteEvent(noteEvent);
}

void CabbageProcessor::stopIdleThread()
{
    isIdleRunning.store(false, std::memory_order_relaxed);

    if (idleThread.joinable())
    {        
//        std::cout << "Joining thread before reset..." << std::endl;
        idleThread.join(); // Ensure the thread is joined before reset
//        std::cout << "Thread joined" << std::endl;
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
        if (w.contains("channel")) // only let valid objects through.
        {
            auto updatedWidget = cabbage.getUpdatedWidgetJsonStr(w["channel"].get<std::string>(), w.dump());
            sendWebViewMessage(updatedWidget);
        }
    }
    
    // Check if editor has any pending messages when loaded..
    for (const auto &param : webviewMessageQueue)
    {
        auto widgetOpt = cabbage.getWidget(param.name);
        if (widgetOpt.has_value())
        {
            auto &j = widgetOpt.value().get();
            j["value"] = param.value;
            auto updatedWidget = cabbage.getUpdatedWidgetJsonStr(param.name, param.value);
            cabbage.setControlChannel(param.name, param.value);
            sendWebViewMessage(updatedWidget);
        }
    }
    webviewMessageQueue.clear();
    
}
//========================================================================================
// Plugin state/loding functions
//========================================================================================
nlohmann::json CabbageProcessor::savePluginState()
{
    const auto parameters = getParameters();
    
    // Use array instead of object so we can maintain order
    nlohmann::json j = nlohmann::json::array();

    // Store parameters by index in array
    for (size_t i = 0; i < parameters.size(); i++)
    {
        nlohmann::json param;
        param["name"] = parameters[i].name;
        param["value"] = parameters[i].value;
        j.push_back(param);
    }
    
    return j;
}

void CabbageProcessor::loadPluginState(nlohmann::json state)
{
    auto json = nlohmann::json::parse(state.dump(4));
    int idx = 0;

    // Iterate through array
    for (const auto& param : json)
    {
        // Read values
        float value = param.value("value", 0.f);

        // Add parameter updates to queue for host - these should be normalised
        addParameterChange({idx, getParameter(idx).toNormalised(value), lattice::ParamChangeType::Value});
        // Set parameter values for plugin
        getParameters()[idx].value = value;
        
        // Save to message queue - denormalise value for webview
        auto updatedWidget = cabbage.getUpdatedWidgetJsonStr(getParameters()[idx].name, getParameter(idx).fromNormalised(value));
        webviewMessageQueue.push_back({getParameters()[idx].name, -1, -1, value, -1, -1});
        idx++;
    }
}

//========================================================================================
// This is called from the host
//========================================================================================
void CabbageProcessor::setParameter(int paramId, double value)
{
    const float denormalValue = getParameter(paramId).fromNormalised(value);
    getParameters()[paramId].value = denormalValue;
    cabbage.setControlChannel(getParameters()[paramId].name, denormalValue);
    
    // This method is called from the host, therefore we need to update out UI
    // and the widgets vector which contains all the widget json objects
//    const auto channel = getParameters()[paramId].name;
//    auto widgetOpt = cabbage.getWidget(channel);
//    if (widgetOpt.has_value())
//    {
//        auto &j = widgetOpt.value().get();
//        j["value"] = denormalValue;
//        auto updatedWidget = cabbage.getUpdatedWidgetJsonStr(channel, denormalValue);
//        sendWebViewMessage(updatedWidget);
//    }
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
int CabbageProcessor::ReadMidiData(CSOUND* /*csound*/, void* userData, unsigned char* mbuf, int nbytes) {
    auto* pluginData = static_cast<cabbage::Engine*>(userData);
    if (!userData) {
        cabbage::Utils::check(userData, "\nInvalid user data");
        return 0;
    }

    int cnt = 0;
    auto& noteEvents = pluginData->getProcessor().getNoteEvents();

    while (!noteEvents.empty() && cnt + 3 <= nbytes) 
    {
        const auto& event = noteEvents.front(); // Get event

        // Skip unsupported types (e.g., noteChoke)
        if (event.type != lattice::NoteEvent::Type::noteOn &&
            event.type != lattice::NoteEvent::Type::noteOff) 
        {
            noteEvents.pop_front();
            continue;
        }

        // Convert to MIDI message
        uint8_t statusByte = (event.type == lattice::NoteEvent::Type::noteOn) ? 0x90 : 0x80;
        uint8_t velocity = static_cast<uint8_t>(event.velocity * 127.0); // Scale to 0-127

        // Write MIDI bytes
        *mbuf++ = statusByte;     // Status (note-on/off + channel 1)
        *mbuf++ = event.key;      // MIDI note number (0-127)
        *mbuf++ = velocity;       // Velocity (0-127)

        cnt += 3;
        noteEvents.pop_front();    // Remove processed event
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
