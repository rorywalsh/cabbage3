// Cumhdach.h
#pragma once

#include <cstddef> // for std::size_t
#include <vector>
#include <CabbageClap/Plugin.h> // Include the necessary CLAP headers
#include <CabbageClap/Utils.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

class Cumhdach {
    struct Parameter {
        const char* name;
        float min;
        float max;
        float value;
        float skew;
        float increment;

        // Constructor with default values
        Parameter(const char* paramName = "", float paramMin = 0.f, float paramMax = 1.f,
                  float paramValue = 0.f, float paramIncrement = 0.01f, float paramSkew = 1.f)
            : name(paramName), min(paramMin), max(paramMax),
              value(paramValue), skew(paramSkew), increment(paramIncrement) {}
    };

    
public:
    // Constructor that initializes the CLAP plugin
    Cumhdach(int numInputs, int numOutputs);
    
    // Destructor to clean up resources
    ~Cumhdach();

    // Process method to handle audio processing
    void process(float** inputs, float** outputs, std::size_t blockSize);

    // Set a parameter value
    void setParameter(int paramId, double value);

    // Get a parameter value
    double getParameter(int paramId) const;

    int getNumOutputs(){    return numOutputs;  }
    int getNumInputs(){    return numInputs;  }
    std::vector<Parameter> getParameters(){ return parameters;  }
    
private:
    // Number of audio inputs and outputs
    int numInputs = 0;
    int numOutputs = 0;
    SineOscillator<float> sine[2];
    // Store parameters (could be a more complex structure if needed)
    std::vector<Parameter> parameters;
};
