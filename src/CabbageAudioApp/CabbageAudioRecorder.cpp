//
// AudioRecorder.cpp
// Real-time audio recording to WAV files using Choc
//
// Copyright (c) 2024 Rory Walsh
// Licensed under the MIT License
//

#include "CabbageAudioRecorder.h"
#include "lattice/LatticeUtils.h"
#include <iostream>

namespace cabbage
{

AudioRecorder::AudioRecorder()
{
    fifo.reset(fifoSize);
}

AudioRecorder::~AudioRecorder()
{
    stopRecording();
}

bool AudioRecorder::startRecording(const std::string& filepath,
                                   double sampleRate,
                                   uint32_t numChannels,
                                   choc::audio::BitDepth bitDepth)
{
    // Stop any existing recording
    if (isRecording())
    {
        stopRecording();
    }

    // Validate parameters
    if (filepath.empty() || sampleRate <= 0 || numChannels == 0)
    {
        lattice::logError << "AudioRecorder: Invalid parameters";
        return false;
    }

    try
    {
        // Create output stream
        outputStream = std::make_shared<std::ofstream>(
            filepath,
            std::ios::binary | std::ios::out | std::ios::trunc
        );

        if (!outputStream->is_open() || outputStream->fail())
        {
            lattice::logError << "AudioRecorder: Failed to open file: " << filepath;
            return false;
        }

        // Configure WAV properties
        choc::audio::AudioFileProperties properties;
        properties.formatName = "WAV";
        properties.sampleRate = sampleRate;
        properties.numChannels = numChannels;
        properties.bitDepth = bitDepth;
        properties.numFrames = 0; // Will be updated as we write

        // Create WAV writer
        choc::audio::WAVAudioFileFormat<true> wavFormat;
        writer = wavFormat.createWriter(outputStream, std::move(properties));

        if (!writer)
        {
            lattice::logError << "AudioRecorder: Failed to create WAV writer";
            outputStream.reset();
            return false;
        }

        // Store recording properties
        recordNumChannels = numChannels;
        recordSampleRate = sampleRate;

        // Clear FIFO
        fifo.reset();

        // Start recording flag and writer thread
        recording.store(true, std::memory_order_release);
        shouldStopThread.store(false, std::memory_order_release);

        writerThread = std::thread(&AudioRecorder::writerThreadFunc, this);

        lattice::logInfo << "AudioRecorder: Started recording to " << filepath
                        << " (" << numChannels << " channels, "
                        << sampleRate << " Hz, "
                        << static_cast<int>(bitDepth) << " bit depth)";

        return true;
    }
    catch (const std::exception& e)
    {
        lattice::logError << "AudioRecorder: Exception starting recording: " << e.what();
        writer.reset();
        outputStream.reset();
        return false;
    }
}

void AudioRecorder::stopRecording()
{
    if (!isRecording())
    {
        return;
    }

    // Signal recording stopped
    recording.store(false, std::memory_order_release);

    // Signal writer thread to stop
    shouldStopThread.store(true, std::memory_order_release);

    // Wait for writer thread to finish
    if (writerThread.joinable())
    {
        writerThread.join();
    }

    // Reset the flag for potential future recordings
    shouldStopThread.store(false, std::memory_order_release);

    // Flush and close writer
    if (writer)
    {
        writer->flush();
        writer.reset();
    }

    // Close output stream
    if (outputStream)
    {
        outputStream->close();
        outputStream.reset();
    }

    lattice::logInfo << "AudioRecorder: Recording stopped";
}

void AudioRecorder::pushSamples(float** deinterleavedSamples, uint32_t numFrames, uint32_t numChannels)
{
    if (!isRecording() || numChannels != recordNumChannels)
    {
        return;
    }

    // Interleave and push to FIFO
    // FIFO stores interleaved data: [L0, R0, L1, R1, ...]
    for (uint32_t frame = 0; frame < numFrames; ++frame)
    {
        for (uint32_t ch = 0; ch < numChannels; ++ch)
        {
            if (!fifo.push(deinterleavedSamples[ch][frame]))
            {
                // FIFO full - we're dropping samples
                // This shouldn't happen with a properly sized FIFO
                static bool warningShown = false;
                if (!warningShown)
                {
                    lattice::logError << "AudioRecorder: FIFO overflow - dropping samples";
                    warningShown = true;
                }
                return;
            }
        }
    }
}

void AudioRecorder::writerThreadFunc()
{
    lattice::logDebug << "AudioRecorder: Writer thread started";

    // Temporary buffer for batching writes
    std::vector<float> interleavedBuffer;
    interleavedBuffer.reserve(writeBatchFrames * recordNumChannels);

    // Create a non-interleaved buffer view for the writer
    choc::buffer::ChannelArrayBuffer<float> writeBuffer(recordNumChannels, writeBatchFrames);

    while (!shouldStopThread.load(std::memory_order_acquire))
    {
        interleavedBuffer.clear();

        // Try to accumulate a batch of frames
        uint32_t samplesRead = 0;
        const uint32_t samplesToRead = writeBatchFrames * recordNumChannels;

        while (samplesRead < samplesToRead)
        {
            float sample;
            if (fifo.pop(sample))
            {
                interleavedBuffer.push_back(sample);
                samplesRead++;
            }
            else
            {
                // FIFO empty
                break;
            }
        }

        // If we have data, write it
        if (samplesRead > 0)
        {
            uint32_t framesRead = samplesRead / recordNumChannels;

            // Deinterleave into write buffer
            for (uint32_t frame = 0; frame < framesRead; ++frame)
            {
                for (uint32_t ch = 0; ch < recordNumChannels; ++ch)
                {
                    writeBuffer.getSample(ch, frame) = interleavedBuffer[frame * recordNumChannels + ch];
                }
            }

            // Write to file
            if (writer)
            {
                auto view = writeBuffer.getView().getStart(framesRead);
                if (!writer->appendFrames(view))
                {
                    lattice::logError << "AudioRecorder: Failed to write frames to file";
                    break;
                }
            }
        }
        else
        {
            // No data available, sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Drain remaining samples from FIFO
    lattice::logDebug << "AudioRecorder: Draining remaining samples from FIFO";

    interleavedBuffer.clear();
    float sample;
    while (fifo.pop(sample))
    {
        interleavedBuffer.push_back(sample);
    }

    // Write remaining data
    if (!interleavedBuffer.empty() && writer)
    {
        uint32_t framesRemaining = static_cast<uint32_t>(interleavedBuffer.size()) / recordNumChannels;

        // Ensure buffer is large enough
        if (framesRemaining > writeBuffer.getNumFrames())
        {
            writeBuffer.resize({ recordNumChannels, framesRemaining });
        }

        // Deinterleave
        for (uint32_t frame = 0; frame < framesRemaining; ++frame)
        {
            for (uint32_t ch = 0; ch < recordNumChannels; ++ch)
            {
                writeBuffer.getSample(ch, frame) = interleavedBuffer[frame * recordNumChannels + ch];
            }
        }

        // Write final frames
        auto view = writeBuffer.getView().getStart(framesRemaining);
        writer->appendFrames(view);

        lattice::logDebug << "AudioRecorder: Wrote final " << framesRemaining << " frames";
    }

    lattice::logDebug << "AudioRecorder: Writer thread finished";
}

} // namespace cabbage
