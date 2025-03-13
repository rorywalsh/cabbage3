// CawProcessor.h
#pragma once

#include <cstddef> // for std::size_t
#include <vector>
#include "cabs/clap/ClapPlugin.h" // Include the necessary CLAP headers
#include "cabs/cabsProcessor.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//===========================================================
namespace ProjectInfo
{
    const char* const  projectName    = "DemoRunner";
    const char* const  companyName    = "Raw Material Software Limited";
    const char* const  versionString  = "8.0.6";
    const int          versionNumber  = 0x80006;
}

//===========================================================

template <typename T>
class SineOscillator
{
  private:
    T sampleRate;
    T frequency;
    T phase;
    T phaseIncrement;
    T amplitude;

    void updatePhaseIncrement() { phaseIncrement = (static_cast<T>(2.0) * M_PI * frequency) / sampleRate; }

  public:
    SineOscillator() : sampleRate(44100), frequency(440), phase(0.0), amplitude(1.0) { updatePhaseIncrement(); }

    SineOscillator(T freq, T rate, T amp = 1.0) : frequency(freq), sampleRate(rate), phase(0.0), amplitude(amp)
    {
        updatePhaseIncrement();
    }

    void setFrequency(T newFreq)
    {
        frequency = newFreq;
        updatePhaseIncrement();
    } 

    void setAmplitude(T newAmp) { amplitude = (newAmp < 0) ? 0 : (newAmp > 1 ? 1 : newAmp); }

    T process(float amp, float freq)
    {
        setAmplitude(amp);
        setFrequency(freq);
        T output = amplitude * std::sin(phase);
        phase += phaseIncrement;
        if (phase >= static_cast<T>(2.0) * M_PI)
            phase -= static_cast<T>(2.0) * M_PI;
        return output;
    }
};



class TestProcessor : public cabs::Processor {
    
public:
    // Constructor that initializes the CLAP plugin
    TestProcessor(int numInputs, int numOutputs);
    
    // Destructor to clean up resources
    ~TestProcessor(){};

    // Process method to handle audio processing
    void process(float** inputs, float** outputs, std::size_t blockSize) override;

    // Set a parameter value
    void setParameter(int paramId, double value) override;

    // Get a parameter value
    double getParameter(int paramId) override;
    
    // Called whenever the webview sends a message
    void onMesssgeFromWebView(nlohmann::json j) override;
    
    // Called at least once before the processing starts
    void prepareToPlay(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount);
    
private:
    // Number of audio inputs and outputs
    int numInputs = 0;
    int numOutputs = 0;
    SineOscillator<float> sine[2];
    // Store parameters (could be a more complex structure if needed)
};
