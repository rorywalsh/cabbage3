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

#include "Cabbage.h"
#include "CabbageProcessor.h"
#include "CabbageARADataPool.h"
#include "CabbageARAOpcodes.h"
#include <algorithm>
#include <string>

#if LATTICE_HAS_ARA || defined(CabbageApp)

static cabbage::Engine* getEngine(csnd::Csound* cs)
{
    return static_cast<cabbage::Engine*>(cs->host_data());
}

// ============================================================================
// Dot-notation JSON helper (from CabbageOpcodes.h)
// ============================================================================

static nlohmann::json getJson(const nlohmann::json& obj, const std::string& path)
{
    // Single key — check directly
    if (path.find('.') == std::string::npos)
    {
        if (obj.is_array() && !path.empty() && std::all_of(path.begin(), path.end(), ::isdigit))
        {
            size_t idx = std::stoul(path);
            return (idx < obj.size()) ? obj[idx] : nlohmann::json(nullptr);
        }
        return obj.contains(path) ? obj[path] : nlohmann::json(nullptr);
    }

    std::vector<std::string> keys;
    std::stringstream ss(path);
    std::string token;
    while (std::getline(ss, token, '.'))
        keys.push_back(token);

    nlohmann::json current = obj;
    for (const auto& key : keys)
    {
        if (current.is_array() && !key.empty() && std::all_of(key.begin(), key.end(), ::isdigit))
        {
            size_t idx = std::stoul(key);
            if (idx >= current.size())
                return nlohmann::json(nullptr);
            current = current[idx];
        }
        else if (current.contains(key))
        {
            current = current[key];
        }
        else
        {
            return nlohmann::json(nullptr);
        }
    }
    return current;
}

// ============================================================================
// Dot-notation property name mapping
// ============================================================================

static const std::unordered_map<std::string, std::string> dotNotationMap = {
    // AudioSource properties
    {"audioSource.name",                    "name"},
    {"audioSource.channels",                "channels"},
    {"audioSource.sampleCount",             "sampleCount"},
    {"audioSource.sampleRate",              "sampleRate"},
    {"audioSource.duration",                "duration"},

    // Region properties (crop within source)
    {"audioSource.region.start",            "regionStart"},
    {"audioSource.region.startInSamples",   "regionStartInSamples"},
    {"audioSource.region.duration",         "regionDuration"},
    {"audioSource.region.durationInSamples","regionDurationInSamples"},

    // Top-level counts
    {"audioSourceCount",                    "sourceCount"},
    {"musicalContextCount",                 "musicalContextCount"},
    {"regionSequenceCount",                 "regionSequenceCount"},
    {"audioModificationCount",              "audioModificationCount"},

    // PlaybackRegion properties (all regions)
    {"playbackRegion.name",                 "playbackRegionName"},
    {"playbackRegion.sequenceName",         "playbackRegionSequenceName"},
    {"playbackRegion.startInSamples",       "playbackRegionStartInSamples"},
    {"playbackRegion.durationInSamples",    "playbackRegionDurationInSamples"},
    {"playbackRegion.start",                "playbackRegionStart"},
    {"playbackRegion.duration",             "playbackRegionDuration"},
    {"playbackRegion.sourceIndex",          "playbackRegionSourceIndex"},
    {"playbackRegion.color.r",              "playbackRegionColorR"},
    {"playbackRegion.color.g",              "playbackRegionColorG"},
    {"playbackRegion.color.b",              "playbackRegionColorB"},

    // MusicalContext properties
    {"musicalContext.name",                 "musicalContextName"},
    {"musicalContext.orderIndex",           "musicalContextOrderIndex"},
    {"musicalContext.color.r",              "musicalContextColorR"},
    {"musicalContext.color.g",              "musicalContextColorG"},
    {"musicalContext.color.b",              "musicalContextColorB"},

    // RegionSequence properties
    {"regionSequence.name",                 "regionSequenceName"},
    {"regionSequence.orderIndex",           "regionSequenceOrderIndex"},
    {"regionSequence.musicalContextIndex",  "regionSequenceMusicalContextIndex"},
    {"regionSequence.color.r",              "regionSequenceColorR"},
    {"regionSequence.color.g",              "regionSequenceColorG"},
    {"regionSequence.color.b",              "regionSequenceColorB"},

    // AudioModification properties
    {"audioModification.name",              "audioModificationName"},
    {"audioModification.persistentId",      "audioModificationPersistentId"},

    // EditorView - overview (no index)
    {"editorView.timeRange.start",              "timeRangeStart"},
    {"editorView.timeRange.duration",           "timeRangeDuration"},
    {"editorView.hiddenSequenceCount",          "hiddenSequenceCount"},
    {"editorView.selectedPlaybackRegionCount",  "selectedPlaybackRegionCount"},

    // EditorView - selection region (indexed)
    {"editorView.selectedRegion.name",              "selectedRegionName"},
    {"editorView.selectedRegion.startInSamples",    "selectedRegionStartInSamples"},
    {"editorView.selectedRegion.duration",          "selectedRegionDuration"},
    {"editorView.selectedRegion.durationInSamples", "selectedRegionDurationInSamples"},
    {"editorView.selectedPlayback.start",           "selectedPlaybackStart"},
    {"editorView.selectedPlayback.duration",        "selectedPlaybackDuration"},
};

static std::string resolveProperty(const std::string& prop) {
    auto it = dotNotationMap.find(prop);
    return (it != dotNotationMap.end()) ? it->second : prop;
}

// ============================================================================
// CabbageAraGetNum — iVal cabbageAraGet "property" [, iSourceIdx]
// ============================================================================

int CabbageAraGetNum::init()
{
    stateCopy = cabbage::ARADataPool::instance().getAraState();
    std::string prop(resolveProperty(inargs.str_data(0).data));

    // Handle selectedRegion* properties (indexed into editorView.selectedRegions[])
    if (prop == "selectedPlaybackRegionCount")
    {
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        outargs[0] = regions.is_array() ? static_cast<double>(regions.size()) : 0;
    }
    else if ((prop.rfind("selectedRegion", 0) == 0 || prop == "selectedPlaybackStart" || prop == "selectedPlaybackDuration") && in_count() >= 2 && inargs[1] >= 0)
    {
        int selIdx = static_cast<int>(inargs[1]);
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        if (regions.is_array() && selIdx < static_cast<int>(regions.size()))
        {
            std::string key;
            if (prop == "selectedRegionStartInSamples")    key = "startInSamples";
            else if (prop == "selectedRegionDuration")    key = "duration";
            else if (prop == "selectedRegionDurationInSamples") key = "durationInSamples";
            else if (prop == "selectedPlaybackStart")     key = "playbackStart";
            else if (prop == "selectedPlaybackDuration")  key = "playbackDuration";
            else                                          key = "startInSamples";
            auto& obj = regions[selIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    // Handle playbackRegionCount (top-level, no index)
    else if (prop == "playbackRegionCount")
    {
        auto& prs = stateCopy["playbackRegions"];
        outargs[0] = prs.is_array() ? static_cast<double>(prs.size()) : 0;
    }
    // Handle playbackRegion* properties (indexed into playbackRegions[])
    else if (prop.rfind("playbackRegion", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int prIdx = static_cast<int>(inargs[1]);
        auto& prs = stateCopy["playbackRegions"];
        if (prs.is_array() && prIdx < static_cast<int>(prs.size()))
        {
            std::string key;
            if (prop == "playbackRegionStartInSamples")            key = "regionStartInSamples";
            else if (prop == "playbackRegionDurationInSamples")    key = "regionDurationInSamples";
            else if (prop == "playbackRegionStart")                key = "playbackStart";
            else if (prop == "playbackRegionDuration")             key = "playbackDuration";
            else if (prop == "playbackRegionSourceIndex")
            {
                auto& obj = prs[prIdx];
                std::string srcName = obj.contains("name") ? obj["name"].get<std::string>() : "";
                outargs[0] = static_cast<double>(cabbage::ARADataPool::instance().getIndexByName(srcName));
                return IS_OK;
            }
            else if (prop == "playbackRegionColorR")   key = "colorR";
            else if (prop == "playbackRegionColorG")   key = "colorG";
            else if (prop == "playbackRegionColorB")   key = "colorB";
            else key = "regionStartInSamples";
            auto& obj = prs[prIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    // Handle musicalContextCount (top-level, no index)
    else if (prop == "musicalContextCount")
    {
        auto& mcs = stateCopy["musicalContexts"];
        outargs[0] = mcs.is_array() ? static_cast<double>(mcs.size()) : 0;
    }
    // Handle musicalContext* properties (indexed into musicalContexts[])
    else if (prop.rfind("musicalContext", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int mcIdx = static_cast<int>(inargs[1]);
        auto& mcs = stateCopy["musicalContexts"];
        if (mcs.is_array() && mcIdx < static_cast<int>(mcs.size()))
        {
            std::string key;
            if (prop == "musicalContextOrderIndex")  key = "orderIndex";
            else if (prop == "musicalContextColorR") key = "colorR";
            else if (prop == "musicalContextColorG") key = "colorG";
            else if (prop == "musicalContextColorB") key = "colorB";
            else key = "orderIndex";
            auto& obj = mcs[mcIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    // Handle regionSequenceCount (top-level, no index)
    else if (prop == "regionSequenceCount")
    {
        auto& rss = stateCopy["regionSequences"];
        outargs[0] = rss.is_array() ? static_cast<double>(rss.size()) : 0;
    }
    // Handle regionSequence* properties (indexed into regionSequences[])
    else if (prop.rfind("regionSequence", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int rsIdx = static_cast<int>(inargs[1]);
        auto& rss = stateCopy["regionSequences"];
        if (rss.is_array() && rsIdx < static_cast<int>(rss.size()))
        {
            std::string key;
            if (prop == "regionSequenceOrderIndex")           key = "orderIndex";
            else if (prop == "regionSequenceMusicalContextIndex") key = "musicalContextIndex";
            else if (prop == "regionSequenceColorR")          key = "colorR";
            else if (prop == "regionSequenceColorG")          key = "colorG";
            else if (prop == "regionSequenceColorB")          key = "colorB";
            else key = "orderIndex";
            auto& obj = rss[rsIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    // Handle audioModificationCount (top-level, no index)
    else if (prop == "audioModificationCount")
    {
        auto& mods = stateCopy["audioModifications"];
        outargs[0] = mods.is_array() ? static_cast<double>(mods.size()) : 0;
    }
    else if (in_count() >= 2 && inargs[1] >= 0)
    {
        int idx = static_cast<int>(inargs[1]);
        auto val = getJson(stateCopy, "sources." + std::to_string(idx) + "." + prop);
        outargs[0] = val.is_number() ? val.get<double>() : 0;
    }
    else if (prop.rfind("timeRange", 0) == 0)
    {
        auto val = getJson(stateCopy, "editorView." + prop);
        outargs[0] = val.is_number() ? val.get<double>() : 0;
    }
    else
    {
        auto val = getJson(stateCopy, prop);
        outargs[0] = val.is_number() ? val.get<double>() : 0;
    }
    return IS_OK;
}

// ============================================================================
// CabbageAraGetNum — kperf re-reads JSON every k-cycle for live updates
// ============================================================================

int CabbageAraGetNum::kperf()
{
    stateCopy = cabbage::ARADataPool::instance().getAraState();
    std::string prop(resolveProperty(inargs.str_data(0).data));

    if (prop == "selectedPlaybackRegionCount")
    {
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        outargs[0] = regions.is_array() ? static_cast<double>(regions.size()) : 0;
    }
    else if ((prop.rfind("selectedRegion", 0) == 0 || prop == "selectedPlaybackStart" || prop == "selectedPlaybackDuration") && in_count() >= 2 && inargs[1] >= 0)
    {
        int selIdx = static_cast<int>(inargs[1]);
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        if (regions.is_array() && selIdx < static_cast<int>(regions.size()))
        {
            std::string key;
            if (prop == "selectedRegionStartInSamples")    key = "startInSamples";
            else if (prop == "selectedRegionDuration")    key = "duration";
            else if (prop == "selectedRegionDurationInSamples") key = "durationInSamples";
            else if (prop == "selectedPlaybackStart")     key = "playbackStart";
            else if (prop == "selectedPlaybackDuration")  key = "playbackDuration";
            else                                          key = "startInSamples";
            auto& obj = regions[selIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    else if (prop == "playbackRegionCount")
    {
        auto& prs = stateCopy["playbackRegions"];
        outargs[0] = prs.is_array() ? static_cast<double>(prs.size()) : 0;
    }
    else if (prop.rfind("playbackRegion", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int prIdx = static_cast<int>(inargs[1]);
        auto& prs = stateCopy["playbackRegions"];
        if (prs.is_array() && prIdx < static_cast<int>(prs.size()))
        {
            std::string key;
            if (prop == "playbackRegionStartInSamples")            key = "regionStartInSamples";
            else if (prop == "playbackRegionDurationInSamples")    key = "regionDurationInSamples";
            else if (prop == "playbackRegionStart")                key = "playbackStart";
            else if (prop == "playbackRegionDuration")             key = "playbackDuration";
            else if (prop == "playbackRegionSourceIndex")
            {
                auto& obj = prs[prIdx];
                std::string srcName = obj.contains("name") ? obj["name"].get<std::string>() : "";
                outargs[0] = static_cast<double>(cabbage::ARADataPool::instance().getIndexByName(srcName));
                return IS_OK;
            }
            else if (prop == "playbackRegionColorR")   key = "colorR";
            else if (prop == "playbackRegionColorG")   key = "colorG";
            else if (prop == "playbackRegionColorB")   key = "colorB";
            else key = "regionStartInSamples";
            auto& obj = prs[prIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    else if (prop == "musicalContextCount")
    {
        auto& mcs = stateCopy["musicalContexts"];
        outargs[0] = mcs.is_array() ? static_cast<double>(mcs.size()) : 0;
    }
    else if (prop.rfind("musicalContext", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int mcIdx = static_cast<int>(inargs[1]);
        auto& mcs = stateCopy["musicalContexts"];
        if (mcs.is_array() && mcIdx < static_cast<int>(mcs.size()))
        {
            std::string key;
            if (prop == "musicalContextOrderIndex")  key = "orderIndex";
            else if (prop == "musicalContextColorR") key = "colorR";
            else if (prop == "musicalContextColorG") key = "colorG";
            else if (prop == "musicalContextColorB") key = "colorB";
            else key = "orderIndex";
            auto& obj = mcs[mcIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    else if (prop == "regionSequenceCount")
    {
        auto& rss = stateCopy["regionSequences"];
        outargs[0] = rss.is_array() ? static_cast<double>(rss.size()) : 0;
    }
    else if (prop.rfind("regionSequence", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int rsIdx = static_cast<int>(inargs[1]);
        auto& rss = stateCopy["regionSequences"];
        if (rss.is_array() && rsIdx < static_cast<int>(rss.size()))
        {
            std::string key;
            if (prop == "regionSequenceOrderIndex")           key = "orderIndex";
            else if (prop == "regionSequenceMusicalContextIndex") key = "musicalContextIndex";
            else if (prop == "regionSequenceColorR")          key = "colorR";
            else if (prop == "regionSequenceColorG")          key = "colorG";
            else if (prop == "regionSequenceColorB")          key = "colorB";
            else key = "orderIndex";
            auto& obj = rss[rsIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    else if (prop == "audioModificationCount")
    {
        auto& mods = stateCopy["audioModifications"];
        outargs[0] = mods.is_array() ? static_cast<double>(mods.size()) : 0;
    }
    else if (in_count() >= 2 && inargs[1] >= 0)
    {
        int idx = static_cast<int>(inargs[1]);
        auto val = getJson(stateCopy, "sources." + std::to_string(idx) + "." + prop);
        outargs[0] = val.is_number() ? val.get<double>() : 0;
    }
    else if (prop.rfind("timeRange", 0) == 0)
    {
        auto val = getJson(stateCopy, "editorView." + prop);
        outargs[0] = val.is_number() ? val.get<double>() : 0;
    }
    else
    {
        auto val = getJson(stateCopy, prop);
        outargs[0] = val.is_number() ? val.get<double>() : 0;
    }
    return IS_OK;
}

// ============================================================================
// CabbageAraGetString — SVal cabbageAraGet "property" [, iSourceIdx]
// ============================================================================

int CabbageAraGetString::init()
{
    stateCopy = cabbage::ARADataPool::instance().getAraState();
    std::string prop(resolveProperty(inargs.str_data(0).data));
    std::string result;

    // Handle selectedRegionName (indexed into editorView.selectedRegions[])
    if (prop == "selectedRegionName" && in_count() >= 2 && inargs[1] >= 0)
    {
        int selIdx = static_cast<int>(inargs[1]);
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        if (regions.is_array() && selIdx < static_cast<int>(regions.size()))
        {
            auto& obj = regions[selIdx];
            if (obj.contains("name") && obj["name"].is_string())
                result = obj["name"].get<std::string>();
        }
    }
    // Handle playbackRegionName and playbackRegionSequenceName (indexed into playbackRegions[])
    else if ((prop == "playbackRegionName" || prop == "playbackRegionSequenceName") && in_count() >= 2 && inargs[1] >= 0)
    {
        int prIdx = static_cast<int>(inargs[1]);
        auto& prs = stateCopy["playbackRegions"];
        if (prs.is_array() && prIdx < static_cast<int>(prs.size()))
        {
            auto& obj = prs[prIdx];
            std::string key = (prop == "playbackRegionName") ? "name" : "regionSequenceName";
            if (obj.contains(key) && obj[key].is_string())
                result = obj[key].get<std::string>();
        }
    }
    // Handle musicalContext.name (indexed into musicalContexts[])
    else if (prop == "musicalContextName" && in_count() >= 2 && inargs[1] >= 0)
    {
        int mcIdx = static_cast<int>(inargs[1]);
        auto& mcs = stateCopy["musicalContexts"];
        if (mcs.is_array() && mcIdx < static_cast<int>(mcs.size()))
        {
            auto& obj = mcs[mcIdx];
            if (obj.contains("name") && obj["name"].is_string())
                result = obj["name"].get<std::string>();
        }
    }
    // Handle regionSequence.name (indexed into regionSequences[])
    else if (prop == "regionSequenceName" && in_count() >= 2 && inargs[1] >= 0)
    {
        int rsIdx = static_cast<int>(inargs[1]);
        auto& rss = stateCopy["regionSequences"];
        if (rss.is_array() && rsIdx < static_cast<int>(rss.size()))
        {
            auto& obj = rss[rsIdx];
            if (obj.contains("name") && obj["name"].is_string())
                result = obj["name"].get<std::string>();
        }
    }
    // Handle audioModification.name and audioModification.persistentId (indexed into audioModifications[])
    else if ((prop == "audioModificationName" || prop == "audioModificationPersistentId") && in_count() >= 2 && inargs[1] >= 0)
    {
        int modIdx = static_cast<int>(inargs[1]);
        auto& mods = stateCopy["audioModifications"];
        if (mods.is_array() && modIdx < static_cast<int>(mods.size()))
        {
            auto& obj = mods[modIdx];
            std::string key = (prop == "audioModificationName") ? "name" : "persistentId";
            if (obj.contains(key) && obj[key].is_string())
                result = obj[key].get<std::string>();
        }
    }
    else if (in_count() >= 2 && inargs[1] >= 0)
    {
        int idx = static_cast<int>(inargs[1]);
        auto val = getJson(stateCopy, "sources." + std::to_string(idx) + "." + prop);
        if (val.is_string())
            result = val.get<std::string>();
    }
    else
    {
        auto val = getJson(stateCopy, prop);
        if (val.is_string())
            result = val.get<std::string>();
    }

    outargs.str_data(0).size = static_cast<int32_t>(result.size()) + 1;
    outargs.str_data(0).data = csound->strdup(const_cast<char*>(result.c_str()));
    return IS_OK;
}

// ============================================================================
// CabbageAraGetUpdate — kTrig cabbageAraGetUpdate
// ============================================================================

int CabbageAraGetUpdate::kperf()
{
    auto* engine = getEngine(csound);
    if (engine)
        outargs[0] = static_cast<MYFLT>(engine->getProcessor().getAraUpdateCounter());
    else
        outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetUpdateEvent — SEvent, kCounter cabbageAraGetUpdateEvent
// ============================================================================

int CabbageAraGetUpdateEvent::kperf()
{
    auto* engine = getEngine(csound);
    if (engine)
    {
        auto& proc = engine->getProcessor();
        outargs[1] = static_cast<MYFLT>(proc.getAraUpdateCounter());
        auto evtType = proc.getAraLastEventType();
        outargs.str_data(0).data = csound->strdup(const_cast<char*>(evtType.c_str()));
        outargs.str_data(0).size = static_cast<int32_t>(evtType.size()) + 1;
    }
    else
    {
        outargs[1] = 0;
        outargs.str_data(0).data = csound->strdup(const_cast<char*>(""));
        outargs.str_data(0).size = 1;
    }
    return IS_OK;
}

// ============================================================================
// PCM sample access (kept separate — a-rate performance)
// ============================================================================

int CabbageAraGetSourceSamplesAudio::init()
{
    sourceIndex = static_cast<int>(inargs[2]);
    channelIndex = static_cast<int>(inargs[1]);

    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(sourceIndex));
    if (!entry)
    {
        csound->message("cabbageAraSourceSamples: source index out of range");
        return NOT_OK;
    }
    if (channelIndex < 0 || channelIndex >= static_cast<int>(entry->channels))
    {
        csound->message("cabbageAraSourceSamples: channel index out of range");
        return NOT_OK;
    }
    if (!entry->pcm || static_cast<size_t>(channelIndex) >= entry->pcm->size())
    {
        csound->message("cabbageAraSourceSamples: no PCM data for source");
        return NOT_OK;
    }

    pcmData = entry->pcm;
    numSamples = static_cast<int>((*pcmData)[static_cast<size_t>(channelIndex)].size());
    return IS_OK;
}

int CabbageAraGetSourceSamplesAudio::aperf()
{
    if (!pcmData || channelIndex >= static_cast<int>(pcmData->size()))
    {
        std::fill(outargs(0), outargs(0) + nsmps, static_cast<MYFLT>(0));
        return IS_OK;
    }

    MYFLT* aPos = inargs(0);
    MYFLT* out = outargs(0);
    const auto& channelData = (*pcmData)[static_cast<size_t>(channelIndex)];

    for (uint32_t i = offset; i < nsmps; i++)
    {
        int pos = static_cast<int>(aPos[i]);
        if (pos < 0 || pos >= numSamples)
            out[i] = 0;
        else
            out[i] = static_cast<MYFLT>(channelData[static_cast<size_t>(pos)]);
    }
    return IS_OK;
}

int CabbageAraGetSourceSamplesK::init()
{
    sourceIndex = static_cast<int>(inargs[2]);
    channelIndex = static_cast<int>(inargs[1]);

    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(sourceIndex));
    if (!entry)
    {
        csound->message("cabbageAraSourceSamples: source index out of range");
        return NOT_OK;
    }
    if (channelIndex < 0 || channelIndex >= static_cast<int>(entry->channels))
    {
        csound->message("cabbageAraSourceSamples: channel index out of range");
        return NOT_OK;
    }
    if (!entry->pcm || static_cast<size_t>(channelIndex) >= entry->pcm->size())
    {
        csound->message("cabbageAraSourceSamples: no PCM data for source");
        return NOT_OK;
    }

    pcmData = entry->pcm;
    numSamples = static_cast<int>((*pcmData)[static_cast<size_t>(channelIndex)].size());
    return IS_OK;
}

int CabbageAraGetSourceSamplesK::kperf()
{
    if (!pcmData || channelIndex >= static_cast<int>(pcmData->size()))
    {
        outargs[0] = 0;
        return IS_OK;
    }

    int pos = static_cast<int>(inargs[0]);
    const auto& channelData = (*pcmData)[static_cast<size_t>(channelIndex)];

    if (pos < 0 || pos >= numSamples)
        outargs[0] = 0;
    else
        outargs[0] = static_cast<MYFLT>(channelData[static_cast<size_t>(pos)]);
    return IS_OK;
}

int CabbageAraGetSourceSamplesArray::init()
{
    int start = static_cast<int>(inargs[0]);
    int count = static_cast<int>(inargs[1]);
    int channelIndex = static_cast<int>(inargs[2]);
    int sourceIndex = static_cast<int>(inargs[3]);

    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(sourceIndex));
    if (!entry || !entry->pcm || channelIndex < 0 || channelIndex >= static_cast<int>(entry->pcm->size()))
    {
        csnd::Vector<MYFLT>& out = outargs.myfltvec_data(0);
        out.init(csound, 1, this->insdshead);
        out[0] = 0;
        return IS_OK;
    }

    const auto& channelData = (*entry->pcm)[static_cast<size_t>(channelIndex)];
    int totalSamples = static_cast<int>(channelData.size());

    csnd::Vector<MYFLT>& out = outargs.myfltvec_data(0);
    out.init(csound, count, this->insdshead);

    for (int i = 0; i < count; i++)
    {
        int pos = start + i;
        if (pos < 0 || pos >= totalSamples)
            out[i] = 0;
        else
            out[i] = static_cast<MYFLT>(channelData[static_cast<size_t>(pos)]);
    }
    return IS_OK;
}

int CabbageAraGetSourceSamplesIArray::init()
{
    int start = static_cast<int>(inargs[0]);
    int count = static_cast<int>(inargs[1]);
    int channelIndex = static_cast<int>(inargs[2]);
    int sourceIndex = static_cast<int>(inargs[3]);

    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(sourceIndex));
    if (!entry || !entry->pcm || channelIndex < 0 || channelIndex >= static_cast<int>(entry->pcm->size()))
    {
        csnd::Vector<MYFLT>& out = outargs.myfltvec_data(0);
        out.init(csound, 1, this->insdshead);
        out[0] = 0;
        return IS_OK;
    }

    const auto& channelData = (*entry->pcm)[static_cast<size_t>(channelIndex)];
    int totalSamples = static_cast<int>(channelData.size());

    csnd::Vector<MYFLT>& out = outargs.myfltvec_data(0);
    out.init(csound, count, this->insdshead);

    for (int i = 0; i < count; i++)
    {
        int pos = start + i;
        if (pos < 0 || pos >= totalSamples)
            out[i] = 0;
        else
            out[i] = static_cast<MYFLT>(channelData[static_cast<size_t>(pos)]);
    }
    return IS_OK;
}

// ============================================================================
// CabbageAraDump — prints entire ARA state to Csound output
// ============================================================================

static std::string fmt3(double v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.3f", v);
    return buf;
}

static std::string fmt0(double v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f", v);
    return buf;
}

int CabbageAraDump::init()
{
    araDumpState();
    return IS_OK;
}

int CabbageAraDump::kperf()
{
    MYFLT trig = args[0];
    if (trig > 0 && prevTrig <= 0)
        araDumpState();
    prevTrig = trig;
    return IS_OK;
}

void CabbageAraDump::araDumpState()
{
    auto state = cabbage::ARADataPool::instance().getAraState();

    csound->message("=========================================");
    csound->message("ARA STATE DUMP");
    csound->message("=========================================");

    // Top-level state
    double update = state.value("update", 0.0);
    std::string lastEvent = state.value("lastEvent", "");
    double currentIndex = state.value("currentIndex", -1.0);
    double sourceCount = state.value("sourceCount", 0.0);
    double prCount = state.value("playbackRegionCount", 0.0);
    double mcCount = state.value("musicalContextCount", 0.0);
    double rsCount = state.value("regionSequenceCount", 0.0);
    double modCount = state.value("audioModificationCount", 0.0);

    csound->message("[Status]  Last Event:   " + lastEvent);
    csound->message("[Status]  Update:       " + fmt0(update));

    // EditorView overview
    auto& ev = state["editorView"];
    double selCount = 0.0;
    if (ev.contains("selectedRegions") && ev["selectedRegions"].is_array())
        selCount = static_cast<double>(ev["selectedRegions"].size());
    double hiddenCount = ev.value("hiddenSequenceCount", 0.0);
    double trStart = ev.value("timeRangeStart", 0.0);
    double trDur = ev.value("timeRangeDuration", 0.0);

    csound->message("[Metrics] Sources: " + fmt0(sourceCount) + "  |  Regions: " + fmt0(prCount)
        + "  |  Contexts: " + fmt0(mcCount) + "  |  Sequences: " + fmt0(rsCount)
        + "  |  Modifications: " + fmt0(modCount)
        + "  |  Selected: " + fmt0(selCount) + "  |  Hidden: " + fmt0(hiddenCount)
        + "  |  Current Index: " + fmt0(currentIndex));
    csound->message("[Time]    Host Range: " + fmt3(trStart) + " to " + fmt3(trStart + trDur)
        + " (" + fmt3(trDur) + "s)\n");

    // Musical Contexts
    csound->message("------------ MUSICAL CONTEXTS -----------");
    auto& mcs = state["musicalContexts"];
    if (!mcs.is_array() || mcs.empty())
    {
        csound->message("    [None]");
    }
    else
    {
        for (size_t i = 0; i < mcs.size(); ++i)
        {
            auto& mc = mcs[i];
            std::string name = mc.value("name", "");
            int order = mc.value("orderIndex", 0);
            float cr = mc.value("colorR", 0.0f);
            float cg = mc.value("colorG", 0.0f);
            float cb = mc.value("colorB", 0.0f);
            csound->message("[" + std::to_string(i + 1) + "] '" + (name.empty() ? "N/A" : name) + "'");
            csound->message("    Order:  " + fmt0(order));
            char colorBuf[64];
            snprintf(colorBuf, sizeof(colorBuf), "    Colour: (%.2f, %.2f, %.2f)", cr, cg, cb);
            csound->message(colorBuf);
        }
    }

    // Region Sequences
    csound->message("\n------------ REGION SEQUENCES -----------");
    auto& rss = state["regionSequences"];
    if (!rss.is_array() || rss.empty())
    {
        csound->message("    [None]");
    }
    else
    {
        for (size_t i = 0; i < rss.size(); ++i)
        {
            auto& rs = rss[i];
            std::string name = rs.value("name", "");
            int order = rs.value("orderIndex", 0);
            int mcIdx = rs.value("musicalContextIndex", 0);
            float cr = rs.value("colorR", 0.0f);
            float cg = rs.value("colorG", 0.0f);
            float cb = rs.value("colorB", 0.0f);
            csound->message("[" + std::to_string(i + 1) + "] '" + (name.empty() ? "N/A" : name) + "'");
            csound->message("    Order:     " + fmt0(order));
            csound->message("    Context:   #" + fmt0(mcIdx));
            char colorBuf[64];
            snprintf(colorBuf, sizeof(colorBuf), "    Colour:    (%.2f, %.2f, %.2f)", cr, cg, cb);
            csound->message(colorBuf);
        }
    }

    // Playback Regions
    csound->message("\n------------ PLAYBACK REGIONS ------------");
    auto& prs = state["playbackRegions"];
    if (!prs.is_array() || prs.empty())
    {
        csound->message("    [None]");
    }
    else
    {
        for (size_t i = 0; i < prs.size(); ++i)
        {
            auto& pr = prs[i];
            std::string name = pr.value("name", "");
            double start = pr.value("playbackStart", 0.0);
            double dur = pr.value("playbackDuration", 0.0);
            double srcStart = pr.value("regionStartInSamples", 0.0);
            double srcDur = pr.value("regionDurationInSamples", 0.0);
            float cr = pr.value("colorR", 0.0f);
            float cg = pr.value("colorG", 0.0f);
            float cb = pr.value("colorB", 0.0f);
            csound->message("[" + std::to_string(i + 1) + "] '" + name + "'");
            csound->message("    Start:    " + fmt3(start));
            csound->message("    Duration: " + fmt3(dur));
            csound->message("    Source Crop: Start=" + std::to_string((int)srcStart)
                + " samples, Dur=" + std::to_string((int)srcDur) + " samples");
            char colorBuf[64];
            snprintf(colorBuf, sizeof(colorBuf), "    Colour:   (%.2f, %.2f, %.2f)", cr, cg, cb);
            csound->message(colorBuf);
        }
    }

    // Selected Regions
    csound->message("\n------------ SELECTED REGIONS ------------");
    auto& selRegs = ev["selectedRegions"];
    if (!selRegs.is_array() || selRegs.empty())
    {
        csound->message("    [None]");
    }
    else
    {
        for (size_t i = 0; i < selRegs.size(); ++i)
        {
            auto& sr = selRegs[i];
            std::string name = sr.value("name", "");
            double startSamp = sr.value("startInSamples", 0.0);
            double durSec = sr.value("duration", 0.0);
            double durSamp = sr.value("durationInSamples", 0.0);
            double pbStart = sr.value("playbackStart", 0.0);
            double pbDur = sr.value("playbackDuration", 0.0);
            csound->message("[" + std::to_string(i + 1) + "] '" + name + "'");
            csound->message("    Timeline Pos: " + fmt3(pbStart) + "s (Dur: " + fmt3(pbDur));
            csound->message("    Source Crop:  Start=" + std::to_string((int)startSamp)
                + " samples, Dur=" + std::to_string((int)durSamp)
                + " samples (" + fmt3(durSec));
        }
    }

    // Audio Modifications
    csound->message("\n------------ AUDIO MODIFICATIONS ---------");
    auto& mods = state["audioModifications"];
    if (!mods.is_array() || mods.empty())
    {
        csound->message("    [None]");
    }
    else
    {
        for (size_t i = 0; i < mods.size(); ++i)
        {
            auto& mod = mods[i];
            std::string name = mod.value("name", "");
            std::string persistentId = mod.value("persistentId", "");
            csound->message("[" + std::to_string(i + 1) + "] '" + (name.empty() ? "N/A" : name) + "'");
            csound->message("    Persistent ID: '" + (persistentId.empty() ? "N/A" : persistentId) + "'");
        }
    }

    // Sources
    csound->message("\n------------ SOURCES ---------------------");
    auto& srcs = state["sources"];
    if (!srcs.is_array() || srcs.empty())
    {
        csound->message("    [None]");
    }
    else
    {
        for (size_t i = 0; i < srcs.size(); ++i)
        {
            auto& src = srcs[i];
            std::string name = src.value("name", "");
            double channels = src.value("channels", 0.0);
            double sr = src.value("sampleRate", 0.0);
            double sampCnt = src.value("sampleCount", 0.0);
            double duration = src.value("duration", 0.0);
            double regStart = src.value("regionStartInSamples", 0.0);
            double regDur = src.value("regionDurationInSamples", 0.0);
            double regStartSec = src.value("regionStart", 0.0);
            double regDurSec = src.value("regionDuration", 0.0);
            csound->message("[" + std::to_string(i + 1) + "] '" + name + "'");
            csound->message("    Channels:    " + fmt0(channels));
            csound->message("    Sample Rate: " + fmt0(sr));
            csound->message("    Sample Count: " + fmt0(sampCnt));
            csound->message("    Duration:    " + fmt3(duration));
            csound->message("    Region:      Start=" + std::to_string((int)regStart)
                + " (" + fmt3(regStartSec) + "s), Duration=" + std::to_string((int)regDur)
                + " (" + fmt3(regDurSec));
        }
    }

    csound->message("=========================================");
    return;
}

#endif // LATTICE_HAS_ARA || CabbageApp
