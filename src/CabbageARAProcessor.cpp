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
#include "CabbageARADataPool.h"
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

// Helper: update processor state + araState JSON (called under araMutex)
static void updateState(int& counter, std::string& lastEvent, const char* eventType)
{
    lastEvent = eventType;
    counter++;
    cabbage::ARADataPool::instance().updateAraState("lastEvent", eventType);
    cabbage::ARADataPool::instance().updateAraState("update", static_cast<double>(counter));
}

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
    const double sr = source->getSampleRate();
    const double durationSec = (sr > 0.0) ? duration / sr : 0.0;

    ARA_LOG("CabbageARA: didUpdatePlaybackRegionProperties source='%s'", name);
    ARA_LOG("  mod: start=%.3f dur=%.3f",
            playbackRegion->getStartInAudioModificationTime(), playbackRegion->getDurationInAudioModificationTime());
    ARA_LOG("  pb:  start=%.3f dur=%.3f",
            playbackRegion->getStartInPlaybackTime(), playbackRegion->getDurationInPlaybackTime());
    ARA_LOG("  modSamples: start=%d dur=%d",
            (int)playbackRegion->getStartInAudioModificationSamples(), (int)playbackRegion->getDurationInAudioModificationSamples());
    ARA_LOG("  srcSamples: count=%d sr=%.0f", (int)source->getSampleCount(), source->getSampleRate());

    cabbage::ARADataPool::instance().updateRegionByName(sourceName, start, duration);
    cabbage::ARADataPool::instance().updateSelectedRegionByName(
        sourceName, start, durationSec, duration,
        static_cast<double>(playbackRegion->getStartInPlaybackTime()),
        static_cast<double>(playbackRegion->getDurationInPlaybackTime()));

    // Also store audio source crop position (within the source file)
    const auto asStart = static_cast<double>(playbackRegion->getStartInAudioModificationTime());
    const auto asDur = static_cast<double>(playbackRegion->getDurationInAudioModificationTime());
    cabbage::ARADataPool::instance().updateRegionTimeByName(
        sourceName, asStart, asDur);

    // Also update the playback region entry in the full playback regions list
    const ARA::ARAColor* pbColor = playbackRegion->getColor();
    cabbage::ARADataPool::instance().addOrUpdatePlaybackRegion(
        playbackRegion,
        sourceName,
        "",  // region sequence name not available here; set on add
        -1,  // region sequence index not available here; set on add
        start, duration,
        static_cast<double>(playbackRegion->getStartInPlaybackTime()),
        static_cast<double>(playbackRegion->getDurationInPlaybackTime()),
        pbColor ? pbColor->r : 0.0f,
        pbColor ? pbColor->g : 0.0f,
        pbColor ? pbColor->b : 0.0f);

    ARA_LOG("CabbageARA: stored audioSource for '%s' start=%.3f dur=%.3f",
            name, asStart, asDur);

    // Update currentIndex so the CSD reads from the correct source
    {
        int poolIdx = cabbage::ARADataPool::instance().getIndexByName(sourceName);
        if (poolIdx >= 0)
        {
            std::lock_guard<std::mutex> lk(araMutex);
            araCurrentSourceIndex = poolIdx;
            cabbage::ARADataPool::instance().updateAraState("currentIndex", static_cast<double>(poolIdx));
        }
        std::lock_guard<std::mutex> lk(araMutex);
        araUpdateCounter++;
    }

    lattice::logDebug << "ARA: region updated for '" << sourceName
                      << "' start=" << start << " duration=" << duration;
}

// ---------------------------------------------------------------------------
// Called when the host begins an edit cycle.
// ---------------------------------------------------------------------------
void CabbageProcessor::araBeginEditing()
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "beginEditing");
}

// ---------------------------------------------------------------------------
// Called when the host ends an edit cycle.
// ---------------------------------------------------------------------------
void CabbageProcessor::araEndEditing()
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "endEditing");
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
    updateState(araUpdateCounter, araLastEventType, "documentPropertiesUpdated");
}

void CabbageProcessor::araMusicalContextPropertiesUpdated(ARA::PlugIn::MusicalContext* /*musicalContext*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "musicalContextPropertiesUpdated");
}

void CabbageProcessor::araRegionSequencePropertiesUpdated(ARA::PlugIn::RegionSequence* regionSequence)
{
    if (regionSequence)
    {
        const ARA::ARAColor* seqColor = regionSequence->getColor();
        const auto ctx = regionSequence->getMusicalContext();
        cabbage::ARADataPool::instance().addOrUpdateRegionSequence(
            regionSequence,
            regionSequence->getName() ? regionSequence->getName() : "",
            regionSequence->getOrderIndex(),
            ctx ? static_cast<int>(cabbage::ARADataPool::instance().getMusicalContextIndex(ctx)) : 0,
            seqColor ? seqColor->r : 0.0f,
            seqColor ? seqColor->g : 0.0f,
            seqColor ? seqColor->b : 0.0f);
    }
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "regionSequencePropertiesUpdated");
}

void CabbageProcessor::araAudioSourcePropertiesUpdated(ARA::PlugIn::AudioSource* /*audioSource*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioSourcePropertiesUpdated");
}

void CabbageProcessor::araAudioModificationPropertiesUpdated(ARA::PlugIn::AudioModification* /*audioModification*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioModificationPropertiesUpdated");
}

void CabbageProcessor::araPlaybackRegionAddedToRegionSequence(ARA::PlugIn::RegionSequence* regionSequence,
                                                               ARA::PlugIn::PlaybackRegion* playbackRegion)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "playbackRegionAddedToRegionSequence");

    const auto* src = playbackRegion->getAudioModification()->getAudioSource();
    const char* srcName = src ? src->getName() : "";
    const char* seqName = regionSequence ? regionSequence->getName() : "";
    const ARA::ARAColor* color = playbackRegion->getColor();
    float cr = color ? color->r : 0.0f;
    float cg = color ? color->g : 0.0f;
    float cb = color ? color->b : 0.0f;
    int seqIndex = regionSequence ? cabbage::ARADataPool::instance().getRegionSequenceOrderIndex(regionSequence) : -1;
    cabbage::ARADataPool::instance().addOrUpdatePlaybackRegion(
        playbackRegion,
        srcName ? srcName : "",
        seqName ? seqName : "",
        seqIndex,
        static_cast<double>(playbackRegion->getStartInAudioModificationSamples()),
        static_cast<double>(playbackRegion->getDurationInAudioModificationSamples()),
        static_cast<double>(playbackRegion->getStartInPlaybackTime()),
        static_cast<double>(playbackRegion->getDurationInPlaybackTime()),
        cr, cg, cb);
}

void CabbageProcessor::araPlaybackRegionRemovedFromRegionSequence(ARA::PlugIn::RegionSequence* /*regionSequence*/,
                                                                     ARA::PlugIn::PlaybackRegion* playbackRegion)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "playbackRegionRemovedFromRegionSequence");
    cabbage::ARADataPool::instance().removePlaybackRegion(playbackRegion);
}

void CabbageProcessor::araMusicalContextAddedToDocument(ARA::PlugIn::Document* /*document*/,
                                                           ARA::PlugIn::MusicalContext* musicalContext)
{
    {
        std::lock_guard<std::mutex> lk(araMutex);
        updateState(araUpdateCounter, araLastEventType, "musicalContextAddedToDocument");
    }
    if (musicalContext)
    {
        const char* name = musicalContext->getName();
        int orderIndex = musicalContext->getOrderIndex();
        const ARA::ARAColor* color = musicalContext->getColor();
        float cr = color ? color->r : 0.0f;
        float cg = color ? color->g : 0.0f;
        float cb = color ? color->b : 0.0f;
        cabbage::ARADataPool::instance().addOrUpdateMusicalContext(
            musicalContext, name ? name : "", orderIndex, cr, cg, cb);
    }
}

void CabbageProcessor::araMusicalContextRemovedFromDocument(ARA::PlugIn::Document* /*document*/,
                                                               ARA::PlugIn::MusicalContext* musicalContext)
{
    if (musicalContext)
        cabbage::ARADataPool::instance().removeMusicalContext(musicalContext);
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "musicalContextRemovedFromDocument");
}

void CabbageProcessor::araRegionSequenceAddedToDocument(ARA::PlugIn::Document* /*document*/,
                                                           ARA::PlugIn::RegionSequence* /*regionSequence*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "regionSequenceAddedToDocument");
}

void CabbageProcessor::araRegionSequenceRemovedFromDocument(ARA::PlugIn::Document* /*document*/,
                                                               ARA::PlugIn::RegionSequence* /*regionSequence*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "regionSequenceRemovedFromDocument");
}

void CabbageProcessor::araAudioSourceAddedToDocument(ARA::PlugIn::Document* /*document*/,
                                                        ARA::PlugIn::AudioSource* audioSource)
{
    {
        std::lock_guard<std::mutex> lk(araMutex);
        updateState(araUpdateCounter, araLastEventType, "audioSourceAddedToDocument");
    }
    if (audioSource && isMyAudioSource(audioSource))
    {
        lattice::logInfo << "ARA: audio source added — queuing analysis for '"
                         << (audioSource->getName() ? audioSource->getName() : "?") << "'";
        enqueueAraSource(audioSource);
    }
}

void CabbageProcessor::araAudioSourceRemovedFromDocument(ARA::PlugIn::Document* /*document*/,
                                                            ARA::PlugIn::AudioSource* audioSource)
{
    const char* name = audioSource ? audioSource->getName() : nullptr;
    if (name)
    {
        std::lock_guard<std::mutex> lk(araMutex);
        cabbage::ARADataPool::instance().removeByName(name);
    }
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioSourceRemovedFromDocument");
}

void CabbageProcessor::araDocumentWillDestroy(ARA::PlugIn::Document* /*document*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "documentWillDestroy");
}

void CabbageProcessor::araDocumentPropertiesWillUpdate(ARA::PlugIn::Document* /*document*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "documentPropertiesWillUpdate");
}

void CabbageProcessor::araMusicalContextPropertiesWillUpdate(ARA::PlugIn::MusicalContext* /*musicalContext*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "musicalContextPropertiesWillUpdate");
}

void CabbageProcessor::araRegionSequenceAddedToMusicalContext(ARA::PlugIn::MusicalContext* musicalContext,
                                                                 ARA::PlugIn::RegionSequence* regionSequence)
{
    {
        std::lock_guard<std::mutex> lk(araMutex);
        updateState(araUpdateCounter, araLastEventType, "regionSequenceAddedToMusicalContext");
    }
    if (regionSequence)
    {
        const char* name = regionSequence->getName();
        int orderIndex = regionSequence->getOrderIndex();
        int mcIdx = cabbage::ARADataPool::instance().getMusicalContextIndex(musicalContext);
        const ARA::ARAColor* color = regionSequence->getColor();
        float cr = color ? color->r : 0.0f;
        float cg = color ? color->g : 0.0f;
        float cb = color ? color->b : 0.0f;
        cabbage::ARADataPool::instance().addOrUpdateRegionSequence(
            regionSequence, name ? name : "", orderIndex, mcIdx, cr, cg, cb);
    }
}

void CabbageProcessor::araRegionSequenceRemovedFromMusicalContext(ARA::PlugIn::MusicalContext* /*musicalContext*/,
                                                                     ARA::PlugIn::RegionSequence* regionSequence)
{
    if (regionSequence)
        cabbage::ARADataPool::instance().removeRegionSequence(regionSequence);
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "regionSequenceRemovedFromMusicalContext");
}

void CabbageProcessor::araMusicalContextWillDestroy(ARA::PlugIn::MusicalContext* /*musicalContext*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "musicalContextWillDestroy");
}

void CabbageProcessor::araRegionSequencePropertiesWillUpdate(ARA::PlugIn::RegionSequence* /*regionSequence*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "regionSequencePropertiesWillUpdate");
}

void CabbageProcessor::araRegionSequenceWillDestroy(ARA::PlugIn::RegionSequence* /*regionSequence*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "regionSequenceWillDestroy");
}

void CabbageProcessor::araAudioSourcePropertiesWillUpdate(ARA::PlugIn::AudioSource* /*audioSource*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioSourcePropertiesWillUpdate");
}

void CabbageProcessor::araAudioSourceDeactivatedForUndo(ARA::PlugIn::AudioSource* /*audioSource*/,
                                                           bool /*deactivate*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioSourceDeactivatedForUndo");
}

void CabbageProcessor::araAudioSourceReactivatedFromUndo(ARA::PlugIn::AudioSource* /*audioSource*/,
                                                           bool /*deactivate*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioSourceReactivatedFromUndo");
}

void CabbageProcessor::araAudioModificationAddedToAudioSource(ARA::PlugIn::AudioSource* audioSource,
                                                                 ARA::PlugIn::AudioModification* audioModification)
{
    {
        std::lock_guard<std::mutex> lk(araMutex);
        updateState(araUpdateCounter, araLastEventType, "audioModificationAddedToAudioSource");
    }
    if (audioModification)
    {
        const char* name = audioModification->getName();
        const std::string persistentId = audioModification->getPersistentID();
        cabbage::ARADataPool::instance().addOrUpdateAudioModification(
            audioModification, name ? name : "", persistentId.c_str(), 0);
    }
}

void CabbageProcessor::araAudioModificationRemovedFromAudioSource(ARA::PlugIn::AudioSource* /*audioSource*/,
                                                                     ARA::PlugIn::AudioModification* audioModification)
{
    if (audioModification)
        cabbage::ARADataPool::instance().removeAudioModification(audioModification);
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioModificationRemovedFromAudioSource");
}

void CabbageProcessor::araAudioSourceWillDestroy(ARA::PlugIn::AudioSource* audioSource)
{
    const char* name = audioSource ? audioSource->getName() : nullptr;
    if (name)
    {
        std::lock_guard<std::mutex> lk(araMutex);
        cabbage::ARADataPool::instance().removeByName(name);
    }
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioSourceWillDestroy");
}

void CabbageProcessor::araAudioModificationPropertiesWillUpdate(ARA::PlugIn::AudioModification* /*audioModification*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioModificationPropertiesWillUpdate");
}

void CabbageProcessor::araAudioModificationDeactivatedForUndo(ARA::PlugIn::AudioModification* /*audioModification*/,
                                                                bool /*deactivate*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioModificationDeactivatedForUndo");
}

void CabbageProcessor::araAudioModificationReactivatedFromUndo(ARA::PlugIn::AudioModification* /*audioModification*/,
                                                                 bool /*deactivate*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioModificationReactivatedFromUndo");
}

void CabbageProcessor::araPlaybackRegionAddedToAudioModification(ARA::PlugIn::AudioModification* /*audioModification*/,
                                                                   ARA::PlugIn::PlaybackRegion* /*playbackRegion*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "playbackRegionAddedToAudioModification");
}

void CabbageProcessor::araPlaybackRegionRemovedFromAudioModification(ARA::PlugIn::AudioModification* /*audioModification*/,
                                                                       ARA::PlugIn::PlaybackRegion* /*playbackRegion*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "playbackRegionRemovedFromAudioModification");
}

void CabbageProcessor::araAudioModificationWillDestroy(ARA::PlugIn::AudioModification* /*audioModification*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "audioModificationWillDestroy");
}

void CabbageProcessor::araPlaybackRegionPropertiesWillUpdate(ARA::PlugIn::PlaybackRegion* /*playbackRegion*/)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "playbackRegionPropertiesWillUpdate");
}

void CabbageProcessor::araPlaybackRegionWillDestroy(ARA::PlugIn::PlaybackRegion* playbackRegion)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "playbackRegionWillDestroy");
    cabbage::ARADataPool::instance().removePlaybackRegion(playbackRegion);
}

void CabbageProcessor::araNotifySelection(const ARA::PlugIn::ViewSelection* selection)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "notifySelection");
    nlohmann::json regions = nlohmann::json::array();
    double trStart = 0.0;
    double trDuration = 0.0;
    if (selection)
    {
        // Get overall selection time range (arrangement timeline position)
        const auto& timeRange = selection->getTimeRange();
        if (timeRange != nullptr)
        {
            trStart = timeRange->start;
            trDuration = timeRange->duration;
        }

        for (const auto* pr : selection->getPlaybackRegions())
        {
            nlohmann::json obj;
            const auto* src = pr->getAudioModification()->getAudioSource();
            const char* name = src ? src->getName() : "";
            const std::string srcName = name ? name : "";
            obj["name"] = srcName;
            const auto startSamples = static_cast<double>(pr->getStartInAudioModificationSamples());
            obj["startInSamples"] = startSamples;
            const auto durSamples = static_cast<double>(pr->getDurationInAudioModificationSamples());
            obj["durationInSamples"] = durSamples;
            const double sr = src ? src->getSampleRate() : 0.0;
            obj["duration"] = (sr > 0.0) ? durSamples / sr : 0.0;

            // Per-region arrangement position (where this clip sits on the host timeline)
            obj["playbackStart"] = static_cast<double>(pr->getStartInPlaybackTime());
            obj["playbackDuration"] = static_cast<double>(pr->getDurationInPlaybackTime());

            regions.push_back(std::move(obj));

            // Also update source-level region data so cabbageAraGet("regionStart", idx) stays in sync
            if (!srcName.empty())
            {
                cabbage::ARADataPool::instance().updateRegionByName(srcName, startSamples, durSamples);

                // Also store audio source crop position from the selection
                const auto asStart = static_cast<double>(pr->getStartInAudioModificationTime());
                const auto asDur = static_cast<double>(pr->getDurationInAudioModificationTime());
                cabbage::ARADataPool::instance().updateRegionTimeByName(
                    srcName, asStart, asDur);
            }
        }
    }
    cabbage::ARADataPool::instance().updateAraState("editorView",
        {{"selectedRegions", regions}, {"hiddenSequenceCount", 0},
         {"timeRangeStart", trStart}, {"timeRangeDuration", trDuration}});
}

void CabbageProcessor::araNotifyHideRegionSequences(
    const std::vector<ARA::PlugIn::RegionSequence*>& hiddenSequences)
{
    std::lock_guard<std::mutex> lk(araMutex);
    updateState(araUpdateCounter, araLastEventType, "notifyHideRegionSequences");
    auto state = cabbage::ARADataPool::instance().getAraState();
    auto ev = state.value("editorView", nlohmann::json::object());
    ev["hiddenSequenceCount"] = static_cast<double>(hiddenSequences.size());
    cabbage::ARADataPool::instance().updateAraState("editorView", ev);
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
        cabbage::ARADataPool::instance().updateAraState("currentIndex", static_cast<double>(poolIdx));
        cabbage::ARADataPool::instance().updateAraState("update", static_cast<double>(araUpdateCounter));
    }

    lattice::logDebug << "ARA: source '" << sourceName << "' stored at pool index " << poolIdx;
}

#endif // LATTICE_HAS_ARA

// ---------------------------------------------------------------------------
// Thread-safe getter for the last ARA event type (returns copy under lock).
// Available in both LATTICE_HAS_ARA and CabbageApp builds.
// ---------------------------------------------------------------------------
std::string CabbageProcessor::getAraLastEventType()
{
    std::lock_guard<std::mutex> lk(araMutex);
    return araLastEventType;
}

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
        cabbage::ARADataPool::instance().updateAraState("currentIndex", static_cast<double>(poolIdx));
        cabbage::ARADataPool::instance().updateAraState("update", static_cast<double>(araUpdateCounter));
    }

    lattice::logInfo << "ARA standalone: '" << sourceName << "' stored at pool index " << poolIdx;
}
#endif // CabbageApp

#endif // LATTICE_HAS_ARA || CabbageApp
