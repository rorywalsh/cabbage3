#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "../src/CabbageAudioApp/CabbageAudioApp.h"
#include <memory>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include "TestCsdFiles.h"

// Forward declaration of function to ensure valid settings file exists
void ensureValidSettingsFileExists();

//==============================================================================
// TEST 1: Basic CabbageApp Construction and Core Functionality
//==============================================================================
TEST_CASE("Basic CabbageApp construction and functionality", "[CabbageApp]")
{
    ensureValidSettingsFileExists();
    std::cout << "\n==================== BEGIN TEST: Basic CabbageApp construction and functionality ====================\n";
    // Create app with minimal arguments
    const char* args[] = {"CabbageApp", "--file", "test.csd"};
    CabbageAudioApp app(3, const_cast<char**>(args));
    
    REQUIRE(&app != nullptr);
    

    //--------------------------------------------------------------------------
    // SECTION 1.1: Audio Device Scanning
    //--------------------------------------------------------------------------
    SECTION("Audio device scanning")
    {
        REQUIRE_NOTHROW(app.scanAudioDevices());
    }
    
    //--------------------------------------------------------------------------
    // SECTION 1.2: CSound File Management
    //--------------------------------------------------------------------------
    SECTION("Set Csound file")
    {
        REQUIRE_NOTHROW(app.setCsoundFile("test.csd"));
    }
    
    //--------------------------------------------------------------------------
    // SECTION 1.3: JSON Settings Loading
    //--------------------------------------------------------------------------
    SECTION("Load from JSON settings")
    {
        // Create a valid settings file for testing
        std::string validSettings = R"({
            "currentConfig": {
                "audio": {
                    "driver": 0,
                    "inputDevice": "Built-in Input",
                    "outputDevice": "Built-in Output",
                    "in1": 1,
                    "in2": 2,
                    "out1": 1,
                    "out2": 2,
                    "bufferSize": 512,
                    "sr": 44100
                },
                "midi": {
                    "inputDevice": "no input",
                    "outputDevice": "no output",
                    "inChan": 0,
                    "outChan": 0
                },
                "jsSourceDir": "add path to JS src directory"
            }
        })";
        
        std::ofstream validFile("valid_settings.json");
        validFile << validSettings;
        validFile.close();
        
        REQUIRE(app.audioConfig.loadFromJson("valid_settings.json"));
        REQUIRE(!app.audioConfig.loadFromJson("non_existent_file.json"));
        
        // Clean up
        std::filesystem::remove("valid_settings.json");
    }
    
    //--------------------------------------------------------------------------
    // SECTION 1.4: Message Queue Operations
    //--------------------------------------------------------------------------
    SECTION("Message queue operations")
    {
        app.addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
        REQUIRE(app.getMessageQueueSize() == 1);
    }
    
    //--------------------------------------------------------------------------
    // SECTION 1.5: Idle Processing
    //--------------------------------------------------------------------------
    SECTION("Idle processing")
    {
        REQUIRE_NOTHROW(app.onIdle());
    }
    std::cout << "\n==================== END TEST: Basic CabbageApp construction and functionality ====================\n";
}

//==============================================================================
// TEST 2: Audio Device Scanning (Standalone Test)
//==============================================================================
TEST_CASE("Audio device scanning", "[CabbageApp]")
{
    ensureValidSettingsFileExists();
    
    std::cout << "\n==================== BEGIN TEST: Audio device scanning ====================\n";
    const char* args[] = {"CabbageApp"};
    

    
    CabbageAudioApp app(1, const_cast<char**>(args));
    
    REQUIRE_NOTHROW(app.scanAudioDevices());
    std::cout << "\n==================== END TEST: Audio device scanning ====================\n";
}

//==============================================================================
// TEST 3: Test Server Functionality
//==============================================================================
TEST_CASE("Test WebSocket Server functionality", "[CabbageApp]")
{
    ensureValidSettingsFileExists();
    
    std::cout << "\n==================== BEGIN TEST: Test WebSocket Server functionality ====================\n";
    //--------------------------------------------------------------------------
    // SETUP: Create app with test server enabled
    //--------------------------------------------------------------------------
    const char* args[] = {"CabbageApp", "--startTestServer", "true"};
    auto app = std::make_unique<CabbageAudioApp>(3, const_cast<char**>(args));
    
    REQUIRE(app != nullptr);
    


    //--------------------------------------------------------------------------
    // SETUP: Create temporary CSD file for testing
    //--------------------------------------------------------------------------
    std::string csdContent = TestCsdFiles::rotarySliders;
    
    // Create a temporary file using std::filesystem
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / ("test_" + std::to_string(std::time(nullptr)) + ".csd");
    std::ofstream tempFile(tempPath);
    tempFile << csdContent;
    tempFile.close();
    
    // Set up the temporary CSD file
    std::string filePath = tempPath.string();
    app->setCsoundFile(filePath);
    
    // Verify the file was created and is readable
    REQUIRE(std::filesystem::exists(filePath));
    REQUIRE(std::filesystem::file_size(filePath) > 0);
    
    // Initialize Cabbage with the test file (this creates the processor)
    REQUIRE_NOTHROW(app->initialiseCabbage());
    
    // Now start test server (after processor is created)
    REQUIRE_NOTHROW(app->startWebSocketServerForTesting());
    REQUIRE(app->testServer != nullptr);
    app->testServer->setUpdateInterval(500);
    
    // Wait for the server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Verify test server is running
    REQUIRE(app->testServer != nullptr);
    REQUIRE(app->testServer->isRunning());
    
    // Set up WebSocket client connection to receive data from test server
    REQUIRE_NOTHROW(app->initialiseWebSocketConnection());
    
    // Give the WebSocket connection time to fully establish and register with the server
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    //--------------------------------------------------------------------------
    // VERIFY: Test server is running and can be stopped
    //--------------------------------------------------------------------------

    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(5);
    int iterationCount = 0;
    const int maxIterations = 100; // Safety limit
    
    while (std::chrono::steady_clock::now() < endTime && iterationCount < maxIterations)
    {
        // Process messages with timeout protection
        try {
            app->onIdle();
        } catch (...) {
            // If onIdle throws, break out of the loop
            break;
        }
        
        iterationCount++;
        
        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
        
    
    //--------------------------------------------------------------------------
    // CLEANUP: Stop test server and remove temporary files
    //--------------------------------------------------------------------------
    app->stopWebSocketServerForTesting();
    std::filesystem::remove(tempPath);
    
    // Verify the app is still functional
    REQUIRE(app != nullptr);
    std::cout << "\n==================== END TEST: Test server functionality ====================\n";
}

//==============================================================================
// TEST 4: Stress Test with Message Queue Processing
//==============================================================================
TEST_CASE("Stress test start/stop/destroy", "[CabbageApp]")
{
    ensureValidSettingsFileExists();
    
    std::cout << "\n==================== BEGIN TEST: Stress test start/stop/destroy ====================\n";
    //--------------------------------------------------------------------------
    // SETUP: Create app without test server
    //--------------------------------------------------------------------------
    ensureValidSettingsFileExists();
    const char* args[] = {"CabbageApp"};
    auto app = std::make_unique<CabbageAudioApp>(1, const_cast<char**>(args));
    
    REQUIRE(app != nullptr);
    
    //--------------------------------------------------------------------------
    // SETUP: Create temporary CSD file for testing
    //--------------------------------------------------------------------------
    std::string csdContent = TestCsdFiles::basicOscillator;
    
    // Create a temporary file using std::filesystem
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / ("test_" + std::to_string(std::time(nullptr)) + ".csd");
    std::ofstream tempFile(tempPath);
    tempFile << csdContent;
    tempFile.close();
    
    // Set up the temporary CSD file
    std::string filePath = tempPath.string();
    app->setCsoundFile(filePath);
    
    // Verify the file was created and is readable
    REQUIRE(std::filesystem::exists(filePath));
    REQUIRE(std::filesystem::file_size(filePath) > 0);
    
    // Initialize Cabbage with the test file (this creates the processor)
    REQUIRE_NOTHROW(app->initialiseCabbage());
    
    //--------------------------------------------------------------------------
    // STRESS TEST: Run 5-second continuous message cycling with timeout protection
    //--------------------------------------------------------------------------
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(5);
    int iterationCount = 0;
    const int maxIterations = 100; // Safety limit
    
    while (std::chrono::steady_clock::now() < endTime && iterationCount < maxIterations)
    {
        // Add messages to the queue - including StopAudio now that deadlock is fixed
        app->addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
        app->addMessageToQueue(CabbageAudioApp::CommandType::KillProcessor);
        app->addMessageToQueue(CabbageAudioApp::CommandType::InitCabbage);
        
        // Process messages with timeout protection
        try {
            app->onIdle();
        } catch (...) {
            // If onIdle throws, break out of the loop
            break;
        }
        
        iterationCount++;
        
        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Verify we completed some iterations
    REQUIRE(iterationCount > 0);
    
    //--------------------------------------------------------------------------
    // CLEANUP: Remove temporary files
    //--------------------------------------------------------------------------
    std::filesystem::remove(tempPath);
    
    // Verify the app is still functional after stress test
    REQUIRE(app != nullptr);
    std::cout << "\n==================== END TEST: Stress test start/stop/destroy ====================\n";
}

//==============================================================================
// TEST 5: AudioConfig Class Functionality
//==============================================================================
TEST_CASE("CabbageAudioApp AudioConfig functionality", "[CabbageAudioApp::AudioConfig]") {
    std::cout << "\n==================== BEGIN TEST: CabbageAudioApp AudioConfig functionality ====================\n";
    //--------------------------------------------------------------------------
    // SECTION 4.1: Default Constructor Values
    //--------------------------------------------------------------------------
    SECTION("AudioConfig can be created") {
        CabbageAudioApp::AudioConfig config;
        
        // Test default values
        REQUIRE(config.audioDriverType == 0);
        REQUIRE(config.audioInDev == "");
        REQUIRE(config.audioOutDev == "");
        REQUIRE(config.audioInChanL == 1);
        REQUIRE(config.audioInChanR == 2);
        REQUIRE(config.audioOutChanL == 1);
        REQUIRE(config.audioOutChanR == 2);
        REQUIRE(config.bufferSize == 512);
        REQUIRE(config.audioSR == 44100);
        REQUIRE(config.midiInDev == "");
        REQUIRE(config.midiOutDev == "");
        REQUIRE(config.midiInChan == 0);
        REQUIRE(config.midiOutChan == 0);
        REQUIRE(config.jsSourceDirectory == "");
    }
    
    //--------------------------------------------------------------------------
    // SECTION 4.2: JSON Loading - Error Cases
    //--------------------------------------------------------------------------
    SECTION("AudioConfig loadFromJson returns false for non-existent file") {
        CabbageAudioApp::AudioConfig config;
        
        REQUIRE(config.loadFromJson("non_existent_file.json") == false);
    }
    
    //--------------------------------------------------------------------------
    // SECTION 4.3: JSON Loading - Success Cases
    //--------------------------------------------------------------------------
    SECTION("AudioConfig loadFromJson returns true for valid settings file") {
        CabbageAudioApp::AudioConfig config;
        
        REQUIRE(config.loadFromJson(cabbage::File::getSettingsFile()) == true);
    }
    std::cout << "\n==================== END TEST: CabbageAudioApp AudioConfig functionality ====================\n";
}

TEST_CASE("CabbageAudioApp command line parsing", "[CabbageAudioApp]") {
    std::cout << "\n==================== BEGIN TEST: CabbageAudioApp command line parsing ====================\n";
    SECTION("CabbageAudioApp can parse file argument") {
        const char* argv[] = {"CabbageApp", "--file", "test.csd"};
        int argc = 3;
        ensureValidSettingsFileExists();
        auto app = std::make_unique<CabbageAudioApp>(argc, const_cast<char**>(argv));
        
        REQUIRE(app != nullptr);
    }
    
    SECTION("CabbageAudioApp can parse port number argument") {
        const char* argv[] = {"CabbageApp", "--portNumber", "8080"};
        int argc = 3;
        ensureValidSettingsFileExists();
        auto app = std::make_unique<CabbageAudioApp>(argc, const_cast<char**>(argv));
        
        REQUIRE(app != nullptr);
    }
    std::cout << "\n==================== END TEST: CabbageAudioApp command line parsing ====================\n";
}

void ensureValidSettingsFileExists() {
    std::string settingsPath = cabbage::File::getSettingsFile();
    std::filesystem::path settingsFilePath(settingsPath);
    std::filesystem::path parentDir = settingsFilePath.parent_path();

    // Path to widgets directory
    std::string widgetsDir = "/Users/runner/work/cabbage3/cabbage3/vscabbage/src/cabbage/widgets";
    if (!std::filesystem::exists(widgetsDir)) {
        std::cerr << "Warning: widgets directory does not exist: " << widgetsDir << std::endl;
        return;
    }

    // Create parent directories if they don't exist
    std::error_code ec;
    std::filesystem::create_directories(parentDir, ec);

    std::string validSettings = R"({
        "currentConfig": {
            "audio": {
                "driver": 0,
                "inputDevice": "Built-in Input",
                "outputDevice": "Built-in Output",
                "in1": 1,
                "in2": 2,
                "out1": 1,
                "out2": 2,
                "bufferSize": 512,
                "sr": 44100
            },
            "midi": {
                "inputDevice": "no input",
                "outputDevice": "no output",
                "inChan": 0,
                "outChan": 0
            },
            "jsSourceDir": "/Users/runner/work/cabbage3/cabbage3/vscabbage/src"
        }
    })";
    std::ofstream validFile(settingsPath);
    validFile << validSettings;
    validFile.close();
} 
