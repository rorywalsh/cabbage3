#include "CabbageAudioApp.h"
#include <iostream>
#include <algorithm>

CabbageAudioApp::CabbageAudioApp()
    : numChannels(2), bufferSize(512), isRunning(false), processor(2, 2)
{
    // Preallocate the empty input buffer
    emptyInputBuffer = new float *[numChannels];
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        emptyInputBuffer[ch] = new float[bufferSize];
        std::fill(emptyInputBuffer[ch], emptyInputBuffer[ch] + bufferSize, 0.0f); // Initialize with zeros
    }

    // Initialize audio
    initialiseAudio();
    // Initialise midi
    initialiseMidi();
}

CabbageAudioApp::~CabbageAudioApp()
{
    if (isRunning)
    {
        try
        {
            audio->stopStream();
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        if (audio->isStreamOpen())
        {
            audio->closeStream();
        }
    }
    // Clean up the preallocated empty input buffer
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        delete[] emptyInputBuffer[ch];
    }
    delete[] emptyInputBuffer;
}


void CabbageAudioApp::initialiseMidi()
{
    try
    {
        midiIn = std::make_unique<RtMidiIn>();
    }
    catch (RtMidiError &error)
    {
        midiIn = nullptr;
        error.printMessage();
        return false;
    }

    try
    {
        midiOut = std::make_unique<RtMidiOut>();
    }
    catch (RtMidiError &error)
    {
        midiOut = nullptr;
        error.printMessage();
        return false;
    }

    midiIn->setCallback(&midiCallback, this);
    midiIn->ignoreTypes(false, true, false);

    return true;
}

void CabbageAudioApp::initialiseAudio()
{
    // Create an instance of RtAudio
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    audio = std::make_unique<RtAudio>(apis[0], errorCallback);

    // Check if audio devices are available
    if (audio->getDeviceCount() < 1)
    {
        std::cerr << "No audio devices found!" << std::endl;
        return;
    }

    // Set up stream parameters
    RtAudio::StreamParameters parameters;
    parameters.deviceId = audio->getDefaultOutputDevice();
    parameters.nChannels = 2; // Stereo
    parameters.firstChannel = 0;

    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = bufferSize; // 512 sample frames

    try
    {
        // Open the audio stream
        audio->openStream(&parameters, nullptr, RTAUDIO_FLOAT32, sampleRate,
                          &bufferFrames, &CabbageAudioApp::audioCallback, this); // Pass 'this' as userData
        audio->startStream();
        isRunning = true; // Mark the stream as running
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
}

void CabbageAudioApp::errorCallback(RtAudioErrorType type, const std::string &errorText)
{
    std::cout << errorText;
}

bool CabbageAudioApp::isStreamRunning() const
{
    return isRunning;
}

float **CabbageAudioApp::getEmptyInputBuffer() const
{
    return emptyInputBuffer;
}

unsigned int CabbageAudioApp::getNumChannels() const
{
    return numChannels;
}

void CabbageAudioApp::midiCallback(double deltatime, std::vector<uint8_t> *msg, void *userData)
{
    // Cast userData to CabbageAudioApp* (or whatever your app class is)
    CabbageAudioApp *app = static_cast<CabbageAudioApp *>(userData);

    // Ensure the MIDI message is not empty
    if (msg->empty())
    {
        std::cerr << "Empty MIDI message received!" << std::endl;
        return;
    }

    // Extract the MIDI status byte (first byte)
    uint8_t status = (*msg)[0];

    // Determine the MIDI message type (note on, note off, etc.)
    lattice::Processor::NoteEvent::Type type;
    int16_t key = -1;              // MIDI note number
    double velocity = 0.0;         // MIDI velocity (normalized to 0.0-1.0)
    int32_t noteId = -1;           // Unique note ID (you can generate this if needed)
    uint32_t sampleOffset = 0;     // Sample offset (you can calculate this if needed)

    // Handle Note On and Note Off messages
    if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) // Note On (0x90) or Note Off (0x80)
    {
        if (msg->size() >= 2)
        {
            key = static_cast<int16_t>((*msg)[1]); // MIDI note number
            velocity = static_cast<double>((*msg)[2]) / 127.0; // Normalize velocity to 0.0-1.0

            // For Note Off messages with velocity 0, treat them as Note On with velocity 0
            if ((status & 0xF0) == 0x80 || velocity == 0.0)
            {
                type = lattice::Processor::NoteEvent::Type::noteOff; // Mark as Note Off
                velocity = 0.0; // Force velocity to 0 for Note Off
            }
            else
            {
                type = lattice::Processor::NoteEvent::Type::noteOn; // Mark as Note On
            }

            // Generate a unique note ID (you can use a counter or another method)
            static int32_t nextNoteId = 0;
            noteId = nextNoteId++;

            // Calculate sample offset (if needed)
            // For now, we'll set it to 0, but you can calculate it based on deltatime
            sampleOffset = static_cast<uint32_t>(deltatime * 44100); // Assuming 44100 Hz sample rate
        }
    }

    // Add the NoteEvent to the processor
    app->processor.addNoteEvent({type, key, velocity, noteId, sampleOffset});

}


int CabbageAudioApp::audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                   double streamTime, RtAudioStreamStatus status, void *userData)
{
    // Cast userData to CabbageAudioApp*
    CabbageAudioApp *app = static_cast<CabbageAudioApp *>(userData);

    // Cast buffers to float*
    float *floatInputBuffer = static_cast<float *>(inputBuffer);
    float *floatOutputBuffer = static_cast<float *>(outputBuffer);

    // Get the number of channels
    unsigned int numChannels = app->getNumChannels();

    // Deinterleave the input buffer into separate channels
    float **deinterleavedInput = nullptr;
    if (floatInputBuffer)
    {
        deinterleavedInput = new float *[numChannels];
        for (unsigned int ch = 0; ch < numChannels; ++ch)
        {
            deinterleavedInput[ch] = new float[nBufferFrames];
            for (unsigned int i = 0; i < nBufferFrames; ++i)
            {
                deinterleavedInput[ch][i] = floatInputBuffer[i * numChannels + ch];
            }
        }
    }
    else
    {
        // Use the preallocated empty input buffer
        deinterleavedInput = app->getEmptyInputBuffer();
    }

    // Deinterleave the output buffer into separate channels
    float **deinterleavedOutput = new float *[numChannels];
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        deinterleavedOutput[ch] = new float[nBufferFrames];
    }

    // Pass the deinterleaved buffers to the process method
    app->processor.process(deinterleavedInput, deinterleavedOutput, nBufferFrames);

    // Interleave the processed output back into the RtAudio buffer
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        for (unsigned int i = 0; i < nBufferFrames; ++i)
        {
            floatOutputBuffer[i * numChannels + ch] = deinterleavedOutput[ch][i];
        }
    }

    // Clean up the deinterleaved buffers (except the preallocated empty input buffer)
    if (floatInputBuffer)
    {
        for (unsigned int ch = 0; ch < numChannels; ++ch)
        {
            delete[] deinterleavedInput[ch];
        }
        delete[] deinterleavedInput;
    }
    for (unsigned int ch = 0; ch < numChannels; ++ch)
    {
        delete[] deinterleavedOutput[ch];
    }
    delete[] deinterleavedOutput;

    return 0;
}
