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
// CabbageProcessor Constructor
// =============================
// DESIGN: Single Source of Truth for CSD File Path
//
// This constructor is responsible for determining the final CSD file path ONCE and setting
// it via cabbage.setCsdFile(). All other code should use cabbage.getCsdFile() or rely on
// the static cache in getCsdFileAndPath() instead of re-computing the path.
//
// Flow:
// 1. Determine initial CSD path (from parameter or default lookup)
// 2. For Pro builds: Check for .cabz archive and extract if present
// 3. If cabz exists: Override CSD path to point to extracted .ecsd file in temp directory
// 4. Call setCsdFile() ONCE with the final path
//    - This sets csdFile on the Engine instance
//    - This calls setCsdFileAndPath() to populate the static cache
// 5. All subsequent calls (WidgetDescriptors, parseCsdForWidgets, etc.) use the cached path
//========================================================================================
CabbageProcessor::CabbageProcessor(std::string csdFile, std::string config) : Processor(), cabbage(*this, "")
{
    std::string finalCsdPath;

#ifdef CabbagePro
    // First, determine the root path for cabz extraction
    std::string rootPath;
    if (!csdFile.empty() && lattice::File::exists(csdFile))
    {
        rootPath = cabbage::File::getParentDirectory(csdFile);
        finalCsdPath = csdFile;
    }
    else
    {
        // Use default path lookup
        finalCsdPath = cabbage::File::getCsdFileAndPath();
        rootPath = cabbage::File::getParentDirectory(cabbage::File::getParentDirectory(finalCsdPath));
    }

    // Check for .cabz archive and extract if present
    cabzTempDir = cabbage::File::extractCabzArchive(rootPath);
    if (!cabzTempDir.empty())
    {
        auto files = lattice::File::getFilesOfType(cabzTempDir, "*");
        for(auto &f : files)
        {
            lattice::logInfo << "File: " << f;
        }

        setMountPoint(cabzTempDir);
        // Override with the encrypted CSD file from the cabz archive
        finalCsdPath = cabzTempDir + "/" + cabbage::File::getBinaryWithoutExtension() + ".ecsd";
    }
    else
    {
        setMountPoint(rootPath);
    }
#else
    // Free version: use provided path or default lookup
    if (!csdFile.empty() && lattice::File::exists(csdFile))
    {
        finalCsdPath = csdFile;
    }
    else
    {
        finalCsdPath = cabbage::File::getCsdFileAndPath();
    }
    auto rootPath = cabbage::File::getParentDirectory(finalCsdPath);
    setMountPoint(rootPath);
#endif

    // Set the final CSD path once - this is the single source of truth
    cabbage.setCsdFile(finalCsdPath);
    lattice::logInfo << "CabbageProcessor: Final CSD path set to: " << finalCsdPath;
    lattice::logDebug << lattice::File::getFileAsString(cabbage::File::getCsdFileAndPath());
    
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
    
    
    if (auto json = cabbage::File::parseCabbageSection(cabbage::File::getCsdFileAndPath()))
    {
        lattice::logDebug << json->dump(4);
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

    // Drain any pending operations from queues before stopping threads
    // This prevents race conditions where the audio thread might try to
    // access the queue while it's being destroyed
    allowDequeuing = false;

    // Give any in-flight queue operations time to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    stopIdleThread();

#ifdef CabbagePro
    // Cleanup temp directory if we extracted a .cabz archive
    if (!cabzTempDir.empty())
    {
        cabbage::File::cleanupCabzTempDir(cabzTempDir);
    }
#endif
}

//========================================================================================
// Add channels based on channelConfig property
//========================================================================================
void CabbageProcessor::addChannels(const std::string &config)
{
    // Use the CSD file path set in the constructor - single source of truth
    auto file = cabbage.getCsdFile();

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

    // Get transport info from host and update Csound channels
    auto transport = getTransportInfo();
    
    // Update transport-related control channels
    cabbage.setControlChannel("HOST_BPM", transport.tempo);
    cabbage.setControlChannel("IS_PLAYING", transport.isPlaying ? 1.0f : 0.0f);
    cabbage.setControlChannel("IS_RECORDING", transport.isRecording ? 1.0f : 0.0f);
    cabbage.setControlChannel("TIME_IN_SECONDS", transport.songPosSeconds);
    cabbage.setControlChannel("TIME_IN_SAMPLES", transport.songPosSeconds * sampleRate);
    cabbage.setControlChannel("TIME_SIG_NUM", static_cast<float>(transport.timeSigNum));
    cabbage.setControlChannel("TIME_SIG_DENOM", static_cast<float>(transport.timeSigDenom));
    cabbage.setControlChannel("HOST_PPQ_POS", transport.barStart);
    
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
            // Make an explicit copy to ensure deep copy of JSON data
            CabbageOpcodeData dataCopy = latestData;

            // Check if this message contains a populate update AND should be processed
            bool hasPopulateUpdate = dataCopy.cabbageJson.contains("populate") && !dataCopy.skipPopulateProcessing;

            cabbage.updateWidget(dataCopy.channel, [dataCopy](nlohmann::json &j) {
                cabbage::Parser::mergeJsonProperties(j, dataCopy.cabbageJson);
            });

            // If this update included populate data and should be processed, trigger async processing
            if (hasPopulateUpdate)
            {
                auto widgetOpt = cabbage.getWidgetCopyById(dataCopy.channel);
                lattice::logDebug << "Checking widget for populate config: " << dataCopy.channel;
                if (widgetOpt.has_value()) {
                    if (widgetOpt->contains("populate")) {
                        lattice::logDebug << "Widget has populate field, type: " << (*widgetOpt)["populate"].type_name();
                    } else {
                        lattice::logDebug << "Widget does NOT have populate field";
                    }
                }

                if (widgetOpt.has_value() && widgetOpt->contains("populate") && (*widgetOpt)["populate"].is_object())
                {
                    std::string widgetChannel = dataCopy.channel;
                    nlohmann::json populateConfig = (*widgetOpt)["populate"];

                    lattice::logDebug << "Populate config: " << populateConfig.dump();

                    // Normalize directories: convert single string to array for consistency
                    if (populateConfig.contains("directories") && populateConfig["directories"].is_string()) {
                        std::string singleDir = populateConfig["directories"].get<std::string>();
                        populateConfig["directories"] = nlohmann::json::array({singleDir});
                        lattice::logDebug << "Converted single string directories to array for widget: " << widgetChannel;
                    }

                    // Verify it has the required fields before processing
                    if (populateConfig.contains("directories") && populateConfig["directories"].is_array() && populateConfig.contains("fileType"))
                    {
                        lattice::logDebug << "Triggering processPopulateAsync for widget: " << widgetChannel;

                        // Process populate on background thread, then update widget with results
                        cabbage::Parser::processPopulateAsync(widgetChannel, populateConfig,
                            [this, widgetChannel](const nlohmann::json& result) {
                                lattice::logDebug << "Populate callback received for widget: " << widgetChannel;
                                lattice::logDebug << "Result contains items: " << (result.contains("items") ? "yes" : "no");
                                if (result.contains("items")) {
                                    lattice::logDebug << "Items count: " << result["items"].size();
                                }

                                // Update widget with populated items (this runs on background thread)
                                cabbage.updateWidget(widgetChannel, [result](nlohmann::json &j) {
                                    if (result.contains("items")) {
                                        j["items"] = result["items"];
                                        lattice::logDebug << "Updated widget items in internal state";
                                    }
                                    // Note: We DON'T update j["populate"] here to avoid triggering
                                    // another populate detection in onIdle (which would create an infinite loop)
                                });

                                // Enqueue widget update to be sent from idle thread
                                // Create opcode data to trigger a widget update on the idle thread
                                CabbageOpcodeData opcodeUpdate;
                                opcodeUpdate.channel = widgetChannel;
                                opcodeUpdate.type = CabbageOpcodeData::MessageType::Identifier;
                                opcodeUpdate.identifier = "items";  // This will trigger a full widget update
                                opcodeUpdate.cabbageJson = result;  // Contains the items
                                opcodeUpdate.skipPopulateProcessing = true;  // CRITICAL: Prevent infinite loop

                                // Enqueue to opcode queue so it's processed by onIdle
                                cabbage.opcodeData.enqueue(opcodeUpdate);
                                lattice::logDebug << "Enqueued widget update for idle thread processing";
                            });
                    }
                    else
                    {
                        lattice::logWarning << "Populate config missing directories array or fileType for: " << widgetChannel;
                        lattice::logWarning << "Current populate config: " << populateConfig.dump();
                    }
                }
                else
                {
                    lattice::logWarning << "Widget not found or populate not an object for: " << dataCopy.channel;
                    if (widgetOpt.has_value()) {
                        lattice::logWarning << "Widget JSON: " << widgetOpt->dump();
                    }
                }
            }

#ifdef CabbageApp
            if (hostCallback) {
                hostCallback(dataCopy);
            }
#else
            cabbage.processCsoundMessages();
            updateWidgetData(dataCopy);
#endif
        }
    }
}

//========================================================================================
// this function will be called from the onIdle function if Csound has sent update messages
//========================================================================================
void CabbageProcessor::updateWidgetData(const CabbageOpcodeData &data)
{
    // Handle batch updates from loadWidgetState
    if (data.channel == "BATCH-UPDATE-7f3d2a" && data.cabbageJson.contains("command") &&
        data.cabbageJson["command"] == "batchWidgetUpdate")
    {
        lattice::logDebug << "Processing batch widget update with " << data.cabbageJson["widgets"].size() << " widgets";
        sendWebViewMessage(data.cabbageJson);
        return;
    }
    
    // For value-only updates, update the channel's range.value (not top-level value)
    if (data.type == CabbageOpcodeData::MessageType::Value)
    {
        float value = 0.0f;
        bool updated = cabbage.updateWidget(data.channel, [data, &value](nlohmann::json &j) {
            // Extract value from the message
            if (data.cabbageJson.contains("value") && data.cabbageJson["value"].is_number())
            {
                value = data.cabbageJson["value"].get<float>();
                
                // Update the first channel's range.value (proper location for values)
                if (j.contains("channels") && j["channels"].is_array() && !j["channels"].empty())
                {
                    auto& firstChannel = j["channels"][0];
                    if (firstChannel.contains("range") && firstChannel["range"].is_object())
                    {
                        firstChannel["range"]["value"] = value;
                    }
                }
            }
        });
        
        if (updated)
        {
            // Send proper JSON message like CabbageApp does
            nlohmann::json msg;
            msg["command"] = "widgetUpdate";
            msg["id"] = data.channel;
            msg["value"] = value;
            sendWebViewMessage(msg);
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
            
            lattice::logDebug << "Sending full widget update to UI for channel: " << data.channel;
            sendWebViewMessage(msg);
        }
        else
        {
            lattice::logWarning << "processOpcodeData returned nullopt for channel: " << data.channel;
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
        auto widgetOpt = cabbage.getWidgetCopyById(data.channel);
        if (widgetOpt)
        {
            auto j = widgetOpt.value();
            if (j["type"].get<std::string>() == "genTable")
            {
                cabbage.updateFunctionTable(data, j);
            }
            lattice::logDebug << data.cabbageJson.dump(4); 
            cabbage::Parser::mergeJsonProperties(j, data.cabbageJson);
            return j;
        }
    }
    else if (data.type == CabbageOpcodeData::MessageType::Widget)
    {
        auto widgetOpt = cabbage.getWidgetCopyById(data.channel);
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
            auto testWidgetOpt = cabbage.getWidgetCopyById(data.channel);
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

#ifdef CabbageApp
    // For CabbageApp, don't call updateUI() here because:
    // 1. The webview sends cabbageIsReadyToLoad explicitly when ready
    // 2. setCabbageIsReady() will be called which enables the opcode queue
    // 3. Calling updateUI() here creates a race condition where widgets are sent
    //    both from the opcode queue (with table data) and from updateUI() (without table data)
    // 4. The updateUI() call after setCabbageIsReady() in onMessageFromWebView handles initial state
    lattice::logDebug << "onWebViewIsReady() called for CabbageApp - skipping updateUI(), waiting for cabbageIsReadyToLoad";
#else
    // For plugins, update UI immediately when webview is ready
    updateUI();
#endif
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
        lattice::logInfo << "CabbageProcessor: Calling queueGenTableUpdates() for webview reconnection";
        setCabbageIsReady();
        updateUI();
        // Re-queue table data for the reconnected webview
        cabbage.queueGenTableUpdates();
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

    // Process populate configurations for widgets that have them
    for (auto &w : cabbage.getWidgets())
    {
        if (w.contains("populate") && w["populate"].is_object())
        {
            std::string widgetChannel;
            if (w.contains("id") && w["id"].is_string())
            {
                widgetChannel = w["id"].get<std::string>();
            }
            else if (w.contains("channels") && w["channels"].is_array() && !w["channels"].empty() &&
                     w["channels"][0].contains("id"))
            {
                widgetChannel = w["channels"][0]["id"].get<std::string>();
            }

            if (!widgetChannel.empty())
            {
                nlohmann::json populateConfig = w["populate"];

                // Convert single string to array for consistency
                if (populateConfig.contains("directories") && populateConfig["directories"].is_string())
                {
                    std::string singleDir = populateConfig["directories"].get<std::string>();
                    populateConfig["directories"] = nlohmann::json::array({singleDir});
                }

                // Verify it has the required fields before processing
                if (populateConfig.contains("directories") && populateConfig["directories"].is_array() &&
                    populateConfig.contains("fileType"))
                {
                    lattice::logDebug << "Processing initial populate for widget: " << widgetChannel;

                    // Process populate on background thread, then update widget with results
                    cabbage::Parser::processPopulateAsync(
                        widgetChannel, populateConfig, [this, widgetChannel](const nlohmann::json &result) {
                            // Update widget with populated items (this runs on background thread)
                            cabbage.updateWidget(widgetChannel, [result](nlohmann::json &j) {
                                if (result.contains("items"))
                                {
                                    j["items"] = result["items"];
                                }
                            });

                            // Send the updated widget directly to UI
                            auto widgetCopy = cabbage.getWidgetCopyById(widgetChannel);
                            if (widgetCopy.has_value())
                            {
                                nlohmann::json msg;
                                msg["command"] = "widgetUpdate";
                                msg["id"] = widgetChannel;
                                msg["widgetJson"] = widgetCopy->dump();
                                sendWebViewMessage(msg);
                            }

                            lattice::logDebug << "Finished processing initial populate for widget: " << widgetChannel;
                        });
                }
            }
        }
    }

    // Check if editor has any pending messages when loaded..
    for (const auto &param : webviewMessageQueue)
    {
        bool updated = cabbage.updateWidget(param.name, [&](nlohmann::json &j) {
            j["value"] = param.value;
        });
        
        if (updated)
        {
            // Send proper JSON message like CabbageApp does
            nlohmann::json msg;
            msg["command"] = "widgetUpdate";
            msg["id"] = param.name;
            msg["value"] = param.value;
            cabbage.setControlChannel(param.name, param.value);
            sendWebViewMessage(msg);
            lattice::logDebug << "Widget sent via queue: " << msg.dump();
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

        // Update the channel's range.value (not top-level value property)
        if (j.contains("channels") && j["channels"].is_array() && !j["channels"].empty())
        {
            auto& firstChannel = j["channels"][0];
            if (firstChannel.contains("range") && firstChannel["range"].is_object())
            {
                firstChannel["range"]["value"] = denormalValue;
            }
        }

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
            
            // Update channel range value with the index
            if (j.contains("channels") && j["channels"].is_array() && !j["channels"].empty())
            {
                auto& firstChannel = j["channels"][0];
                if (firstChannel.contains("range") && firstChannel["range"].is_object())
                {
                    firstChannel["range"]["value"] = static_cast<double>(index);
                }
            }
            
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
