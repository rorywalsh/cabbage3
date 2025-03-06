// TestProcessor.cpp
#include "TestProcessor.h"
#include <iostream>

pluginType* CumhdachPluginFactory::createPlugin(const clap_host* host)
{
    // Default values for inputs and outputs
    return new pluginType(host, 2, 2);
}

//===================================================================================

TestProcessor::TestProcessor(int numInputs, int numOutputs)
    : Cumhdach(numInputs, numOutputs)
{
    addParameter({"Gain", 0, 1});
}

void TestProcessor::process(float** /*inputs*/, float** outputs, std::size_t blockSize)
{
    const auto channels = 2;
    
    for (uint32_t i = 0; i < blockSize; i++)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
            outputs[ch][i] = sine[ch].process(1, 220);
    }
}

void TestProcessor::setParameter(int paramId, double value) {
    getParameters()[paramId].value = value;
}

double TestProcessor::getParameter(int paramId) const {
    return 0;
    //getParameters()[paramId].value;
}
