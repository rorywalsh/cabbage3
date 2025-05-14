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

// Signal handler for Unix-like systems
void signalHandler(int signal) 
{
    if (appInstance) {
        try {
            lattice::logInfo << "Received signal " << signal << ". Cleaning up...";
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

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else 
        std::signal(SIGKILL, signalHandler);
#endif
    
    // Create an instance of CabbageAudioApp
    appInstance = new CabbageAudioApp(argc, argv);

    // Keep the program running while the stream is active
    while (appInstance->isStreamRunning() || appInstance->isRunningInDebugMode())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep to avoid busy-waiting
    }

    // Clean up
    delete appInstance;
    appInstance = nullptr;

    return 0;
}
