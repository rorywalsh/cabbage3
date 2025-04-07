/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include "Cabbage.h"
#include "CabbageProcessor.h"


#include <text/choc_StringUtilities.h>

namespace cabbage
{

Engine::Engine(CabbageProcessor &p, std::string file) : csdFile(file), processor(p) 
{

};

Engine::~Engine()
{
    if (csound)
    {
        csCompileResult = false;
        csound = nullptr;
    }
}

void Engine::addOpcodes()
{
    csnd::plugin<CabbageSetValue>((csnd::Csound *)csound->GetCsound(), "cabbageSetValue", "", "SkP", csnd::thread::k);
    csnd::plugin<CabbageSetValue>((csnd::Csound *)csound->GetCsound(), "cabbageSetValue", "", "Si", csnd::thread::i);

    csnd::plugin<CabbageSetPerfString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "kSSW",
                                       csnd::thread::k);
    csnd::plugin<CabbageSetInitString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "SW",
                                       csnd::thread::i);
    csnd::plugin<CabbageSetPerfMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "kSSM",
                                      csnd::thread::k);
    csnd::plugin<CabbageSetInitMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "SSM",
                                      csnd::thread::i);

    csnd::plugin<CabbageSetInitMYFLTArray>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "SSi[]",
                                           csnd::thread::i);
    csnd::plugin<CabbageSetPerfMYFLTArray>((csnd::Csound *)getCsound()->GetCsound(), "cabbageSet", "", "kSSk[]",
                                           csnd::thread::k);
    //**cabbageSet** *kTrig*, *SChannel*, *SProperty*, *kValue[]*
    //**cabbageSet** *SChannel*, *SProperty*, *iValue[]*

    csnd::plugin<CabbageGetValue>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "k", "S",
                                  csnd::thread::ik);
    csnd::plugin<CabbageGetValue>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "i", "S",
                                  csnd::thread::i);
    csnd::plugin<CabbageGetValueString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "S", "S",
                                        csnd::thread::ik);
    csnd::plugin<CabbageGetValueWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "kk", "S",
                                             csnd::thread::ik);
    csnd::plugin<CabbageGetValueStringWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGetValue", "Sk",
                                                   "So", csnd::thread::ik);

    csnd::plugin<CabbageGetMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "k", "SW", csnd::thread::ik);
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "i", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetString>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "S", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetStringWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageGet", "Sk", "SS",
                                              csnd::thread::k);

    csnd::plugin<CabbageDump>((csnd::Csound *)getCsound()->GetCsound(), "cabbageDump", "", "So", csnd::thread::i);
    csnd::plugin<CabbageDumpWithTrigger>((csnd::Csound *)getCsound()->GetCsound(), "cabbageDump", "", "kSo",
                                         csnd::thread::ik);
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
    csound->SetOption(std::string("--nchnls=" + std::to_string(processor.getChannelConfig().getTotalNumOutputChannels())).c_str());
    csound->SetOption(std::string("--nchnls_i=" + std::to_string(processor.getChannelConfig().getTotalNumInputChannels())).c_str());

    //    csdFile = "/Users/rwalsh/Library/CabbageAudio/CabbagePluginEffect/CabbagePluginEffect.csd";
    std::filesystem::path file = csdFile.empty() ? cabbage::File::getCsdFileAndPath() : csdFile;
    csdFile = file.string();

    bool exists = std::filesystem::exists(csdFile);
    if (exists)
    {
        csCompileResult = csound->Compile(csdFile.c_str());
        csound->Start();
        if (csdCompiledWithoutError())
        {
            csdKsmps = csound->GetKsmps();
            csSpin = csound->GetSpin();
            csScale = csound->Get0dBFS();
            setReservedChannels();

            lattice::logDebug << "Resetting csound ...\ncsound = " << csound.get();
        }
        else
        {
            // Csound could not compile your file?
            while (csound->GetMessageCnt() > 0)
            {
                lattice::logInfo << csound->GetFirstMessage();
                compileErrors += csound->GetFirstMessage();
                csound->PopFirstMessage();
            }
            return false;
        }

        widgets.clear();
        widgets = cabbage::Parser::parseCsdForWidgets(csdFile);
   

        return true;
    }
    else
        return false;
}

//===========================================================================================
void Engine::initParameter(const nlohmann::json& w)
{
    parameterChannels.push_back(
        {cabbage::Parser::removeQuotes(w["channel"].get<std::string>()), w["range"]["value"].get<float>()});
    
    csound->SetControlChannel(w["channel"].get<std::string>().c_str(), w["range"]["value"].get<float>());
    
    numberOfParameters++;
}

//===========================================================================================
void Engine::setReservedChannels()
{
    auto path = cabbage::File::getCsdPath(csdFile);
    csound->SetStringChannel("CSD_PATH", (char *)path.c_str());
}

//==========================================================================================
std::vector<std::string> Engine::getRangeWidgetTypes(const std::vector<nlohmann::json> widgets)
{
    std::vector<std::string> typesWithRange;
    for (const auto &obj : widgets)
    {
        if (obj.contains("range") && obj["type"] != "genTable")
        {
            if (obj.contains("type"))
            {
                typesWithRange.push_back(obj["type"].get<std::string>());
            }
        }
    }
    return typesWithRange;
}
//===========================================================================================
int Engine::getNumberOfParameters(const std::string &csdFile)
{
    std::vector<nlohmann::json> widgets =
        cabbage::Parser::parseCsdForWidgets(csdFile.empty() ? cabbage::File::getCsdFileAndPath() : csdFile);
    int numParams = 0;
    for (auto &w : widgets)
    {
        if (w.contains("automatable") && w["automatable"] == 1)
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
    while (getCsound()->GetMessageCnt() > 0)
    {
        std::string message(getCsound()->GetFirstMessage());
        message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
        lattice::logInfo << message;
        // EvaluateJavaScript(cabbage.getCsoundOutputUpdateScript(message).c_str());
        getCsound()->PopFirstMessage();
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

std::optional<std::reference_wrapper<nlohmann::json>> Engine::getWidget(const std::string &channel)
{
    for (auto &w : widgets)
    {
        if (cabbage::Parser::removeQuotes(w["channel"].get<std::string>()) == channel)
        {
            return std::ref(w); // Use std::ref to wrap the reference
        }
    }
    return std::nullopt;
}

const std::string Engine::updateWidgetState(nlohmann::json j)
{
    auto const channel = j["channel"].get<std::string>();
    auto widgetOpt = getWidget(channel);
    if (widgetOpt.has_value())
    {
        auto &w = widgetOpt.value().get();
        w.merge_patch(j);
        auto result = getUpdatedWidgetJsonStr(w["channel"], w.dump());
        return result;
    }

    return "";
}
//===========================================================================================

std::string Engine::getUpdatedWidgetJsonStr(const std::string& channel, std::string data)
{
    std::string result;
    result = choc::text::replace(R"(
        {
            command: "widgetUpdate",
            channel: "$CHANNEL",
            data: `$DATA`
        }
    )", "$CHANNEL", channel, "$DATA", data);
    return result.c_str();
}

std::string Engine::getUpdatedWidgetJsonStr(const std::string& channel, float value)
{
    std::string result;
    result = choc::text::replace(R"(
        {
            command: "widgetUpdate",
            channel: "$CHANNEL",
            value: $VALUE
        }
    )", "$CHANNEL", channel, "$VALUE", std::to_string(value));
            
    return result.c_str();
}

void Engine::updateFunctionTable(CabbageOpcodeData data, nlohmann::json &jsonObj)
{
    if (data.cabbageJson.contains("tableNumber") || data.cabbageJson.contains("range"))
    {
        try
        {
            cabbage::Parser::updateJson(jsonObj, data.cabbageJson, widgets.size());
            const int tableNumber = int(jsonObj["tableNumber"]);
            const int tableSize = getCsound()->TableLength(tableNumber);

            if (tableSize != -1)
            {
                MYFLT *tablePtr = nullptr;
                auto length = csound->GetTable(&tablePtr, tableNumber);
                std::vector<MYFLT> temp(tablePtr, tablePtr + length);
                setTableJSON(data.channel, temp, jsonObj);
            }
        }
        catch (nlohmann::json::exception &e)
        {
            lattice::logDebug << e.what();
        }
    }
    else if (data.cabbageJson.contains("file"))
    {
        if (jsonObj["type"].get<std::string>() == "genTable")
        {
            cabbage::Parser::updateJson(jsonObj, data.cabbageJson, widgets.size());
            const int tableNumber = jsonObj["tableNumber"];
            auto soundfile = cabbage::File::readAudioFile<double>(jsonObj["file"].get<std::string>(), sampleRate);
            auto samples = soundfile.audioData;

            if (samples.size() == 0)
                return;

            std::stringstream ss;
            ss << "giTable" << tableNumber << " ftgen " << samples.size() << " 0, -7, 0, 0";
            getCsound()->CompileOrc(ss.str().c_str());
            const int tableSize = getCsound()->TableLength(tableNumber);
            
            if (tableSize != -1)
            {
                MYFLT *tablePtr = nullptr;
                getCsound()->GetTable(&tablePtr, tableNumber);
                std::memcpy(tablePtr, samples.data(), tableSize * sizeof(MYFLT));
                setTableJSON(data.channel, samples, jsonObj);
            }
        }
    }
}

void Engine::setTableJSON(std::string /*channel*/, std::vector<double> samples, nlohmann::json &jsonObj)
{
    // this is a condensed version of the sample data that is passed around between C++ and JS.
    std::vector<double> widgetSampleData;
    const int startSample = jsonObj["range"]["start"].get<int>() == 0 ? 0 : jsonObj["range"]["start"].get<int>();
    const int endSample = jsonObj["range"]["end"].get<int>() == -1 ? static_cast<int>(samples.size())
                                                                   : jsonObj["range"]["end"].get<int>();

    // no point in sending more samples that can be displayed per pixel...
    const float incr = float(endSample - startSample) / ((jsonObj["bounds"]["width"].get<float>()));
    lattice::logDebug << "Updating function table";
    for (float i = startSample; i < static_cast<int>(endSample); i += incr)
    {
        widgetSampleData.push_back(samples[int(i)]);
    }
    lattice::logDebug << "Table size" << widgetSampleData.size();
    //
    //    while(widgetSampleData.size() < jsonObj["bounds"]["width"].get<int>()))
    //    {
    //        widgetSampleData.push_back(widgetSampleData[widgetSampleData.size()-1]);
    //    }

    jsonObj["samples"] = widgetSampleData;
}

const std::string Engine::getCsoundOutputUpdateScript(const std::string &output)
{
    auto outputText = choc::text::replace(output, "`", "");

    std::string result;
    result = choc::text::replace(R"(
         window.postMessage({ command: "csoundOutputUpdate", text: `$OUTPUT_TEXT` });
        )", "$OUTPUT_TEXT", outputText);

    return result.c_str();
}

//===========================================================================================

float Engine::remap(double n, double start1, double stop1, double start2, double stop2)
{
    return ((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2;
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

} // namespace cabbage
