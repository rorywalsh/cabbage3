#include "CabbageProcessor.h"
#include "IPlug_include_in_plug_src.h"



CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo& info, std::string csdFile)
: iplug::Plugin(info, iplug::MakeConfig(Cabbage::getNumberOfParameters(csdFile), 0)),
cabbage(*this, csdFile)
{
    
    if(!cabbage.setupCsound())
        assertm(false, "couldn't set up Csound");
        
        
    
    csndIndex = 0;
#ifdef DEBUG
    SetEnableDevTools(true);
#endif
    
    // Hard-coded paths must be modified!
    editorInitFunc = [&]() {
#ifdef OS_WIN
        LoadFile(R"(C:\Users\oli\Dev\iPlug2\Examples\CabbageProcessor\resources\web\index.html)", nullptr);
#else
        LoadFile("index.html", GetBundleID());
#endif
        
        EnableScroll(false);
        //setCabbage(cabbage);
    };

    
    timer.Start(this, &CabbageProcessor::timerCallback, 1);

    
}

CabbageProcessor::~CabbageProcessor()
{
    timer.Stop();
}

//timer thread listens for incoming data from Csound using a lock free fifo
void CabbageProcessor::timerCallback()
{
    //Csound could not compile your file?
    while (cabbage.getCsound()->GetMessageCnt() > 0)
    {
        std::string message(cabbage.getCsound()->GetFirstMessage());
        std::cout << message << std::endl;
        cabbage.getCsound()->PopFirstMessage();
    }
    
    auto** od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)cabbage.getCsound()->QueryGlobalVariable("cabbageOpcodeData");
    if (od != nullptr)
    {
        auto cabbageOpcodeData = *od;
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
                    if(widget["channel"] == data.channel)
                        widget["value"] = data.value;
                }
            }
            else
            {
                for(auto& widget : cabbage.getWidgets())
                {
                    std::cout << "Incoming channel:" << data.channel << "- Widget channel: " << CabbageParser::removeQuotes(widget["channel"]) << std::endl;
                    //update widget objects in case UI is closed and reopened...
                    if(data.channel == CabbageParser::removeQuotes(widget["channel"]))
                    {
                        CabbageParser::updateJsonFromSyntax(widget, data.identifier);
                    }
                }
            }
            
            hostCallback(data);
            
            //in plugins this data needs to get sent to webview, but in this case
            //it goes back to the host
            
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


void CabbageProcessor::OnParamChange(int paramIdx)
{
    if(cabbage.getNumberOfParameter() > 0)
    {
//        std::cout << "Processor Channel:" << cabbage.getParameterChannel(paramIdx) << " Value:" << GetParam(paramIdx)->Value() << std::endl;
        cabbage.setControlChannel(cabbage.getParameterChannel(paramIdx).c_str(), GetParam(paramIdx)->Value());
        GetParam(0)->SetNormalized(0.1);
    }
}


void CabbageProcessor::ProcessMidiMsg(const iplug::IMidiMsg& msg)
{
    TRACE;
    msg.PrintMsg();
    SendMidiMsg(msg);
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
    auto* midiData = static_cast<CabbageProcessor*>(userData);
    
    if (!userData)
    {
        assertm(false, "\nInvalid");
        return 0;
    }
    
    int cnt = 0;
    
    
    //    if (!midiData->midiBuffer.isEmpty() && cnt <= (nbytes - 3))
    //    {
    //        juce::MidiMessage message (0xf4, 0, 0, 0);
    //        juce::MidiBuffer::Iterator i (midiData->midiBuffer);
    //        int messageFrameRelativeTothisProcess;
    //
    //        while (i.getNextEvent (message, messageFrameRelativeTothisProcess))
    //        {
    //
    //            const juce::uint8* data = message.getRawData();
    //            *mbuf++ = *data++;
    //
    //            if(message.isChannelPressure() || message.isProgramChange())
    //            {
    //                *mbuf++ = *data++;
    //                cnt += 2;
    //            }
    //            else
    //            {
    //                *mbuf++ = *data++;
    //                *mbuf++ = *data++;
    //                cnt  += 3;
    //            }
    //        }
    //
    //        midiData->midiBuffer.clear();
    //
    //    }
    
    
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
        assertm(false, "\n\nInvalid");
        return 0;
    }
    
    //    juce::MidiMessage message (mbuf, nbytes, 0);
    //    userData->midiOutputBuffer.addEvent (message, 0);
    return nbytes;
}
