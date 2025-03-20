#ifndef CABBAGEAUDIOAPP_H
#define CABBAGEAUDIOAPP_H

#include <RtAudio.h>
#include <RtMidi.h>

#include <atomic>
#include <memory>
#include <vector>
#include "CabbageProcessor.h"

class CabbageAudioApp {
public:
    CabbageAudioApp();
    ~CabbageAudioApp();

    bool isStreamRunning() const;
    static void errorCallback(RtAudioErrorType type, const std::string& errorText);

    MultiChannelProcessor processor; // Main processor
    
private:
    void initialiseAudio();
    void initialiseMidi();
    
    std::unique_ptr<RtAudio> audio = nullptr;
    std::unique_ptr<RtMidiIn> midiIn = nullptr;
    std::unique_ptr<RtMidiOut> midiOut = nullptr;
    int midiOutChannel = -1;
    int midiInChannel = -1;

    unsigned int numChannels; // Number of audio channels
    std::atomic<bool> isRunning; // Flag to track stream state
    float** emptyInputBuffer; // Preallocated empty input buffer
    unsigned int bufferSize; // Size of the audio buffer (in frames)

    float** getEmptyInputBuffer() const;
    unsigned int getNumChannels() const;

    static int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                            double streamTime, RtAudioStreamStatus status, void* userData);
    
    static void midiCallback(double deltatime, std::vector<uint8_t> *pMsg, void *userData);
};

#endif // CABBAGEAUDIOAPP_H
