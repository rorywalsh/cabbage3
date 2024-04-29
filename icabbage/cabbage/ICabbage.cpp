#include "ICabbage.h"
#include "IPlug_include_in_plug_src.h"


ICabbage::ICabbage(const iplug::InstanceInfo& info, std::string csdFile)
: iplug::Plugin(info, iplug::MakeConfig(static_cast<int>(parseCsd(csdFile).size()), 0))
{
    
#ifdef DEBUG
    SetEnableDevTools(true);
#endif
    
    // Hard-coded paths must be modified!
    mEditorInitFunc = [&]() {
#ifdef OS_WIN
        LoadFile(R"(C:\Users\oli\Dev\iPlug2\Examples\ICabbage\resources\web\index.html)", nullptr);
#else
        LoadFile("index.html", GetBundleID());
#endif
        
        EnableScroll(false);
    };
    
    
}

ICabbage::~ICabbage()
{
    
    if (csound)
    {
        csCompileResult = false;
        csound = nullptr;
        csoundParams = nullptr;
    }
}

//===============================================================================
bool ICabbage::setupAndStartCsound()
{
    csound = std::make_unique<Csound>();
    csound->SetHostImplementedMIDIIO(true);
    csound->SetHostImplementedAudioIO(1, 0);
    csound->SetHostData(this);
    
    csound->CreateMessageBuffer(0);
    csound->SetExternalMidiInOpenCallback(OpenMidiInputDevice);
    csound->SetExternalMidiReadCallback(ReadMidiData);
    csound->SetExternalMidiOutOpenCallback(OpenMidiOutputDevice);
    csound->SetExternalMidiWriteCallback(WriteMidiData);
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
    
    //csdFile = "/Users/rwalsh/Library/CloudStorage/OneDrive-Personal/Csoundfiles/addy.csd";
    std::filesystem::path file = csdFile;
    
    bool exists = std::filesystem::is_directory(file.parent_path());
    if(exists)
    {
        csCompileResult = csound->Compile (csdFile.c_str());
        
        if (csdCompiledWithoutError())
        {
            csdKsmps = csound->GetKsmps();
            csSpout = csound->GetSpout();
            csSpin = csound->GetSpin();
            csScale = csound->Get0dBFS();
            csndIndex = csound->GetKsmps();
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
        

        cabbageParameters = parseCsd(csdFile);
        int paramCnt = 0;
        for(auto& p : cabbageParameters)
        {
            this->GetParam(paramCnt)->InitDouble(p.channel.c_str(), p.range.value, p.range.min, p.range.max, p.range.increment,
                                                 std::string(p.channel+"Label1").c_str(), iplug::IParam::EFlags::kFlagsNone, "", iplug::IParam::ShapePowCurve(p.range.skew));

            paramCnt++;
        }
        
        return true;
    }
    else
        return false;
    
}
//===============================================================================
void ICabbage::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{
    //const double gain = GetParam(kGain)->DBToAmp();
    
    if (csdCompiledWithoutError())
    {
        for(int i = 0; i < nFrames ; i++, ++csndIndex)
        {
            if (csndIndex >= csdKsmps)
            {
                csCompileResult = csound->PerformKsmps();
                csndIndex = 0;
            }
            
            for (int channel = 0; channel < NOutChansConnected(); channel++)
            {
                pos = csndIndex*NOutChansConnected();
                csSpin[channel+pos] =  inputs[channel][i]*csScale;
                outputs[channel][i] = csSpout[channel+pos]/csScale;
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
void ICabbage::OnReset()
{
    auto sr = GetSampleRate();
}

//===============================================================================
bool ICabbage::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
    return false;
}

//===============================================================================
void ICabbage::OnIdle()
{
    
}

std::vector<ICabbage::Parameter> ICabbage::parseCsd(std::string csdFile)
{
    std::vector<Parameter> params;
    //fill vector with channels, type and ranges for all parameters
    std::ifstream file(csdFile);
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string csdContents = oss.str();
    
    std::istringstream iss(csdContents);
    std::string line;
    while (std::getline(iss, line)) {
        std::cout << line << std::endl;
        std::regex pattern("\\b(\\w+)\\(([^)]+)\\)");

        std::smatch matches;
        std::string::const_iterator searchStart(line.cbegin());
        Parameter param;
        while (std::regex_search(searchStart, line.cend(), matches, pattern)) {
            if(matches[1] == "channel")
            {
                //remove quotes from identifier parameters
                std::string channel(matches[2]);
                channel.erase(std::remove_if(channel.begin(), channel.end(), [](char c) { return c == '\"'; }), channel.end());
                param.channel = channel;
            }
            else if(matches[1] == "range")
            {
                param.range = Range(matches[2]);
            }
            // Update the search start position
            searchStart = matches.suffix().first;
        }
        
        if(!param.channel.empty())
            params.push_back(param);
    }
    
    return params;

}
//======================== CSOUND MIDI FUNCTIONS ================================
void ICabbage::OnParamChange(int paramIdx)
{
    if(cabbageParameters.size() > 0)
    {
        std::cout << "Channel:" << cabbageParameters[paramIdx].channel << " Value:" << GetParam(paramIdx)->Value() << std::endl;
        csound->SetControlChannel(cabbageParameters[paramIdx].channel.c_str(), GetParam(paramIdx)->Value());
    }
}

//======================== CSOUND MIDI FUNCTIONS ================================
void ICabbage::ProcessMidiMsg(const iplug::IMidiMsg& msg)
{
    TRACE;
    msg.PrintMsg();
    SendMidiMsg(msg);
}

//======================== CSOUND MIDI FUNCTIONS ================================
int ICabbage::OpenMidiInputDevice (CSOUND* csound, void** userData, const char* /*devName*/)
{
    *userData = csoundGetHostData (csound);
    return 0;
}

//==============================================================================
// Reads MIDI input data from host, gets called every time there is MIDI input to our plugin
//==============================================================================
int ICabbage::ReadMidiData (CSOUND* /*csound*/, void* userData,
                            unsigned char* mbuf, int nbytes)
{
    auto* midiData = static_cast<ICabbage*>(userData);
    
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
int ICabbage::OpenMidiOutputDevice (CSOUND* csound, void** userData, const char* /*devName*/)
{
    *userData = csoundGetHostData (csound);
    return 0;
}

//==============================================================================
// Write MIDI data to plugin's MIDI output. Each time Csound outputs a midi message this
// method should be called. Note: you must have -Q set in your CsOptions
//==============================================================================
int ICabbage::WriteMidiData (CSOUND* /*csound*/, void* _userData,
                             const unsigned char* mbuf, int nbytes)
{
    auto* userData = static_cast<ICabbage*>(_userData);
    
    if (!userData)
    {
        assertm(false, "\n\nInvalid");
        return 0;
    }
    
    //    juce::MidiMessage message (mbuf, nbytes, 0);
    //    userData->midiOutputBuffer.addEvent (message, 0);
    return nbytes;
}
