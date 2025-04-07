
#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include <random>
//==============================================================================
// WebSocket Test Server with configurable test data generation
//
// To run this, pass the following command line to CabbageApp
//
//  --file="../../tests/rotarySlider.csd" --portNumber=9991 --startTestServer=True
//==============================================================================

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
    WebSocketTestServer(int port, bool repeatable)
        : portNumber(port),
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
    
    void initialise(const std::vector<nlohmann::json>& widgets)
    {
        std::lock_guard<std::mutex> lock(controlsMutex);
        controls.clear();
        
        for (const auto& widget : widgets)
        {
            try
            {
                const std::string type = widget["type"].get<std::string>();
                // Skip if not a control we care about
                if (type != "rotarySlider" && type != "verticalSlider" && type != "horizontalSlider")
                {
                    continue;
                }

                
                const std::string channel = widget["channel"].get<std::string>();
                lattice::logInfo << widget.dump(4);
                const int paramIdx = widget["parameterIndex"].get<int>();

                // Sliders define a range object, other widgets don't..
                if (type == "rotarySlider" || type == "verticalSlider" || type == "horizontalSlider")
                {
                    if (!widget.contains("range"))
                    {
                        continue;
                    }

                    ControlInfo control;
                    control.type = type;
                    control.channel = channel;
                    
                    const auto& range = widget["range"];
                    control.range.min = range.value("min", 0.0f);
                    control.range.max = range.value("max", 1.0f);
                    control.range.value = range.value("value", 0.0f);
                    control.range.skew = range.value("skew", 1.0f);
                    control.range.increment = range.value("increment", 0.001f);
                    control.paramIdx = paramIdx;
                    controls.push_back(control);
                }
                // todo - add other types
                else if (type == "checkBox")
                {
                    
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
                client->send(message.dump());
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
            if (control.type == "rotarySlider" || control.type == "linearSlider")
            {
                float skewedValue = control.range.min + 
                                   (control.range.max - control.range.min) * 
                                   pow(random, control.range.skew);
                
                if (control.range.increment > 0)
                {
                    float steps = (control.range.max - control.range.min) / control.range.increment;
                    skewedValue = round(skewedValue * steps) / steps;
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
};

