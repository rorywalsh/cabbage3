#include "CabbageAudioApp.h"
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

// Static pointer to the CabbageAudioApp instance
static CabbageAudioApp* appInstance = nullptr;
volatile sig_atomic_t terminateRequested = 0;

// Signal handler for Unix-like systems
void signalHandler(int signal) 
{
    if (appInstance) {
        try {
            lattice::logInfo << "Received signal " << signal << ". Cleaning up...";
            terminateRequested = 1;
            delete appInstance;
            appInstance = nullptr;
        } catch (...) {
            std::cerr << "Error during cleanup" << std::endl;
        }
    }
    std::_Exit(signal); // Use _Exit to avoid re-entering destructors
}

#ifdef _WIN32
// Console handler for Windows
BOOL WINAPI consoleHandler(DWORD signal) 
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        if (appInstance) {
            std::cout << "Received console event. Cleaning up..." << std::endl;
            delete appInstance; // Call the destructor to clean up
            appInstance = nullptr;
        }
        std::exit(signal); // Exit the program
    }
    return TRUE;
}
#endif

int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGABRT, signalHandler);
    bool testRtAudioStartStop = false;

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#endif
    
    // Create an instance of CabbageAudioApp
    appInstance = new CabbageAudioApp(argc, argv);
    
    // Scan audio devices and initialize Cabbage
    appInstance->scanAudioDevices();
    appInstance->initialiseCabbage();
    appInstance->initialiseWebSocketConnection();

    //simple test for start/stop/compile/destroy
    if(testRtAudioStartStop)
    {
        appInstance->setCsoundFile("../../test.csd");
        appInstance->addMessageToQueue(CabbageAudioApp::CommandType::InitCabbage);
    }
    
    // Keep the program running until termination is requested
    while (!terminateRequested)
    {        
        appInstance->onIdle();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sleep to avoid busy-waiting
        
        if(testRtAudioStartStop)
        {
            appInstance->addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
            appInstance->addMessageToQueue(CabbageAudioApp::CommandType::KillProcessor);
            appInstance->addMessageToQueue(CabbageAudioApp::CommandType::InitCabbage);
        }

    }

    // Clean up
    delete appInstance;
    appInstance = nullptr;

    return 0;
}
