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
#include <fstream>
#include <regex>
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
    if (araCsdPath.empty())
    {
        lattice::logWarning << "ARA: no companion .ara.csd found — skipping analysis";
        return;
    }

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
    const nlohmann::json data = runAraCsdWithPcm(pcm, numChannels, sr,
                                                  static_cast<int64_t>(totalSamples),
                                                  sourceName, &araWorkerRunning);
    AraForwardPayload forwardPayload;
    {
        std::lock_guard<std::mutex> lk(araMutex);
        AraSourceResult result;
        result.sourceName = sourceName;
        result.samples    = static_cast<double>(totalSamples);
        result.channels   = static_cast<double>(numChannels);
        result.sr         = sr;
        result.duration   = static_cast<double>(totalSamples) / sr;
        result.data       = data;

        auto it = std::find_if(araSourceResults.begin(), araSourceResults.end(),
                               [&](const AraSourceResult& r) { return r.sourceName == sourceName; });
        int idx;
        if (it != araSourceResults.end())
        {
            *it  = result;
            idx  = static_cast<int>(std::distance(araSourceResults.begin(), it));
        }
        else
        {
            araSourceResults.push_back(result);
            idx = static_cast<int>(araSourceResults.size()) - 1;
        }
        araCurrentSourceIndex = idx;  // this analysis was already gated by isMyAudioSource() at enqueue time
        forwardPayload = {araSourceResults, idx};
    }
    // Enqueue a full snapshot for the idle thread — do NOT call forwardAllChannelsToProcessor()
    // directly here: that would manipulate Csound's channel table from the worker thread while
    // the audio thread may be running PerformKsmps, causing heap corruption.
    araForwardQueue.try_enqueue(std::move(forwardPayload));
}

#endif // LATTICE_HAS_ARA

// ---------------------------------------------------------------------------
// Shared Csound execution helper — returns the collected <CabbageARA> channel
// values as JSON so callers can accumulate across multiple source runs.
// Forwarding to the main Csound instance is handled by forwardAllChannelsToProcessor().
// ---------------------------------------------------------------------------
nlohmann::json CabbageProcessor::runAraCsdWithPcm(const std::vector<std::vector<float>>& pcm,
                                         int numChannels, double sr,
                                         int64_t totalSamples,
                                         const std::string& sourceName,
                                         std::atomic<bool>* runningFlag)
{
    // Csound has process-global state; serialise concurrent analyses across
    // all plugin instances so two worker threads never run Csound simultaneously.
    static std::mutex csoundGlobalMutex;
    std::lock_guard<std::mutex> csoundLock(csoundGlobalMutex);

    auto cs = std::make_unique<Csound>();
    cs->SetOption("-n");
    cs->SetOption("-d");
    cs->SetOption(std::string("--nchnls=" + std::to_string(numChannels)).c_str());
    cs->SetOption(std::string("--nchnls_i=" + std::to_string(numChannels)).c_str());
    cs->SetOption(std::string("--sample-rate=" + std::to_string(static_cast<int>(sr))).c_str());

    if (cs->Compile(araCsdPath.c_str()) != 0)
    {
        lattice::logError << "ARA: Csound compile failed for " << araCsdPath;
        return {};
    }

    cs->Start();

    // Set source-info channels on the offline Csound before performance.
    lattice::logInfo << "ARA: running .ara.csd for source '" << sourceName << "'";
    cs->SetStringChannel ("ARA_SOURCE_NAME",     const_cast<char*>(sourceName.c_str()));
    cs->SetControlChannel("ARA_SOURCE_SAMPLES",  static_cast<MYFLT>(totalSamples));
    cs->SetControlChannel("ARA_SOURCE_CHANNELS", static_cast<MYFLT>(numChannels));
    cs->SetControlChannel("ARA_SOURCE_SR",       static_cast<MYFLT>(sr));
    cs->SetControlChannel("ARA_SOURCE_DURATION", static_cast<MYFLT>(static_cast<double>(totalSamples) / sr));
    cs->SetControlChannel("ARA_ENDED",           static_cast<MYFLT>(0));

    const int   ksmps = cs->GetKsmps();
    MYFLT*      spin  = cs->GetSpin();
    const MYFLT scale = cs->Get0dBFS();
    int64_t samplePos = 0;

    for (;;)
    {
        if (runningFlag && !runningFlag->load(std::memory_order_relaxed))
            break;

        for (int i = 0; i < ksmps; ++i)
            for (int c = 0; c < numChannels; ++c)
            {
                const float v = (samplePos + i < totalSamples)
                                    ? pcm[static_cast<size_t>(c)][static_cast<size_t>(samplePos + i)]
                                    : 0.0f;
                spin[i * numChannels + c] = static_cast<MYFLT>(v) * scale;
            }

        samplePos += ksmps;
        if (samplePos >= totalSamples)
            cs->SetControlChannel("ARA_ENDED", static_cast<MYFLT>(1));
        if (cs->PerformKsmps() != 0) break;
        if (samplePos >= totalSamples) break;
    }

    // Drain any diagnostic messages from the .ara.csd.
    while (cs->GetMessageCnt() > 0)
    {
        const char* msg = cs->GetFirstMessage();
        if (msg) lattice::logDebug << "ARA: " << msg;
        cs->PopFirstMessage();
    }

//    lattice::logDebug << "ARA: reading back " << araChannelDefs.size() << " channel(s) after " << samplePos << " samples";
    nlohmann::json data = nlohmann::json::object();
    for (const auto& ch : araChannelDefs)
    {
        if (ch.type == "number")
        {
            int err = 0;
            const MYFLT val = cs->GetControlChannel(ch.id.c_str(), &err);
//            lattice::logDebug << "ARA: GetControlChannel '" << ch.id << "' err=" << err << " val=" << val;
            if (err == 0) data[ch.id] = static_cast<double>(val);
        }
        else if (ch.type == "string")
        {
            char buf[4096] = {};
            cs->GetStringChannel(ch.id.c_str(), buf);
            const std::string raw = buf;
//            lattice::logDebug << "ARA: GetStringChannel '" << ch.id << "' raw='" << raw << "'";
            if (!raw.empty())
                data[ch.id] = nlohmann::json::accept(raw) ? nlohmann::json::parse(raw)
                                                           : nlohmann::json(raw);
        }
    }

//    lattice::logDebug << "ARA: runAraCsdWithPcm complete for source '" << sourceName << "', returning " << data.size() << " channel(s)";
    return data;
}

// ---------------------------------------------------------------------------
// Forward all accumulated source results to the main Csound instance.
//
// Numeric metadata channels (ARA_SOURCE_SAMPLES, ARA_SOURCE_SRS, etc.) and
// all declared <CabbageARA> number channels are set as Csound k-array
// channels so users can index by source:
//
//   kRMS[]   chnget "ara_rms"
//   kMyRms = kRMS[chnget("ARA_CURRENT_SOURCE_INDEX")]
//
// String channels (ARA_SOURCE_NAMES and declared string channels) are set as
// Csound S-arrays so users can index by source:
//
//   SNames[]   chnget "ARA_SOURCE_NAMES"
//   SSrcName = SNames[chnget("ARA_CURRENT_SOURCE_INDEX")]
//
// Bare scalar convenience channels are also set for this instance's own
// source so single-track plugins need no changes.
// ---------------------------------------------------------------------------
void CabbageProcessor::forwardAllChannelsToProcessor(const AraForwardPayload& payload)
{
    const std::vector<AraSourceResult>& results    = payload.results;
    const int                           currentIdx = payload.currentIdx;

    const int count = static_cast<int>(results.size());

    if (count == 0)
    {
        cabbage.setControlChannel("ARA_SOURCE_COUNT",         0.0f);
        cabbage.setControlChannel("ARA_CURRENT_SOURCE_INDEX", static_cast<float>(currentIdx));
        return;
    }

    Csound* cppCsound = cabbage.getCsound();
    if (!cppCsound)
    {
        lattice::logWarning << "ARA: forwardAllChannelsToProcessor — main Csound not ready yet, re-queuing payload";
        araForwardQueue.try_enqueue(payload);
        return;
    }
    CSOUND* cs = cppCsound->GetCsound();

    // csoundInitArrayChannel does NOT resize an existing channel — it returns the
    // already-allocated ARRAYDAT unchanged.  To avoid out-of-range errors when the
    // source count grows across sessions we always allocate a fixed maximum and pad
    // unused slots with 0 / empty string.  ARA_SOURCE_COUNT tells Csound code the
    // number of valid entries.
    static constexpr int ARA_MAX_SOURCES = 64;

    // Helper: allocate/update a 1-D k-array channel on the main Csound instance.
    auto setKArray = [&](const char* name, const std::vector<MYFLT>& values)
    {
        int32_t sz = ARA_MAX_SOURCES;
        ARRAYDAT* arr = csoundInitArrayChannel(cs, name, "k", 1, &sz);
        if (!arr)
        {
            lattice::logError << "ARA: csoundInitArrayChannel returned null for k-array '" << name << "'";
            return;
        }
        // Write real values then pad the rest with 0.
        std::vector<MYFLT> padded(ARA_MAX_SOURCES, static_cast<MYFLT>(0));
        for (size_t i = 0; i < values.size() && i < ARA_MAX_SOURCES; ++i)
            padded[i] = values[i];
        csoundSetArrayData(arr, padded.data());
    };

    // Helper: allocate/update a 1-D S-array channel on the main Csound instance.
    auto setStrArray = [&](const char* name, const std::vector<std::string>& strs)
    {
        int32_t sz = ARA_MAX_SOURCES;
        ARRAYDAT* arr = csoundInitArrayChannel(cs, name, "S", 1, &sz);
        if (!arr)
        {
            lattice::logError << "ARA: csoundInitArrayChannel returned null for S-array '" << name << "'";
            return;
        }
        const int32_t slots_n = arr->sizes ? arr->sizes[0] : sz;
        std::vector<STRINGDAT> sd(static_cast<size_t>(slots_n));
        for (int32_t i = 0; i < slots_n; ++i)
        {
            const std::string& s = (i < static_cast<int32_t>(strs.size())) ? strs[static_cast<size_t>(i)] : "";
            sd[static_cast<size_t>(i)].data = cs->Strdup(cs, s.c_str());
            sd[static_cast<size_t>(i)].size = static_cast<int32_t>(s.size()) + 1;
        }
        csoundSetArrayData(arr, sd.data());
    };

    // Numeric source metadata → k-array channels.
    {
        std::vector<MYFLT> samples(count), channels(count), srs(count), durations(count);
        for (int i = 0; i < count; ++i)
        {
            samples[i]   = static_cast<MYFLT>(results[i].samples);
            channels[i]  = static_cast<MYFLT>(results[i].channels);
            srs[i]       = static_cast<MYFLT>(results[i].sr);
            durations[i] = static_cast<MYFLT>(results[i].duration);
        }
        setKArray("ARA_SOURCE_SAMPLES",   samples);
        setKArray("ARA_SOURCE_CHANNELS",  channels);
        setKArray("ARA_SOURCE_SRS",       srs);
        setKArray("ARA_SOURCE_DURATIONS", durations);
    }

    // Source names → S-array channel.
    std::vector<std::string> names(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        names[static_cast<size_t>(i)] = results[i].sourceName;
    setStrArray("ARA_SOURCE_NAMES", names);

    // Declared <CabbageARA> channels.
    for (const auto& ch : araChannelDefs)
    {
        if (ch.type == "number")
        {
            std::vector<MYFLT> vals(count, static_cast<MYFLT>(0));
            for (int i = 0; i < count; ++i)
                if (results[i].data.contains(ch.id))
                    vals[i] = static_cast<MYFLT>(results[i].data[ch.id].get<double>());
            setKArray(ch.id.c_str(), vals);
        }
        else if (ch.type == "string")
        {
            std::vector<std::string> vals(static_cast<size_t>(count));
            for (int i = 0; i < count; ++i)
                if (results[i].data.contains(ch.id))
                    vals[static_cast<size_t>(i)] = results[i].data[ch.id].is_string()
                        ? results[i].data[ch.id].get<std::string>()
                        : results[i].data[ch.id].dump();
            setStrArray(ch.id.c_str(), vals);
        }
    }

    // Derive the current source for THIS plugin instance.
    //
    // Strategy:
    //  1. Primary: iterate araAnalysedSources and call isMyAudioSource() at forward
    //     time (idle thread). By the time analysis completes the playback renderer
    //     usually has regions assigned even when it didn't at queue time — so this
    //     correctly distinguishes instances that share the fallback analysis path.
    //  2. Fallback: if isMyAudioSource() still finds nothing (host has no
    //     PlaybackRenderer role), use payload.currentIdx with a name-match to
    //     keep the index consistent with the ARA_SOURCE_NAMES array.
    int verifiedIdx = currentIdx;
    std::string currentName;
    {
#if LATTICE_HAS_ARA
        std::lock_guard<std::mutex> lk(araSourcesMutex);
        for (auto* src : araAnalysedSources)
        {
            if (src && src->getName() && isMyAudioSource(src))
            {
                const std::string sname = static_cast<const char*>(src->getName());
                auto it = std::find(names.begin(), names.end(), sname);
                if (it != names.end())
                {
                    verifiedIdx = static_cast<int>(std::distance(names.begin(), it));
                    currentName = sname;
                    lattice::logDebug << "ARA: forwardAllChannels — own source identified via isMyAudioSource: '"
                                      << currentName << "' idx=" << verifiedIdx;
                    break;
                }
            }
        }
#endif
    }
    // Fallback: isMyAudioSource found nothing — derive by name-matching payload.currentIdx.
    if (currentName.empty())
    {
        if (currentIdx >= 0 && currentIdx < count)
            currentName = results[static_cast<size_t>(currentIdx)].sourceName;
        if (!currentName.empty())
        {
            auto it = std::find(names.begin(), names.end(), currentName);
            if (it != names.end())
                verifiedIdx = static_cast<int>(std::distance(names.begin(), it));
        }
        lattice::logDebug << "ARA: forwardAllChannels — own source via fallback: '"
                          << currentName << "' idx=" << verifiedIdx;
    }

    // Write count/index LAST so the Csound instrument never sees a non-zero count
    // before the arrays are fully populated.
    cabbage.setControlChannel("ARA_SOURCE_COUNT",         static_cast<float>(count));
    cabbage.setControlChannel("ARA_CURRENT_SOURCE_INDEX", static_cast<float>(verifiedIdx));

    // Convenience scalar: name of the currently-indexed source so .csd code can
    // read it with a plain  Sname chnget "ARA_CURRENT_SOURCE_NAME"  (S-array chnget
    // at k-rate is not supported by Csound's channel system).
    if (!currentName.empty())
        cabbage.setStringChannel("ARA_CURRENT_SOURCE_NAME", currentName);

    // Increment ARA_UPDATE so Csound code can use it as a trigger / change-detection signal.
    // Example usage in .csd:
    //   kTrig = changed(chnget:k("ARA_UPDATE"))
    //   if kTrig == 1 then ... endif
    cabbage.setControlChannel("ARA_UPDATE", static_cast<float>(++araUpdateCounter));

    lattice::logDebug << "ARA: forwardAllChannelsToProcessor — " << count
                      << "ARA update counter=" << araUpdateCounter
                      << " source(s), currentIdx=" << currentIdx
                      << " update=" << araUpdateCounter;
}

// ---------------------------------------------------------------------------
// Parse the <CabbageARA> section of an .ara.csd file and return the raw JSON.
// Callers extract what they need:
//   j["channels"]  — array of { "id", "type" } output channel declarations
//   j["testFile"]  — (CabbageApp only) local audio file path for standalone testing
// Returns an empty object if the section is absent or malformed.
// ---------------------------------------------------------------------------
nlohmann::json CabbageProcessor::parseAraCsdSection(const std::string& csdPath)
{
    try
    {
        std::ifstream     ifs(csdPath);
        const std::string text((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
        const std::regex  re(R"(<CabbageARA>(\s*[\s\S]*?\s*)</CabbageARA>)");
        std::smatch       m;
        if (!std::regex_search(text, m, re) || m.size() < 2)
        {
            lattice::logDebug << "ARA: no <CabbageARA> section found in " << csdPath;
            return nlohmann::json::object();
        }
        return nlohmann::json::parse(m[1].str());
    }
    catch (const std::exception& e)
    {
        lattice::logDebug << "ARA: failed to parse <CabbageARA> section: " << e.what();
    }
    return nlohmann::json::object();
}

#ifdef CabbageApp
// ---------------------------------------------------------------------------
// Standalone ARA analysis: reads an audio file via Choc and feeds it through
// the same .ara.csd/runAraCsdWithPcm pipeline used by the plugin.
// Triggered when <CabbageARA> contains a "testFile" path.
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

    const nlohmann::json data = runAraCsdWithPcm(pcm, numChannels, sr, totalSamples, sourceName, nullptr);

    {
        std::lock_guard<std::mutex> lk(araMutex);
        AraSourceResult result;
        result.sourceName = sourceName;
        result.samples    = static_cast<double>(totalSamples);
        result.channels   = static_cast<double>(numChannels);
        result.sr         = sr;
        result.duration   = (sr > 0.0) ? (static_cast<double>(totalSamples) / sr) : 0.0;
        result.data       = data;
        araSourceResults.clear();   // standalone: always a single source
        araSourceResults.push_back(std::move(result));
        araCurrentSourceIndex = 0;
        araForwardQueue.try_enqueue(AraForwardPayload{araSourceResults, 0});
    }
}
#endif // CabbageApp

#endif // LATTICE_HAS_ARA || CabbageApp
