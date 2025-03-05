// CabbageProcessor.cpp
#include "CabbageProcessor.h"
#include <iostream>

pluginType* CabbagePluginFactory::createPlugin(const clap_host* host)
{
    // Default values for inputs and outputs
    return new pluginType(host, 2, 2);
}

//===================================================================================

CabbageProcessor::CabbageProcessor(int numInputs, int numOutputs)
    : numInputs(numInputs),
    numOutputs(numOutputs)
{
    parameters.push_back({"Gain", 0, 1});
}

void CabbageProcessor::process(float** /*inputs*/, float** outputs, std::size_t blockSize)
{
    const auto channels = 2;
    
    for (uint32_t i = 0; i < blockSize; i++)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
            outputs[ch][i] = sine[ch].process(1, 220);
    }
}

void CabbageProcessor::setParameter(int paramId, double value) {
    parameters[paramId].value = value;
}

double CabbageProcessor::getParameter(int paramId) const {
    return parameters[paramId].value;
}
