#include "CabbageProcessor.h"
#include "IPlug_include_in_plug_src.h"


CabbageProcessor::CabbageProcessor(const iplug::InstanceInfo& info, std::string csdFile)
: iplug::Plugin(info, iplug::MakeConfig(Cabbage::getNumberOfParameters(csdFile), 0)),
cabbage(*this, csdFile)
{
    cabbage.setupCsound();
    
    csndIndex = 0;
#ifdef DEBUG
    SetEnableDevTools(true);
#endif
    
    // Hard-coded paths must be modified!
    mEditorInitFunc = [&]() {
#ifdef OS_WIN
        LoadFile(R"(C:\Users\oli\Dev\iPlug2\Examples\CabbageProcessor\resources\web\index.html)", nullptr);
#else
        LoadFile("index.html", GetBundleID());
#endif
        
        EnableScroll(false);
    };
    
    
}

CabbageProcessor::~CabbageProcessor()
{
    
}

//===============================================================================

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
                //std::cout << channel+pos << std::endl;
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

//======================== CSOUND MIDI FUNCTIONS ================================
void CabbageProcessor::OnParamChange(int paramIdx)
{
    if(cabbage.getNumberOfParameter() > 0)
    {
        std::cout << "Processor Channel:" << cabbage.getParameterChannel(paramIdx) << " Value:" << GetParam(paramIdx)->Value() << std::endl;
        cabbage.setControlChannel(cabbage.getParameterChannel(paramIdx).c_str(), GetParam(paramIdx)->Value());
    }
}

//======================== CSOUND MIDI FUNCTIONS ================================
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