#pragma once

#include "lattice/LatticeProcessor.h"
#include "Cabbage.h"


class CabbageProcessor : public lattice::Processor {
    
public:
    CabbageProcessor(std::string = "");
    
    // Destructor to clean up resources
    ~CabbageProcessor(){};

    // Csound API functions for deailing with midi input
    static int OpenMidiInputDevice(CSOUND *csnd, void **userData, const char *devName);
    static int OpenMidiOutputDevice(CSOUND *csnd, void **userData, const char *devName);
    static int ReadMidiData(CSOUND *csound, void *userData, unsigned char *mbuf, int nbytes);
    static int WriteMidiData(CSOUND *csound, void *userData, const unsigned char *mbuf, int nbytes);

    // Add parameters based on automatable widgets
    void addParameters(); 

    // Set up channel config
    void addChannels();

    // Process method to handle audio processing
    void process(float** inputs, float** outputs, std::size_t blockSize) override;

    // Set a parameter value
    void setParameter(int paramId, double value) override;
    
    // Called whenever the webview sends a message
    void onMesssgeFromWebView(const nlohmann::json &j) override;
    
    // Called whenever the UI iwdgets need updating
    void updateUI();
    
    // Called at least once before the processing starts
    void prepareToPlay(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) override;
        
    int getSampleRate(){    return sampleRate;  }
 
    cabbage::Engine &getCabbageEngine() { return cabbage; }
#ifdef CabbageApp
    std::function<void(CabbageOpcodeData)> hostCallback = nullptr;
#endif
    
private:
    void onIdle();
    void onIdleScheduler();
    void startOnIdle();
    void stopOnIdle();
    void updateWidgetData(const CabbageOpcodeData &data);
    
    bool uiIsOpen = false;
    bool allowDequeuing = false;
    int sampleRate = 44100;
    cabbage::Engine cabbage;
    bool matchingNumInputsOutputs = true;
    int csndIndex = 0;
    int pos = 0;
    int totalNumOutputs = 0;
    int totalNumInputs = 0;
    std::atomic<bool> isIdleRunning;
    std::thread idleThread;
    int idleCounter = 0;
};
