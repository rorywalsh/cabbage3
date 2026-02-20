//
// AudioRecorder.h
// Real-time audio recording to WAV files using Choc
//
// Copyright (c) 2024 Rory Walsh
// Licensed under the MIT License
//

#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <string>
#include <fstream>
#include "../../build/CabbageApp/_deps/choc-src/choc/audio/choc_AudioFileFormat_WAV.h"
#include "../../build/CabbageApp/_deps/choc-src/choc/containers/choc_SingleReaderSingleWriterFIFO.h"
#include "../../build/CabbageApp/_deps/choc-src/choc/audio/choc_SampleBuffers.h"

namespace cabbage
{

/**
 * @brief Real-time audio recorder that writes to WAV files
 *
 * This class captures audio from the audio callback thread via a lock-free FIFO
 * and writes it to disk on a separate thread to avoid blocking the audio thread.
 */
class AudioRecorder
{
public:
    AudioRecorder();
    ~AudioRecorder();

    /**
     * @brief Starts recording audio to a WAV file
     * @param filepath Path to the output WAV file
     * @param sampleRate Sample rate of the audio
     * @param numChannels Number of audio channels
     * @param bitDepth Bit depth for the WAV file (default: float32)
     * @return true if recording started successfully, false otherwise
     */
    bool startRecording(const std::string& filepath,
                       double sampleRate,
                       uint32_t numChannels,
                       choc::audio::BitDepth bitDepth = choc::audio::BitDepth::float32);

    /**
     * @brief Stops the current recording and finalizes the WAV file
     */
    void stopRecording();

    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    bool isRecording() const { return recording.load(std::memory_order_acquire); }

    /**
     * @brief Push audio samples from the audio thread to the recording queue
     * @param deinterleavedSamples Non-interleaved audio data [channel][frame]
     * @param numFrames Number of frames to push
     * @param numChannels Number of channels in the data
     *
     * This is called from the audio thread and must be lock-free
     */
    void pushSamples(float** deinterleavedSamples, uint32_t numFrames, uint32_t numChannels);

private:
    /**
     * @brief Writer thread function - drains FIFO and writes to disk
     */
    void writerThreadFunc();

    // Thread synchronization
    std::atomic<bool> recording{false};
    std::atomic<bool> shouldStopThread{false};

    // Lock-free FIFO for audio data (audio thread -> writer thread)
    choc::fifo::SingleReaderSingleWriterFIFO<float> fifo;

    // WAV file writer
    std::unique_ptr<choc::audio::AudioFileWriter> writer;
    std::shared_ptr<std::ofstream> outputStream;

    // Writer thread
    std::thread writerThread;

    // Recording properties
    uint32_t recordNumChannels{0};
    double recordSampleRate{0.0};

    // FIFO size: 4 seconds of stereo audio at 48kHz
    static constexpr size_t fifoSize = 48000 * 2 * 4;

    // Batch size for writing to disk
    static constexpr uint32_t writeBatchFrames = 512;
};

} // namespace cabbage
