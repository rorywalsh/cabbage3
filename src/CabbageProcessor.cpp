#include "CabbageProcessor.h"
#include <iostream>
#include "CabbageUtils.h"

//========================================================================================
pluginType *LatticeProcessorPluginFactory::createPlugin(const clap_host *host)
{
    // create a new instance of CabbageProcessor
    auto *processor = new CabbageProcessor();
    return new pluginType(host, *processor);
}
//========================================================================================
CabbageProcessor::CabbageProcessor(std::string csdFile, std::string config) : Processor(), cabbage(*this, csdFile)
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

    // For CabbageApp, enable dequeuing immediately so widgets created during
    // init are processed right away. For plugins, this is set when UI is ready.
#ifdef CabbageApp
    allowDequeuing = true;
#endif
}

CabbageProcessor::~CabbageProcessor()
{
    suspendProcessing();

    // Drain any pending operations from queues before stopping threads
    // This prevents race conditions where the audio thread might try to
    // access the queue while it's being destroyed
    allowDequeuing = false;

    // Give any in-flight queue operations time to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    stopIdleThread();
}

//========================================================================================
// Add channels based on channelConfig property
//========================================================================================
void CabbageProcessor::addChannels(const std::string &config)
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

        if (hasChildren && !childrenIsArray)
        {
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
                        std::string grandchildType =
                            grandchild.contains("type") ? grandchild["type"].get<std::string>() : "unknown";
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
void CabbageProcessor::addParametersForWidget(nlohmann::json &w)
{
    std::string widgetType = w.contains("type") ? w["type"].get<std::string>() : "unknown";

    // Check for automatable - expect boolean true
    bool isAutomatable = w.contains("automatable") && w["automatable"].is_boolean() && w["automatable"].get<bool>();

    // Ensure default ranges are set if missing
    cabbage::Parser::assignDefaultRangesToChannels(w);

    // Check if channel type is numeric (default) or explicitly set to number
    bool isNumericChannel = true;
    if (w.contains("channels") && w["channels"].is_array() && !w["channels"].empty()) {
        if (w["channels"][0].contains("type") && w["channels"][0]["type"].is_string()) {
            isNumericChannel = (w["channels"][0]["type"].get<std::string>() == "number");
        }
    }
    
    if (isAutomatable && isNumericChannel)
    {
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
                    const std::string event = ch.contains("event") && ch["event"].is_string()
                                                  ? ch["event"].get<std::string>()
                                                  : std::string("valueChanged");
                    const bool isClickEvent = (event.find("mousePress") == 0) || (event.find("mouseRelease") == 0) ||
                                              (event.find("mouseClick") == 0);
                    const float minVal = ch["range"]["min"].get<float>();

                    // Determine max value - widgets send denormalized index values for comboBox/optionButton
                    float maxVal = ch["range"]["max"].get<float>();
                    float minValAdjusted = minVal;
                    if (w["type"].get<std::string>() == "comboBox" || w["type"].get<std::string>() == "optionButton")
                    {
                        bool hasIndexOffset =
                            w.contains("indexOffset") && w["indexOffset"].is_boolean() && w["indexOffset"].get<bool>();
                        size_t itemCount = (ch.contains("items") && ch["items"].is_array()) ? ch["items"].size() : 3;
                        if (hasIndexOffset)
                        {
                            minValAdjusted = 1.0f;
                            maxVal = static_cast<float>(itemCount);
                        }
                        else
                        {
                            minValAdjusted = 0.0f;
                            maxVal = static_cast<float>(itemCount - 1);
                        }
                    }

                    const float defVal = ch["range"]["defaultValue"].get<float>();
                    const float incVal = ch["range"]["increment"].get<float>();
                    const float skewVal = ch["range"]["skew"].get<float>();

                    lattice::logInfo << "Creating parameter '" << channel << "': min=" << minValAdjusted
                                     << ", max=" << maxVal << ", default=" << defVal << ", inc=" << incVal
                                     << ", skew=" << skewVal;

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
        }
    }
    else
    {
        // Non-automatable widget - still needs Csound channel created for cabbageGetValue/cabbageSetValue
        try
        {
            if (w.contains("channels") && w["channels"].is_array())
            {
                for (auto &ch : w["channels"])
                {
                    if (!ch.contains("id") || !ch["id"].is_string())
                        continue;

                    const std::string channel = ch["id"].get<std::string>();
                    
                    // Check channel type - default to "number" if not specified
                    std::string channelType = "number";
                    if (ch.contains("type") && ch["type"].is_string())
                    {
                        channelType = ch["type"].get<std::string>();
                    }
                    
                    // Create appropriate channel type
                    if (channelType == "string")
                    {
                        // For string channels, set empty string as default
                        cabbage.getCsound()->SetStringChannel(channel.c_str(), "");
                        lattice::logDebug << "Created string channel for non-automatable widget '" << channel << "'";
                    }
                    else
                    {
                        // For numeric channels, use default value from range
                        const float defVal = ch["range"]["defaultValue"].get<float>();
                        cabbage.setControlChannel(channel, defVal);
                        lattice::logDebug << "Created numeric channel for non-automatable widget '" << channel
                                          << "' with default value " << defVal;
                    }
                }
            }
            // Legacy schema: single id
            else if (w.contains("id") && w["id"].is_string())
            {
                const std::string channel = w["id"].get<std::string>();
                float defVal = 0.0f;
                if (w.contains("value"))
                {
                    if (w["value"].is_number())
                        defVal = w["value"].get<float>();
                    else if (w["value"].is_boolean())
                        defVal = w["value"].get<bool>() ? 1.0f : 0.0f;
                }

                cabbage.setControlChannel(channel, defVal);
                lattice::logDebug << "Created channel for non-automatable widget (legacy) '" << channel
                                  << "' with default value " << defVal;
            }
        }
        catch (nlohmann::json::exception &e)
        {
            lattice::logError << "JSON error while creating channel for non-automatable widget: " << e.what();
        }
    }
}

//========================================================================================
// Main processing function - this is called by the CLAP process function
//========================================================================================
void CabbageProcessor::process(float **inputs, float **outputs, std::size_t blockSize)
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
            cabbage.flushChannelCache();
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
    if (!isIdleThreadRunning())
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
        // Dequeue all messages and deduplicate by channel
        // This prevents processing redundant updates when multiple instances
        // write to the same channel (e.g., multiple notes updating the same ADSR)
        std::unordered_map<std::string, CabbageOpcodeData> latestMessages;

        while (cabbage.opcodeData.try_dequeue(data))
        {
            // For each channel, keep only the latest message
            // Newer messages automatically overwrite older ones
            latestMessages[data.channel] = data;
        }

        // Now process only the latest message for each channel
        for (const auto &[channel, latestData] : latestMessages)
        {
            auto widgetOpt = cabbage.getWidgetFromId(cabbage.getWidgets(), latestData.channel);
            if (widgetOpt)
            {
                auto &j = widgetOpt->get();
                if (j.is_null())
                {
                    continue;
                }

                cabbage::Parser::mergeJsonProperties(j, latestData.cabbageJson);
            }

#ifdef CabbageApp
            hostCallback(latestData);
#else
            cabbage.processCsoundMessages();
            updateWidgetData(latestData);
#endif
        }
    }
}

//========================================================================================
// this function will be called from the onIdle function if Csound has sent update messages
//========================================================================================
void CabbageProcessor::updateWidgetData(const CabbageOpcodeData &data)
{
    // For value-only updates, send just the value
    if (data.type == CabbageOpcodeData::MessageType::Value)
    {
        auto widgetOpt = cabbage.getWidgetFromId(cabbage.getWidgets(), data.channel);
        if (widgetOpt)
        {
            auto &j = widgetOpt->get();
            cabbage::Parser::mergeJsonProperties(j, data.cabbageJson);

            // Extract the float value from the merged JSON
            if (j.contains("value") && j["value"].is_number())
            {
                float value = j["value"].get<float>();
                // Send proper JSON message like CabbageApp does
                nlohmann::json msg;
                msg["command"] = "widgetUpdate";
                msg["id"] = data.channel;
                msg["value"] = value;
                lattice::logDebug << "Sending value update to webview: " << msg.dump();
                sendWebViewMessage(msg);
            }
        }
    }
    else
    {
        // For full widget updates, send the complete widget JSON
        auto updatedWidgetJsonOpt = processOpcodeData(data);
        if (updatedWidgetJsonOpt.has_value())
        {
            auto &j = updatedWidgetJsonOpt.value();
            // Send proper JSON message like CabbageApp does
            nlohmann::json msg;
            msg["command"] = "widgetUpdate";
            msg["id"] = data.channel;
            msg["widgetJson"] = j.dump(); // Send as JSON string, consistent with updateUI()
            sendWebViewMessage(msg);
        }
    }
}

//========================================================================================
// Process opcode data and return the updated widget JSON if applicable
// Note: Value-only updates are now handled in updateWidgetData() to use the float overload
//========================================================================================
std::optional<nlohmann::json> CabbageProcessor::processOpcodeData(const CabbageOpcodeData &data)
{
    if (data.type == CabbageOpcodeData::MessageType::Identifier || data.type == CabbageOpcodeData::MessageType::Value)
    {
        auto widgetOpt = cabbage.getWidgetFromId(cabbage.getWidgets(), data.channel);
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
        auto widgetOpt = cabbage.getWidgetFromId(cabbage.getWidgets(), data.channel);
        if (widgetOpt)
        {
            lattice::logDebug << "A widget with channel: " << data.channel
                              << " already exists and cannot be overwritten.";
            return std::nullopt;
        }
        else
        {
            lattice::logDebug << "Creating widget: " << data.channel;

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
            auto testWidgetOpt = cabbage.getWidgetFromId(cabbage.getWidgets(), data.channel);
            if (!testWidgetOpt)
            {
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
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz update rate for smoother UI
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
    // Give the idle thread time to exit its current onIdle() call
    // before we try to join it. This prevents race conditions where
    // the thread might be in the middle of a queue operation.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    lattice::logDebug << "setCabbageIsReady() called";
    uiIsOpen = true;
    allowDequeuing = true;

#ifndef CabbageApp
    // We update the UI each time the plugin window is shown
    lattice::logDebug << "Calling updateUI() from setCabbageIsReady()";
    updateUI();
    lattice::logDebug << "updateUI() call completed";
#endif
}

//========================================================================================
// Callback function - triggered when a message is sent from the webview. Values are
// denormalized (in the actual parameter range) and will be normalized for host communication.
//========================================================================================
void CabbageProcessor::onMessageFromWebView(const nlohmann::json &j)
{
    // Handle both array-wrapped messages (legacy plugin format) and plain object messages (VSCode extension)
    nlohmann::json incomingMessage;
    if (j.is_array() && !j.empty())
    {
        incomingMessage = j.at(0);
    }
    else if (j.is_object())
    {
        incomingMessage = j;
    }
    else
    {
        lattice::logError << "Invalid message format received from webview: " << j.dump();
        return;
    }

    // Unpack 'obj' field if present (for plugin mode messages)
    if (incomingMessage.contains("obj") && incomingMessage["obj"].is_string())
    {
        try
        {
            auto objData = nlohmann::json::parse(incomingMessage["obj"].get<std::string>());
            // Merge obj data into the message, keeping the command field
            std::string cmd = incomingMessage["command"].get<std::string>();
            incomingMessage = objData;
            incomingMessage["command"] = cmd;
        }
        catch (const nlohmann::json::exception &e)
        {
            lattice::logError << "Failed to parse 'obj' field: " << e.what();
        }
    }

    // Try to handle with Engine first
    if (cabbage.processWebViewCommand(incomingMessage))
    {
        return;
    }

    // Handle environment-specific commands
    const std::string command = incomingMessage["command"].get<std::string>();

    if (command == "cabbageIsReadyToLoad")
    {
        setCabbageIsReady();
        updateUI();
    }
    else if (command == "parameterChange")
    {
        std::string gesture = cabbage.handleParameterUpdate(incomingMessage);
        if (gesture.empty())
            return; // Validation failed, error already logged

        // Engine updated the parameter, just handle gesture for DAW automation
        int paramIdx = incomingMessage["paramIdx"].get<int>();
        float normalizedValue = getParameters()[paramIdx].value;

        if (gesture == "begin")
            addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::GestureBegin});
        else if (gesture == "value")
            addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::Value});
        else if (gesture == "end")
            addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::GestureEnd});
        else
            addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::Complete});
    }
    else if (command == "midiMessage")
    {
        addNoteEventFromJson(nlohmann::json::parse(incomingMessage["obj"].get<std::string>()));
    }
    else if (command == "fileOpen")
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
        catch (const nlohmann::json::exception &e)
        {
            lattice::logError << "Failed to parse fileOpen 'obj': " << e.what();
        }
    }
}

//========================================================================================
// Adds a note event from a JSON message
//========================================================================================
void CabbageProcessor::addNoteEventFromJson(const nlohmann::json &j)
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
    { // Note-off message
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
    int widgetsSent = 0;
    for (auto &w : cabbage.getWidgets())
    {
        std::string channelStr;
        if (w.contains("id") && w["id"].is_string())
        {
            channelStr = w["id"].get<std::string>();
        }
        else if (w.contains("channel") && w["channel"].is_string())
        {
            channelStr = w["channel"].get<std::string>();
        }
        else if (w.contains("channel") && w["channel"].is_object() && w["channel"].contains("id"))
        {
            channelStr = w["channel"]["id"].get<std::string>();
        }
        else
        {
            lattice::logDebug << "Widget skipped - no valid channel identifier";
            continue;
        }
        // Send proper JSON message like CabbageApp does
        nlohmann::json msg;
        msg["command"] = "widgetUpdate";
        msg["id"] = channelStr;
        msg["widgetJson"] = w.dump(); // Send as JSON string, like CabbageApp does
        if (uiIsOpen)
        {
            sendWebViewMessage(msg);
            widgetsSent++;
        }
        else
        {
            lattice::logDebug << "Widget not sent (uiIsOpen=false): " << channelStr;
        }
    }

    lattice::logDebug << "Total widgets sent: " << widgetsSent;

    // Check if editor has any pending messages when loaded..
    for (const auto &param : webviewMessageQueue)
    {
        auto widgetOpt = cabbage.getWidgetFromId(cabbage.getWidgets(), param.name);
        if (widgetOpt)
        {
            auto &j = widgetOpt->get();
            j["value"] = param.value;
            // Send proper JSON message like CabbageApp does
            nlohmann::json msg;
            msg["command"] = "widgetUpdate";
            msg["id"] = param.name;
            msg["value"] = param.value;
            cabbage.setControlChannel(param.name, param.value);
            sendWebViewMessage(msg);
        }
    }
    webviewMessageQueue.clear();
}
//========================================================================================
// Plugin state/loading functions
//========================================================================================
nlohmann::json CabbageProcessor::savePluginState()
{
    // Use the Engine utility function to save complete widget state
    return cabbage.saveWidgetState();
}

void CabbageProcessor::loadPluginState(nlohmann::json state)
{
    // Use the Engine utility function to load complete widget state
    // This handles: widgets array, Csound channels, parameters, and UI updates
    cabbage.loadWidgetState(state);
}

//========================================================================================
// This is called from the host
//========================================================================================
void CabbageProcessor::setParameter(int paramId, double value)
{
    // Store the normalized value (value parameter is already normalized)
    getParameters()[paramId].value = value;

    // Calculate denormalized value for Csound channel
    const float denormalValue = getParameter(paramId).fromNormalised(value);
    const auto channel = getParameters()[paramId].name;

    // cabbage2 -> cabbage3 combobox quirk
    auto widgetOpt = std::optional<std::reference_wrapper<nlohmann::json>>();
    for (auto &w : cabbage.getWidgets())
    {
        // New schema: channels array
        if (w.contains("channels") && w["channels"].is_array())
        {
            for (const auto &ch : w["channels"])
            {
                if (ch.contains("id") && ch["id"].is_string() && ch["id"].get<std::string>() == channel)
                {
                    widgetOpt = w;
                    break;
                }
            }
            if (widgetOpt)
                break;
        }
    }
    if (widgetOpt)
    {
        auto &j = widgetOpt->get();
        std::string widgetType = j["type"].get<std::string>();

        // Update the widget's value property so it persists when UI reopens
        j["value"] = denormalValue;

        // For comboBox and optionButton, the frontend already sends the correct index
        // (not a normalized value), so we should use it directly
        if (widgetType == "comboBox" || widgetType == "optionButton")
        {
            // The denormalValue is already the index from the frontend
            size_t index = static_cast<size_t>(round(denormalValue));

            if (j.contains("indexOffset") && j["indexOffset"].is_boolean() && j["indexOffset"].get<bool>())
            {
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
int CabbageProcessor::ReadMidiData(CSOUND * /*csound*/, void *userData, unsigned char *mbuf, int nbytes)
{
    auto *pluginData = static_cast<cabbage::Engine *>(userData);
    if (!userData)
    {
        cabbage::Utils::check(userData, "\nInvalid user data");
        return 0;
    }

    int cnt = 0;
    auto &noteEvents = pluginData->getProcessor().getNoteEvents();

    while (!noteEvents.empty() && cnt + 3 <= nbytes)
    {
        const auto &event = noteEvents.front(); // Get event

        // Skip unsupported types (e.g., noteChoke)
        if (event.type != lattice::NoteEvent::Type::noteOn && event.type != lattice::NoteEvent::Type::noteOff)
        {
            noteEvents.pop_front();
            continue;
        }

        // Convert to MIDI message
        uint8_t statusByte = (event.type == lattice::NoteEvent::Type::noteOn) ? 0x90 : 0x80;
        uint8_t velocity = static_cast<uint8_t>(event.velocity * 127.0); // Scale to 0-127

        // Write MIDI bytes
        *mbuf++ = statusByte; // Status (note-on/off + channel 1)
        *mbuf++ = event.key;  // MIDI note number (0-127)
        *mbuf++ = velocity;   // Velocity (0-127)

        cnt += 3;
        noteEvents.pop_front(); // Remove processed event
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
int CabbageProcessor::WriteMidiData(CSOUND * /*csound*/, void *_userData, const unsigned char * /*mbuf*/, int nbytes)
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
void CabbageProcessor::openFileDialog(const std::string &channel, const std::string &directory,
                                      const std::string &filters, bool openAtLastKnownLocation)
{
    std::string initialDir = directory;
    if (initialDir.empty() && openAtLastKnownLocation)
    {
        initialDir = cabbage::File::getCsdPath(); // Use CSD directory as initial if no directory specified and
                                                  // openAtLastKnownLocation is true
    }

    std::string path = cabbage::File::browseForFile("Choose a file", initialDir, filters);
    if (!path.empty())
    {
        // Send the path to Csound via the channel
        cabbage.setStringChannel(channel, path.c_str());
        lattice::logDebug << "File selected for channel " << channel << ": " << path;
    }
}

//========================================================================================
// Generate error page HTML with compile errors overlaid
//========================================================================================
std::string CabbageProcessor::generateErrorPageHtml(const std::string &errors)
{
    std::string html = errorPageHtml;
    // Find the closing </style> tag to add overlay styles
    size_t styleEnd = html.find("</style>");
    if (styleEnd != std::string::npos)
    {
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
    if (bodyEnd != std::string::npos)
    {
        std::string errorOverlay = "\n  <div class=\"error-overlay\">\n"
                                   "    <h3 style=\"color: #ff0000; margin-top: 0;\">Csound Compile Errors:</h3>\n"
                                   "    <pre style=\"margin: 0; white-space: pre-wrap; font-size: 11px;\">" +
                                   errors +
                                   "</pre>\n"
                                   "  </div>\n";
        html.insert(bodyEnd, errorOverlay);
    }
    else
    {
        // Fallback: append to end if no </body> found
        html += "\n  <div style=\"position: absolute; inset: 20px; background-color: rgba(255, 255, 255, 0.95); "
                "border: 2px solid #ff0000; border-radius: 8px; padding: 15px; font-family: monospace; font-size: "
                "12px; color: #000; max-height: calc(100% - 40px); overflow-y: auto; z-index: 1000; box-shadow: 0 4px "
                "8px rgba(0,0,0,0.3);\">\n"
                "    <h3 style=\"color: #ff0000; margin-top: 0;\">Csound Compile Errors:</h3>\n"
                "    <pre style=\"margin: 0; white-space: pre-wrap; font-size: 11px;\">" +
                errors +
                "</pre>\n"
                "  </div>\n";
    }
    return html;
}
