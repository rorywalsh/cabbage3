/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#pragma once
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <optional>

#include "CabbageUtils.h"
#include "csound.hpp"
#include "CabbageParser.h"
#include <readerwriterqueue.h>
#include "opcodes/CabbageSetOpcodes.h"
#include "opcodes/CabbageGetOpcodes.h"

class CabbageProcessor;
struct CabbageOpcodeData;

namespace cabbage
{

class Engine
{

    // a vector containined all Cabbage widgets
    std::vector<nlohmann::json> widgets;

  public:
    // a parameter struct whose namees match that of the corresponding Csound channel
    struct ParameterChannel
    {
        std::string name;
        void setValue(float v) { value = v; }

        float getValue() { return value; }

        bool hasValueChanged(float newValue) { return init ? (init = false, true) : (value != newValue); }

        float value;
        bool init = true;
    };

    Engine(CabbageProcessor &p, std::string file);
    ~Engine();

    // Get the Csound object
    Csound *getCsound() { return csound.get(); }

    // Setup Csound
    bool setupCsound();

    // Set the CSD file
    void setCsdFile(std::string file) { csdFile = file; }

    // Get the CSD file
    std::string getCsdFile() { return csdFile; }

    // Compile the CSD file
    void compileCsdFile(std::string csoundFile) { csCompileResult = csound->Compile(csoundFile.c_str()); }

    // Perform KSMPS (control periods)
    void performKsmps() { csCompileResult = csound->PerformKsmps(); }

    // Set input value for a specific index in csSpin array
    void setSpIn(int index, MYFLT value) { csSpin[index] = value * csScale; }

    // Get output value for a specific index in csSpout array
    MYFLT getSpOut(int index)
    {
        auto spout = csound->GetSpout();
        if (spout)
            return spout[index] / csScale;

        return 0;
    }

    // Check if CSD compiled without error
    bool csdCompiledWithoutError() { return csCompileResult == 0 ? true : false; }

    // Get the KSMPS value
    int getKsmps() { return csdKsmps; }

    // Stop processing
    void stopProcessing() { csCompileResult = -1; }

    // init and set up parameter
    void initParameter(const nlohmann::json &w);

    // Get a parameter channel by index
    ParameterChannel &getParameterChannel(int index) { return parameterChannels[index]; }

    // Get the number of parameters
    int getNumberOfParameters() { return numberOfParameters; }

    // Get the widgets
    std::vector<nlohmann::json> &getWidgets() { return widgets; }

    // Update widget with JSON object
    const std::string updateWidgetState(nlohmann::json j);
    // Get the index for a parameter channel by name
    size_t getIndexForParamChannel(std::string name);

    // Set control channel value
    void setControlChannel(const std::string channel, MYFLT value);

    // Set string channel data
    void setStringChannel(const std::string channel, std::string data);

    // Returns a JSON widget references from the lists of widget
    std::optional<std::reference_wrapper<nlohmann::json>> getWidget(const std::string &channel);

    // Returns number of plugin paremters - even though lots of widgets have channels, only a select few can be plugin
    // parameters
    static int getNumberOfParameters(const std::string &csdFile);
    
    // Returns the current number of parameters registered
    int getCurrentParameterCount(){ return numberOfParameters;  }

    // Return the channel config string, e.g., '2-2'
    static const std::string getIOChannalConfig(const std::string &csdFile);

    // Return a vector pair contining input and output buses
    static std::pair<std::vector<int>, std::vector<int>> parseBusConfiguration(const std::string &config);

    // Utility script to remove control characters from string - needed to santise Cabbage code going to JS
    static std::string removeControlCharacters(const std::string &input);

    // Process Csound console messages
    void processCsoundMessages();

    // Return a JS script that will trigger a widget's properties to be updated
    static std::string getUpdatedWidgetJsonStr(const std::string& channel, std::string data);
    static std::string getUpdatedWidgetJsonStr(const std::string& channel, float value);

    // These two methods return combine with getWidgetIdentifierUpdateScript() to return a JS method
    // that packs samples for a given table
    void updateFunctionTable(CabbageOpcodeData data, nlohmann::json &jsonObj);
    static void setTableJSON(std::string channel, std::vector<double> samples, nlohmann::json &jsonObj);

    // Returns a script that will update a csoundoutput widget
    const std::string getCsoundOutputUpdateScript(const std::string& output);

    // Return a vector of all widget types that have a range object
    static std::vector<std::string> getRangeWidgetTypes(const std::vector<nlohmann::json> widgets);

    // Setup reserved channel
    void setReservedChannels();

    // Get full range value from widget
    static float remap(double n, double start1, double stop1, double start2, double stop2);
    float getFullRangeValue(std::string channel, float normalValue);

    moodycamel::ReaderWriterQueue<CabbageOpcodeData> opcodeData;

    std::string getCompileErrors() { return compileErrors; }

    void displayAndClearCompileErrors()
    {
        if (compileErrors.length() > 0)
        {
            lattice::logInfo << compileErrors.c_str();
            compileErrors.clear();
        }
    }
    
    CabbageProcessor& getProcessor(){   return processor;  }

  private:
    void addOpcodes();
    int numberOfParameters = 0;
    std::vector<ParameterChannel> parameterChannels;
    std::string csoundOutput = {};
    int csCompileResult = -1;
    int csdKsmps = 0;
    MYFLT csScale = 0.0;
    MYFLT *csSpin = nullptr;
    double sampleRate = 44100;
    std::string csdFile = {};
    std::string compileErrors;
    std::unique_ptr<Csound> csound;
    CabbageProcessor &processor;
};
} // namespace cabbage
