// TestProcessor.cpp
#include "CabbageProcessor.h"
#include <iostream>

//===================================================================================
pluginType* LatticeProcessorPluginFactory::createPlugin(const clap_host* host)
{
    //create a new instance of CabbageProcessor 
    auto* processor = new CabbageProcessor();
    return new pluginType(host, *processor);
}
//===================================================================================
#if defined(CABBAGE_SERVICE_APP)

#else
CabbageProcessor::CabbageProcessor()
    : Processor(), cabbage(*this, "")
{
    addParameters();
    addChannels();

    auto rootPath = cabbage::File::getCsdPath(cabbage.getCsdFile());
    setMountPoint(rootPath);

    
    if (auto json = cabbage::File::parseCabbageSection(cabbage.getCsdFile()))
    {
        auto w = cabbage::Utils::findPropertyInForm<int>(*json, "size.width");
        auto h = cabbage::Utils::findPropertyInForm<int>(*json, "size.height");
        setEditorSize(400, 300);
    }
      

  
}
#endif

//==================================================================================
void CabbageProcessor::addChannels()
{
    auto channelConfig = cabbage::Engine::getIOChannalConfig(cabbage.getCsdFile());
    auto [inputBuses, outputBuses] = cabbage.parseBusConfiguration(channelConfig);

    int inputBusIndex = 1;
    for (int bus : inputBuses)
    {
        addInputBus("Input Bus" + std::to_string(inputBusIndex), bus, lattice::ChannelLayout(bus));
        inputBusIndex++;
    }

    int outputBusIndex = 1;
    for (int bus : outputBuses)
    {
        addInputBus("Output Bus" + std::to_string(outputBusIndex), bus, lattice::ChannelLayout(bus));
        outputBusIndex++;
    }
}

void CabbageProcessor::addParameters()
{
    std::vector<std::string> rangeTypes = cabbage.getRangeWidgetTypes(cabbage.getWidgets());
    for (auto &w : cabbage.getWidgets())
    {
        if (w.contains("automatable") && w["automatable"] == 1 &&
            (!w.contains("channelType") || w["channelType"] == "number"))
        {
            const std::string widgetType = w["type"].get<std::string>();

            try
            {
                // check if widget has a range - range widget parameters are initialised differently to other
                // widgets
                if (std::any_of(rangeTypes.begin(), rangeTypes.end(),
                                [&](const std::string &type) { return widgetType == type; }))
                {
                    addParameter({w["channel"].get<std::string>(), 
                        w["range"]["min"].get<float>(),
                        w["range"]["max"].get<float>(), 
                        w["range"]["defaultValue"].get<float>(),
                        w["range"]["increment"].get<float>(), 
                        w["range"]["skew"].get<float>()});
                }
                else
                {
                    addParameter({w["channel"].get<std::string>(), 
                        w["min"].get<float>(),
                        w["max"].get<float>(), 
                        w["defaultValue"].get<float>()});
                }

                cabbage.initParameter(w);
            }
            catch (nlohmann::json::exception &e)
            {
                lattice::logInfo << "JSON error: " << e.what() << "\n" << w.dump(4);
                cabbage::Utils::check(false, "");
            }
        }
    }
}


//=================================================================================
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
