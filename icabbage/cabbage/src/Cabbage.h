#pragma once



#include <iostream>
#include <regex>
#include <string>
#include <vector>


#include "CabbageWidgetDescriptors.h"

#include "csound.hpp"

#include "Opcodes/CabbageGetOpcodes.h"



#define assertm(exp, msg) assert(((void)msg, exp))

class CabbageProcessor;

class Cabbage {
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
    
    Cabbage(CabbageProcessor& p, std::string file);
    ~Cabbage();
    
    bool setupCsound();
    
    void setCsdFile(std::string file)
    {
        csdFile = file;
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
        return csSpout[index]/csScale;
    }
    
    bool csdCompiledWithoutError()
    {
        return csCompileResult == 0 ? true : false;
    }
    
    int getKsmps()
    {
        return csdKsmps;
    }
    
    void setControlChannel(std::string channel, MYFLT value)
    {
        csound->SetControlChannel(channel.c_str(), value);
    }
    
    void stopProcessing()
    {
        csCompileResult = -1;
    }
    
    std::string getParameterChannel(int index)
    {
        return parameterChannels[index];
    }
    
    int getNumberOfParameter()
    {
        return numberOfParameters;
    }
    
    std::vector<nlohmann::json> getWidgets()
    {
        return widgets;
    }
    
    std::vector<std::string> getParameterChannel()
    {
        return parameterChannels;
    }
    //===============================================================================
    static std::vector<nlohmann::json> parseCsdForWidgets(std::string csdFile);
    static int getNumberOfParameters(const std::string& csdFile);
    static std::string getWidgetType(const std::string& line);
    static void updateJsonFromSyntax(nlohmann::json& jsonObj, const std::string& syntax);
    static std::vector<Identifier> tokeniseLine(const std::string& syntax);
    
private:
    int numberOfParameters = 0;
    std::vector<std::string> parameterChannels;
    std::vector<nlohmann::json> widgets;
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
    
    std::string csdFile = {}, csdFilePath = {};
    std::unique_ptr<Csound> csound;
    CabbageProcessor& processor;
};
