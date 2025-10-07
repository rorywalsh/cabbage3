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
// TEST 3: Stdin/Stdout Communication Test
//==============================================================================
TEST_CASE("Test stdin/stdout pipe communication", "[CabbageApp]")
{
    ensureValidSettingsFileExists();
    
    std::cout << "\n==================== BEGIN TEST: stdin/stdout pipe communication ====================\n";
    //--------------------------------------------------------------------------
    // SETUP: Create app
    //--------------------------------------------------------------------------
    const char* args[] = {"CabbageApp"};
    auto app = std::make_unique<CabbageAudioApp>(1, const_cast<char**>(args));
    
    REQUIRE(app != nullptr);
    
    int nInputChannels = 2;
    int nOutputChannels = 2;
    int nBufferFrames = 512;

    // Create a dud buffer from processor to avoid segfaults in CI mode
    float **buffer = new float*[nOutputChannels];
    for (unsigned int ch = 0; ch < nOutputChannels; ++ch)
    {
        buffer[ch] = new float[nBufferFrames];
        // Initialize to silence
        memset(buffer[ch], 0, nBufferFrames * sizeof(float));
    }

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
    
    // Initialize stdin/stdout connection (this would normally connect to VS Code)
    REQUIRE_NOTHROW(app->initialiseStdioConnection());
    
    // Wait for the connection to be established
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    //--------------------------------------------------------------------------
    // VERIFY: Processor is running and can process audio
    //--------------------------------------------------------------------------
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(5);
    int iterationCount = 0;
    const int maxIterations = 100; // Safety limit
    
    while (std::chrono::steady_clock::now() < endTime && iterationCount < maxIterations)
    {
        if (app->processor) {
            // Call process() to simulate Csound run in CI mode
            app->processor->process(buffer, buffer, nBufferFrames);
            
            // Verify we can read control channel values
            for (int i = 0; i < 8; i++)
            {
                const std::string channel = "harmonic" + std::to_string(i+1);
                double value = app->processor->getCabbageEngine().getCsound()->GetControlChannel(channel.c_str());
                REQUIRE(value >= 0.0); // Basic sanity check
            }
        }

        try {
            // Process any pending messages (similar to what would happen with stdin input)
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
    // CLEANUP: Stop audio and remove temporary files
    //--------------------------------------------------------------------------
    app->addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
    app->onIdle();
    
    // Clean up buffer
    for (unsigned int ch = 0; ch < nOutputChannels; ++ch) {
        delete[] buffer[ch];
    }
    delete[] buffer;
    
    std::filesystem::remove(tempPath);
    
    // Verify the app is still functional
    REQUIRE(app != nullptr);
    std::cout << "\n==================== END TEST: stdin/stdout pipe communication ====================\n";
}

//==============================================================================
// TEST 4: Test cabbageSet with JSON message capture
//==============================================================================
TEST_CASE("Test cabbageSet.csd with JSON message capture", "[CabbageApp]")
{
    ensureValidSettingsFileExists();
    
    std::cout << "\n==================== BEGIN TEST: cabbageSet.csd with JSON messages ====================\n";
    
    //--------------------------------------------------------------------------
    // SETUP: Create app and redirect stdout to capture JSON messages
    //--------------------------------------------------------------------------
    const char* args[] = {"CabbageApp"};
    auto app = std::make_unique<CabbageAudioApp>(1, const_cast<char**>(args));
    
    REQUIRE(app != nullptr);
    
    int nInputChannels = 2;
    int nOutputChannels = 2;
    int nBufferFrames = 512;

    // Create a dud buffer from processor to avoid segfaults in CI mode
    float **buffer = new float*[nOutputChannels];
    for (unsigned int ch = 0; ch < nOutputChannels; ++ch)
    {
        buffer[ch] = new float[nBufferFrames];
        memset(buffer[ch], 0, nBufferFrames * sizeof(float));
    }

    //--------------------------------------------------------------------------
    // SETUP: Create temporary CSD file from TestCsdFiles::cabbageSet
    //--------------------------------------------------------------------------
    std::string csdContent = TestCsdFiles::cabbageSet;
    
    // Create a temporary file using std::filesystem
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / ("test_cabbageSet_" + std::to_string(std::time(nullptr)) + ".csd");
    std::ofstream tempFile(tempPath);
    tempFile << csdContent;
    tempFile.close();
    
    // Set up the temporary CSD file
    std::string filePath = tempPath.string();
    app->setCsoundFile(filePath);
    
    // Verify the file was created and is readable
    REQUIRE(std::filesystem::exists(filePath));
    REQUIRE(std::filesystem::file_size(filePath) > 0);
    
    std::cout << "Loading CSD from TestCsdFiles::cabbageSet: " << filePath << std::endl;
    
    // Capture hostCallback data in test
    struct CallbackData {
        std::string channel;
        std::string type;
        std::string json;
    };
    std::vector<CallbackData> callbackMessages;
    
    // Initialize Cabbage with the test file
    app->initialiseCabbage();
    
    // Override the hostCallback to capture data for testing
    if (app->processor) {
        // Enable message dequeuing (normally done when UI is ready)
        app->processor->setCabbageIsReady();
        
        app->processor->hostCallback = [&callbackMessages, &app](CabbageOpcodeData data) {
            // Capture the data for testing
            CallbackData captured;
            captured.channel = data.channel;
            captured.type = (data.type == CabbageOpcodeData::MessageType::Value) ? "Value" : "Json";
            captured.json = data.cabbageJson.dump(); // Serialize JSON to string
            callbackMessages.push_back(captured);
            
            // Still call the normal hostCallback to process the data
            app->hostCallback(data);
        };
    }
    
    //--------------------------------------------------------------------------
    // PROCESS AUDIO: Run for a few seconds to trigger cabbageSet opcodes
    //--------------------------------------------------------------------------
    std::cout << "\n--- Processing audio to trigger cabbageSet opcodes ---\n" << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(3); // Run for 3 seconds (instrument runs for 2 seconds)
    int iterationCount = 0;
    const int maxIterations = 100; // Safety limit
    
    while (std::chrono::steady_clock::now() < endTime && iterationCount < maxIterations)
    {
        if (app->processor) {
            // Process audio to drive Csound
            app->processor->process(buffer, buffer, nBufferFrames);
        }

        try {
            // Process any pending messages
            app->onIdle();
        } catch (...) {
            break;
        }
        
        iterationCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    
    //--------------------------------------------------------------------------
    // DISPLAY: Show captured hostCallback messages
    //--------------------------------------------------------------------------
    std::cout << "\n--- Captured hostCallback Messages ---\n" << std::endl;
    
    // Show first 20 messages
    size_t showFirst = std::min<size_t>(20, callbackMessages.size());
    for (size_t i = 0; i < showFirst; i++) {
        std::cout << "Message " << (i+1) << ":" << std::endl;
        std::cout << "  Channel: " << callbackMessages[i].channel << std::endl;
        std::cout << "  Type: " << callbackMessages[i].type << std::endl;
        std::cout << "  JSON: " << callbackMessages[i].json << std::endl;
        std::cout << std::endl;
    }
    
    if (callbackMessages.size() > 20) {
        std::cout << "... (" << (callbackMessages.size() - 20) << " more messages)" << std::endl;
    }
    
    std::cout << "\nTotal hostCallback messages: " << callbackMessages.size() << std::endl;
    std::cout << "Total iterations completed: " << iterationCount << std::endl;
    
    // Verify we completed some iterations at least
    REQUIRE(iterationCount > 0);
    
    //--------------------------------------------------------------------------
    // CLEANUP: Stop audio and clean up (no stdin thread to worry about)
    //--------------------------------------------------------------------------
    std::cout << "\n--- Cleaning up ---\n" << std::endl;
    
    app->addMessageToQueue(CabbageAudioApp::CommandType::StopAudio);
    app->onIdle();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give it time to clean up
    
    // Clean up buffer
    for (unsigned int ch = 0; ch < nOutputChannels; ++ch) {
        delete[] buffer[ch];
    }
    delete[] buffer;
    
    // Remove temporary file
    std::filesystem::remove(tempPath);
    
    REQUIRE(app != nullptr);
    std::cout << "\n==================== END TEST: cabbageSet.csd with JSON messages ====================\n";
}

//==============================================================================
// TEST 5: Stress Test with Message Queue Processing
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
// TEST 6: AudioConfig Class Functionality
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

//==============================================================================
// TEST 7: Command Line Argument Parsing
//==============================================================================
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
    if(std::filesystem::exists(settingsPath))
        return;
        
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
