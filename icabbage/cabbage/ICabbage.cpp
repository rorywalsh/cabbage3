#include "ICabbage.h"
#include "IPlug_include_in_plug_src.h"


ICabbage::ICabbage(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets)), server(9999, "127.0.0.1")
{
    
    this->GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
    
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
    
    
    server.setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket & webSocket, const ix::WebSocketMessagePtr & msg) {
        // The ConnectionState object contains information about the connection,
        // at this point only the client ip address and the port.
        std::cout << "Remote ip: " << connectionState->getRemoteIp() << std::endl;
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            std::cout << "New connection" << std::endl;
            std::cout << "id: " << connectionState->getId() << std::endl;
            std::cout << "Uri: " << msg->openInfo.uri << std::endl;
            std::cout << "Headers:" << std::endl;
            for (auto it : msg->openInfo.headers)
            {
                std::cout << "\t" << it.first << ": " << it.second << std::endl;
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Message)
        {
            //all incoming messages
            auto value = 1-(std::abs(std::stod(msg->str))/70.0);
            GetParam(0)->SetNormalized(value);
            SendParameterValueFromDelegate(0, value, true);
            std::cout << "Received: " << msg->str << " value:" << value << std::endl;
        }
    });
    
    
    auto res = server.listen();
    if (!res.first)
    {
        // Error handling
        return 1;
    }
    
    server.disablePerMessageDeflate();
    
    // Run the server in the background. Server can be stoped by calling server.stop()
    server.start();
    
}

ICabbage::~ICabbage(){
    server.stop();
    if (csound)
    {
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
        return true;
    }
    else
        return false;
    
}
//===============================================================================
void ICabbage::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
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

//======================== CSOUND MIDI FUNCTIONS ================================
void ICabbage::OnParamChange(int paramIdx)
{
    DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
    auto client = server.getClients();
    if(client.size()>0)
    {
        auto socket = *(client.begin());
        socket->sendText(std::to_string(GetParam(paramIdx)->Value()));
    }
}

//======================== CSOUND MIDI FUNCTIONS ================================
void ICabbage::ProcessMidiMsg(const IMidiMsg& msg)
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
