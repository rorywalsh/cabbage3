#pragma once

#include "lattice/LatticeProcessor.h"
#include "Cabbage.h"

class CabbageProcessor : public lattice::Processor {
    
public:
    CabbageProcessor(std::string csdFile = "", std::string config = "");
    
    // Destructor to clean up resources
    ~CabbageProcessor();

    // Csound API functions for deailing with midi input
    static int OpenMidiInputDevice(CSOUND *csnd, void **userData, const char *devName);
    static int OpenMidiOutputDevice(CSOUND *csnd, void **userData, const char *devName);
    static int ReadMidiData(CSOUND *csound, void *userData, unsigned char *mbuf, int nbytes);
    static int WriteMidiData(CSOUND *csound, void *userData, const unsigned char *mbuf, int nbytes);

    // Add parameters based on automatable widgets
    void addParameters(); 

    // Set up channel config
    void addChannels(const std::string& config = "");

    // Process method to handle audio processing
    void process(float** inputs, float** outputs, std::size_t blockSize) override;

    // Set a parameter value
    void setParameter(int paramId, double value) override;
    
    // Called whenever the webview sends a message
    void onMessageFromWebView(const nlohmann::json &j) override;
   
    // Called whenever the plugin webview is ready
    void onWebViewIsReady() override;
    
    // Called whenever the UI iwdgets need updating
    void updateUI();
    
    // Called at least once before the processing starts
    void prepareToPlay(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) override;
        
    int getSampleRate(){    return sampleRate;  }
 
    // Triggered by CabbageApp whenever websocket connection is established
    void setCabbageIsReady();
    
    // Plugin load/save state
    void loadPluginState(nlohmann::json state) override;
    nlohmann::json savePluginState() override;

    
    cabbage::Engine &getCabbageEngine() { return cabbage; }
    
#ifdef CabbageApp
    std::function<void(CabbageOpcodeData)> hostCallback = nullptr;
#endif
    
    // Process opcode data and return the updated widget JSON if applicable
    std::optional<nlohmann::json> processOpcodeData(const CabbageOpcodeData &data);
    
    // Adds a MIDI note event to the midi event queue
    void addNoteEventFromJson(const nlohmann::json &j);
    void suspendProcessing() { processingEnabled.store(false); }
    void stopIdleThread();
    bool isIdleThreadRunning(){   return isIdleRunning.load(std::memory_order_acquire);   }

  private:
    
    void onIdle();
    void onIdleScheduler();
    void startOnIdle();
    void stopOnIdle();
    void updateWidgetData(const CabbageOpcodeData &data);
    void addParameterForWidget(nlohmann::json& w);
    void openFileDialog(const std::string& channel, const std::string& directory, const std::string& filters, bool openAtLastKnownLocation);
    std::string generateErrorPageHtml(const std::string& errors);
    std::atomic<bool> processingEnabled{true};
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
    std::vector<lattice::Parameter> webviewMessageQueue;
    
    std::string errorPageHtml = "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\" />\n"
    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n"
    "  <title>Bouncing Cabbage</title>\n"
    "  <style>\n"
    "    body {\n"
    "      margin: 0;\n"
    "      height: 100vh;\n"
    "      display: flex;\n"
    "      justify-content: center;\n"
    "      align-items: center;\n"
    "      background-color: #3CB1F2;\n"
    "    }\n"
    "\n"
    "    svg {\n"
    "      width: 200px;\n"
    "      height: auto;\n"
    "      animation: bounce 1.0s ease-in-out infinite;\n"
    "    }\n"
    "\n"
    "    @keyframes bounce {\n"
    "      0%, 100% {\n"
    "        transform: translateY(0);\n"
    "      }\n"
    "      30% {\n"
    "        transform: translateY(-40px);\n"
    "      }\n"
    "      50% {\n"
    "        transform: translateY(0);\n"
    "      }\n"
    "      70% {\n"
    "        transform: translateY(-15px);\n"
    "      }\n"
    "      85% {\n"
    "        transform: translateY(0);\n"
    "      }\n"
    "    }\n"
    "  </style>\n"
    "</head>\n"
    "<body>\n"
    "\n"
    "  <!-- Your SVG starts here -->\n"
    "  <?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
    "  <svg\n"
    "     xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
    "     xmlns:cc=\"http://creativecommons.org/ns#\"\n"
    "     xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
    "     xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
    "     xmlns=\"http://www.w3.org/2000/svg\"\n"
    "     width=\"420\"\n"
    "     height=\"400\"\n"
    "     viewBox=\"0 0 335.99977 319.99982\"\n"
    "     id=\"svg2\"\n"
    "     version=\"1.1\">\n"
    "    <path\n"
    "       id=\"Selection #1\"\n"
    "       d=\"m 200.37937,24.833718 c 53.94784,10.344682 89.98007,47.887315 109.69256,95.327092 0,0 10.52672,30.68879 10.52672,30.68879 0,0 8.0459,11.5083 8.0459,11.5083 5.72603,9.28336 5.95399,13.91225 5.8333,24.29531 -0.28159,21.82741 -18.8006,37.91344 -41.57053,37.03114 -33.29664,-1.27869 -48.23521,-39.49905 -29.78326,-63.88384 8.78345,-11.6234 20.81209,-15.30604 35.14719,-15.34441 0,0 -12.48456,-33.2462 -12.48456,-33.2462 -13.67803,-28.476641 -34.36941,-49.869297 -62.61057,-65.725176 -13.54393,-7.59548 -30.2258,-12.671921 -45.59347,-15.600142 -7.22791,-1.380996 -18.63967,-0.792793 -22.79674,-6.904979 18.07648,-1.393782 27.42314,-1.636736 45.59346,1.854115 z M 62.298194,72.848908 c -7.817932,7.915154 -14.563087,17.10901 -19.658833,26.852704 -6.543998,12.505678 -12.2566,32.082578 -13.678038,46.033218 0,0 -1.327575,17.90177 -1.327575,17.90177 -0.134107,4.55217 1.340982,6.96892 0,11.50832 -9.158925,-21.37987 -12.632076,-41.76235 0,-62.65631 5.726008,-8.61844 9.909878,-12.23717 17.191422,-19.180502 -8.059312,2.57018 -13.289165,4.296424 -20.061124,9.615822 C 6.5534767,117.2198 1.2297598,144.48168 9.7316079,164.91533 16.543811,181.34659 24.643345,182.10103 30.074333,193.0467 0.0899247,176.34686 -4.9119466,132.46189 16.020829,107.3738 c 4.210686,-5.05087 9.400295,-9.002037 15.394497,-11.98141 4.130239,-2.058713 9.42712,-3.094458 12.565028,-6.303996 0,0 10.553539,-14.960785 10.553539,-14.960785 C 63.451439,62.990124 65.597026,62.491431 75.667815,53.668402 75.305748,60.816334 67.206202,67.887544 62.298194,72.848908 z M 319.67338,235.09037 c -24.70095,19.39787 -61.69871,14.51325 -80.49931,-10.07617 -10.80833,-14.11685 -13.90599,-33.02882 -9.9501,-49.86928 4.10341,-17.4415 15.52862,-33.46357 32.84071,-41.04629 4.46548,-1.9564 18.96153,-6.90497 23.14539,-3.72101 2.0517,1.56001 2.0517,4.16856 2.33332,6.40629 -7.79114,0.66493 -13.51713,2.6597 -20.11477,6.71317 -14.61674,8.96368 -25.37143,24.97302 -25.37143,41.87744 0,25.56119 26.22965,51.51881 53.53212,49.63913 20.15498,-1.3938 25.35799,-8.8486 40.22951,-18.95034 -4.46547,7.91515 -8.81026,13.27291 -16.14544,19.02706 z m 17.48645,-21.58446 c 0,0 -1.34101,1.27869 -1.34101,1.27869 0,0 0,-1.27869 0,-1.27869 0,0 1.34101,0 1.34101,0 z\"\n"
    "       style=\"fill:#ffffff;stroke:#ffffff;stroke-width:1.30947196\" />\n"
    "    <path\n"
    "       d=\"m 187.74022,296.72683 c 0,0 -2.87883,0 -2.87883,0 0,0 -7.19709,0.45789 -7.19709,0.45789 0,0 -3.83843,-0.41496 -3.83843,-0.41496 -28.29415,-1.93169 -59.30398,-10.50266 -82.52658,-27.11041 -32.23334,-23.04193 -48.69547,-58.56592 -50.39878,-97.41909 0,0 -0.4606,-5.24655 -0.4606,-5.24655 -0.17274,-14.84777 2.16391,-34.07403 6.88519,-48.17297 4.33745,-12.93993 10.08552,-24.916385 18.49172,-35.771995 8.29584,-10.717298 14.76361,-14.313574 21.52409,-27.663685 0,0 14.12546,-27.186724 14.12546,-27.186724 1.08917,-1.774291 4.48618,-8.384958 5.82484,-9.191019 1.36266,-0.825141 2.61976,0.300485 3.68012,1.092238 1.70811,1.268714 4.04955,3.391187 6.23748,3.596279 2.52858,0.233711 4.37582,-1.779061 5.73847,-3.605817 0.83485,-1.111319 1.70811,-2.914226 3.39702,-2.303718 1.06037,0.381568 2.9652,2.408649 3.81926,3.257637 2.57175,2.551737 8.21907,8.971618 11.51533,9.744293 2.61494,0.61528 4.90841,-1.006385 6.71248,-1.321179 2.69171,-0.46742 4.48138,2.589893 5.82964,4.454807 0,0 5.89682,8.585281 5.89682,8.585281 0,0 6.34782,8.585278 6.34782,8.585278 0,0 9.43778,14.308804 9.43778,14.308804 8.58851,12.806378 16.42374,23.566595 20.74679,38.63375 2.58135,8.99547 2.7109,15.14349 2.60534,24.32497 -0.0624,5.04624 -2.01519,15.36765 -3.15712,20.50928 -3.59855,16.20235 -9.09712,32.00879 -10.32063,48.64994 0,0 -0.43661,4.76958 -0.43661,4.76958 0,0 0,6.67745 0,6.67745 0.0288,17.99092 8.46856,39.35874 18.08387,54.37343 0,0 12.56611,17.64753 12.56611,17.64753 0,0 6.77485,8.58529 6.77485,8.58529 -7.92639,3.45795 -26.32693,7.04947 -35.02581,7.15439 z\"\n"
    "       style=\"fill:#93d200;fill-opacity:1;stroke:#000000;stroke-width:0.47838068;stroke-opacity:0\" />\n"
    "  </svg>\n"
    "  <!-- SVG ends here -->\n"
    "\n"
    "</body>\n"
    "</html>\n"
    "";
};
