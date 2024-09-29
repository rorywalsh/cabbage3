#include "Cabbage.h"
#include "CabbageProcessor.h"
#include "opcodes/CabbageSetOpcodes.h"
#include "opcodes/CabbageGetOpcodes.h"

Cabbage::Cabbage(CabbageProcessor& p, std::string file): processor(p), csdFile(file)
{
    
};

Cabbage::~Cabbage()
{
    if (csound)
    {
        csCompileResult = false;
        csound = nullptr;
        csoundParams = nullptr;
    }
}

void Cabbage::addOpcodes()
{
    csnd::plugin<CabbageSetValue>((csnd::Csound*)csound->GetCsound(), "cabbageSetValue", "", "Sk", csnd::thread::k);
    csnd::plugin<CabbageSetValue>((csnd::Csound*)csound->GetCsound(), "cabbageSetValue", "", "Si", csnd::thread::i);
    
    csnd::plugin<CabbageSetPerfString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "kSSW", csnd::thread::k);
    csnd::plugin<CabbageSetInitString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "SW", csnd::thread::i);
    csnd::plugin<CabbageSetPerfMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "kSSM", csnd::thread::k);
    csnd::plugin<CabbageSetInitMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "SSM", csnd::thread::i);
    
    csnd::plugin<CabbageGetValue>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "k", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValue>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "i", "S", csnd::thread::i);
    csnd::plugin<CabbageGetValueString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "S", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValueWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "kk", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValueStringWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "Sk", "So", csnd::thread::ik);
    
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "k", "SW", csnd::thread::ik);
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "i", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "S", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetStringWithTrigger>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "Sk", "SS", csnd::thread::k);

}

bool Cabbage::setupCsound()
{
    csound = std::make_unique<Csound>();
    csound->SetHostImplementedMIDIIO(true);
    csound->SetHostImplementedAudioIO(1, 0);
    csound->SetHostData(this);
    
    addOpcodes();
    
    csound->CreateMessageBuffer(0);
    csound->SetExternalMidiInOpenCallback(CabbageProcessor::OpenMidiInputDevice);
    csound->SetExternalMidiReadCallback(CabbageProcessor::ReadMidiData);
    csound->SetExternalMidiOutOpenCallback(CabbageProcessor::OpenMidiOutputDevice);
    csound->SetExternalMidiWriteCallback(CabbageProcessor::WriteMidiData);
    csoundParams = nullptr;
    csoundParams = std::make_unique<CSOUND_PARAMS> ();
    
    csoundParams->displays = 0;
    
    csound->SetOption((char*)"-n");
    csound->SetOption((char*)"-d");
    csound->SetOption((char*)"-b0");
    
    //    csoundParams->nchnls_override = numCsoundOutputChannels;
    //    csoundParams->nchnls_i_override = numCsoundInputChannels;
    
    csoundParams->sample_rate_override = 44100;
    csound->SetParams(csoundParams.get());
    //    compileCsdFile(csdFile);
    
//    csdFile = "/Users/rwalsh/Library/CabbageAudio/CabbagePluginEffect/CabbagePluginEffect.csd";
    std::filesystem::path file = csdFile.empty() ? CabbageFile::getCsdFileAndPath() : csdFile;
    csdFile = file.string();
    
    bool exists = std::filesystem::exists(csdFile);
    if(exists)
    {
        csCompileResult = csound->Compile (csdFile.c_str());
        
        if (csdCompiledWithoutError())
        {
            csdKsmps = csound->GetKsmps();
            csSpout = csound->GetSpout();
            csSpin = csound->GetSpin();
            csScale = csound->Get0dBFS();
            setReservedChannels();
        }
        else
        {
            //Csound could not compile your file?
            while (csound->GetMessageCnt() > 0)
            {
                std::string message(csound->GetFirstMessage());
                std::cout << message << std::endl;
                csound->PopFirstMessage();
            }
            return false;
        }
        
        widgets.clear();
        widgets =  CabbageParser::parseCsdForWidgets(csdFile);
        for(auto& w : widgets)
        {
            if (w.contains("automatable") && w["automatable"] == 1 &&
                (!w.contains("channelType") || w["channelType"] == "number"))
            {
//                _log(w.dump(4));
                if(w["type"].get<std::string>().find("slider") != std::string::npos)
                {
                    try{
                        processor.GetParam(numberOfParameters)->InitDouble(w["channel"].get<std::string>().c_str(),
                                                                           w["value"].get<float>(),
                                                                           w["min"].get<float>(),
                                                                           w["max"].get<float>(),
                                                                           w["increment"].get<float>(),
                                                                           std::string(w["channel"].get<std::string>()+"Label1").c_str(),
                                                                           iplug::IParam::EFlags::kFlagsNone,
                                                                           "",
                                                                           iplug::IParam::ShapePowCurve(w["skew"].get<float>()));
                        parameterChannels.push_back({CabbageParser::removeQuotes(w["channel"].get<std::string>()), w["value"].get<float>()});
                        numberOfParameters++;
                    }
                    catch (nlohmann::json::exception& e) {
                        _log(w.dump(4));
                        _log(e.what());
                        cabAssert(false, "");
                    }
                }
                else
                {
                    try{
                        processor.GetParam(numberOfParameters)->InitInt(w["channel"].get<std::string>().c_str(),
                                                                           w["value"].get<int>(),
                                                                           w["min"].get<int>(),
                                                                           w["max"].get<int>(),
                                                                           std::string(w["channel"].get<std::string>()+"Label1").c_str(),
                                                                           iplug::IParam::EFlags::kFlagsNone,
                                                                           "");
                        parameterChannels.push_back({CabbageParser::removeQuotes(w["channel"].get<std::string>()), w["value"].get<float>()});
                        csound->SetControlChannel(w["channel"].get<std::string>().c_str(), w["value"].get<float>());
                        numberOfParameters++;
                    }
                    catch (nlohmann::json::exception& e) {
                        _log(w.dump(4));
                        _log(e.what());
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
void Cabbage::setReservedChannels()
{
    auto path = CabbageFile::getCsdPath();
    csound->SetStringChannel("CSD_PATH", (char*)path.c_str());
    
}
//===========================================================================================
int Cabbage::getNumberOfParameters(const std::string& csdFile)
{
    std::vector<nlohmann::json> widgets = CabbageParser::parseCsdForWidgets(csdFile.empty() ? CabbageFile::getCsdFileAndPath() : csdFile);
    int numParams = 0;
    for(auto& w : widgets)
    {
        if(w.contains("automatable") && w["automatable"] == 1)
            numParams++;
    }
    
    return numParams;
}


void Cabbage::setControlChannel(const std::string channel, MYFLT value)
{
    //update Csound channel, and update ParameterChannel values..
    csound->SetControlChannel(channel.c_str(), value);

}

void Cabbage::setStringChannel(const std::string channel, std::string data)
{
    //update Csound channel
    csound->SetStringChannel(channel.c_str(), (char*)data.c_str());
}

//===========================================================================================

size_t  Cabbage::getIndexForParamChannel(std::string name)
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

std::optional<std::reference_wrapper<nlohmann::json>> Cabbage::getWidget(const std::string& channel)
{
    for (auto& w : widgets)
    {
        if (CabbageParser::removeQuotes(w["channel"]) == channel)
        {
            return std::ref(w); // Use std::ref to wrap the reference
        }
    }
    return std::nullopt;
}

const std::string Cabbage::updateWidgetState(nlohmann::json j)
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

std::string Cabbage::getWidgetUpdateScript(std::string channel, std::string data)
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

std::string Cabbage::getWidgetUpdateScript(std::string channel, float value)
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

void Cabbage::updateFunctionTable(CabbageOpcodeData data, nlohmann::json& jsonObj)
{
    if(data.cabbageJson.contains("tableNumber"))
    {
        CabbageParser::updateJson(jsonObj, data.cabbageJson, widgets.size());
        const int tableNumber = jsonObj["tableNumber"];
        const int tableSize = getCsound()->TableLength(tableNumber);
        if(tableSize != -1)
        {
            std::vector<double> temp (tableSize);
            getCsound()->TableCopyOut (tableNumber, &temp[0]);
            setTableJSON(data.channel, temp, jsonObj);
        }
    }
    else if(data.cabbageJson.contains("file"))
    {
        if(jsonObj["type"].get<std::string>() == "gentable")
        {
            CabbageParser::updateJson(jsonObj, data.cabbageJson, widgets.size());
            const int tableNumber = jsonObj["tableNumber"];
            auto samples = Cabbage::readAudioFile(jsonObj["file"].get<std::string>());
            
            if(samples.size() == 0)
                return;
            
            auto createTable = StringFormatter::format(R"(giTable<> ftgen <>, 0, <>, -7, 0, 0)", tableNumber, tableNumber, samples.size());

            getCsound()->CompileOrc(createTable.c_str());
            const int tableSize = getCsound()->TableLength(tableNumber);
            if(tableSize != -1)
            {
                getCsound()->TableCopyIn (tableNumber, &samples[0]);
                setTableJSON(data.channel, samples, jsonObj);
            }
        }
    }
}

void Cabbage::setTableJSON(std::string channel, std::vector<double> samples, nlohmann::json& jsonObj)
{
    
    //this is a condensed version of the sample data that is passed around between C++ and JS
    std::vector<double> widgetSampleData;
    const int startSample = jsonObj["startSample"].get<int>() != -1 ? jsonObj["startSample"].get<int>() : 0;
    const int endSample = jsonObj["endSample"].get<int>() != -1 ? jsonObj["endSample"].get<int>() : static_cast<int>(samples.size());
    
    //no point in sending more samples that can be displayed per pixel...
    const float incr = float(endSample-startSample) / (jsonObj["width"].get<float>())-1;
    
    std::string data = "samples(";
    for(float i = startSample ; i < static_cast<int>(samples.size()) ; i+=incr)
    {
        data += std::to_string(samples[int(i)]) + (i<endSample-incr ? "," : "");
        widgetSampleData.push_back(samples[int(i)]);
    }
    data += ")";
    
    jsonObj["samples"] = widgetSampleData;
}

const std::string Cabbage::getCsoundOutputUpdateScript(std::string output)
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

std::string Cabbage::removeControlCharacters(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (!iscntrl(static_cast<unsigned char>(c)) || c == ' ') {
            result += c;
        }
    }
    return result;
}

//===========================================================================================

std::vector<double> Cabbage::readAudioFile(const std::string &filePath)
{
    if(!CabbageFile::fileExists(filePath))
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




