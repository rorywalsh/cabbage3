// TestProcessor.cpp
#include "CabbageProcessor.h"
#include <iostream>

//===================================================================================
pluginType* LatticeProcessorPluginFactory::createPlugin(const clap_host* host)
{
    //create a new instance of CabbageProcessor 
    auto *processor = new CabbageProcessor();
    return new pluginType(host, *processor);
}
//===================================================================================
#if defined(CABBAGE_SERVICE_APP)

#else
CabbageProcessor::CabbageProcessor()
    : Processor(), cabbage(*this, "")
{
    auto rootPath = cabbage::File::getCsdPath(cabbage.getCsdFile());
    setMountPoint(rootPath);
    
	addInputBus("Input Bus", 2, lattice::ChannelLayout::Stereo);
    addInputBus("Output Bus", 2, lattice::ChannelLayout::Stereo);


    setEditorSize(400, 300);
}
#endif

void CabbageProcessor::process(float** inputs, float** outputs, std::size_t blockSize)
{
	const auto channels = getChannelConfig().getTotalNumInputChannels();
    
    for (uint32_t i = 0; i < blockSize; i++)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
            outputs[ch][i] = inputs[ch][i]*getParameter("Gain");
    }
}

void CabbageProcessor::onMesssgeFromWebView(const nlohmann::json& j)
{
    std::cout << j.at(0).dump(4);
    float value = j.at(0).value("value", 0.f);
    auto paramIdx = j.at(0).value("paramIdx", -1);
    sendParameterUpdateToHost(paramIdx, value);
}

// This can be called from the host - if so update the
// corresponding parameter value using updateParameter() function
void CabbageProcessor::setParameter(int paramId, double value)
{
    getParameters()[paramId].value = value;
}

void CabbageProcessor::prepareToPlay(double sr, uint32_t /*minFrameCount*/, uint32_t /*maxFrameCount*/)
{
    sampleRate = sr;
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
        cabbage::Utils::check(userData, "\nInvalid");
        return 0;
    }

    int cnt = 0;

    // Access the note event queue
    auto noteEvents = pluginData->getProcessor().getNoteEvents();
    while (!noteEvents.empty())
    {
        // Get the front event
        auto event = noteEvents.front();

        // Prevent overflow
        if (cnt + 3 > nbytes) break;

        // Determine the MIDI status byte
        uint8_t statusByte = 0;
        uint8_t velocity = static_cast<uint8_t>(event.velocity * 127); // Normalize velocity to MIDI range

        if (event.type == lattice::NoteEvent::Type::noteOn)
            statusByte = 0x90; // Note On, channel 1
        else if (event.type == lattice::NoteEvent::Type::noteOff)
            statusByte = 0x80; // Note Off, channel 1
        else
        {
            noteEvents.pop_front(); // Move to the next event
            continue; // Skip unsupported types
        }

        // Fill the MIDI buffer
        *mbuf++ = statusByte;
        *mbuf++ = static_cast<uint8_t>(event.key); // MIDI note number (0-127)
        *mbuf++ = velocity;

        cnt += 3;

        // Remove the processed event from the queue
        noteEvents.pop_front();
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
        cabbage::Utils::check(userData, "\n\nInvalid");
        return 0;
    }

    //    juce::MidiMessage message (mbuf, nbytes, 0);
    //    userData->midiOutputBuffer.addEvent (message, 0);
    return nbytes;
}
