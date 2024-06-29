#include "CabbageProcessor.h"
#include "IPlug_include_in_plug_src.h"

//Users/rwalsh/Library/CloudStorage/OneDrive-Personal/Csoundfiles/addy.csd;
#ifdef CabbageApp
CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo& info, std::string csdFile)
: iplug::Plugin(info, iplug::MakeConfig(Cabbage::getNumberOfParameters(csdFile), 0)),
cabbage(*this, csdFile)
{
    
    if(!cabbage.setupCsound())
        return;
    
    timer.Start(this, &CabbageProcessor::timerCallback, 1);
}
#else
CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo& info)
: iplug::Plugin(info, iplug::MakeConfig(Cabbage::getNumberOfParameters(""), 0)),
cabbage(*this, "")
{
    
    cabbage.getMidiQueue().clear();
    
    if(!cabbage.setupCsound())
        cabAssert(false, "couldn't set up Csound");
    
#ifdef DEBUG
    SetEnableDevTools(true);
#endif
    
    
    setupCallbacks();
    timer.Start(this, &CabbageProcessor::timerCallback, 10);
    
}
#endif


CabbageProcessor::~CabbageProcessor()
{
    timer.Stop();
}

void CabbageProcessor::setupCallbacks()
{
    //editor onInit callback function
    editorInitFuncCallback = [&]() {
#ifdef OS_WIN
        LoadFile(R"(C:\Users\oli\Dev\iPlug2\Examples\CabbageProcessor\resources\web\index.html)", nullptr);
#else
#ifndef CabbageApp
        if(!server.isThreadRunning())
            server.start("/Users/rwalsh/Library/CabbageAudio/CabbagePluginEffect/");
        const std::string mntPoint = "http://127.0.0.1:" + std::to_string(server.getCurrentPort()) + "/index.html";
        LoadURL(mntPoint.c_str());
#endif
#endif
        EnableScroll(false);
    };
    
    //editor onInit callback function
    editorOnLoadCallback = [&]() {
        auto csdText = CabbageFile::getCabbageSection();
        std::string result = StringFormatter::format(R"(
        window.addEventListener('message', event => {
            const message = event.data;
            if(message.command === "main.js ready"){
                console.log("DOM has loaded - sending Cabbage section to JS..");
                window.postMessage({ command: "onFileChanged", text: "<>" });
            }
        });
        
        )", csdText);
        
        EvaluateJavaScript(result.c_str());
        updateWidgetsOnUIOpen();
        uiIsOpen = true;
    };
    
    editorDeleteFuncCallback = [&]() {
        uiIsOpen = false;
    };
    
    editorCloseCallback = [&]() {
        uiIsOpen = false;
    };
    
    updateStringChannelCallback = [&](std::string channel, std::string data){
        cabbage.setStringChannel(channel, data);
    };
    
    updateWidgetState = [&](nlohmann::json json){
        auto j = json;
        auto const channel = j["channel"].get<std::string>();
        auto widgetOpt = cabbage.getWidget(channel);
        if (widgetOpt.has_value())
        {
            auto& w = widgetOpt.value().get();
            w.merge_patch(j);
            auto result = cabbage.getWidgetUpdateScript(w["channel"], w.dump());
            EvaluateJavaScript(result.c_str());
        }
    };
    
}

//this method gets called whenever the UI first opens. Although messages have been
void CabbageProcessor::updateWidgetsOnUIOpen()
{
    //have to load the Cabbage interface each time the UI is opened

   // if(!firstTime)
   // {
        //now update all JS widgets with the most up to date properties
        for(auto& widget : cabbage.getWidgets())
        {
            std::string result = StringFormatter::format(R"(
            window.addEventListener('message', event => {
                const message = event.data;
                if(message.command === "Cabbage file has been parsed"){
                <>
                }
            });
            )", cabbage.getWidgetUpdateScript(widget["channel"], widget.dump()));
            std::cout << "-------------+++++++++----------------\n" << result << std::endl;
            EvaluateJavaScript(result.c_str());
        }
   // }
//
//    //now we have to iterate over all the existing objects and update their state..
//    const std::string updateWidgets = R"(
//    window.postMessage({ command: "uiOpened", text: "opening UI" });
//    )";
//    EvaluateJavaScript(updateWidgets.c_str());
    
    while (cabbage.getCsound()->GetMessageCnt() > 0)
    {
        std::string message(cabbage.getCsound()->GetFirstMessage());
        std::cout << message << std::endl;
        EvaluateJavaScript(cabbage.getCsoundOutputUpdateScript(message).c_str());
        cabbage.getCsound()->PopFirstMessage();
    }
    
    firstTime = false;
}

//timer thread listens for incoming data from Csound using a lock free fifo
void CabbageProcessor::timerCallback()
{
#ifndef CabbageApp
    if(uiIsOpen)
    {
#endif
        while (cabbage.getCsound()->GetMessageCnt() > 0)
        {
            std::string message(cabbage.getCsound()->GetFirstMessage());
            std::cout << message << std::endl;
            EvaluateJavaScript(cabbage.getCsoundOutputUpdateScript(message).c_str());
            cabbage.getCsound()->PopFirstMessage();
        }
#ifndef CabbageApp
    }
#endif
    //any of these messages from the Cabbage opcodes can be sent when the UI is not open, or before it opens when first loaded..
    auto** od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)cabbage.getCsound()->QueryGlobalVariable("cabbageOpcodeData");
    if (od != nullptr)
    {
        auto cabbageOpcodeData = *od;
        //data contains the channel and the identifier string including args, i.e, bounds(10, 10, 100, 100)
        CabbageOpcodeData data;
        while (cabbageOpcodeData->try_dequeue(data))
        {
            if(data.type == CabbageOpcodeData::MessageType::Value)
            {
                //if incoming data is from a value opcode update Csound...
                cabbage.setControlChannel(data.channel, data.value);
                for(auto &widget : cabbage.getWidgets())
                {
                    //update widget objects in case UI is closed and reopened...
                    if(CabbageParser::removeQuotes(widget["channel"]) == data.channel)
                    {
                        widget["value"] = data.value;
                    }
                }
            }
            else
            {
                for(auto& widget : cabbage.getWidgets())
                {
                    if(data.channel == CabbageParser::removeQuotes(widget["channel"]))
                    {
                        //this will update the widget JSON with new arguments tied to the identifier, e.g, bounds(x, y, w, h)
                        CabbageParser::updateJsonFromSyntax(widget, data.identifierText);
                    }
                }
            }
            
#ifdef CabbageApp
            //send data to vscode extension..
            hostCallback(data);
#else
            while (cabbage.getCsound()->GetMessageCnt() > 0)
            {
                std::string message(cabbage.getCsound()->GetFirstMessage());
                std::cout << message << std::endl;
                EvaluateJavaScript(cabbage.getCsoundOutputUpdateScript(message).c_str());
                cabbage.getCsound()->PopFirstMessage();
            }
            
            std::string message = {};
            if(data.type == CabbageOpcodeData::MessageType::Value)
            {
                message =  cabbage.getWidgetUpdateScript(data.channel, data.value);
            }
            else
            {
                
                auto widgetOpt = cabbage.getWidget(data.channel);
                if (widgetOpt.has_value())
                {
                    auto& j = widgetOpt.value().get();
                    if(j["type"].get<std::string>() == "gentable")
                    {
                        cabbage.updateFunctionTable(data, j);
                        message = cabbage.getWidgetUpdateScript(data.channel, j.dump());
                    }
                    else
                        message = cabbage.getWidgetUpdateScript(data.channel, data.identifierText);
                }
            }

            EvaluateJavaScript(message.c_str());

            
#endif
        }
    }
}

void CabbageProcessor::OnParamChange(int paramIdx)
{
    if(cabbage.getNumberOfParameter() > 0)
    {
        //only update if we need to...
        auto& p = cabbage.getParameterChannel(paramIdx);
        if(p.hasValueChanged(GetParam(paramIdx)->Value()))
        {
            p.setValue(GetParam(paramIdx)->Value());
            cabbage.setControlChannel(p.name.c_str(), GetParam(paramIdx)->Value());
            EvaluateJavaScript(cabbage.getWidgetUpdateScript(p.name, GetParam(paramIdx)->Value()).c_str());
        }
    }
}

//===============================================================================
void CabbageProcessor::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{
    //const double gain = GetParam(kGain)->DBToAmp();
    
    
    if (cabbage.csdCompiledWithoutError())
    {
        for(int i = 0; i < nFrames ; i++, ++csndIndex)
        {
            if (csndIndex >= cabbage.getKsmps())
            {
                cabbage.performKsmps();
                csndIndex = 0;
            }
            
            for (int channel = 0; channel < NOutChansConnected(); channel++)
            {
                pos = csndIndex*NOutChansConnected();
                cabbage.setSpIn(channel+pos, inputs[channel][i]);
                outputs[channel][i] = cabbage.getSpOut(channel+pos);
            }
        }
    }
    else
    {
        for(int i = 0; i < nFrames ; i++)
            for (int channel = 0; channel < NOutChansConnected(); channel++)
                outputs[channel][i] = 0;
    }
    
    
    
}

//===============================================================================
void CabbageProcessor::OnReset()
{
    auto sr = GetSampleRate();
}

//===============================================================================
bool CabbageProcessor::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
    return false;
}

//===============================================================================
void CabbageProcessor::OnIdle()
{
    
}

void CabbageProcessor::ProcessMidiMsg(const iplug::IMidiMsg& msg)
{
    msg.PrintMsg();
    SendMidiMsg(msg);
    cabbage.getMidiQueue().push_back(msg);
    /* */SendMidiMsgFromDelegate(msg);
}

//======================== CSOUND MIDI FUNCTIONS ================================
int CabbageProcessor::OpenMidiInputDevice (CSOUND* csound, void** userData, const char* /*devName*/)
{
    *userData = csoundGetHostData (csound);
    return 0;
}

//==============================================================================
// Reads MIDI input data from host, gets called every time there is MIDI input to our plugin
//==============================================================================
int CabbageProcessor::ReadMidiData (CSOUND* /*csound*/, void* userData,
                                    unsigned char* mbuf, int nbytes)
{
    auto* pluginData = static_cast<Cabbage*>(userData);
    
    if (!userData)
    {
        cabAssert(false, "\nInvalid");
        return 0;
    }
    
    int cnt = 0;
    
    if(pluginData->getMidiQueue().size()>0)
    {
        for(const auto msg : pluginData->getMidiQueue())
        {
            if(msg.StatusMsg() != iplug::IMidiMsg::kProgramChange)
            {
                *mbuf++ = msg.mStatus;
                *mbuf++ = msg.mData1;
                *mbuf++ = msg.mData2;
                cnt+=3;
            }
        }
        pluginData->getMidiQueue().clear();
    }
    
    return cnt;
    
}

//==============================================================================
// Opens MIDI output device, adding -QN to your CsOptions will causes this method to be called
// as soon as your plugin loads
//==============================================================================
int CabbageProcessor::OpenMidiOutputDevice (CSOUND* csound, void** userData, const char* /*devName*/)
{
    *userData = csoundGetHostData (csound);
    return 0;
}

//==============================================================================
// Write MIDI data to plugin's MIDI output. Each time Csound outputs a midi message this
// method should be called. Note: you must have -Q set in your CsOptions
//==============================================================================
int CabbageProcessor::WriteMidiData (CSOUND* /*csound*/, void* _userData,
                                     const unsigned char* mbuf, int nbytes)
{
    auto* userData = static_cast<CabbageProcessor*>(_userData);
    
    if (!userData)
    {
        cabAssert(false, "\n\nInvalid");
        return 0;
    }
    
    //    juce::MidiMessage message (mbuf, nbytes, 0);
    //    userData->midiOutputBuffer.addEvent (message, 0);
    return nbytes;
}
