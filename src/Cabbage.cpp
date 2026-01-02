/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include "Cabbage.h"
#include "CabbageProcessor.h"

#include <choc/text/choc_StringUtilities.h>

namespace cabbage
{

Engine::Engine(CabbageProcessor &p, std::string file)
    : csdFile(file), processor(p)
{
    
};

Engine::~Engine()
{
    if (csound)
    {
        csCompileResult = false;
        csound.reset();
    }
}

void Engine::addOpcodes()
{
    // The order in which these are registered is important!
    csnd::plugin<CabbageSetValue>((csnd::Csound *)csound->GetCsound(), "cabbageSetValue", "", "Si", csnd::thread::i);
    csnd::plugin<CabbageSetValue>((csnd::Csound *)csound->GetCsound(), "cabbageSetValue", "", "SkP", csnd::thread::k);

    csnd::plugin<CabbageSetInitString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "SW", csnd::thread::i);
    csnd::plugin<CabbageSetPerfString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "kSSW", csnd::thread::k);
    
    csnd::plugin<CabbageSetInitMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "SSM", csnd::thread::i);
    csnd::plugin<CabbageSetPerfMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "kSSM", csnd::thread::k);
    

    csnd::plugin<CabbageSetInitMYFLTArray>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "SSi[]", csnd::thread::i);
    csnd::plugin<CabbageSetPerfMYFLTArray>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "kSSk[]", csnd::thread::k);

    csnd::plugin<CabbageGetValue>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "i", "S", csnd::thread::i);
    csnd::plugin<CabbageGetValue>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "k", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValueString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "S", "S", csnd::thread::i);
    csnd::plugin<CabbageGetValueWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "kk", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValueStringWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "Sk", "S", csnd::thread::ik);

    csnd::plugin<CabbageGetMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "k", "SW", csnd::thread::ik);
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "i", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "S", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetStringWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "Sk", "SS", csnd::thread::k);

    csnd::plugin<CabbageCreate>((csnd::Csound *)getCsound()->GetCsound(), "cabbageCreate", "", "S", csnd::thread::i);
    csnd::plugin<CabbageDump>((csnd::Csound *)getCsound()->GetCsound(), "cabbageDump", "", "So", csnd::thread::i);
    csnd::plugin<CabbageDumpWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageDump", "", "kSo", csnd::thread::ik);
    
    csnd::plugin<CabbageSaveState>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSaveState", "", "S", csnd::thread::k);
    csnd::plugin<CabbageLoadState>((csnd::Csound *)getCsound()->GetCsound(), "cabbageLoadState", "", "S", csnd::thread::k);
    
    csnd::plugin<CabbageGetFiles>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetFiles", "S[]", "SS", csnd::thread::i);
    csnd::plugin<CabbageCreateFileName>((csnd::Csound *)getCsound()->GetCsound(), "cabbageCreateFileName", "S", "SS", csnd::thread::i);
    csnd::plugin<CabbageJoinPath>((csnd::Csound *)getCsound()->GetCsound(), "cabbageJoinPath", "S", "SW", csnd::thread::i);

}

bool Engine::setupCsound()
{
    csound = std::make_unique<Csound>();
    csound->SetHostMIDIIO();
    csound->SetHostAudioIO();
    csound->SetHostData(this);

    addOpcodes();

    csound->CreateMessageBuffer(0);
    csound->SetExternalMidiInOpenCallback(CabbageProcessor::OpenMidiInputDevice);
    csound->SetExternalMidiReadCallback(CabbageProcessor::ReadMidiData);
    csound->SetExternalMidiOutOpenCallback(CabbageProcessor::OpenMidiOutputDevice);
    csound->SetExternalMidiWriteCallback(CabbageProcessor::WriteMidiData);

    csound->SetOption((char *)"-n");
    csound->SetOption((char *)"-d");
    csound->SetOption((char *)"-b0");
    csound->SetOption(std::string("--sample-rate=" + std::to_string(processor.getSampleRate())).c_str());
    csound->SetOption(
        std::string("--nchnls=" + std::to_string(processor.getChannelConfig().getTotalNumOutputChannels())).c_str());
    csound->SetOption(
        std::string("--nchnls_i=" + std::to_string(processor.getChannelConfig().getTotalNumInputChannels())).c_str());
    //    csdFile = "/Users/rwalsh/Library/CabbageAudio/CabbagePluginEffect/CabbagePluginEffect.csd";
    std::filesystem::path file = csdFile.empty() ? cabbage::File::getCsdFileAndPath() : csdFile;
    csdFile = file.string();

    bool exists = std::filesystem::exists(csdFile);
    if (exists)
    {
        // Check for compile time errors
        csCompileResult = csound->Compile(csdFile.c_str());
        setReservedChannels();
        // No check for i-time errors and instr0 issues
        if (csound->Start() == CSOUND_SUCCESS && csdCompiledWithoutError())
        {
            csdKsmps = csound->GetKsmps();
            csSpin = csound->GetSpin();
            csScale = csound->Get0dBFS();
            lattice::logDebug << "Resetting csound ...\ncsound = " << csound.get();
        }
        else
        {
            // Csound could not compile your file?
            while (csound->GetMessageCnt() > 0)
            {
                compileErrors += csound->GetFirstMessage();
                csound->PopFirstMessage();
            }
            return false;
        }

        widgets.clear();
        std::string jsonError;
        widgets = cabbage::Parser::parseCsdForWidgets(csdFile, &jsonError);

        // If there was a JSON parse error, add it to compileErrors and return false
        if (!jsonError.empty())
        {
            compileErrors += "\n" + jsonError;
            return false;
        }

        // Initialise genTable widgets that have file properties
        initialiseGenTableWidgets();

        // Queue automatic updates for genTable widgets with tableNumber > 0
        queueGenTableUpdates();

        return true;
    }
    else
        return false;
}

//===========================================================================================
void Engine::initParameter(nlohmann::json &w)
{
    // Ensure default ranges are set if missing
    cabbage::Parser::assignDefaultRangesToChannels(w);

    // Handle new schema: channels array
    if (w.contains("channels") && w["channels"].is_array())
    {
        for (const auto &ch : w["channels"])
        {
            if (!ch.contains("id") || !ch["id"].is_string())
                continue;
            std::string channel = cabbage::Parser::removeQuotes(ch["id"].get<std::string>());

            if (ch.contains("range"))
            {
                auto &channelRange = ch["range"];
                float defaultValue =
                    channelRange.contains("value")
                        ? channelRange["value"].get<float>()
                        : (channelRange.contains("defaultValue") ? channelRange["defaultValue"].get<float>() : 0.5f);

                parameterChannels.push_back({channel, defaultValue});
                csound->SetControlChannel(channel.c_str(), defaultValue);
                numberOfParameters++;
            }
        }
    }
    // Handle old schema: multi-channel widgets (e.g., xyPad with x and y)
    else if (w.contains("channel") && w["channel"].is_object())
    {
        lattice::logError << "Old schema is no longer supported";
    }
    // Handle old schema: single-channel widgets
    else if (w.contains("channel") && w["channel"].is_string())
    {
        std::string channel = cabbage::Parser::removeQuotes(w["channel"].get<std::string>());
        float defaultValue = 0.5f;

        if (w.contains("range"))
        {
            defaultValue = w["range"]["defaultValue"].get<float>();
        }
        else if (w.contains("defaultValue"))
        {
            defaultValue = w["defaultValue"].get<float>();
        }

        parameterChannels.push_back({channel, defaultValue});
        csound->SetControlChannel(channel.c_str(), defaultValue);
        numberOfParameters++;
    }
}

//===========================================================================================
void Engine::setReservedChannels()
{
    auto path = cabbage::File::getCsdPath(csdFile);
    csound->SetStringChannel("CSD_PATH", (char *)path.c_str());

    // Set all reserved directory channels using lattice::File::getSpecialLocation
    // All paths are converted to generic string format, i.e, forward slashes, for cross-platform compatibility
    csound->SetStringChannel("USER_HOME_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_HOME_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("USER_DOCUMENTS_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_DOCUMENTS_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("USER_DESKTOP_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_DESKTOP_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("USER_MUSIC_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_MUSIC_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("USER_MOVIES_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_MOVIES_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("USER_PICTURES_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_PICTURES_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("USER_APPLICATION_DATA_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("USER_APPLICATION_DATA_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("COMMON_APPLICATION_DATA_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("COMMON_APPLICATION_DATA_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("COMMON_DOCUMENTS_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("COMMON_DOCUMENTS_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("WINDOWS_SYSTEM_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("WINDOWS_SYSTEM_DIRECTORY")).generic_string().c_str());

    csound->SetStringChannel("GLOBAL_APPLICATIONS_DIRECTORY",
        (char *)std::filesystem::path(lattice::File::getSpecialLocation("GLOBAL_APPLICATIONS_DIRECTORY")).generic_string().c_str());
}

//===========================================================================================
int Engine::getCurrentParameterCount()
{
    return static_cast<int>(processor.getParameters().size());
}

//===========================================================================================
int Engine::getNumberOfParameters(const std::string &csdFile)
{
    std::vector<nlohmann::json> widgets =
        cabbage::Parser::parseCsdForWidgets(csdFile.empty() ? cabbage::File::getCsdFileAndPath() : csdFile);
    int numParams = 0;
    for (auto &w : widgets)
    {
        // Check for automatable - expect boolean true
        if (w.contains("automatable") && w["automatable"].is_boolean() && w["automatable"].get<bool>())
            numParams++;
    }

    return numParams;
}

const std::string Engine::getIOChannalConfig(const std::string &csdFile)
{
    // get channel config from JSON
    const std::string channelConfig = cabbage::Utils::getChannelConfig(csdFile);
    // get channel config defined in Csd file
    const int numOutputs = cabbage::File::getNumberOfOutputChannels(csdFile);
    const int numInputs = cabbage::File::getNumberOfInputChannels(csdFile) == -1
                              ? numOutputs
                              : cabbage::File::getNumberOfInputChannels(csdFile);

    if (cabbage::Utils::validateChannelConfig(channelConfig, numInputs, numOutputs))
        return channelConfig;
    else
        return "2-2";
}

std::pair<std::vector<int>, std::vector<int>> Engine::parseBusConfiguration(const std::string &config)
{
    auto splitAndParse = [](const std::string &str) -> std::vector<int>
    {
        std::vector<int> buses;
        std::stringstream ss(str);
        std::string segment;

        while (std::getline(ss, segment, '.'))
            buses.push_back(std::stoi(segment)); // Convert to int and store

        return buses;
    };

    size_t dashPos = config.find('-');
    if (dashPos == std::string::npos)
        throw std::invalid_argument("Invalid format. Expected '-' in input.");

    std::string inputPart = config.substr(0, dashPos);
    std::string outputPart = config.substr(dashPos + 1);

    std::vector<int> inputBuses = splitAndParse(inputPart);
    std::vector<int> outputBuses = splitAndParse(outputPart);

    return {inputBuses, outputBuses};
}
//===========================================================================================

void Engine::setControlChannel(const std::string channel, MYFLT value)
{
    // update Csound channel, and update ParameterChannel values..
    csound->SetControlChannel(channel.c_str(), value);
}

void Engine::setStringChannel(const std::string channel, std::string data)
{
    // update Csound channel
    csound->SetStringChannel(channel.c_str(), (char *)data.c_str());
}

//=============================================================================================
void Engine::processCsoundMessages()
{
    auto *csound = getCsound();
    if (!csound)
        return;

    // set max number of message to prevent UI freezing..
    const int maxMessagesPerCycle = 100;
    int processedCount = 0;

    while (csound->GetMessageCnt() > 0 && processedCount++ < maxMessagesPerCycle)
    {
        const char *msg = csound->GetFirstMessage();
        if (!msg)
        {
            csound->PopFirstMessage();
            continue;
        }

        std::string message(msg);

        if (!message.empty() && message.back() == '\n')
        {
            message.pop_back();
        }

        // Skip repetitive "end of Performance" messages
        if (message.find("end of Performance") != std::string::npos)
        {
            csound->PopFirstMessage();
            continue;
        }

        lattice::logInfo << message; // Log the message
        csound->PopFirstMessage();   // Remove from queue
    }
}

//===========================================================================================
size_t Engine::getIndexForParamChannel(std::string name)
{
    auto it = std::find_if(parameterChannels.begin(), parameterChannels.end(),
                           [&name](const ParameterChannel &paramChannel) { return paramChannel.name == name; });

    if (it != parameterChannels.end())
    {
        size_t index = std::distance(parameterChannels.begin(), it);
        return index;
    }

    return -1;
}

//===========================================================================================
std::string Engine::extractChannelName(const nlohmann::json &widget)
{
    // First priority: explicit widget id
    if (widget.contains("id") && widget["id"].is_string())
    {
        return cabbage::Parser::removeQuotes(widget["id"].get<std::string>());
    }

    // Second priority: new channels array format
    if (widget.contains("channels") && widget["channels"].is_array() && !widget["channels"].empty())
    {
        auto &firstChannel = widget["channels"][0];
        if (firstChannel.contains("id") && firstChannel["id"].is_string())
        {
            return cabbage::Parser::removeQuotes(firstChannel["id"].get<std::string>());
        }
    }

    // Third priority: legacy channel formats for backward compatibility
    if (widget.contains("channel"))
    {
        if (widget["channel"].is_string())
        {
            lattAssert(widget["channel"].is_string(), "Channel should be a string");
        }
        else if (widget["channel"].is_object() && widget["channel"].contains("id"))
        {
            return cabbage::Parser::removeQuotes(widget["channel"]["id"].get<std::string>());
        }
    }

    return "";
}

//===========================================================================================
std::optional<std::reference_wrapper<nlohmann::json>> Engine::findWidgetInArray(nlohmann::json &jsonArray,
                                                                                const std::string &channel)
{
    for (auto &w : jsonArray)
    {
        // New schema: channels array
        if (w.contains("channels") && w["channels"].is_array())
        {
            for (const auto &ch : w["channels"])
            {
                if (ch.contains("id") && ch["id"].is_string() && cabbage::Parser::removeQuotes(ch["id"]) == channel)
                {
                    return std::ref(w);
                }
            }
        }
        // Old schema: channel as string or object
        if (w.contains("channel"))
        {
            // Handle single-channel widgets (channel is a string)
            if (w["channel"].is_string() && cabbage::Parser::removeQuotes(w["channel"]) == channel)
            {
                return std::ref(w);
            }
            // Handle multi-channel widgets (channel is an object with x/y properties)
            else if (w["channel"].is_object())
            {
                for (auto &[key, value] : w["channel"].items())
                {
                    if (value.is_string() && cabbage::Parser::removeQuotes(value) == channel)
                    {
                        return std::ref(w);
                    }
                }
            }
        }
        if (w.contains("children") && w["children"].is_array())
        {
            auto child = findWidgetInArray(w["children"], channel);
            if (child)
                return child;
        }
    }
    return std::nullopt;
}

// Overload for std::vector<nlohmann::json>
std::optional<std::reference_wrapper<nlohmann::json>> Engine::getWidgetFromId(std::vector<nlohmann::json> &widgets,
                                                                                 const std::string &channel)
{
    for (auto &w : widgets)
    {
        // New schema: channels array
        if (w.contains("channels") && w["channels"].is_array())
        {
            for (const auto &ch : w["channels"])
            {
                if (ch.contains("id") && ch["id"].is_string() && cabbage::Parser::removeQuotes(ch["id"]) == channel)
                {
                    return std::ref(w);
                }
            }
        }

        if (w.contains("id") && w["id"].is_string() && cabbage::Parser::removeQuotes(w["id"]) == channel)
        {
            return std::ref(w);
        }
        
        if (w.contains("children") && w["children"].is_array())
        {
            auto child = findWidgetInArray(w["children"], channel);
            if (child)
                return child;
        }
    }
    return std::nullopt;
}

// Overload for nlohmann::json (assuming it's an array)
std::optional<std::reference_wrapper<nlohmann::json>> Engine::getWidgetByChannel(nlohmann::json &widgets,
                                                                                 const std::string &channel)
{
    return findWidgetInArray(widgets, channel);
}

const std::string Engine::updateWidgetState(nlohmann::json j)
{
    // Accept either 'id' (new) or 'channel' (legacy)
    std::string channel;
    if (j.contains("id") && j["id"].is_string())
    {
        channel = j["id"].get<std::string>();
    }
    else if (j.contains("channel") && j["channel"].is_string())
    {
        channel = j["channel"].get<std::string>();
    }
    else
    {
        lattice::logError << "updateWidgetState: missing 'id' or 'channel' field";
        return "";
    }
    
    auto widgetOpt = getWidgetFromId(widgets, channel);
    if (widgetOpt.has_value())
    {
        auto &w = widgetOpt.value().get();
        w.merge_patch(j);
        auto result = getUpdatedWidgetJsonStr(channel, w.dump(), false);
        return result;
    }

    return "";
}
//===========================================================================================

std::string Engine::getUpdatedWidgetJsonStr(const std::string &channel, std::string data, bool includeValue)
{
    std::string result;
    if (includeValue)
    {
        // Parse the data to extract the value
        nlohmann::json widgetJson = nlohmann::json::parse(data);
        float value = 0.0f;

        if (widgetJson.contains("value") && !widgetJson["value"].is_null())
        {
            value = widgetJson["value"].get<float>();
        }
        else if (widgetJson.contains("range") && widgetJson["range"].contains("defaultValue"))
        {
            value = widgetJson["range"]["defaultValue"].get<float>();
        }
        else
        {
            value = 0.0f; // Default fallback value
        }
        // Escape the data string for JSON by replacing backslashes and quotes
        std::string escapedData = data;
        size_t pos = 0;
        while ((pos = escapedData.find('\\', pos)) != std::string::npos) {
            escapedData.replace(pos, 1, "\\\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = escapedData.find('"', pos)) != std::string::npos) {
            escapedData.replace(pos, 1, "\\\"");
            pos += 2;
        }
        
        result = choc::text::replace(R"(
        {
            "command": "widgetUpdate",
            "id": "$CHANNEL",
            "widgetJson": "$DATA",
            "value": $VALUE
        }
        )",
                                     "$CHANNEL", channel, "$DATA", escapedData, "$VALUE", std::to_string(value));
    }
    else
    {
        result = choc::text::replace(R"(
        {
            command: "widgetUpdate",
            id: "$CHANNEL",
            widgetJson: `$DATA`
        }
        )",
                                     "$CHANNEL", channel, "$DATA", data);
    }
    return result;
}

std::string Engine::getUpdatedWidgetJsonStr(const std::string &channel, float value)
{
    std::string result;
    result = choc::text::replace(R"(
    {
        command: "widgetUpdate",
        id: "$CHANNEL",
        value: $VALUE
    }
    )",
                                 "$CHANNEL", channel, "$VALUE", std::to_string(value));

    return result.c_str();
}

void Engine::updateFunctionTable(CabbageOpcodeData data, nlohmann::json &jsonObj)
{
    if (jsonObj["type"].get<std::string>() == "genTable")
    {
        if (data.cabbageJson.contains("tableNumber") && data.cabbageJson["tableNumber"].get<int>() != -9999)
        {
            try
            {
                cabbage::Parser::mergeJsonProperties(jsonObj, data.cabbageJson);

                // Get tableNumber from data.cabbageJson since initialiseWidgetJson may not set it reliably
                int tableNumber = -1;
                if (data.cabbageJson.contains("tableNumber") && data.cabbageJson["tableNumber"].is_number())
                {
                    tableNumber = data.cabbageJson["tableNumber"].get<int>();
                }
                else if (jsonObj.contains("tableNumber") && jsonObj["tableNumber"].is_number())
                {
                    tableNumber = jsonObj["tableNumber"].get<int>();
                }

                if (tableNumber != -1)
                {
                    const int tableSize = getCsound()->TableLength(tableNumber);

                    if (tableSize != -1)
                    {
                        MYFLT *tablePtr = nullptr;
                        auto length = csound->GetTable(&tablePtr, tableNumber);
                        std::vector<MYFLT> temp(tablePtr, tablePtr + length);
                        setTableJSON(data.channel, temp, jsonObj);
                    }
                }
            }
            catch (nlohmann::json::exception &e)
            {
                lattice::logDebug << "JSON Error:" << e.what();
            }
        }
        else if (data.cabbageJson.contains("file"))
        {
            try
            {

                cabbage::Parser::mergeJsonProperties(jsonObj, data.cabbageJson);

                auto soundfile = cabbage::File::readAudioFile<double>(jsonObj["file"].get<std::string>(), static_cast<int>(sampleRate));
                auto samples = soundfile.audioData;

                if (samples.size() == 0)
                    return;

                setTableJSON(data.channel, samples, jsonObj);
            }
            catch (nlohmann::json::exception &e)
            {
                lattice::logDebug << "JSON Error:" << e.what();
            }
        }
    }
}

void Engine::setTableJSON(std::string /*channel*/, std::vector<double> samples, nlohmann::json &jsonObj)
{
    // this is a condensed version of the sample data that is passed around between C++ and JS.
    std::vector<double> widgetSampleData;

    // Ensure range object exists and set default y-axis range for waveform display
    if (!jsonObj.contains("range"))
    {
        jsonObj["range"] = nlohmann::json::object();
    }
    if (!jsonObj["range"].contains("y"))
    {
        jsonObj["range"]["y"] = {{"min", -1.0}, {"max", 1.0}};
    }

    // Handle nested range structure for genTable (sampleRange for sample selection)
    int startSample = 0;
    int endSample = static_cast<int>(samples.size());

    if (jsonObj.contains("range") && jsonObj["range"].is_object())
    {
        if (jsonObj["range"].contains("x") && jsonObj["range"]["x"].is_object())
        {
            // Legacy nested structure: range.x.start/end for sample range
            if (jsonObj["range"]["x"].contains("start"))
                startSample = jsonObj["range"]["x"]["start"].get<int>();
            if (jsonObj["range"]["x"].contains("end"))
                endSample = jsonObj["range"]["x"]["end"].get<int>() == -1 ? static_cast<int>(samples.size())
                                                                          : jsonObj["range"]["x"]["end"].get<int>();
        }
        else if (jsonObj["range"].contains("start"))
        {
            // Legacy flat structure: range.start/end
            startSample = jsonObj["range"]["start"].get<int>() == 0 ? 0 : jsonObj["range"]["start"].get<int>();
            endSample = jsonObj["range"]["end"].get<int>() == -1 ? static_cast<int>(samples.size())
                                                                 : jsonObj["range"]["end"].get<int>();
        }
    }

    // no point in sending more samples that can be displayed per pixel...
    const float incr = float(endSample - startSample) / ((jsonObj["bounds"]["width"].get<float>()));
    lattice::logDebug << "Updating function table";
    for (int i = startSample; i < static_cast<int>(endSample); i += static_cast<int>(incr))
    {
        widgetSampleData.push_back(samples[int(i)]);
    }

    jsonObj["samples"] = widgetSampleData;
    jsonObj["totalSamples"] = static_cast<int>(samples.size());
}

void Engine::initialiseGenTableWidgets()
{
    for (auto &widget : widgets)
    {
        if (widget["type"].get<std::string>() == "genTable" && widget.contains("file") && widget["file"].is_string() &&
            !widget["file"].get<std::string>().empty())
        {
            try
            {
                const int tableNumber = widget["tableNumber"].get<int>();
                auto soundfile = cabbage::File::readAudioFile<double>(widget["file"].get<std::string>(), static_cast<int>(sampleRate));
                auto samples = soundfile.audioData;

                if (samples.size() > 0)
                {
                    std::stringstream ss;
                    ss << "giTable" << tableNumber << " ftgen " << tableNumber << ", 0, " << samples.size()
                       << ", -7, 0, 0";
                    getCsound()->CompileOrc(ss.str().c_str());
                    const int tableSize = getCsound()->TableLength(tableNumber);

                    if (tableSize != -1)
                    {
                        MYFLT *tablePtr = nullptr;
                        getCsound()->GetTable(&tablePtr, tableNumber);
                        std::memcpy(tablePtr, samples.data(),
                                    std::min(tableSize, static_cast<int>(samples.size())) * sizeof(MYFLT));
                        setTableJSON("", samples, widget);
                    }
                    else
                    {
                        lattice::logError << "Failed to create/update Csound table for genTable: "
                                          << widget["file"].get<std::string>();
                    }
                }
                else
                {
                    lattice::logError << "No samples loaded from file: " << widget["file"].get<std::string>();
                }
            }
            catch (const std::exception &e)
            {
                lattice::logError << "Failed to load genTable file: " << e.what();
            }
        }
        else if (widget["type"].get<std::string>() == "genTable" && widget.contains("tableNumber") &&
                 widget["tableNumber"].is_number() && widget["tableNumber"] != -9999)
        {
            std::string channelName;
            if (widget["channel"].is_string())
            {
                lattice::logDebug << "Error widget[\"channel\"] " << widget["channel"] << "should not be string ";
            }
            else if (widget["channel"].is_object() && widget["channel"].contains("id"))
            {
                channelName = cabbage::Parser::removeQuotes(widget["channel"]["id"].get<std::string>());
            }
            else
            {
                // Skip if no valid channel
                continue;
            }
            CabbageOpcodeData data;
            data.channel = channelName;
            data.type = CabbageOpcodeData::MessageType::Identifier;
            data.cabbageJson = {{"tableNumber", widget["tableNumber"].get<int>()}};
            opcodeData.enqueue(data);
        }
    }
}

void Engine::queueGenTableUpdates()
{
    // Automatically queue table data updates for genTable widgets with tableNumber > 0.
    // This handles function tables defined in the Csound score (e.g., f 1 0 8 -2 ...)
    // which don't exist until after Csound's init pass. Without this, users would need
    // to manually call cabbageSet "tableId", "tableNumber", N to trigger the update.
    // Iterate through all widgets looking for genTable widgets with tableNumber > 0
    for (auto &widget : widgets)
    {
        if (widget["type"].get<std::string>() == "genTable" && widget.contains("tableNumber") &&
            widget["tableNumber"].is_number_integer())
        {
            const int tableNumber = widget["tableNumber"].get<int>();

            // Only process if tableNumber is valid (> 0)
            if (tableNumber > 0)
            {
                // Check if this table exists in Csound
                const int tableSize = getCsound()->TableLength(tableNumber);

                if (tableSize > 0)
                {
                    // Get the channel name
                    std::string channelId = extractChannelName(widget);

                    if (!channelId.empty())
                    {
                        // Create an opcode data message to trigger table update
                        CabbageOpcodeData data;
                        data.channel = channelId;
                        data.type = CabbageOpcodeData::MessageType::Identifier;
                        data.cabbageJson["tableNumber"] = tableNumber;

                        // Queue the update - this will be processed in onIdle()
                        opcodeData.enqueue(data);

                        lattice::logDebug << "Queued automatic table update for genTable '" << channelId
                                          << "' with tableNumber " << tableNumber << " (size: " << tableSize << ")";
                    }
                }
            }
        }
    }
}

const std::string Engine::getCsoundOutputUpdateScript(const std::string &output)
{
    auto outputText = choc::text::replace(output, "`", "");

    std::string result;
    result = choc::text::replace(R"(
         window.postMessage({ command: "csoundOutputUpdate", text: `$OUTPUT_TEXT` });
        )",
                                 "$OUTPUT_TEXT", outputText);

    return result.c_str();
}

//===========================================================================================

float Engine::remap(double n, double start1, double stop1, double start2, double stop2)
{
    return static_cast<float>(((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2);
}

float Engine::getFullRangeValue(std::string channel, float normalValue)
{
    for (const auto &w : getWidgets())
    {
        if (w["channel"] == channel && w.contains("range"))
        {
            return Engine::remap(normalValue, 0.f, 1.f, w["range"]["min"], w["range"]["max"]);
        }
        else
            return normalValue;
    }

    return normalValue;
}

//===========================================================================================
std::string Engine::removeControlCharacters(const std::string &input)
{
    std::string result;
    for (char c : input)
    {
        if (!iscntrl(static_cast<unsigned char>(c)) || c == ' ')
        {
            result += c;
        }
    }
    return result;
}

// Check if a widget has a specific channel (searches id then channels array)
bool Engine::hasChannel(const nlohmann::json &widget, const std::string &channel)
{
    // First check widget["id"]
    if (widget.contains("id") && widget["id"].is_string())
    {
        std::string widgetId = cabbage::Parser::removeQuotes(widget["id"].get<std::string>());
        if (widgetId == channel)
        {
            return true;
        }
    }

    // Then check widget["channels"] array
    if (widget.contains("channels") && widget["channels"].is_array())
    {
        for (const auto &ch : widget["channels"])
        {
            if (ch.contains("id") && ch["id"].is_string())
            {
                std::string chId = cabbage::Parser::removeQuotes(ch["id"].get<std::string>());
                if (chId == channel)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void Engine::updateChannelCache(const CabbageOpcodeData &data)
{
    if (channelCache.find(data.channel) != channelCache.end())
    {
        // For value-only updates, don't merge - just update the value field
        if (data.type == CabbageOpcodeData::MessageType::Value)
        {
            channelCache[data.channel].cabbageJson["value"] = data.cabbageJson["value"];
            channelCache[data.channel].type = CabbageOpcodeData::MessageType::Value;
        }
        else
        {
            channelCache[data.channel].cabbageJson.merge_patch(data.cabbageJson);
            // If the new data is of type Identifier, we must update the cached type to Identifier
            // so that the full JSON is sent to the frontend, not just the value.
            if (data.type == CabbageOpcodeData::MessageType::Identifier)
            {
                channelCache[data.channel].type = CabbageOpcodeData::MessageType::Identifier;
            }
        }
    }
    else
    {
        channelCache[data.channel] = data;
    }
    dirtyChannels.insert(data.channel);
}

bool Engine::isValueDifferent(const CabbageOpcodeData &data)
{
    if (channelCache.find(data.channel) == channelCache.end())
    {
        return true;
    }

    const auto &cachedData = channelCache[data.channel];

    // Helper lambda to check if json is subset
    auto isSubset = [](const nlohmann::json &j1, const nlohmann::json &j2) -> bool {
        if (j1.is_object() && j2.is_object())
        {
            for (auto it = j1.begin(); it != j1.end(); ++it)
            {
                if (j2.find(it.key()) == j2.end())
                {
                    return false;
                }
                if (it.value() != j2[it.key()])
                {
                    return false;
                }
            }
            return true;
        }
        return j1 == j2;
    };

    if (data.type == CabbageOpcodeData::MessageType::Value)
    {
        return cachedData.cabbageJson["value"] != data.cabbageJson["value"];
    }
    else
    {
        // Check if new data is already contained in cached data
        return !isSubset(data.cabbageJson, cachedData.cabbageJson);
    }
}

void Engine::flushChannelCache()
{
    if (!dirtyChannels.empty())
    {
        lattice::logDebug << "Flushing " << dirtyChannels.size() << " dirty channels to opcodeData queue";
    }
    
    for (const auto &channel : dirtyChannels)
    {
        const auto &cachedData = channelCache[channel];

        // For value-only updates, create minimal message
        if (cachedData.type == CabbageOpcodeData::MessageType::Value)
        {
            CabbageOpcodeData minimalData;
            minimalData.channel = cachedData.channel;
            minimalData.type = CabbageOpcodeData::MessageType::Value;
            minimalData.cabbageJson["value"] = cachedData.cabbageJson["value"];
            opcodeData.enqueue(minimalData);
        }
        else
        {
            // For identifier updates, send full JSON
            opcodeData.enqueue(cachedData);
        }
    }
    dirtyChannels.clear();
}

//=====================================================================================
// State Management Utilities - Used by both opcodes and CabbageProcessor
//=====================================================================================

nlohmann::json Engine::saveWidgetState()
{
    nlohmann::json state;
    state["cabbageWidgetsState"] = widgets;
    return state;
}

void Engine::loadWidgetState(const nlohmann::json &state)
{
    // Check if we have the widget state
    if (!state.contains("cabbageWidgetsState")) {
        lattice::logError << "Invalid state: missing 'cabbageWidgetsState' key";
        return;
    }
    
    // Restore the complete widget state
    widgets = state["cabbageWidgetsState"];

    // Update Csound control channels and plugin parameters
    for (const auto &widget : widgets) {
        if (widget.contains("channels") && widget["channels"].is_array()) {
            for (const auto &channel : widget["channels"]) {
                if (channel.contains("id") && channel["id"].is_string()) {
                    std::string channelId = channel["id"].get<std::string>();
                    
                    if (widget.contains("value") && widget["value"].is_number()) {
                        float value = widget["value"].get<float>();
                        
                        // Update Csound control channel
                        setControlChannel(channelId, value);
                        
                        // If this is an automatable parameter, update it for the host
                        if (channel.contains("parameterIndex") && channel["parameterIndex"].is_number()) {
                            int paramIdx = channel["parameterIndex"].get<int>();
                            if (paramIdx < static_cast<int>(processor.getParameters().size())) {
                                auto &param = processor.getParameters()[paramIdx];
                                float normalizedValue = param.toNormalised(value);
                                param.value = normalizedValue;
                                
                                // Notify host of parameter change
                                processor.addParameterChange({paramIdx, normalizedValue, lattice::ParamChangeType::Value});
                                
                                lattice::logDebug << "Loaded parameter " << paramIdx << " (" << channelId 
                                                 << ") with value: " << value << " (normalized: " << normalizedValue << ")";
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Queue UI updates for all widgets
    for (const auto &widget : widgets) {
        std::string channelId;
        if (widget.contains("id") && widget["id"].is_string()) {
            channelId = widget["id"].get<std::string>();
        } else if (widget.contains("channels") && widget["channels"].is_array() && 
                   !widget["channels"].empty() && widget["channels"][0].contains("id")) {
            channelId = widget["channels"][0]["id"].get<std::string>();
        }
        
        if (!channelId.empty()) {
            // Queue the full widget update
            CabbageOpcodeData data;
            data.channel = channelId;
            data.type = CabbageOpcodeData::MessageType::Identifier;
            data.cabbageJson = widget;
            opcodeData.enqueue(data);
        }
    }
    
    lattice::logInfo << "Widget state loaded successfully";
}

//=====================================================================================
// Process webview commands - central handler for UI messages
// Returns true if handled, false if environment-specific handling needed
//=====================================================================================
bool Engine::processWebViewCommand(const nlohmann::json &message)
{
    if (!message.contains("command"))
    {
        lattice::logError << "Message missing command field";
        return false;
    }

    const std::string command = message["command"].get<std::string>();

    // Handle cabbageIsReadyToLoad
    if (command == "cabbageIsReadyToLoad")
    {
        // This is typically handled by the processor to trigger UI updates
        return false; // Let processor handle this
    }

    // Handle widgetStateUpdate
    else if (command == "widgetStateUpdate")
    {
        updateWidgetState(message);
        return true;
    }

    // Handle midiMessage
    else if (command == "midiMessage")
    {
        // MIDI messages need processor's addNoteEvent functionality
        return false; // Let processor handle this
    }

    // Handle channelData
    else if (command == "channelData")
    {
        // Extract channel - accept 'id' (new) or 'channel' (legacy)
        std::string channel;
        
        // Try 'id' first (new format)
        if (message.contains("id"))
        {
            if (message["id"].is_string())
            {
                channel = message["id"].get<std::string>();
            }
            else if (message["id"].is_object() && message["id"].contains("id"))
            {
                channel = message["id"]["id"].get<std::string>();
            }
        }
        // Fall back to 'channel' (legacy format)
        else if (message.contains("channel"))
        {
            if (message["channel"].is_string())
            {
                channel = message["channel"].get<std::string>();
            }
            else if (message["channel"].is_object() && message["channel"].contains("id"))
            {
                channel = message["channel"]["id"].get<std::string>();
            }
            else if (message["channel"].is_object())
            {
                // For multi-channel, find the first string value
                for (auto &[key, value] : message["channel"].items())
                {
                    if (value.is_string())
                    {
                        channel = value.get<std::string>();
                        break;
                    }
                }
            }
        }
        
        if (channel.empty())
        {
            lattice::logError << "Invalid channel format in channelData message: " << message.dump();
            return false;
        }

        // Check if we have string data or float data
        if (message.contains("stringData"))
        {
            std::string stringData = message.value("stringData", "");
            // Set the Csound string channel
            getCsound()->SetChannel(channel.c_str(), stringData.c_str());
        }
        else if (message.contains("floatData"))
        {
            double floatData = message.value("floatData", 0.0);
            // Set the Csound control channel
            setControlChannel(channel, floatData);
            
            // Update the widget JSON
            auto widgetOpt = getWidgetFromId(widgets, channel);
            if (widgetOpt)
            {
                auto &j = widgetOpt->get();
                j["value"] = floatData;
            }
        }
        else
        {
            lattice::logError << "channelData message missing both stringData and floatData fields";
            return false;
        }
        
        return true;
    }

    // Handle parameterChange - needs processor interaction
    else if (command == "parameterChange")
    {
        return false; // Let processor handle parameter changes
    }

    // Handle fileOpen - environment specific
    else if (command == "fileOpen" || command == "fileOpenFromVSCode")
    {
        return false; // Environment-specific
    }

    // Handle stopAudio - CabbageApp specific
    else if (command == "stopAudio")
    {
        return false; // CabbageApp-specific
    }

    // Handle onFileChanged - CabbageApp specific
    else if (command == "onFileChanged")
    {
        return false; // CabbageApp-specific
    }

    // Handle initialiseWidgets - CabbageApp specific
    else if (command == "initialiseWidgets")
    {
        return false; // CabbageApp-specific
    }

    // Unknown command
    lattice::logDebug << "Unknown or unhandled command: " << command;
    return false;
}

//=====================================================================================
// Handle parameter update from UI - validates, normalizes, and applies changes
// Returns gesture string for plugin automation, or empty string on failure
//=====================================================================================
std::string Engine::handleParameterUpdate(const nlohmann::json &message)
{
    // Check for old format (wrapped in "obj") - no longer supported
    if (message.contains("obj"))
    {
        lattice::logDebug << "parameterChange message using deprecated 'obj' wrapper format is no longer supported. "
                             "Please update to use direct properties.";
        return "";
    }

    // Validate required fields
    if (!message.contains("paramIdx") || !message.contains("value") || !message.contains("channel"))
    {
        lattice::logError << "parameterChange message missing required fields (paramIdx, value, or channel)";
        return "";
    }

    // Extract paramIdx
    int paramIdx = message["paramIdx"].get<int>();
    if (paramIdx < 0 || paramIdx >= static_cast<int>(processor.getParameters().size()))
    {
        lattice::logError << "Invalid paramIdx: " << paramIdx 
                         << " (valid range: 0-" << processor.getParameters().size() - 1 << ")";
        return "";
    }

    // Extract denormalized value
    double denormValue = message["value"].get<double>();

    // Extract channel - can be a string or an object with 'id'
    std::string channel;
    if (message["channel"].is_string())
    {
        channel = message["channel"].get<std::string>();
    }
    else if (message["channel"].is_object() && message["channel"].contains("id"))
    {
        channel = message["channel"]["id"].get<std::string>();
    }
    else
    {
        lattice::logError << "Invalid channel format in parameterChange message";
        return "";
    }

    // Extract gesture (optional, defaults to "complete")
    std::string gesture = message.value("gesture", "complete");

    // Normalize the value
    auto param = processor.getParameter(paramIdx);
    double normalizedValue = param.toNormalised(denormValue);


    // Update parameter value
    processor.setParameter(paramIdx, normalizedValue);

    // Update Csound channel with denormalized value
    setControlChannel(channel, denormValue);

    // Update widget JSON
    auto widgetOpt = getWidgetFromId(widgets, channel);
    if (widgetOpt)
    {
        auto &j = widgetOpt->get();
        j["value"] = denormValue;
    }

    return gesture;
}

} // namespace cabbage
