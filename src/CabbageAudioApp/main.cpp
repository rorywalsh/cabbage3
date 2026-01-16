/*
 * Copyright (c) 2024 Rory Walsh
 *
 * This file is part of Cabbage3
 *
 * Cabbage3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cabbage3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cabbage3.  If not, see <https://www.gnu.org/licenses/>.
 */

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


void signalHandler(int signal)
{
    lattice::logInfo << "Received signal " << signal << ". Cleaning up...";
    if (appInstance) {
        try {
            terminateRequested = 1;
            // Send both commands needed for clean shutdown
            appInstance->addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
            appInstance->addMessageToQueue(CabbageAudioApp::CommandType::KillProcessor);
        } catch (...) {
            std::cerr << "Error during cleanup" << std::endl;
        }
    }
}

#ifdef _WIN32
// Console handler for Windows
BOOL WINAPI consoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        if (appInstance) {
            std::cout << "Received console event. Cleaning up..." << std::endl;
            terminateRequested = 1;
            appInstance->addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
            // Don't wait here - let the main thread handle the cleanup
            // This prevents the race condition between signal handler and main thread
        }
        // Don't call exit here - let the main thread exit naturally
        // This allows proper cleanup of resources
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
#endif
    
    // Create an instance of CabbageAudioApp
    appInstance = new CabbageAudioApp(argc, argv);
    
    // Scan audio devices and initialize Cabbage
    appInstance->scanAudioDevices();
    appInstance->initialiseCabbage();
    appInstance->initialiseStdioConnection();

   
    // Keep the program running until termination is requested
    while (!terminateRequested)
    {
        appInstance->onIdle();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sleep to avoid busy-waiting
    }

    // Clean up
    delete appInstance;
    appInstance = nullptr;

    return 0;
}
