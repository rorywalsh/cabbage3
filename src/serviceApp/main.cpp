#include <iostream>
#include <rtaudio.h>
#include <atomic>
#include <thread>
#include <chrono>
#include "TestProcessor.h"

class AudioApp {
public:
    AudioApp() : isRunning(false), processor(2, 2) {
        // Initialize RtAudio
        audio = std::make_unique<RtAudio>();

        // Check if audio devices are available
        if (audio->getDeviceCount() < 1) {
            std::cerr << "No audio devices found!" << std::endl;
            return;
        }

        // Set up stream parameters
        RtAudio::StreamParameters parameters;
        parameters.deviceId = audio->getDefaultOutputDevice();
        parameters.nChannels = 2; // Stereo
        parameters.firstChannel = 0;

        unsigned int sampleRate = 44100;
        unsigned int bufferFrames = 512; // 512 sample frames

        try {
            // Open the audio stream
            audio->openStream(&parameters, nullptr, RTAUDIO_FLOAT32, sampleRate,
                              &bufferFrames, &AudioApp::audioCallback, this); // Pass 'this' as userData
            audio->startStream();
            isRunning = true; // Mark the stream as running
        } catch (const std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
    }

    ~AudioApp() {
        if (isRunning) {
            try {
                audio->stopStream();
            } catch (const std::runtime_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
            if (audio->isStreamOpen()) {
                audio->closeStream();
            }
        }
    }

    bool isStreamRunning() const {
        return isRunning;
    }

private:
    std::unique_ptr<RtAudio> audio;
    TestProcessor processor; // Member variable
    std::atomic<bool> isRunning; // Flag to track stream state

    // Static callback function
    static int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                             double streamTime, RtAudioStreamStatus status, void* userData) {
        // Cast userData to AudioApp*
        AudioApp* app = static_cast<AudioApp*>(userData);

        // Access the TestProcessor object
//        app->processor.process(static_cast<float*>(outputBuffer), nBufferFrames);

        return 0;
    }
};

int main() {
    AudioApp app; // Create an instance of AudioApp

    // Keep the program running while the stream is active
    while (app.isStreamRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep to avoid busy-waiting
    }

    return 0;
}
