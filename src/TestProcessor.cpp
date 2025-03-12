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

void TestProcessor::onMesssgeFromWebView(nlohmann::json j)
{
    std::cout << j.at(0).dump(4);
    float value = j.at(0).value("value", 0.f);
    auto paramIdx = j.at(0).value("paramIdx", -1);
    sendParameterUpdateToHost(paramIdx, value);
}

void TestProcessor::setParameter(int paramId, double value)
{
    getParameters()[paramId].value = value;
}

double TestProcessor::getParameter(int paramId) 
{
    return getParameters()[paramId].value;
}
