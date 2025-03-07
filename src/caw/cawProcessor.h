// CawProcessor.h
#pragma once

#include <cstddef> // for std::size_t
#include <vector>
#include <caw/cawClap/Plugin.h> // Include the necessary CLAP headers
#include <caw/cawClap/Utils.h>


class CawProcessor {
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
    CawProcessor(int numInputs, int numOutputs): numInputs(numInputs), numOutputs(numOutputs) {};
    
    // Destructor to clean up resources
    ~CawProcessor() {
        // Destructor implementation (if needed)
    }

    // Process method to handle audio processing
    virtual void process(float** inputs, float** outputs, std::size_t blockSize) = 0;

    // Set a parameter value
    virtual void setParameter(int paramId, double value) = 0;

    // Get a parameter value
    virtual double getParameter(int paramId) const = 0;

    // Get the number of audio outputs
    int getNumOutputs(){    return numOutputs;  };

    // Get the number of audio inputs
    int getNumInputs(){     return numInputs;   };

    // Get the parameters
    std::vector<Parameter> getParameters(){ return parameters;  }
    
    void addParameter(CawProcessor::Parameter parameter) { parameters.push_back(parameter); }
    
    
private:
    // Number of audio inputs and outputs
    int numInputs = 0;
    int numOutputs = 0;
    // Store parameters (could be a more complex structure if needed)
    std::vector<Parameter> parameters;
};
