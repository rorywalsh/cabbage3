#include "CabbageAudioApp.h"
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

// Static pointer to the CabbageAudioApp instance
static CabbageAudioApp* appInstance = nullptr;
static std::atomic<bool> terminateRequested = false;
static std::atomic<bool> shutdownInProgress = false; // Prevent double shutdown
std::mutex shutdownMutex;

// Signal handler for Unix-like systems
void signalHandler(int signal) 
{
    lattice::logInfo << "Received signal " << signal << ". Cleaning up...";
    terminateRequested = true;
    // Do not delete appInstance or call _Exit here!
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

    // Robust, idempotent shutdown
    {
        std::lock_guard<std::mutex> lock(shutdownMutex);
        if (!shutdownInProgress && appInstance) {
            shutdownInProgress = true;
            
            // CRITICAL: Stop audio stream before deleting to prevent race condition
            // This ensures the audio callback stops running before the destructor
            appInstance->closeAudioDevice();
            
            // Give the audio callback a moment to finish any pending operations
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            delete appInstance;
            appInstance = nullptr;
        }
    }

    return 0;
}
