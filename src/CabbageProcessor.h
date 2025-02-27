// CabbageProcessor.h
#pragma once

#include <cstddef> // for std::size_t
#include <vector>
#include <CabbageClap/Plugin.h> // Include the necessary CLAP headers

template <typename T>
class SineOscillator {
private:
    T sampleRate;
    T frequency;
    T phase;
    T phaseIncrement;
    T amplitude;

    void updatePhaseIncrement() {
        phaseIncrement = (static_cast<T>(2.0) * M_PI * frequency) / sampleRate;
    }

public:
    SineOscillator() : sampleRate(44100), frequency(440), phase(0.0), amplitude(1.0) {
        updatePhaseIncrement();
    }

    SineOscillator(T freq, T rate, T amp = 1.0) 
        : frequency(freq), sampleRate(rate), phase(0.0), amplitude(amp) {
        updatePhaseIncrement();
    }

    void setFrequency(T newFreq) {
        frequency = newFreq;
        updatePhaseIncrement();
    }

    void setAmplitude(T newAmp) {
        amplitude = (newAmp < 0) ? 0 : (newAmp > 1 ? 1 : newAmp);
    }

    T process(float amp, float freq) {
        setAmplitude(amp);
        setFrequency(freq);
        T output = amplitude * std::sin(phase);
        phase += phaseIncrement;
        if (phase >= static_cast<T>(2.0) * M_PI) phase -= static_cast<T>(2.0) * M_PI;
        return output;
    }
};

class CabbageProcessor {
public:
    // Constructor that initializes the CLAP plugin
    CabbageProcessor(int numInputs, int numOutputs);
    
    // Destructor to clean up resources
    ~CabbageProcessor();

    // Process method to handle audio processing
    void process(float** inputs, float** outputs, std::size_t blockSize);

    // Set a parameter value
    void setParameter(int paramId, double value);

    // Get a parameter value
    double getParameter(int paramId) const;

private:
    // Number of audio inputs and outputs
    int numInputs = 0;
    int numOutputs = 0;
    int numParameters = 0;
    SineOscillator<float> sine[2];
    // Store parameters (could be a more complex structure if needed)
    std::vector<double> parameters;
};
