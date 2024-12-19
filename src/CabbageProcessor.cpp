/*
 * Copyright (c) 2024 Rory Walsh
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */



#include "CabbageProcessor.h"
#include "IPlug_include_in_plug_src.h"
#include "../opcodes/CabbageOpcodes.h"
//===============================================================================
// There are two different constructors here depending on whether the instrument is loaded
// in VS Code, or a plugin
//===============================================================================
#ifdef CabbageApp
CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo& info, std::string csdFile)
: iplug::Plugin(info, iplug::MakeConfig(cabbage::Engine::getNumberOfParameters(csdFile),
                                        0,
                                        cabbage::Engine::getIOChannalConfig(csdFile))),
cabbage(*this, csdFile)
{
    if(!cabbage.setupCsound())
    {
        LOG_VERBOSE(cabbage.getCompileErrors());
        return;
    }
}
#else
CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo& info)
: iplug::Plugin(info, iplug::MakeConfig(cabbage::Engine::getNumberOfParameters(""), 0, "")),
cabbage(*this, "")
{
    
    cabbage.getMidiQueue().clear();
    
    if(!cabbage.setupCsound())
    {
        LOG_INFO("Csound file could not be compiled");
        return;
//        cabAssert(false, "couldn't set up Csound");
    }
    
#ifdef DEBUG
    SetEnableDevTools(true);
#endif
    
    
    setupCallbacks();

    //timer.Start(this, &CabbageProcessor::timerCallback, 10);
    LOG_VERBOSE("Cabbage Processor constructor finished setting up\n");
}
#endif

//===============================================================================
CabbageProcessor::~CabbageProcessor()
{

}

//===============================================================================
// Various callback functions, most are triggered from the wbeview
//===============================================================================
void CabbageProcessor::setupCallbacks()
{
    //editor onInit callback function - loads inde.html and starts server when running plugin, but not when
    //working in vscode
    editorInitFuncCallback = [&]() 
    {
#ifndef CabbageApp
        if(!server.isThreadRunning())
            server.start(cabbage::File::getCsdPath());
        const std::string mntPoint = "http://127.0.0.1:" + std::to_string(server.getCurrentPort()) + "/index.html";
        LoadURL(mntPoint.c_str());
#endif
        EnableScroll(false);
    };
    
    //editor onInit callback function
    editorOnLoadCallback = [&]() 
    {
            for(auto &widget : cabbage.getWidgets())
            {
                //update widget objects in case UI is closed and reopened...
                try {
                    if(widget.contains("type") && widget["type"].get<std::string>() == "form")
                    {
                        Resize(widget["size"]["width"].get<int>(), widget["size"]["height"].get<int>());
                    }
                }
                catch (nlohmann::json::exception& e) {
                    LOG_VERBOSE(e.what());
//                    cabAssert(false, "");
                }
            }
        
    };
    
    editorDeleteFuncCallback = [&]() 
    {
        uiIsOpen = false;
    };
    
    editorCloseCallback = [&]() 
    {
        uiIsOpen = false;
    };
    
    updateStringChannelCallback = [&](std::string channel, std::string data)
    {
        cabbage.setStringChannel(channel, data);
    };
    
    cabbageIsReadyToLoadCsdCallback = [&]()
    {
        updateJSWidgets();
    };
    
    interfaceHasLoadedCallback = [&]()
    {
        uiIsOpen = true;
        allowDequeuing = true;
    };
    
    //called whenever the state of a widget is updated in the UI - the C++ widget array
    //and the JS widget array should always be in sync
    updateWidgetStateCallback = [&](nlohmann::json json)
    {
        //auto msg = cabbage.updateWidgetState(json);
        //EvaluateJavaScript(msg.c_str());
        for( auto& w : cabbage.getWidgets())
        {
            cabbage.getWidgetUpdateScript(w["channel"].get<std::string>(), w.dump());
        }
    };
    
}

//===============================================================================
// Triggered from the VS Code web panel when the instrument has loaded. The
// callback function above, called interfaceLoaded() is triggered from the
// plugin's webview
//===============================================================================
void CabbageProcessor::interfaceHasLoaded()
{
    LOG_VERBOSE("Interface has loaded.");
    uiIsOpen = true;
    allowDequeuing = true;
}


// This is called when the editor has finished loading. Or if an editor is closed and reopened
void CabbageProcessor::updateJSWidgets()
{
    //iterate over all widget objects and send to webview
    for( auto& w : cabbage.getWidgets())
    {
        if(w.contains("channel")) //only let valid object through.
        {
            auto result = cabbage.getWidgetUpdateScript(w["channel"].get<std::string>(), w.dump());
            EvaluateJavaScript(result.c_str());
            if(w.contains("value"))
            {
                if(w["value"].is_number())
                {
                    result = cabbage.getWidgetUpdateScript(w["channel"].get<std::string>(), w["value"].get<float>());
                    EvaluateJavaScript(result.c_str());
                }
            }
        }
    }
    
    uiIsOpen = true;
    allowDequeuing = true;
}

//===============================================================================
//timer thread listens for incoming data from Csound using a lock free fifo
//===============================================================================
void CabbageProcessor::pollFIFOQueue()
{

}

//===============================================================================
void CabbageProcessor::OnParamChange(int paramIdx)
{
    if(cabbage.getNumberOfParameters() > 0)
    {
        //only update if we need to...
        auto& p = cabbage.getParameterChannel(paramIdx);
        if(p.hasValueChanged(GetParam(paramIdx)->Value()))
        {
            //update parameter value..
            p.setValue(GetParam(paramIdx)->Value());
            //update channel..
            cabbage.setControlChannel(p.name.c_str(), GetParam(paramIdx)->Value());
//            LOG_VERBOSE("OnParameter:" , p.name.c_str(), ":",  GetParam(paramIdx)->Value());
            for( auto& w : cabbage.getWidgets())
            {
                if(w.contains("channel") && w["channel"] == p.name.c_str()) //only let valid object through.
                {
                    if(w.contains("value"))
                    {
                        w["value"] = GetParam(paramIdx)->Value();
                    }
                }
            }
        }
    }
}

//===============================================================================
void CabbageProcessor::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{   
    // if no audio input device is found in the standalone wrapper
    // inputs will be null 
    if (*inputs == nullptr)
        hasValidInputs = false;

    //must be careful here that we sum our newly processed signal with the current input.
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

                // if valid inputs are detected, sum newly processed
                // signal with incoming one
                if(hasValidInputs)
                {
                    cabbage.setSpIn(channel + pos, inputs[channel][i]);
                    //outputs[channel][i] = inputs[channel][i] + cabbage.getSpOut(channel + pos);
                    outputs[channel][i] = cabbage.getSpOut(channel + pos);
                }
                else
                {
                    outputs[channel][i] = cabbage.getSpOut(channel + pos);
                }
            }
        }
    }
    else
    {
        // calling this once here in case errors are missed in vscode logger
        cabbage.displayAndClearCompileErrors();
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
// called on main thread, invoked during periods when the system isn't busy processing other tasks
//===============================================================================
void CabbageProcessor::OnIdle()
{
#ifndef CabbageApp
    if (uiIsOpen)
    {
#endif
        while (cabbage.getCsound()->GetMessageCnt() > 0)
        {
            std::string message(cabbage.getCsound()->GetFirstMessage());
            message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
            LOG_INFO(message);
            //EvaluateJavaScript(cabbage.getCsoundOutputUpdateScript(message).c_str());
            cabbage.getCsound()->PopFirstMessage();
        }
#ifndef CabbageApp
    }
#endif
    CabbageOpcodeData data;
    //data contains the channel and the Cabbage code that can be comprised of any number of identifiers, i.e,
	    // bounds(10, 10, 100, 100), text("hello"), etc.

    //only start accessing messages from the queue when the interface is open..
    if (allowDequeuing)
    {
        while (cabbage.opcodeData.try_dequeue(data))
        {
            for (auto& widget : cabbage.getWidgets())
            {
                if (data.channel == cabbage::Parser::removeQuotes(widget["channel"]))
                {
                    //this will update the widget JSON with new arguments tied to the identifier, e.g, bounds(x, y, w, h)
                    cabbage::Parser::updateJson(widget, data.cabbageJson, widget.size());
                }
            }


#ifdef CabbageApp
            //send data to vscode extension..
            hostCallback(data);
#else
            while (cabbage.getCsound()->GetMessageCnt() > 0)
            {
                std::string message(cabbage.getCsound()->GetFirstMessage());
                LOG_INFO(message);
               // EvaluateJavaScript(cabbage.getCsoundOutputUpdateScript(message).c_str());
                cabbage.getCsound()->PopFirstMessage();
            }

            std::string message = {};
            if (data.type == CabbageOpcodeData::MessageType::Value)
            {
                message = cabbage.getWidgetUpdateScript(data.channel, data.cabbageJson["value"].get<float>());
                EvaluateJavaScript(message.c_str());
            }
            else
            {

                auto widgetOpt = cabbage.getWidget(data.channel);
                if (widgetOpt.has_value())
                {
                    auto& j = widgetOpt.value().get();
                    // one of the special cases where we need to check the widget type
                    if (j["type"].get<std::string>() == "genTable")
                    {
                        cabbage.updateFunctionTable(data, j);
                        message = cabbage.getWidgetUpdateScript(data.channel, j.dump());
                    }
                    else
                    {
                        cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
                        message = cabbage.getWidgetUpdateScript(data.channel, j.dump());
                    }
                }
                //LOG_INFO(message);
                EvaluateJavaScript(message.c_str());
            }
#endif
        }
    }
}

//===============================================================================
void CabbageProcessor::ProcessMidiMsg(const iplug::IMidiMsg& msg)
{
    msg.PrintMsg();
    SendMidiMsg(msg);
    cabbage.getMidiQueue().push_back(msg);
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
    auto* pluginData = static_cast<cabbage::Engine*>(userData);
    
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
