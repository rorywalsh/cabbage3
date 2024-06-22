#pragma once



#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <optional>

#include "IPlug_include_in_plug_hdr.h"
#include "CabbageWidgetDescriptors.h"
#include "CabbageUtils.h"
#include "csound.hpp"
#include "opcodes/CabbageSetOpcodes.h"
#include "opcodes/CabbageGetOpcodes.h"
#include "CabbageParser.h"

//choc classes for reading audio files
#include "../../../choc-main/audio/choc_AudioFileFormat.h"
#include "../../../choc-main/audio/choc_AudioFileFormat_Ogg.h"
#include "../../../choc-main/audio/choc_AudioFileFormat_WAV.h"
#include "../../../choc-main/audio/choc_AudioFileFormat_FLAC.h"
#include "../../../choc-main/audio/choc_AudioFileFormat_MP3.h"
#include "../../../choc-main/audio/choc_SampleBuffers.h"


class CabbageProcessor;

class Cabbage {
    
    //a vector containined all Cabbage widgets
    std::vector<nlohmann::json> widgets;
    
public:

    //a parameter struct whose namees match that of the corresponding Csound channel
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
    
    // Get the Csound object
    Csound* getCsound()                   { return csound.get(); }
    
    // Setup Csound
    bool setupCsound();
    
    // Set the CSD file
    void setCsdFile(std::string file)     { csdFile = file; }
    
    // Get the CSD file
    std::string getCsdFile()              { return csdFile; }
    
    // Compile the CSD file
    void compileCsdFile(std::string csoundFile) { csCompileResult = csound->Compile(csoundFile.c_str()); }
    
    // Perform KSMPS (control periods)
    void performKsmps()                   { csCompileResult = csound->PerformKsmps(); }
    
    // Set input value for a specific index in csSpin array
    void setSpIn(int index, MYFLT value)  { csSpin[index] = value * csScale; }
    
    // Get output value for a specific index in csSpout array
    MYFLT getSpOut(int index)             { return csSpout[index] / csScale; }
    
    // Check if CSD compiled without error
    bool csdCompiledWithoutError()        { return csCompileResult == 0 ? true : false; }
    
    // Get the KSMPS value
    int getKsmps()                        { return csdKsmps; }
    
    // Get the MIDI queue
    std::vector<iplug::IMidiMsg>& getMidiQueue() { return midiQueue; }
    
    // Stop processing
    void stopProcessing()                 { csCompileResult = -1; }
    
    // Get a parameter channel by index
    ParameterChannel& getParameterChannel(int index) { return parameterChannels[index]; }
    
    // Get the number of parameters
    int getNumberOfParameter()            { return numberOfParameters; }
    
    // Get the widgets
    std::vector<nlohmann::json>& getWidgets() { return widgets; }
    
    // Get the index for a parameter channel by name
    size_t getIndexForParamChannel(std::string name);
    
    // Set control channel value
    void setControlChannel(const std::string channel, MYFLT value);
    
    // Set string channel data
    void setStringChannel(const std::string channel, std::string data);
    
    //returns a JSON widget references from the lists of widget
    std::optional<std::reference_wrapper<nlohmann::json>> getWidget(const std::string& channel);

    //returns number of plugin paremters - even though lots of widgets have channels, only a select few can be plugin parameters
    static int getNumberOfParameters(const std::string& csdFile);
    
    //utility script to remove control characters from string - needed to santise Cabbage code going to JS
    static std::string removeControlCharacters(const std::string& input);
    
    //return a JS script that will trigger a widget's properties to be updated
    static std::string getWidgetUpdateScript(std::string channel, std::string data);
    static std::string getWidgetUpdateScript(std::string channel, float value);

    
    //these two methods return combine with getWidgetIdentifierUpdateScript() to return a JS method
    //that packs samples for a given table
    std::string updateFunctionTable(CabbageOpcodeData data, nlohmann::json& jsonObj);
    static void setTableJSON(std::string channel, std::vector<double> samples, nlohmann::json& jsonObj);
    
    //returns a script that will update a csoundoutput widget
    const std::string getCsoundOutputUpdateScript(std::string output);

    //utlity function to loads samples from a sound file on disk.
    static std::vector<double> readAudioFile(const std::string& filePath);
    
    

    
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
