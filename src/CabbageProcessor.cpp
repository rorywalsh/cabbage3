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
// There are two different constructors here depending on whether the instrument
// is loaded in VS Code, or a plugin
//===============================================================================
#ifdef CabbageApp
CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo &info, std::string csdFile)
    : iplug::Plugin(info, iplug::MakeConfig(cabbage::Engine::getNumberOfParameters(csdFile), 0,
                                            cabbage::Engine::getIOChannalConfig(csdFile))),
      cabbage(*this, csdFile)
{

    if (!cabbage.setupCsound())
    {
        cabbage::logDebug << cabbage.getCompileErrors();
        return;
    }

    matchingNumInputsOutputs = (NInChansConnected() == NOutChansConnected());

#if (defined(LINUX) || defined(OS_MAC)) && defined(CabbageApp)
    std::thread idleThread(&CabbageProcessor::startIdleTimer, this, 100);
    idleThread.detach();
#endif
}

#else
CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo &info)
    : iplug::Plugin(info, iplug::MakeConfig(cabbage::Engine::getNumberOfParameters(""), 0,
                                            cabbage::Engine::getIOChannalConfig(""))),
      cabbage(*this, "")
#if defined(LINUX)
      ,
      instanceMap(cabbage::SharedMemoryQueue::CreateDefaultInstanceTracker()),
      memoryQueue("/cabbage_" + instanceMap.getInstanceId(), 100, 1024)
#endif
{

    cabbage.getMidiQueue().clear();
    cabbage.setCsdFile(cabbage::File::getCsdFileAndPath());
    cabbage::Logger::getInstance().setLogFile(cabbage::File::withExtension(cabbage.getCsdFile(), ".log"));

    if (!cabbage.setupCsound())
    {
        cabbage::logInfo << "Csound file could not be compiled";
        return;
    }

    matchingNumInputsOutputs = (NInChansConnected() == NOutChansConnected());

#ifdef DEBUG
    if (cabbage::Utils::getEnableDevTools(cabbage.getCsdFile()))
        SetEnableDevTools(true);
#endif

#if defined(LINUX)
        // Create a unique pipe to handle incoming messages from UI - this pipe has
        // to have the same base name as the one setup from the webview class

#endif

    setupCallbacks();
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
    // editor onInit callback function - loads index.html and starts server when running plugin, but not when
    // working in vscode
    editorInitFuncCallback = [&]()
    {
#ifndef CabbageApp
        if (!server.isThreadRunning())
            server.start(cabbage::File::getCsdPath(cabbage.getCsdFile()));
        const std::string mntPoint = "http://127.0.0.1:" + std::to_string(server.getCurrentPort()) + "/index.html";
        LoadURL(mntPoint.c_str());
        EnableScroll(false);
#endif
    };

    // editor onInit callback function
    editorOnLoadCallback = [&]()
    {
        uiIsOpen = true;
        for (auto &widget : cabbage.getWidgets())
        {
            // update widget objects in case UI is closed and reopened...
            try
            {
                if (widget.contains("type") && widget["type"].get<std::string>() == "form")
                {
                    Resize(widget["size"]["width"].get<int>(), widget["size"]["height"].get<int>());
                }
            }
            catch (nlohmann::json::exception &e)
            {
                cabbage::logDebug << e.what();
            }
        }
    };

    editorDeleteFuncCallback = [&]()
    {
        uiIsOpen = false;
#if defined(LINUX)

#endif
    };

    editorCloseCallback = [&]()
    {
        uiIsOpen = false;
#if defined(LINUX)

#endif
    };

    updateStringChannelCallback = [&](std::string channel, std::string data)
    { cabbage.setStringChannel(channel, data); };

    cabbageIsReadyToLoadCsdCallback = [&]() { updateJSWidgets(); };

    interfaceHasLoadedCallback = [&]()
    {
        uiIsOpen = true;
        allowDequeuing = true;
    };

    // called whenever the state of a widget is updated in the UI - the C++ widget array
    // and the JS widget array should always be in sync
    updateWidgetStateCallback = [&](nlohmann::json json)
    {
        // auto msg = cabbage.updateWidgetState(json);
        // EvaluateJavaScript(msg.c_str());
        for (auto &w : cabbage.getWidgets())
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
    cabbage::logDebug << "Interface has loaded.";
    uiIsOpen = true;
    allowDequeuing = true;
}

// This is called when the editor has finished loading. Or if an editor is closed and reopened
void CabbageProcessor::updateJSWidgets()
{
    // iterate over all widget objects and send to webview
    for (auto &w : cabbage.getWidgets())
    {
        if (w.contains("channel")) // only let valid object through.
        {
            auto result = cabbage.getWidgetUpdateScript(w["channel"].get<std::string>(), w.dump());
            EvaluateJavaScript(result.c_str());
            if (w.contains("value"))
            {
                if (w["value"].is_number())
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
// this can be called on the audio thread..
void CabbageProcessor::OnParamChange(int paramIdx)
{
    if (cabbage.getNumberOfParameters() > 0)
    {
        // only update if we need to...
        auto &p = cabbage.getParameterChannel(paramIdx);
        if (p.hasValueChanged(GetParam(paramIdx)->Value()))
        {
            // update parameter value..
            p.setValue(GetParam(paramIdx)->Value());

            // update channel..
            cabbage.setControlChannel(p.name.c_str(), GetParam(paramIdx)->Value());

            for (auto &w : cabbage.getWidgets())
            {
                if (w.contains("channel") && w["channel"] == p.name.c_str()) // only let valid object through.
                {
                    if (w.contains("value"))
                    {
                        w["value"] = GetParam(paramIdx)->Value();
                    }
                }
            }
        }
    }
}

// this is always called on low-priority thread
void CabbageProcessor::OnParamChangeUI(int paramIdx, iplug::EParamSource source)
{
    if (cabbage.getNumberOfParameters() > 0)
    {
        // only update if we need to...
        auto &p = cabbage.getParameterChannel(paramIdx);
        for (auto &w : cabbage.getWidgets())
        {
            if (w.contains("channel") && w["channel"] == p.name.c_str()) // only let valid object through.
            {
                if (w.contains("value"))
                {
                    const std::string script =
                        cabbage.getWidgetUpdateScript(w["channel"].get<std::string>(), GetParam(paramIdx)->Value());
                    EvaluateJavaScript(script.c_str());
                }
            }
        }
    }
}

//===============================================================================
void CabbageProcessor::ProcessBlock(iplug::sample **inputs, iplug::sample **outputs, int nFrames)
{
    // one process audio if Csound has compiled successfully.
    if (cabbage.csdCompiledWithoutError())
    {
        for (int i = 0; i < nFrames; i++, ++csndIndex)
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
                for (int channel = 0; channel < NOutChansConnected(); channel++)
                {
                    pos = csndIndex * NOutChansConnected();
                    cabbage.setSpIn(channel + pos, inputs[channel][i]);
                    // outputs[channel][i] = inputs[channel][i] + cabbage.getSpOut(channel + pos);
                    outputs[channel][i] = cabbage.getSpOut(channel + pos);
                }
            }
            else
            {
                // Process inputs first
                for (int inputChannel = 0; inputChannel < NInChansConnected(); inputChannel++)
                {
                    pos = csndIndex * NInChansConnected(); // Position in interleaved array
                    cabbage.setSpIn(inputChannel + pos, inputs[inputChannel][i]);
                }

                // Process outputs
                for (int outputChannel = 0; outputChannel < NOutChansConnected(); outputChannel++)
                {
                    pos = csndIndex * NOutChansConnected(); // Position in interleaved array
                    // Fill output buffer from Csound's processed output
                    outputs[outputChannel][i] = cabbage.getSpOut(outputChannel + pos);
                }
            }
        }
    }
    else
    {
        // calling this once here in case errors are missed in vscode logger
        cabbage.displayAndClearCompileErrors();

        // zero outputs so we don't get unwanted signal when csound fails
        for (int i = 0; i < nFrames; i++)
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
bool CabbageProcessor::OnMessage(int msgTag, int ctrlTag, int dataSize, const void *pData)
{
    return false;
}

void CabbageProcessor::OnIdle()
{
#ifndef CabbageApp
    if (uiIsOpen)
    {
#endif
        cabbage.processCsoundMessages();

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
            for (auto &widget : cabbage.getWidgets())
            {
                if (data.channel == cabbage::Parser::removeQuotes(widget["channel"]))
                {
                    cabbage::Parser::updateJson(widget, data.cabbageJson, widget.size());
                }
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

//=============================================================================
void CabbageProcessor::updateWidgetData(const CabbageOpcodeData &data)
{
    std::string message;

    if (data.type == CabbageOpcodeData::MessageType::Value)
    {
        message = cabbage.getWidgetUpdateScript(data.channel, data.cabbageJson["value"].get<float>());
    }
    else
    {
        auto widgetOpt = cabbage.getWidget(data.channel);
        if (widgetOpt.has_value())
        {
            auto &j = widgetOpt.value().get();
            if (j["type"].get<std::string>() == "genTable")
            {
                cabbage.updateFunctionTable(data, j);
            }
            cabbage::Parser::updateJson(j, data.cabbageJson, cabbage.getWidgets().size());
            message = cabbage.getWidgetUpdateScript(data.channel, j.dump());
        }
    }

    if (!message.empty())
        EvaluateJavaScript(message.c_str());
}

//===============================================================================
bool CabbageProcessor::SerializeState(iplug::IByteChunk &chunk) const
{
    return SerializeParams(chunk); // must remember to call SerializeParams at the end
}

int CabbageProcessor::UnserializeState(const iplug::IByteChunk &chunk, int startPos)
{
    return UnserializeParams(chunk, startPos);
}

//===============================================================================
void CabbageProcessor::ProcessMidiMsg(const iplug::IMidiMsg &msg)
{
    //cabbage::logDebug << "Channel: " << msg.Channel() << " NoteNumber: " << msg.NoteNumber();
    SendMidiMsg(msg);
    cabbage.getMidiQueue().push_back(msg);
}

//======================== CSOUND MIDI FUNCTIONS ================================
int CabbageProcessor::OpenMidiInputDevice(CSOUND *csound, void **userData, const char * /*devName*/)
{
    *userData = csoundGetHostData(csound);
    return 0;
}

//==============================================================================
// Reads MIDI input data from host, gets called every time there is MIDI input to our plugin
//==============================================================================
int CabbageProcessor::ReadMidiData(CSOUND * /*csound*/, void *userData, unsigned char *mbuf, int nbytes)
{
    auto *pluginData = static_cast<cabbage::Engine *>(userData);

    if (!userData)
    {
        cabAssert(false, "\nInvalid");
        return 0;
    }

    int cnt = 0;

    if (pluginData->getMidiQueue().size() > 0)
    {
        for (const auto msg : pluginData->getMidiQueue())
        {
            if (msg.StatusMsg() != iplug::IMidiMsg::kProgramChange)
            {
                *mbuf++ = msg.mStatus;
                *mbuf++ = msg.mData1;
                *mbuf++ = msg.mData2;
                cnt += 3;
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
int CabbageProcessor::OpenMidiOutputDevice(CSOUND *csound, void **userData, const char * /*devName*/)
{
    *userData = csoundGetHostData(csound);
    return 0;
}

//==============================================================================
// Write MIDI data to plugin's MIDI output. Each time Csound outputs a midi message this
// method should be called. Note: you must have -Q set in your CsOptions
//==============================================================================
int CabbageProcessor::WriteMidiData(CSOUND * /*csound*/, void *_userData, const unsigned char *mbuf, int nbytes)
{
    auto *userData = static_cast<CabbageProcessor *>(_userData);

    if (!userData)
    {
        cabAssert(false, "\n\nInvalid");
        return 0;
    }

    //    juce::MidiMessage message (mbuf, nbytes, 0);
    //    userData->midiOutputBuffer.addEvent (message, 0);
    return nbytes;
}
