/*
 * Copyright (c) 2024 Rory Walsh
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include "Cabbage.h"
#include "CabbageProcessor.h"
#include "opcodes/CabbageSetOpcodes.h"
#include "opcodes/CabbageGetOpcodes.h"

namespace cabbage {

Engine::Engine(CabbageProcessor& p, std::string file): processor(p), csdFile(file)
{
    sampleRate = p.GetSampleRate();
    LOG_INFO(processor.NInChansConnected());
    LOG_INFO(processor.NOutChansConnected());
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
    //template <typename T>
    //int32_t plugin(Csound * csound, const char* name, const char* oargs,
    //    const char* iargs, uint32_t thr, uint32_t flags = 0) {
    //    CSOUND* cs = (CSOUND*)csound;
    //    if (thr == thread::ia || thr == thread::a) {
    //        return cs->AppendOpcode(cs, (char*)name, sizeof(T), flags,
    //            (char*)oargs, (char*)iargs, (SUBR)init<T>,
    //            (SUBR)aperf<T>, (SUBR)deinit<T>);
    //auto t = csoundAppendOpcode(getCsound()->GetCsound(), "cabbageSetValue", sizeof(), 0, "null", "", nullptr, nullptr, nullptr);
    auto ret = csnd::plugin<CabbageSetValue>((csnd::Csound*)csound->GetCsound(), "cabbageSetValue", "", "SkP", csnd::thread::k);
    csnd::plugin<CabbageSetValue>((csnd::Csound*)csound->GetCsound(), "cabbageSetValue", "", "Si", csnd::thread::i);
    
    csnd::plugin<CabbageSetPerfString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "kSSW", csnd::thread::k);
    csnd::plugin<CabbageSetInitString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "SW", csnd::thread::i);
    csnd::plugin<CabbageSetPerfMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "kSSM", csnd::thread::k);
    csnd::plugin<CabbageSetInitMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "SSM", csnd::thread::i);
    
    ret = csnd::plugin<CabbageGetValue>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "k", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValue>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "i", "S", csnd::thread::i);
    csnd::plugin<CabbageGetValueString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "S", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValueWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "kk", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValueStringWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "Sk", "So", csnd::thread::ik);
    
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "k", "SW", csnd::thread::ik);
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "i", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "S", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetStringWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "Sk", "SS", csnd::thread::k);
    
    csnd::plugin<CabbageDump>((csnd::Csound*) getCsound()->GetCsound(), "cabbageDump", "", "So", csnd::thread::i);
    csnd::plugin<CabbageDumpWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageDump", "", "kSo", csnd::thread::ik);
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
    
    
    csound->SetOption((char*)"-n");
    csound->SetOption((char*)"-d");
    csound->SetOption((char*)"-b0");
    csound->SetOption(std::string("--sample-rate="+std::to_string(sampleRate)).c_str());
    csound->SetOption(std::string("--nchnls="+std::to_string(processor.NOutChansConnected())).c_str());
    csound->SetOption(std::string("--nchnls_i="+std::to_string(processor.NInChansConnected())).c_str());
    

    //    csdFile = "/Users/rwalsh/Library/CabbageAudio/CabbagePluginEffect/CabbagePluginEffect.csd";
    std::filesystem::path file = csdFile.empty() ? cabbage::File::getCsdFileAndPath() : csdFile;
    csdFile = file.string();
    
    bool exists = std::filesystem::exists(csdFile);
    if(exists)
    {
        csCompileResult = csound->Compile (csdFile.c_str());
        csound->Start();
        if (csdCompiledWithoutError())
        {
            csdKsmps = csound->GetKsmps();
            csSpin = csound->GetSpin();
            csScale = csound->Get0dBFS();
            setReservedChannels();
            
            std::string csoundAddress = cabbage::StringFormatter::format("Resetting csound ...\ncsound = 0x<>", csound.get());
            LOG_VERBOSE(csoundAddress);
        }
        else
        {
            //Csound could not compile your file?
            while (csound->GetMessageCnt() > 0)
            {
                LOG_INFO(csound->GetFirstMessage());
                compileErrors += csound->GetFirstMessage();
                csound->PopFirstMessage();
            }
            return false;
        }
        
        widgets.clear();
        widgets =  cabbage::Parser::parseCsdForWidgets(csdFile);
        std::vector<std::string> rangeTypes = getRangeWidgetTypes(widgets);
        for(auto& w : widgets)
        {
            if (w.contains("automatable") && w["automatable"] == 1 &&
                (!w.contains("channelType") || w["channelType"] == "number"))
            {
                const std::string widgetType = w["type"].get<std::string>();
                //check if widget has a range - range widget parameters are initialised differently to other widgets
                if (std::any_of(rangeTypes.begin(), rangeTypes.end(), [&](const std::string& type) {
                    return widgetType == type;
                }))
                {
                    try{
                        processor.GetParam(numberOfParameters)->InitDouble(w["channel"].get<std::string>().c_str(),
                                                                           w["range"]["defaultValue"].get<float>(),
                                                                           w["range"]["min"].get<float>(),
                                                                           w["range"]["max"].get<float>(),
                                                                           w["range"]["increment"].get<float>(),
                                                                           std::string(w["channel"].get<std::string>()+"Label1").c_str(),
                                                                           iplug::IParam::EFlags::kFlagsNone,
                                                                           "",
                                                                           iplug::IParam::ShapePowCurve(w["range"]["skew"].get<float>()));
                        parameterChannels.push_back({cabbage::Parser::removeQuotes(w["channel"].get<std::string>()), w["range"]["defaultValue"].get<float>()});
                        csound->SetControlChannel(w["channel"].get<std::string>().c_str(), w["range"]["defaultValue"].get<float>());
                        numberOfParameters++;
                    }
                    catch (nlohmann::json::exception& e) {
                        LOG_INFO(w.dump(4));
                        LOG_INFO(e.what());
                        cabAssert(false, "");
                    }
                }
                else
                {
                    try{
                        processor.GetParam(numberOfParameters)->InitInt(w["channel"].get<std::string>().c_str(),
                                                                        w["defaultValue"].get<int>(),
                                                                        w["min"].get<int>(),
                                                                        w["max"].get<int>(),
                                                                        std::string(w["channel"].get<std::string>()+"Label1").c_str(),
                                                                        iplug::IParam::EFlags::kFlagsNone,
                                                                        "");
                        parameterChannels.push_back({cabbage::Parser::removeQuotes(w["channel"].get<std::string>()), w["defaultValue"].get<float>()});
                        csound->SetControlChannel(w["channel"].get<std::string>().c_str(), w["defaultValue"].get<float>());
                        numberOfParameters++;
                    }
                    catch (nlohmann::json::exception& e) {
                        LOG_INFO(w.dump(4));
                        LOG_INFO(e.what());
                        cabAssert(false, "");
                        //                        cabAssert(false, "");
                    }
                }
            }
        }
        
        return true;
    }
    else
        return false;
    
}

//===========================================================================================
void Engine::setReservedChannels()
{
    auto path = cabbage::File::getCsdPath();
    //csound->SetStringChannel("CSD_PATH", (char*)path.c_str());
}

//==========================================================================================
std::vector<std::string> Engine::getRangeWidgetTypes(const std::vector<nlohmann::json> widgets)
{
    std::vector<std::string> typesWithRange;
    for (const auto& obj : widgets) {
        if (obj.contains("range") && obj["type"] != "genTable") {
            if (obj.contains("type")) {
                typesWithRange.push_back(obj["type"].get<std::string>());
            }
        }
    }
    return typesWithRange;
}
//===========================================================================================
int Engine::getNumberOfParameters(const std::string& csdFile)
{
    std::vector<nlohmann::json> widgets = cabbage::Parser::parseCsdForWidgets(csdFile.empty() ? cabbage::File::getCsdFileAndPath() : csdFile);
    int numParams = 0;
    for(auto& w : widgets)
    {
        if(w.contains("automatable") && w["automatable"] == 1)
            numParams++;
    }
    
    return numParams;
}


const std::string Engine::getIOChannalConfig(const std::string& csdFile)
{
    const int numInputs = cabbage::File::getNumberOfInputChannels(csdFile);
    const int numOutputs = cabbage::File::getNumberOfOutputChannels(csdFile);
    return std::to_string(numInputs)+"-"+std::to_string(numOutputs);
}

//===========================================================================================

void Engine::setControlChannel(const std::string channel, MYFLT value)
{
    //update Csound channel, and update ParameterChannel values..
    csound->SetControlChannel(channel.c_str(), value);
    
}

void Engine::setStringChannel(const std::string channel, std::string data)
{
    //update Csound channel
    csound->SetStringChannel(channel.c_str(), (char*)data.c_str());
}

//===========================================================================================

size_t  Engine::getIndexForParamChannel(std::string name)
{
    auto it = std::find_if(parameterChannels.begin(), parameterChannels.end(), [&name](const ParameterChannel& paramChannel) {
        return paramChannel.name == name;
    });
    
    if (it != parameterChannels.end()) {
        size_t index = std::distance(parameterChannels.begin(), it);
        return index;
    }
    
    return -1;
}
//===========================================================================================

std::optional<std::reference_wrapper<nlohmann::json>> Engine::getWidget(const std::string& channel)
{
    for (auto& w : widgets)
    {
        if (cabbage::Parser::removeQuotes(w["channel"]) == channel)
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
        auto& w = widgetOpt.value().get();
        w.merge_patch(j);
        auto result = getWidgetUpdateScript(w["channel"], w.dump());
        return result;
    }
    
    return "";
}
//===========================================================================================

std::string Engine::getWidgetUpdateScript(std::string channel, std::string data)
{
    std::string result;
    result = StringFormatter::format(R"(
        window.postMessage({
            command: "widgetUpdate",
            channel: "<>",
            data: `<>`
        });
    )",
                                     channel,
                                     data);
    return result.c_str();
}

std::string Engine::getWidgetUpdateScript(std::string channel, float value)
{
    std::string result;
    result = StringFormatter::format(R"(
        window.postMessage({
            command: "widgetUpdate",
            channel: "<>",
            value: <>
        });
    )",
                                     channel,
                                     value);
    return result.c_str();
}

void Engine::updateFunctionTable(CabbageOpcodeData data, nlohmann::json& jsonObj)
{
    if(data.cabbageJson.contains("tableNumber") || data.cabbageJson.contains("range"))
    {
        try{
            cabbage::Parser::updateJson(jsonObj, data.cabbageJson, widgets.size());
            const int tableNumber = int(jsonObj["tableNumber"]);
            const int tableSize = getCsound()->TableLength(tableNumber);
            
            if(tableSize != -1)
            {
                MYFLT *tablePtr = nullptr;
                auto length = csound->GetTable(&tablePtr, tableNumber);
                std::vector<MYFLT> temp(tablePtr, tablePtr + length);
                setTableJSON(data.channel, temp, jsonObj);
            }
        }
        catch (nlohmann::json::exception& e) {
            LOG_VERBOSE(e.what());
        }
    }
    else if(data.cabbageJson.contains("file"))
    {
        if(jsonObj["type"].get<std::string>() == "genTable")
        {
            cabbage::Parser::updateJson(jsonObj, data.cabbageJson, widgets.size());
            const int tableNumber = jsonObj["tableNumber"];
            auto samples = Engine::readAudioFile(jsonObj["file"].get<std::string>());
            
            if(samples.size() == 0)
                return;
            
            auto createTable = StringFormatter::format(R"(giTable<> ftgen <>, 0, <>, -7, 0, 0)", tableNumber, tableNumber, samples.size());
            
            getCsound()->CompileOrc(createTable.c_str());
            const int tableSize = getCsound()->TableLength(tableNumber);
            if(tableSize != -1)
            {
                MYFLT *tablePtr = nullptr;
                getCsound()->GetTable(&tablePtr, tableNumber);
                std::memcpy(tablePtr, samples.data(), tableSize * sizeof(MYFLT));
                setTableJSON(data.channel, samples, jsonObj);
            }
        }
    }
}

void Engine::setTableJSON(std::string channel, std::vector<double> samples, nlohmann::json& jsonObj)
{
    //this is a condensed version of the sample data that is passed around between C++ and JS.
    std::vector<double> widgetSampleData;
    const int startSample = jsonObj["range"]["start"].get<int>() == 0 ? 0 : jsonObj["range"]["start"].get<int>();
    const int endSample = jsonObj["range"]["end"].get<int>() == -1 ? static_cast<int>(samples.size()) : jsonObj["range"]["end"].get<int>();
    
    //no point in sending more samples that can be displayed per pixel...
    const float incr = float(endSample-startSample) / ((jsonObj["bounds"]["width"].get<float>())-1);
    
    std::string data = "samples(";
    for(float i = startSample ; i < static_cast<int>(endSample) ; i+=incr)
    {
        data += std::to_string(samples[int(i)]) + (i<endSample-incr ? "," : "");
        widgetSampleData.push_back(samples[int(i)]);
    }
    data += ")";
    
    jsonObj["samples"] = widgetSampleData;
}

const std::string Engine::getCsoundOutputUpdateScript(std::string output)
{
    StringFormatter::removeBackticks(output);
    std::string result;
    result = StringFormatter::format(R"(
         window.postMessage({ command: "csoundOutputUpdate", text: `<>` });
        )",
                                     output);
    return result.c_str();
}

//===========================================================================================

float Engine::remap(double n, double start1, double stop1, double start2, double stop2) {
    return ((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2;
}

float Engine::getFullRangeValue(std::string channel, float normalValue)
{
    for( const auto& w : getWidgets())
    {
        if(w["channel"] == channel && w.contains("range"))
        {
            return Engine::remap(normalValue, 0.f, 1.f, w["range"]["min"], w["range"]["max"]);
        }
        else
            return normalValue;
    }
    
    return normalValue;
}

//===========================================================================================
std::string Engine::removeControlCharacters(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (!iscntrl(static_cast<unsigned char>(c)) || c == ' ') {
            result += c;
        }
    }
    return result;
}

//===========================================================================================

std::vector<double> Engine::readAudioFile(const std::string &filePath)
{
    if(!cabbage::File::fileExists(filePath))
        return {};
    
    choc::audio::AudioFileFormatList formats;
    formats.addFormat<choc::audio::WAVAudioFileFormat<false>>();
    formats.addFormat<choc::audio::OggAudioFileFormat<false>>();
    formats.addFormat<choc::audio::MP3AudioFileFormat>();
    formats.addFormat<choc::audio::FLACAudioFileFormat<false>>();
    auto reader = formats.createReader (filePath);
    
    if(!reader.get())
        return {};
    
    
    auto& p = reader->getProperties();
    auto samples = reader->loadFileContent();
    auto bufferView = samples.frames.getView();
    int numFrames = bufferView.getChannel(0).getNumFrames();
    int numChannels = bufferView.getNumChannels();
    int totalSamples = numFrames * numChannels;
    
    // Create a vector of the appropriate size
    std::vector<double> audioData(totalSamples);
    
    for (int frame = 0; frame < numFrames; ++frame)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            audioData[frame * numChannels + channel] = static_cast<double>(bufferView.getSample(channel, frame));
        }
    }
    return audioData;
}

} //end of namespace



