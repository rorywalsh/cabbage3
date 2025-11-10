#include "CabbageProcessor.h"
#include <iostream>
#include "CabbageUtils.h"


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
    auto rootPath = cabbage::File::getParentDirectory(cabbage::File::getCsdFileAndPath(cabbage.getCsdFile()));
    
    setMountPoint(rootPath);
    
    if (!cabbage.setupCsound())
    {
        suspendProcessing();
        
        // Store error HTML to be displayed when webview is ready
        auto errors = cabbage.getCompileErrors();
        lattice::logInfo << "COMPILE ERRORS:\n" << errors;
        compileErrorHtml = generateErrorPageHtml(errors);
        hasCompileErrors = true;
        lattice::logDebug << "Generated error HTML, length: " << compileErrorHtml.length();
        
        setEditorSize(550, 350);
        
        return;
    }


    // All message to webview will be wrapped in window.postMessage()
    setWebViewSendFunctionName("window.postMessage");
    
    addParameters();
    addChannels(config);



    if (auto json = cabbage::File::parseCabbageSection(cabbage::File::getCsdFileAndPath(cabbage.getCsdFile())))
    {
        auto w = cabbage::Utils::findPropertyInForm<int>(*json, "size.width");
        auto h = cabbage::Utils::findPropertyInForm<int>(*json, "size.height");
        if (w.has_value() && h.has_value())
        {
            setEditorSize(w.value(), h.value());
            cabbage.setControlChannel("SCREEN_WIDTH", w.value());
            cabbage.setControlChannel("WINDOW_WIDTH", w.value());
            cabbage.setControlChannel("WINDOW_HEIGHT", h.value());
            cabbage.setControlChannel("SCREEN_HEIGHT", h.value());
        }
    }
      
    startOnIdle();
  
}

CabbageProcessor::~CabbageProcessor()
{
    suspendProcessing();
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
    lattice::logDebug << "=== Starting addParameters() ===";
    lattice::logDebug << "Total widgets: " << cabbage.getWidgets().size();
    
    for (auto &w : cabbage.getWidgets())
    {
        std::string widgetType = w.contains("type") ? w["type"].get<std::string>() : "unknown";
        bool hasChildren = w.contains("children");
        bool childrenIsArray = hasChildren && w["children"].is_array();
        int childCount = childrenIsArray ? w["children"].size() : 0;
        
        if (hasChildren && !childrenIsArray) {
            lattice::logError << "Widget '" << widgetType << "' has a 'children' property but it's not an array. "
                             << "Children must be defined as an array, e.g., \"children\": [{...}]. "
                             << "Found type: " << w["children"].type_name();
        }
        
        addParametersForWidget(w);
        
        // ALWAYS check for child widgets, even if parent is not automatable
        // (containers like image, groupbox are not automatable but their children might be)
        if (w.contains("children") && w["children"].is_array())
        {
            for (auto &child : w["children"])
            {
                // Work directly with the child widget, not a temporary copy
                std::string childType = child.contains("type") ? child["type"].get<std::string>() : "unknown";
                addParametersForWidget(child);
                
                // Recursively process grandchildren
                if (child.contains("children") && child["children"].is_array())
                {
                    for (auto &grandchild : child["children"])
                    {
                        std::string grandchildType = grandchild.contains("type") ? grandchild["type"].get<std::string>() : "unknown";
                        addParametersForWidget(grandchild);
                    }
                }
            }
        }
    }
    
    lattice::logDebug << "=== Finished addParameters() ===";
}

//========================================================================================
// Add parameter for a single widget if it meets the criteria. Parameters must be set
// up to comminucate on an id provided by the widget.channels array object. The top level
// widget.id property is used for UI updating only.
//========================================================================================
void CabbageProcessor::addParametersForWidget(nlohmann::json& w)
{
    std::string widgetType = w.contains("type") ? w["type"].get<std::string>() : "unknown";
    
    // Check for automatable - expect boolean true
    bool isAutomatable = w.contains("automatable") && w["automatable"].is_boolean() && w["automatable"].get<bool>();

    if (isAutomatable && (!w.contains("channelType") || w["channelType"] == "number"))
    {
        // Ensure default ranges are set if missing
        cabbage::Parser::assignDefaultRangesToChannels(w);
        
        try
        {
            // New schema: channels array
            if (w.contains("channels") && w["channels"].is_array())
            {
                int currentIndex = cabbage.getCurrentParameterCount();
                for (auto &ch : w["channels"])
                {
                    if (!ch.contains("id") || !ch["id"].is_string())
                        continue;
                    
                    const std::string channel = ch["id"].get<std::string>();
                    const std::string event = ch.contains("event") && ch["event"].is_string() ? ch["event"].get<std::string>() : std::string("valueChanged");
                    const bool isClickEvent = (event.find("mousePress") == 0) || (event.find("mouseRelease") == 0) || (event.find("mouseClick") == 0);
                    const float minVal = ch["range"]["min"].get<float>();
                    
                    // Determine max value - widgets send denormalized index values for comboBox/optionButton
                    float maxVal = ch["range"]["max"].get<float>();
                    float minValAdjusted = minVal;
                    if (w["type"].get<std::string>() == "comboBox" || w["type"].get<std::string>() == "optionButton") {
                        bool hasIndexOffset = w.contains("indexOffset") && w["indexOffset"].is_boolean() && w["indexOffset"].get<bool>();
                        size_t itemCount = (ch.contains("items") && ch["items"].is_array()) ? ch["items"].size() : 3;
                        if (hasIndexOffset) {
                            minValAdjusted = 1.0f;
                            maxVal = static_cast<float>(itemCount);
                        } else {
                            minValAdjusted = 0.0f;
                            maxVal = static_cast<float>(itemCount - 1);
                        }
                    }
                    
                    const float defVal = ch["range"]["defaultValue"].get<float>();
                    const float incVal = ch["range"]["increment"].get<float>();
                    const float skewVal = ch["range"]["skew"].get<float>();
                    addParameter({channel, minValAdjusted, maxVal, defVal, incVal, skewVal});
                    
                    // Set initial value in Csound
                    cabbage.setControlChannel(channel, defVal);
                    
                    // Store parameterIndex in each channel object
                    ch["parameterIndex"] = currentIndex;
                    lattice::logDebug << "Added parameter for channel '" << channel << "' with parameterIndex "
                                     << currentIndex << " and default value of : " << defVal;
                    currentIndex++;
                }
                
                return;
            }
            
        }
        catch (nlohmann::json::exception &e)
        {
            lattice::logError << "JSON error while adding parameter for widget: " << e.what() << "\n" << w.dump(4);
            // Don't crash - just skip this widget and continue
            // cabbage::Utils::check(false, "");
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
//        cabbage.displayAndClearCompileErrors();

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
    if(!isIdleThreadRunning())
        return;
    
    cabbage.processCsoundMessages();
    
#ifndef CabbageApp
    if (uiIsOpen)
    {
#endif

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
            // when sending a widget value update, or any attribute, nested widgets will be an issue
            // we need to test the nested object's channel name, and update that too
            auto widgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), data.channel);
            if (widgetOpt)
            {
                auto &j = widgetOpt->get();
                if(j.is_null())
                    break;
                
                cabbage::Parser::mergeJsonProperties(j, data.cabbageJson);
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
    // For value-only updates, use the float overload to send just the value
    if (data.type == CabbageOpcodeData::MessageType::Value)
    {
        auto widgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), data.channel);
        if (widgetOpt)
        {
            auto &j = widgetOpt->get();
            cabbage::Parser::mergeJsonProperties(j, data.cabbageJson);
            
            // Extract the float value from the merged JSON
            if (j.contains("value") && j["value"].is_number())
            {
                float value = j["value"].get<float>();
                std::string updatedWidgetJson = cabbage.getUpdatedWidgetJsonStr(data.channel, value);
                sendWebViewMessage(updatedWidgetJson);
            }
        }
    }
    else
    {
        // For full widget updates, use the string overload
        auto updatedWidgetJsonOpt = processOpcodeData(data);
        if (updatedWidgetJsonOpt.has_value())
        {
            std::string updatedWidgetJson = cabbage.getUpdatedWidgetJsonStr(data.channel, updatedWidgetJsonOpt.value().dump());
            sendWebViewMessage(updatedWidgetJson);
        }
    }
}

//========================================================================================
// Process opcode data and return the updated widget JSON if applicable
// Note: Value-only updates are now handled in updateWidgetData() to use the float overload
//========================================================================================
std::optional<nlohmann::json> CabbageProcessor::processOpcodeData(const CabbageOpcodeData &data)
{
    if (data.type == CabbageOpcodeData::MessageType::Identifier)
    {
        auto widgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), data.channel);
        if (widgetOpt)
        {
            auto &j = widgetOpt->get();
            if (j["type"].get<std::string>() == "genTable")
            {
                cabbage.updateFunctionTable(data, j);
            }
            cabbage::Parser::mergeJsonProperties(j, data.cabbageJson);
            return j;
        }
    }
    else if (data.type == CabbageOpcodeData::MessageType::Widget)
    {
        auto widgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), data.channel);
        if (widgetOpt)
        {
            lattice::logDebug << "A widget with channel: " << data.channel << " already exists and cannot be overwritten.";
            return std::nullopt;
        }
        else
        {
            lattice::logDebug << "Creating widget: " << data.channel;
            lattAssert(false, "Double check cabbageCreate still works as expected");
            // Get the widget type
            std::string widgetType = data.cabbageJson["type"].get<std::string>();
            
            // Get the default widget descriptor
            nlohmann::json newWidget = cabbage::WidgetDescriptors::get(widgetType);
            if (newWidget.is_null())
            {
                lattice::logError << "Unknown widget type: " << widgetType << " - cannot create widget";
                return std::nullopt;
            }
            
            // Update the widget with properties from the opcode
            cabbage::Parser::initialiseWidgetJson(newWidget, data.cabbageJson, cabbage.getWidgets().size());
            
            // Add the new widget to the widgets array
            cabbage.getWidgets().push_back(newWidget);
            
            // Debug: Check if widget can now be found
            auto testWidgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), data.channel);
            if (testWidgetOpt) {
                lattice::logDebug << "Widget successfully registered and found: " << data.channel;
            } else {
                lattice::logError << "Widget was added but cannot be found: " << data.channel;
            }
            
            return newWidget;
        }        
    }

    return std::nullopt;
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

void CabbageProcessor::stopIdleThread()
{
    isIdleRunning.store(false);
    if (idleThread.joinable())
    {
        idleThread.join(); // Ensure the thread is joined before reset
    }
}
//========================================================================================
// Triggered when Cabbage is ready to start processing messages
//========================================================================================
void CabbageProcessor::onWebViewIsReady()
{
    // If there were compile errors, display the error page
    if (hasCompileErrors)
    {
        lattice::logDebug << "Displaying error page in webview (called from main thread)";
        setWebViewHtml(compileErrorHtml);
        return;
    }
    
    updateUI();
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
// Callback function - triggered when a message is sent from the webview. Values are
// denormalized (in the actual parameter range) and will be normalized for host communication.
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
            // Check for old format (wrapped in "obj") - no longer supported
            if (incomingMessage.contains("obj")) {
                lattice::logDebug << "parameterChange message using deprecated 'obj' wrapper format is no longer supported. Please update to use direct properties.";
                return; // Don't process old format
            }
            
            // New format: use the message directly
            auto obj = incomingMessage;
            
            lattice::logDebug << obj.dump(4);
            // Extract values (now denormalized from frontend)
            float denormValue = obj.value("value", 0.f);
            auto paramIdx = obj.value("paramIdx", -1);
            if (paramIdx < 0) return;
            auto gesture = obj.value("gesture", "complete");

            // Extract channel - can be a string or an object with 'id'
            std::string channel;
            if (obj["channel"].is_string()) {
                channel = obj["channel"].get<std::string>();
            } else if (obj["channel"].is_object() && obj["channel"].contains("id")) {
                channel = obj["channel"]["id"].get<std::string>();
            } else {
                lattice::logError << "Invalid channel format in parameterChange message";
                return;
            }


            auto num = getParameters().size();
            if(paramIdx >= static_cast<int>(num))
                return;
            
            // Normalize for host communication
            float normalizedValue = getParameter(paramIdx).toNormalised(denormValue);
            getParameters()[paramIdx].value = normalizedValue;

            if (gesture == "begin") {
                addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::GestureBegin});
            } else if (gesture == "value") {
                addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::Value});
            } else if (gesture == "end") {
                addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::GestureEnd});
            }
            else{
                addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::Complete});
            }
            
            lattice::logDebug << "Parameter " << paramIdx << " changed to denorm value: " << denormValue << " (norm: " << normalizedValue << ") on channel: " << channel;
            // Get parameter range for debugging
            auto param = getParameter(paramIdx);
            lattice::logDebug << "Parameter range: min=" << param.min << ", max=" << param.max << ", current=" << param.value;
            // Update Csound channel with denormalized value
            cabbage.setControlChannel(channel, denormValue);
            
            auto widgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), channel);
            if (widgetOpt)
            {
                auto &j = widgetOpt->get();
                j["value"] = denormValue;
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
    else if (incomingMessage["command"] == "channelData")
    {
        try
        {
            // Parse the JSON string contained in "obj"
            auto obj = nlohmann::json::parse(incomingMessage["obj"].get<std::string>());
            
            // Extract channel - can be a string or an object
            std::string channel;
            if (obj["channel"].is_string()) {
                channel = obj["channel"].get<std::string>();
            } else if (obj["channel"].is_object() && obj["channel"].contains("id")) {
                channel = obj["channel"]["id"].get<std::string>();
            } else if (obj["channel"].is_object()) {
                // For multi-channel, find the first string value
                for (auto& [key, value] : obj["channel"].items()) {
                    if (value.is_string()) {
                        channel = value.get<std::string>();
                        break;
                    }
                }
            } else {
                lattice::logError << "Invalid channel format in channelStringData message";
                return;
            }
            
            // Check if we have string data or float data
            if (obj.contains("stringData"))
            {
                std::string stringData = obj.value("stringData", "");
                // Set the Csound string channel
                cabbage.getCsound()->SetChannel(channel.c_str(), stringData.c_str());
                lattice::logDebug << "Set channel " << channel << " to string: " << stringData;
            }
            else if (obj.contains("floatData"))
            {
                double floatData = obj.value("floatData", 0.0);
                // Set the Csound control channel
                cabbage.setControlChannel(channel, floatData);
                lattice::logDebug << "Set channel " << channel << " to float: " << floatData;
            }
            else
            {
                lattice::logError << "channelStringData message missing both stringData and floatData fields";
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            lattice::logError << "Failed to parse channelStringData 'obj': " << e.what();
        }
    }
    else if (incomingMessage["command"] == "fileOpen")
    {
        try
        {
            // Parse the JSON string contained in "obj"
            auto obj = nlohmann::json::parse(incomingMessage["obj"].get<std::string>());
            
            // Extract channel
            std::string channel = obj.value("channel", "");
            if (channel.empty())
            {
                lattice::logError << "fileOpen message missing channel";
                return;
            }
            
            // Extract options
            std::string directory = obj.value("directory", "");
            std::string filters = obj.value("filters", "*");
            bool openAtLastKnownLocation = obj.value("openAtLastKnownLocation", true);
            
            // Open native file dialog
            openFileDialog(channel, directory, filters, openAtLastKnownLocation);
        }
        catch (const nlohmann::json::exception& e)
        {
            lattice::logError << "Failed to parse fileOpen 'obj': " << e.what();
        }
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

//========================================================================================
// Update UI - we typically call this when we want to update widgets in the UI
//========================================================================================
void CabbageProcessor::updateUI()
{
    // iterate over all widget objects and send to webview
    for (auto &w : cabbage.getWidgets())
    {
        std::string channelStr;
        if (w.contains("id") && w["id"].is_string())
        {
            channelStr = w["id"].get<std::string>();
        }
        else if (w.contains("channel") && w["channel"].is_string())
        {
            lattice::logDebug << "Channel string property is deprecated";
            channelStr = w["channel"].get<std::string>();
        }
        else if (w.contains("channel") && w["channel"].is_object() && w["channel"].contains("id"))
        {
            channelStr = w["channel"]["id"].get<std::string>();
        }
        else
        {
            continue;
        }
        auto updatedWidget = cabbage.getUpdatedWidgetJsonStr(channelStr, w.dump(), true);
        if(uiIsOpen)
            sendWebViewMessage(updatedWidget);
    }
    
    // Check if editor has any pending messages when loaded..
    for (const auto &param : webviewMessageQueue)
    {
        auto widgetOpt = cabbage.getWidgetByChannel(cabbage.getWidgets(), param.name);
        if (widgetOpt)
        {
            auto &j = widgetOpt->get();
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
        
        // Store denormalized value for UI and Csound
        float denormalizedValue = getParameter(idx).fromNormalised(value);
        webviewMessageQueue.push_back({getParameters()[idx].name, -1, -1, denormalizedValue, -1, -1});
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

    const auto channel = getParameters()[paramId].name;

    // cabbage2 -> cabbage3 combobox quirk 
    auto widgetOpt = std::optional<std::reference_wrapper<nlohmann::json>>();
    for (auto& w : cabbage.getWidgets()) {
        // New schema: channels array
        if (w.contains("channels") && w["channels"].is_array())
        {
            for (const auto& ch : w["channels"])
            {
                if (ch.contains("id") && ch["id"].is_string() && ch["id"].get<std::string>() == channel)
                {
                    widgetOpt = w;
                    break;
                }
            }
            if (widgetOpt) break;
        }
    }
    if (widgetOpt)
    {
        auto &j = widgetOpt->get();
        std::string widgetType = j["type"].get<std::string>();
        
        // Update the widget's value property so it persists when UI reopens
        j["value"] = denormalValue;
        
        // For comboBox and optionButton, map normalized value to index
        if (widgetType == "comboBox" || widgetType == "optionButton") {
            size_t itemCount = (j.contains("items") && j["items"].is_array()) ? j["items"].size() : 3;
            size_t index = round(denormalValue * (itemCount - 1));
            if (j.contains("indexOffset") && j["indexOffset"].is_boolean() && j["indexOffset"].get<bool>()) {
                index += 1;
            }
            j["value"] = index; // Override with index for these widget types
            cabbage.setControlChannel(getParameters()[paramId].name, index);
            return;
        }
    }
    
    cabbage.setControlChannel(getParameters()[paramId].name, denormalValue);

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

//========================================================================================
// Open native file dialog for fileButton
//========================================================================================
void CabbageProcessor::openFileDialog(const std::string& channel, const std::string& directory, const std::string& filters, bool openAtLastKnownLocation)
{
    std::string initialDir = directory;
    if (initialDir.empty() && openAtLastKnownLocation) {
        initialDir = cabbage::File::getCsdPath(); // Use CSD directory as initial if no directory specified and openAtLastKnownLocation is true
    }
    
    std::string path = cabbage::File::browseForFile("Choose a file", initialDir, filters);
    if (!path.empty()) {
        // Send the path to Csound via the channel
        cabbage.setStringChannel(channel, path.c_str());
        lattice::logDebug << "File selected for channel " << channel << ": " << path;
    }
}

//========================================================================================
// Generate error page HTML with compile errors overlaid
//========================================================================================
std::string CabbageProcessor::generateErrorPageHtml(const std::string& errors) {
    std::string html = errorPageHtml;
    // Find the closing </style> tag to add overlay styles
    size_t styleEnd = html.find("</style>");
    if (styleEnd != std::string::npos) {
        std::string overlayStyles = "\n    .error-overlay {\n"
                                   "      position: absolute;\n"
                                   "      inset: 20px;\n" // equal spacing on all sides
                                   "      background-color: rgba(255, 255, 255, 0.9);\n"
                                   "      border: 2px solid #ff0000;\n"
                                   "      border-radius: 8px;\n"
                                   "      padding: 15px;\n"
                                   "      font-family: monospace;\n"
                                   "      font-size: 12px;\n"
                                   "      color: #000;\n"
                                   "      max-height: calc(100% - 40px);\n"
                                   "      overflow-y: auto;\n"
                                   "      z-index: 1000;\n"
                                   "      box-shadow: 0 4px 8px rgba(0,0,0,0.3);\n"
                                   "    }\n"
                                   "    body {\n"
                                   "      position: relative;\n"
                                   "    }\n";
        html.insert(styleEnd, overlayStyles);
    }

    // Find the closing </body> tag to add the error overlay div
    size_t bodyEnd = html.find("</body>");
    if (bodyEnd != std::string::npos) {
        std::string errorOverlay = "\n  <div class=\"error-overlay\">\n"
                                  "    <h3 style=\"color: #ff0000; margin-top: 0;\">Csound Compile Errors:</h3>\n"
                                  "    <pre style=\"margin: 0; white-space: pre-wrap; font-size: 11px;\">" + errors + "</pre>\n"
                                  "  </div>\n";
        html.insert(bodyEnd, errorOverlay);
    } else {
        // Fallback: append to end if no </body> found
        html += "\n  <div style=\"position: absolute; inset: 20px; background-color: rgba(255, 255, 255, 0.95); border: 2px solid #ff0000; border-radius: 8px; padding: 15px; font-family: monospace; font-size: 12px; color: #000; max-height: calc(100% - 40px); overflow-y: auto; z-index: 1000; box-shadow: 0 4px 8px rgba(0,0,0,0.3);\">\n"
                "    <h3 style=\"color: #ff0000; margin-top: 0;\">Csound Compile Errors:</h3>\n"
                "    <pre style=\"margin: 0; white-space: pre-wrap; font-size: 11px;\">" + errors + "</pre>\n"
                "  </div>\n";
    }
    return html;
}

