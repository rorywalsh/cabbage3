// TestProcessor.cpp
#include "CabbageProcessor.h"
#include <iostream>

//===================================================================================
pluginType* CabsProcessorPluginFactory::createPlugin(const clap_host* host)
{
    auto* processor = new CabbageProcessor(2, 2);
    return new pluginType(host, *processor);
}
//===================================================================================

CabbageProcessor::CabbageProcessor(int numInputs, int numOutputs)
    : Processor(numInputs, numOutputs)
{
    addParameter({"Gain", 0, 1});
}

void CabbageProcessor::process(float** /*inputs*/, float** outputs, std::size_t blockSize)
{
    const auto channels = getNumInputs();
    auto& noteEvents = getNoteEvents();
    
    while (!noteEvents.empty()) {
        auto event = noteEvents.front();
        event.log();
        noteEvents.pop_front();
    }
    
    for (uint32_t i = 0; i < blockSize; i++)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
            outputs[ch][i] = sine[ch].process(1, 220);
    }
}

void CabbageProcessor::onMesssgeFromWebView(nlohmann::json j)
{
    std::cout << j.at(0).dump(4);
    float value = j.at(0).value("value", 0.f);
    auto paramIdx = j.at(0).value("paramIdx", -1);
    sendParameterUpdateToHost(paramIdx, value);
}

void CabbageProcessor::setParameter(int paramId, double value)
{
    getParameters()[paramId].value = value;
}

double CabbageProcessor::getParameter(int paramId)
{
    return getParameters()[paramId].value;
}

void CabbageProcessor::prepareToPlay(double /*sampleRate*/, uint32_t /*minFrameCount*/, uint32_t /*maxFrameCount*/)
{
    
}
