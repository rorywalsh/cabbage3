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

// ---------------------------------------------------------------------------
// CabbageARAProcessor.cpp
// All ARA-related method implementations for CabbageProcessor.
// Compiled when LATTICE_HAS_ARA or CabbageApp is defined.
// ---------------------------------------------------------------------------

#include "CabbageProcessor.h"
#include <algorithm>
#include <filesystem>
#include <thread>

#ifdef CabbageApp
#include "choc/audio/choc_AudioFileFormat_WAV.h"
#include "choc/audio/choc_AudioFileFormat_FLAC.h"
#include "choc/audio/choc_AudioFileFormat_Ogg.h"
#include "choc/audio/choc_AudioFileFormat_MP3.h"
#endif

#if LATTICE_HAS_ARA || defined(CabbageApp)

#if LATTICE_HAS_ARA

#include PLUGIN_INFO_HEADER  // pulls in CABBAGE_ARA_* constants

LATTICE_DEFINE_ARA_FACTORY(CabbageProcessor)

lattice::AraPluginInfo CabbageProcessor::getStaticAraInfo() noexcept
{
    return {
        CABBAGE_ARA_FACTORY_ID,
        CABBAGE_ARA_PLUGIN_NAME,
        CABBAGE_ARA_MANUFACTURER,
        CABBAGE_ARA_INFO_URL,
        CABBAGE_ARA_VERSION,
        CABBAGE_ARA_DOCUMENT_ARCHIVE
    };
}

void CabbageProcessor::araAudioSourceContentUpdated(ARA::PlugIn::AudioSource* source,
                                                      ARA::ContentUpdateScopes scopes)
{
    if (isMyAudioSource(source) && source->isSampleAccessEnabled() && scopes.affectSamples())
    {
        lattice::logInfo << "ARA: content updated with samples access enabled — queuing re-analysis";
//        enqueueAraSource(source);
    }
}

void CabbageProcessor::araDidEnableSamplesAccess(ARA::PlugIn::AudioSource* source, bool enable)
{
//    lattice::logDebug << "araDidEnableSamplesAccess: source=" << (source ? source->getName() : "null")
//                      << " enable=" << enable << " uiIsOpen=" << uiIsOpen
//                      << " thread=" << std::this_thread::get_id();

    // Track which sources currently have sample access (per-instance, mutex-protected).
    {
        std::lock_guard<std::mutex> lk(araSourcesMutex);
        if (enable)
            araAccessibleSources.insert(source);
        else
        {
            araAccessibleSources.erase(source);
            araAnalysedSources.erase(source);  // Allow re-analysis if access is re-granted
        }
    }

    // Only the UI instance (uiIsOpen == true) should run analysis.
    // Background / rendering instances receive the same broadcast from the document
    // controller but must not start their own analysis — that would spin up multiple
    // simultaneous Csound instances and exhaust system resources.
    // If regions haven't been assigned yet, isMyAudioSource() returns false — the
    // onIdle() retry loop will catch the source once regions are known.
    // If the UI is not open yet the analysis will be triggered from setCabbageIsReady().
    if (enable && uiIsOpen)
    {
        bool alreadyQueued = false;
        {
            std::lock_guard<std::mutex> lk(araSourcesMutex);
            alreadyQueued = araAnalysedSources.count(source) > 0;
        }
        if (!alreadyQueued && isMyAudioSource(source))
        {
            std::lock_guard<std::mutex> lk(araSourcesMutex);
            araAnalysedSources.insert(source);
            lattice::logInfo << "ARA: sample access enabled — queuing analysis";
            enqueueAraSource(source);
        }
    }
}

// ---------------------------------------------------------------------------
// Called when the host updates playback region properties (region resized/moved).
// ---------------------------------------------------------------------------
void CabbageProcessor::araPlaybackRegionPropertiesUpdated(ARA::PlugIn::PlaybackRegion* playbackRegion)
{
    auto* source = playbackRegion->getAudioModification()->getAudioSource();
    if (!source)
        return;

    const char* name = source->getName();
    if (!name)
        return;
    const std::string sourceName(name);
    const auto start = static_cast<double>(playbackRegion->getStartInAudioModificationSamples());
    const auto duration = static_cast<double>(playbackRegion->getDurationInAudioModificationSamples());

    cabbage::ARADataPool::instance().updateRegionByName(sourceName, start, duration);

    {
        std::lock_guard<std::mutex> lk(araMutex);
        araUpdateCounter++;
    }

    lattice::logDebug << "ARA: region updated for '" << sourceName
                      << "' start=" << start << " duration=" << duration;
}

// ---------------------------------------------------------------------------
// Thread-safe getter for the last ARA event type (returns copy under lock).
// ---------------------------------------------------------------------------
std::string CabbageProcessor::getAraLastEventType()
{
    std::lock_guard<std::mutex> lk(araMutex);
    return araLastEventType;
}

// ---------------------------------------------------------------------------
// Called when the host begins an edit cycle.
// ---------------------------------------------------------------------------
void CabbageProcessor::araBeginEditing()
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "beginEditing";
    araUpdateCounter++;
}

// ---------------------------------------------------------------------------
// Called when the host ends an edit cycle.
// ---------------------------------------------------------------------------
void CabbageProcessor::araEndEditing()
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "endEditing";
    araUpdateCounter++;
}

// ---------------------------------------------------------------------------
// Called after the host has processed model updates.
// ---------------------------------------------------------------------------
void CabbageProcessor::araDidNotifyModelUpdates()
{
}

// ---------------------------------------------------------------------------
// Called when document properties are updated.
// ---------------------------------------------------------------------------
void CabbageProcessor::araDocumentPropertiesUpdated(ARA::PlugIn::Document* /*document*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "documentPropertiesUpdated";
    araUpdateCounter++;
}

void CabbageProcessor::araMusicalContextPropertiesUpdated(ARA::PlugIn::MusicalContext* /*musicalContext*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "musicalContextPropertiesUpdated";
    araUpdateCounter++;
}

void CabbageProcessor::araRegionSequencePropertiesUpdated(ARA::PlugIn::RegionSequence* /*regionSequence*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "regionSequencePropertiesUpdated";
    araUpdateCounter++;
}

void CabbageProcessor::araAudioSourcePropertiesUpdated(ARA::PlugIn::AudioSource* /*audioSource*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "audioSourcePropertiesUpdated";
    araUpdateCounter++;
}

void CabbageProcessor::araAudioModificationPropertiesUpdated(ARA::PlugIn::AudioModification* /*audioModification*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "audioModificationPropertiesUpdated";
    araUpdateCounter++;
}

void CabbageProcessor::araPlaybackRegionAddedToRegionSequence(ARA::PlugIn::RegionSequence* /*regionSequence*/,
                                                               ARA::PlugIn::PlaybackRegion* /*playbackRegion*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "playbackRegionAddedToRegionSequence";
    araUpdateCounter++;
}

void CabbageProcessor::araPlaybackRegionRemovedFromRegionSequence(ARA::PlugIn::RegionSequence* /*regionSequence*/,
                                                                    ARA::PlugIn::PlaybackRegion* /*playbackRegion*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    araLastEventType = "playbackRegionRemovedFromRegionSequence";
    araUpdateCounter++;
}

// ---------------------------------------------------------------------------
// ARA worker — runs on a dedicated background thread
// ---------------------------------------------------------------------------
void CabbageProcessor::enqueueAraSource(ARA::PlugIn::AudioSource* source)
{
    {
        std::lock_guard<std::mutex> lock(araMutex);
        // Deduplicate: drop any existing pending entry for the same source.
        araPendingSources.erase(
            std::remove(araPendingSources.begin(), araPendingSources.end(), source),
            araPendingSources.end());
        araPendingSources.push_back(source);
    }
    araCv.notify_one();
}

void CabbageProcessor::startAraWorker()
{
    if (araWorkerRunning.exchange(true))
        return;
    araWorkerThread = std::thread(&CabbageProcessor::runAraWorker, this);
}

void CabbageProcessor::stopAraWorker()
{
    if (!araWorkerRunning.exchange(false))
        return;
    araCv.notify_all();
    if (araWorkerThread.joinable())
        araWorkerThread.join();
}

void CabbageProcessor::runAraWorker()
{
    while (araWorkerRunning.load(std::memory_order_acquire))
    {
        ARA::PlugIn::AudioSource* source = nullptr;
        {
            std::unique_lock<std::mutex> lock(araMutex);
            araCv.wait(lock, [this] {
                return !araWorkerRunning.load(std::memory_order_acquire)
                    || !araPendingSources.empty();
            });
            if (!araWorkerRunning.load(std::memory_order_acquire) && araPendingSources.empty())
                break;
            if (araPendingSources.empty())
                continue;
            source = araPendingSources.front();
            araPendingSources.pop_front();
        }
        performAraAnalysis(source);
    }
}

//----------------------------------------------------------------------------------------
void CabbageProcessor::performAraAnalysis(ARA::PlugIn::AudioSource* source)
{
    lattice::logDebug << "performAraAnalysis: START source=" << (source ? source->getName() : "null")
                      << " thread=" << std::this_thread::get_id();

    const int      numChannels  = static_cast<int>(source->getChannelCount());
    const double   sr           = source->getSampleRate();
    const ARA::ARASampleCount totalSamples = source->getSampleCount();

    lattice::logDebug << "ARA: analysing '" << source->getName()
                     << "' ch=" << numChannels << " sr=" << sr
                     << " samples=" << totalSamples;

    // Read all audio from the ARA host into per-channel float buffers.
    std::vector<std::vector<float>> pcm(numChannels,
                                        std::vector<float>(static_cast<size_t>(totalSamples), 0.0f));
    {
        ARA::PlugIn::HostAudioReader reader(source);
        constexpr ARA::ARASampleCount kBlock = 4096;
        for (ARA::ARASampleCount pos = 0; pos < totalSamples; pos += kBlock)
        {
            const ARA::ARASampleCount count = std::min(kBlock, totalSamples - pos);
            std::vector<void*> ptrs(numChannels);
            for (int c = 0; c < numChannels; ++c)
                ptrs[c] = pcm[c].data() + pos;
            if (!reader.readAudioSamples(pos, count, ptrs.data()))
            {
                lattice::logError << "ARA: readAudioSamples failed at pos=" << pos;
                break;
            }
        }
    }

    const std::string sourceName = source->getName() ? source->getName() : "";
    auto pcmPtr = std::make_shared<std::vector<std::vector<float>>>(std::move(pcm));

    // Store in global pool (thread-safe, shared across all instances)
    size_t poolIdx = cabbage::ARADataPool::instance().upsert(
        sourceName,
        static_cast<double>(totalSamples),
        static_cast<double>(numChannels),
        sr,
        static_cast<double>(totalSamples) / sr,
        pcmPtr
    );

    {
        std::lock_guard<std::mutex> lk(araMutex);
        araCurrentSourceIndex = static_cast<int>(poolIdx);
        araUpdateCounter++;
    }

    lattice::logDebug << "ARA: source '" << sourceName << "' stored at pool index " << poolIdx;
}

#endif // LATTICE_HAS_ARA

#ifdef CabbageApp
// ---------------------------------------------------------------------------
// Standalone ARA analysis: reads an audio file and stores in the global pool.
// ---------------------------------------------------------------------------
void CabbageProcessor::performAraAnalysisFromFile(const std::string& filePath)
{
    lattice::logInfo << "ARA standalone: analysing file '" << filePath << "'";

    choc::audio::AudioFileFormatList formats;
    formats.addFormat<choc::audio::WAVAudioFileFormat<false>>();
    formats.addFormat<choc::audio::FLACAudioFileFormat<false>>();
    formats.addFormat<choc::audio::OggAudioFileFormat<false>>();
    formats.addFormat<choc::audio::MP3AudioFileFormat>();

    auto reader = formats.createReader(std::filesystem::path(filePath));
    if (!reader)
    {
        lattice::logError << "ARA standalone: failed to open '" << filePath << "'";
        return;
    }

    const auto&   props        = reader->getProperties();
    const int     numChannels  = static_cast<int>(props.numChannels);
    const double  sr           = props.sampleRate;
    const int64_t totalSamples = static_cast<int64_t>(props.numFrames);
    const std::string sourceName = std::filesystem::path(filePath).filename().string();

    lattice::logInfo << "ARA standalone: '" << sourceName << "' ch=" << numChannels
                     << " sr=" << sr << " samples=" << totalSamples;

    // Read into per-channel float buffers.
    std::vector<std::vector<float>> pcm(static_cast<size_t>(numChannels),
                                        std::vector<float>(static_cast<size_t>(totalSamples), 0.0f));
    {
        constexpr int64_t kBlock = 4096;
        choc::buffer::ChannelArrayBuffer<float> block(static_cast<uint32_t>(numChannels),
                                                      static_cast<uint32_t>(kBlock));
        for (int64_t pos = 0; pos < totalSamples; pos += kBlock)
        {
            const int64_t count = std::min(kBlock, totalSamples - pos);
            auto slice = block.getStart(static_cast<uint32_t>(count));
            if (!reader->readFrames(static_cast<uint64_t>(pos), slice))
            {
                lattice::logError << "ARA standalone: readFrames failed at pos=" << pos;
                break;
            }
            for (int c = 0; c < numChannels; ++c)
                for (int64_t i = 0; i < count; ++i)
                    pcm[static_cast<size_t>(c)][static_cast<size_t>(pos + i)] =
                        block.getSample(static_cast<uint32_t>(c), static_cast<uint32_t>(i));
        }
    }

    auto pcmPtr = std::make_shared<std::vector<std::vector<float>>>(std::move(pcm));

    // Store in global pool
    size_t poolIdx = cabbage::ARADataPool::instance().upsert(
        sourceName,
        static_cast<double>(totalSamples),
        static_cast<double>(numChannels),
        sr,
        (sr > 0.0) ? (static_cast<double>(totalSamples) / sr) : 0.0,
        pcmPtr
    );

    {
        std::lock_guard<std::mutex> lk(araMutex);
        araCurrentSourceIndex = static_cast<int>(poolIdx);
        araUpdateCounter++;
    }

    lattice::logInfo << "ARA standalone: '" << sourceName << "' stored at pool index " << poolIdx;
}
#endif // CabbageApp

#endif // LATTICE_HAS_ARA || CabbageApp
