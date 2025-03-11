// TestProcessor.cpp
#include "TestProcessor.h"
#include <iostream>

//===================================================================================
pluginType* CabsProcessorPluginFactory::createPlugin(const clap_host* host)
{
    auto* processor = new TestProcessor(2, 2);
    return new pluginType(host, *processor, processor->getNumInputs(), processor->getNumOutputs());
}
//===================================================================================

TestProcessor::TestProcessor(int numInputs, int numOutputs)
    : Processor(numInputs, numOutputs)
{
    addParameter({"Gain", 0, 1});
}

void TestProcessor::process(float** /*inputs*/, float** outputs, std::size_t blockSize)
{
    const auto channels = getNumInputs();
    
    for (uint32_t i = 0; i < blockSize; i++)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
            outputs[ch][i] = sine[ch].process(getParameter(0), 220);
    }
}

void TestProcessor::setParameter(int paramId, double value) 
{
    std::cout << "Setting parameter " << paramId << " to " << value << std::endl;
    getParameters()[paramId].value = value;
    std::cout << "Getting parameter " << paramId << " value: " << getParameter(paramId)  << std::endl;
}

double TestProcessor::getParameter(int paramId) 
{
    return getParameters()[paramId].value;
}
