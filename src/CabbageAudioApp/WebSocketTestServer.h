
#pragma once
#include "platform/choc_DisableAllWarnings.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include "platform/choc_ReenableAllWarnings.h"
#include <random>
#include "../CabbageProcessor.h"
#include <text/choc_StringUtilities.h>
//==============================================================================
// WebSocket Test Server with configurable test data generation
//
// To run this, pass the following command line to CabbageApp
//
//  --file="../../tests/rotarySlider.csd" --portNumber=9991 --startTestServer=True
//==============================================================================

#include <map>
#include <vector>
#include <random>
#include <algorithm>

class NoteGenerator {
private:
    std::map<int16_t, int32_t> activeNotes; // Tracks {key: noteId}
    std::vector<int16_t> noteSequence;      // Fixed note sequence
    size_t sequenceIndex = 0;               // Current position in sequence
    int targetPolyphony = 3;                // Desired overlapping notes
    std::mt19937 gen{std::random_device{}()};

public:
    // Initialize with a specific note sequence
    NoteGenerator() 
    {
        noteSequence = {60, 62, 64, 65, 67, 69, 71, 72}; 
    }

    // Optionally set a custom sequence
    void setSequence(const std::vector<int16_t>& sequence) {
        noteSequence = sequence;
        sequenceIndex = 0;
    }

    lattice::NoteEvent generateNoteEvent() 
    {
        std::uniform_real_distribution<double> velDist(0.3, 1.0);

        // Force noteOff if too many active notes
        if (activeNotes.size() >= targetPolyphony) 
        {
            return generateNoteOff();
        }

        // Generate next note in sequence
        int16_t key = noteSequence[sequenceIndex];
        sequenceIndex = (sequenceIndex + 1) % noteSequence.size();

        // If key is active, release it first
        if (activeNotes.count(key)) 
        {
            auto noteOff = generateNoteOff(key);
            activeNotes.erase(key);
            return noteOff;
        }

        // Create new noteOn
        int32_t noteId = static_cast<int32_t>(gen());
        activeNotes[key] = noteId;
        return {
            lattice::NoteEvent::Type::noteOn,
            key,
            velDist(gen),
            noteId,
            0 
        };
    }

private:
    lattice::NoteEvent generateNoteOff(int16_t specificKey = -1) 
    {
        if (activeNotes.empty()) 
        {
            return generateNoteEvent(); // Fallback if no active notes
        }

        // Release oldest note if no key specified
        auto it = (specificKey != -1)
            ? activeNotes.find(specificKey)
            : activeNotes.begin();
        
        if (it == activeNotes.end()) 
        {
            it = activeNotes.begin();
        }

        int16_t key = it->first;
        int32_t noteId = it->second;
        activeNotes.erase(it);

        return {
            lattice::NoteEvent::Type::noteOff,
            key,
            0.0, // Velocity 0 for noteOff
            noteId,
            0
        };
    }
};

class WebSocketTestServer
{
public:
    struct ControlInfo
    {
        std::string type;
        std::string channel;
        int paramIdx;
        struct Range
        {
            float min;
            float max;
            float value;
            float skew;
            float increment;
        } range;
        
    };

    // Disable 'repeatable' to test with truly random values
    WebSocketTestServer(CabbageProcessor &p, int port, bool repeatable)
        : processor(p),
          portNumber(port),
          serverRunning(false)
    {
        setDeterministicMode(repeatable);
    }

    ~WebSocketTestServer()
    {
        stop();
    }

    void setDeterministicMode(bool enable, uint64_t seed = 12345) 
    {
        useDeterministicValues = enable;
        testSeed = seed;
        deterministicGenerator.seed(testSeed);
    }
    
    void initialise()
    {
        std::lock_guard<std::mutex> lock(controlsMutex);
        controls.clear();
        
        // This is where we get the controls from the Cabbage engine
        // and use them to create test data for our WebSocket server
        for (const auto& widget : processor.getCabbageEngine().getWidgets())
        {
            try
            {
                // Only add those controls that are marked as automatable
                if (widget.contains("automatable") && widget["automatable"].get<int>() == 1)
                {
                    ControlInfo control;
                    control.type = widget["type"].get<std::string>();
                    control.channel = widget["channel"].get<std::string>();
                    const int paramIdx = widget["parameterIndex"].get<int>();

                    if (widget.contains("range"))
                    {
                        const auto &range = widget["range"];
                        control.range.min = range.value("min", 0.0f);
                        control.range.max = range.value("max", 1.0f);
                        control.range.value = range.value("defaultValue", 0.0f);
                        control.range.skew = range.value("skew", 1.0f);
                        control.range.increment = range.value("increment", 0.001f);
                    }

                    // We need to create a range for a comboBox 
                    else if (control.type == "comboBox")
                    {
                        const auto &items = choc::text::splitString(widget["items"].get<std::string>(), ',', false);
                        control.range.min = 0;
                        control.range.max = items.size() - 1;
                        control.range.value = 0.0f;
                        control.range.skew = 1;
                        control.range.increment = 1;

                    }
                    // All other controls emit a 0 or 1
                    else
                    {
                        control.range.min = 0;
                        control.range.max = 1;
                        control.range.value = 0.0f;
                        control.range.skew = 1;
                        control.range.increment = 1;
                    }

                    control.paramIdx = paramIdx;
                    controls.push_back(control);
                }
            }
            catch (const nlohmann::json::exception& e)
            {
                // Log error but continue processing other widgets
                lattice::logError << "Error processing widget: " << e.what();
            }
        }

        lattice::logDebug << "Configured " << controls.size() << " controls";
    }

    bool start()
    {
        if (serverRunning)
        {
            return false;
        }

        serverRunning = true;
        serverThread = std::thread(&WebSocketTestServer::runServer, this);
        return true;
    }

    void stop()
    {
        serverRunning = false;
        if (serverThread.joinable())
        {
            serverThread.join();
        }
    }

    void setUpdateInterval(int intervalMs)
    {
        updateInterval = std::chrono::milliseconds(intervalMs);
    }

    bool isRunning(){   return serverRunning;   }
    
    // Enable to send MIDI note during testing
    void testMidi(bool shouldTest){ shouldTestMidi = shouldTest;  }
    
private:
    void runServer()
    {
        ix::WebSocketServer server(portNumber);
        
        server.setOnClientMessageCallback(
            [this](std::shared_ptr<ix::ConnectionState> connectionState,
                   ix::WebSocket& webSocket,
                   const ix::WebSocketMessagePtr& msg)
            {
                if (msg->type == ix::WebSocketMessageType::Message)
                {
//                    lattice::logDebug << "Server received: " << msg->str;
                }
            }
        );

        auto result = server.listen();
        if (result.first)
        {
            lattice::logDebug << "WebSocket Test Server listening on port " << portNumber;
            server.start();
            
            while (serverRunning)
            {
                auto startTime = std::chrono::steady_clock::now();
                
                sendTestData(server);
                
                auto endTime = std::chrono::steady_clock::now();
                auto elapsedTime = endTime - startTime;
                auto sleepTime = updateInterval - elapsedTime;
                
                if (sleepTime > std::chrono::milliseconds(0))
                {
                    std::this_thread::sleep_for(sleepTime);
                }
            }
            
            server.stop();
        }
        else
        {
            lattice::logDebug << "Failed to start WebSocket Server on port " << portNumber;
            lattice::logDebug << "Error: " << result.second;
        }
    }

    void sendTestData(ix::WebSocketServer& server)
    {
        std::lock_guard<std::mutex> lock(controlsMutex);
        auto messages = generateTestData();
        
        for (auto&& client : server.getClients())
        {
            for (const auto& message : messages)
            {
                //lattice::logInfo << message.dump(4);
                client->send(message.dump());
                
            }
            if(shouldTestMidi)
            {
                if(testPacketCount % 2 == 0)
                {
                    
                    processor.addNoteEvent(generator.generateNoteEvent());
                }
                
                testPacketCount++;
            }
        }
    }

    
    std::vector<nlohmann::json> generateTestData()
    {
        std::vector<nlohmann::json> messages;
        float random = 0.f;
        
        if (useDeterministicValues) 
        {
           // Deterministic sequence
           std::uniform_real_distribution<float> dist(0.0f, 1.0f);
           random = dist(deterministicGenerator);
        }
        else
        {
           // Truly random
           random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }
        
        for (auto& control : controls)
        {
            float skewedValue = control.range.min +
                                (control.range.max - control.range.min) *
                                pow(random, control.range.skew);
                
            if (control.range.increment > 0)
            {
                // Correct stepping implementation:
                float steps = round((skewedValue - control.range.min) / control.range.increment);
                skewedValue = control.range.min + (steps * control.range.increment);
                    
                // Clamp to ensure we stay within bounds
                skewedValue = std::clamp(skewedValue, control.range.min, control.range.max);
            }
                
            control.range.value = skewedValue;
                
            nlohmann::json message =
            {
                {"command", "parameterChange"},
                {"obj",
                    {
                        {"paramIdx", control.paramIdx},
                        {"channel", control.channel},
                        {"value", skewedValue},
                        {"channelType", "number"}
                    }
                }
            };
                
            messages.push_back(message);
        }
        
        return messages;
    }
    
    int portNumber;
    bool serverRunning;
    std::thread serverThread;
    std::vector<ControlInfo> controls;
    std::mutex controlsMutex;
    std::chrono::milliseconds updateInterval{1000};
    std::mt19937_64 deterministicGenerator; // Mersenne Twister engine
    bool useDeterministicValues = false;
    uint64_t testSeed = 0;
    bool noteOn = true;
    bool shouldTestMidi = false;
    CabbageProcessor &processor;
    NoteGenerator generator;
    int testPacketCount = 0;
};

