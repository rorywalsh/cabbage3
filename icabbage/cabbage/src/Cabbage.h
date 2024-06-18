#pragma once



#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "IPlug_include_in_plug_hdr.h"
#include "CabbageWidgetDescriptors.h"
#include "CabbageUtils.h"
#include "csound.hpp"
#include "opcodes/CabbageSetOpcodes.h"
#include "opcodes/CabbageGetOpcodes.h"
#include "CabbageParser.h"



class CabbageProcessor;

class Cabbage {
    std::vector<nlohmann::json> widgets;
    
public:
    struct StringArgument {
        std::vector<std::string> values;
    };

    struct NumericArgument {
        std::vector<double> values;
    };

    struct Identifier {
        std::string name;
        std::vector<double> numericArgs;
        std::vector<std::string> stringArgs;
        bool hasStringArgs() const {
            return !stringArgs.empty();
        }
    };
    
    
    struct ParameterChannel {
        std::string name;
        void setValue(float v, float min = 0, float max = 1, float skew= 1, float increment = 1){
            value = v;
        }

        float getValue(){
            return value;
        }
        
        bool hasValueChanged(float newValue){
            return value == newValue;
        }
        
        float value;

    };
    
    Cabbage(CabbageProcessor& p, std::string file);
    ~Cabbage();
    
    Csound* getCsound()
    {
        return csound.get();
    }
    
    bool setupCsound();
    
    void setCsdFile(std::string file)
    {
        csdFile = file;
    }
    
    std::string getCsdFile()
    {
        return csdFile;
    }
    
    void compileCsdFile (std::string csoundFile)
    {
        csCompileResult = csound->Compile (csoundFile.c_str());
    }
    
    void performKsmps()
    {
        csCompileResult = csound->PerformKsmps();
    }
    
    void setSpIn(int index, MYFLT value)
    {
        csSpin[index] =  value*csScale;
    }
    
    MYFLT getSpOut(int index)
    {
        auto outSample = csSpout[index]/csScale;
        return outSample;
    }
    
    bool csdCompiledWithoutError()
    {
        return csCompileResult == 0 ? true : false;
    }
    
    int getKsmps()
    {
        return csdKsmps;
    }
    
    size_t getIndexForParamChannel(std::string name)
    {
        auto it = std::find_if(parameterChannels.begin(), parameterChannels.end(), [&name](const ParameterChannel& paramChannel) {
            return paramChannel.name == name;
        });

        if (it != parameterChannels.end()) {
            size_t index = std::distance(parameterChannels.begin(), it);
            return index;
        }
        
        return -1;
    }
    
    void setControlChannel(const std::string channel, MYFLT value);
    void setStringChannel(const std::string channel, std::string data);
    
    void stopProcessing()
    {
        csCompileResult = -1;
    }
    
    ParameterChannel& getParameterChannel(int index)
    {
        return parameterChannels[index];
    }
    
    int getNumberOfParameter()
    {
        return numberOfParameters;
    }
    
    std::vector<nlohmann::json>& getWidgets()
    {
        return widgets;
    }
    
    nlohmann::json& getWidget(std::string channel)
    {
        for(auto& w : widgets)
        {
            std::cout << w["channel"] << std::endl;
            if(CabbageParser::removeQuotes(w["channel"]) == channel)
                return w;
        }
        

    }
    
//    std::vector<std::string> getParameterChannel()
//    {
//        return parameterChannels;
//    }
    //===============================================================================
//    static std::vector<nlohmann::json> parseCsdForWidgets(std::string csdFile);
    static int getNumberOfParameters(const std::string& csdFile);
    std::vector<iplug::IMidiMsg> &getMidiQueue(){    return midiQueue;   };
    
private:
    void addOpcodes();
    int numberOfParameters = 0;
    std::vector<ParameterChannel> parameterChannels;
    int samplePosForMidi = 0;
    std::string csoundOutput = {};
    std::unique_ptr<CSOUND_PARAMS> csoundParams;
    int csCompileResult = -1;
    int numCsoundOutputChannels = 0;
    int numCsoundInputChannels = 0;
    int csdKsmps = 0;
    MYFLT csScale = 0.0;
    MYFLT *csSpin = nullptr;
    MYFLT *csSpout = nullptr;
    int samplingRate = 44100;
    std::vector<iplug::IMidiMsg> midiQueue;
    std::string csdFile = {};
    std::unique_ptr<Csound> csound;
    CabbageProcessor& processor;
};
