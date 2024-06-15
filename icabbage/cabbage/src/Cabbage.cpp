#include "Cabbage.h"
#include "CabbageProcessor.h"

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

int Cabbage::getNumberOfParameters(const std::string& csdFile)
{
    std::vector<nlohmann::json> widgets = CabbageParser::parseCsdForWidgets(csdFile.empty() ? CabbageFile::getCsdPath() : csdFile);
    int numParams = 0;
    for(auto& w : widgets)
    {
        if(w.contains("automatable") && w["automatable"] == 1)
            numParams++;
    }
    
    return numParams;
}


void Cabbage::addOpcodes()
{
    csnd::plugin<CabbageSetValue>((csnd::Csound*)csound->GetCsound(), "cabbageSetValue", "", "Sk", csnd::thread::k);
    csnd::plugin<CabbageSetValue>((csnd::Csound*)csound->GetCsound(), "cabbageSetValue", "", "Si", csnd::thread::i);
    csnd::plugin<CabbageSet>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "kSS", csnd::thread::ik);
    csnd::plugin<CabbageSet>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "SS", csnd::thread::i);
    csnd::plugin<CabbageSet>((csnd::Csound*) getCsound()->GetCsound(), "cabbageSet", "", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetValue>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "k", "S", csnd::thread::ik);
    csnd::plugin<CabbageGetValue>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGetValue", "i", "S", csnd::thread::i);
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "k", "SS", csnd::thread::ik);
    csnd::plugin<CabbageGetMYFLT>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "i", "SS", csnd::thread::i);
    csnd::plugin<CabbageGetString>((csnd::Csound*) getCsound()->GetCsound(), "cabbageGet", "S", "SS", csnd::thread::i);
    csnd::plugin<CabbageCompare>((csnd::Csound*) getCsound()->GetCsound(), "cabbageCompare", "i", "SS", csnd::thread::i);
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
    
    //csdFile = "/Users/rwalsh/Library/CabbageAudio/CabbagePluginEffect/CabbagePluginEffect.csd";
    std::filesystem::path file = csdFile.empty() ? CabbageFile::getCsdPath() : csdFile;
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
        }
        else
        {
            //Csound could not compile your file?
            while (csound->GetMessageCnt() > 0)
            {
                std::string message(csound->GetFirstMessage());
                std::cout << message << std::endl;
//                csound->PopFirstMessage();
            }
            return false;
        }
        

        widgets =  CabbageParser::parseCsdForWidgets(csdFile);
        for(auto& w : widgets)
        {
            if(w.contains("automatable") && w["automatable"] == 1)
            {
                if(w["type"] == "rslider")
                {
                    try{
                        processor.GetParam(numberOfParameters)->InitDouble(w["channel"].get<std::string>().c_str(),
                                                                           w["defaultValue"].get<float>(),
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
                        std::cout << w.dump(4) << std::endl << e.what();
                        cabAssert(false, "");
                    }
                }
            }
        }
               
        auto** wd = reinterpret_cast<std::vector<nlohmann::json>**>(getCsound()->QueryGlobalVariable("cabbageWidgetData"));
        if (wd == nullptr) {
            getCsound()->CreateGlobalVariable("cabbageWidgetData", sizeof(std::vector<nlohmann::json>*));
            wd = (std::vector<nlohmann::json>**)getCsound()->QueryGlobalVariable("cabbageWidgetData");
            *wd = &widgets;
        }

                
        return true;
    }
    else
        return false;
    
}

void Cabbage::setControlChannel(const std::string channel, MYFLT value)
{
    //update Csound channel, and update ParameterChannel values..
    csound->SetControlChannel(channel.c_str(), value);
    
    auto index = getIndexForParamChannel(channel);
    if(index != -1)
    {
        if(widgets[getIndexForParamChannel(channel)]["type"].contains("slider"))
            getParameterChannel(static_cast<int>(index)).setValue(value);
    }

}

void Cabbage::setStringChannel(const std::string channel, std::string data)
{
    //update Csound channel
    csound->SetStringChannel(channel.c_str(), (char*)data.c_str());
}

