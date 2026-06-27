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

#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace cabbage
{

struct ARADataPool
{
    struct SourceEntry
    {
        std::string name;
        double samples = 0;
        double channels = 0;
        double sr = 0;
        double duration = 0;
        std::shared_ptr<std::vector<std::vector<float>>> pcm; // planar: pcm[channel][sample]
        nlohmann::json data; // declared <CabbageARA> channel results
        double regionStart = 0;           // modification start in samples
        double regionDuration = 0;        // modification duration in samples
        double regionStartSec = 0;        // crop start within the source (seconds)
        double regionDurationSec = 0;     // crop duration within the source (seconds)
        bool removed = false;              // marked for removal, skipped in JSON
    };

    struct PlaybackRegionEntry
    {
        std::string sourceName;           // source file name
        std::string regionSequenceName;   // track/lane name
        double regionStart = 0;           // crop start in samples (within source)
        double regionDuration = 0;        // crop duration in samples
        double playbackStart = 0;         // arrangement position in seconds
        double playbackDuration = 0;      // arrangement duration in seconds
        float colorR = 0;                 // red channel (0.0-1.0)
        float colorG = 0;                 // green channel (0.0-1.0)
        float colorB = 0;                 // blue channel (0.0-1.0)
    };

    struct MusicalContextEntry
    {
        std::string name;
        int orderIndex = 0;
        float colorR = 0;
        float colorG = 0;
        float colorB = 0;
    };

    struct RegionSequenceEntry
    {
        std::string name;
        int orderIndex = 0;
        int musicalContextIndex = 0;
        float colorR = 0;
        float colorG = 0;
        float colorB = 0;
    };

    struct AudioModificationEntry
    {
        std::string name;
        std::string persistentId;
        int audioSourceIndex = 0;
    };

    static ARADataPool& instance()
    {
        static ARADataPool pool;
        return pool;
    }

    std::shared_ptr<SourceEntry> getByIndex(size_t index)
    {
        std::lock_guard<std::mutex> lock(mutex);
        size_t visible = 0;
        for (size_t i = 0; i < sources.size(); ++i)
        {
            if (sources[i].removed)
                continue;
            if (visible == index)
                return std::make_shared<SourceEntry>(sources[i]);
            visible++;
        }
        return nullptr;
    }

    std::shared_ptr<SourceEntry> getByName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it == nameToIndex.end() || it->second >= sources.size() || sources[it->second].removed)
            return nullptr;
        return std::make_shared<SourceEntry>(sources[it->second]);
    }

    size_t getSourceCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        size_t count = 0;
        for (const auto& s : sources)
            if (!s.removed)
                count++;
        return count;
    }

    int getIndexByName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it != nameToIndex.end() && it->second < sources.size() && !sources[it->second].removed)
            return static_cast<int>(it->second);
        return -1;
    }

    size_t upsert(const std::string& name, double samples, double channels,
                  double sr, double duration,
                  std::shared_ptr<std::vector<std::vector<float>>> pcmData,
                  const nlohmann::json& data = nlohmann::json::object())
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it != nameToIndex.end())
        {
            size_t idx = it->second;
            sources[idx].samples = samples;
            sources[idx].channels = channels;
            sources[idx].sr = sr;
            sources[idx].duration = duration;
            sources[idx].pcm = std::move(pcmData);
            sources[idx].data = data;
            rebuildSourcesJson();
            return idx;
        }
        // Also check for a previously removed entry with the same name
        for (size_t i = 0; i < sources.size(); ++i)
        {
            if (sources[i].removed && sources[i].name == name)
            {
                sources[i].removed = false;
                sources[i].samples = samples;
                sources[i].channels = channels;
                sources[i].sr = sr;
                sources[i].duration = duration;
                sources[i].pcm = std::move(pcmData);
                sources[i].data = data;
                nameToIndex[name] = i;
                rebuildSourcesJson();
                return i;
            }
        }
        size_t idx = sources.size();
        sources.push_back({name, samples, channels, sr, duration, std::move(pcmData), data});
        nameToIndex[name] = idx;
        rebuildSourcesJson();
        return idx;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        sources.clear();
        nameToIndex.clear();
        playbackRegions.clear();
        musicalContexts.clear();
        musicalContextIndex.clear();
        regionSequences.clear();
        audioModifications.clear();
        rebuildSourcesJson();
        rebuildPlaybackRegionsJson();
        rebuildMusicalContextsJson();
        rebuildRegionSequencesJson();
        rebuildAudioModificationsJson();
    }

    void updateRegion(size_t index, double start, double duration)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (index < sources.size())
        {
            sources[index].regionStart = start;
            sources[index].regionDuration = duration;
            rebuildSourceJson(index);
        }
    }

    void updateRegionByName(const std::string& name, double start, double duration)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it != nameToIndex.end() && it->second < sources.size())
        {
            sources[it->second].regionStart = start;
            sources[it->second].regionDuration = duration;
            rebuildSourceJson(it->second);
        }
    }

    void updateSelectedRegionByName(const std::string& name, double startInSamples,
                                     double durationSec, double durationInSamples,
                                     double playbackStart, double playbackDuration)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto& regions = araState["editorView"]["selectedRegions"];
        if (!regions.is_array())
            return;
        for (auto& entry : regions)
        {
            if (entry.contains("name") && entry["name"].get<std::string>() == name)
            {
                entry["startInSamples"] = startInSamples;
                entry["duration"] = durationSec;
                entry["durationInSamples"] = durationInSamples;
                entry["playbackStart"] = playbackStart;
                entry["playbackDuration"] = playbackDuration;
            }
        }
    }

    void updateRegionTimeByName(const std::string& name, double startSec,
                                   double durationSec)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it != nameToIndex.end() && it->second < sources.size())
        {
            sources[it->second].regionStartSec = startSec;
            sources[it->second].regionDurationSec = durationSec;
            rebuildSourceJson(it->second);
        }
    }

    void removeByName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it != nameToIndex.end() && it->second < sources.size())
        {
            sources[it->second].removed = true;
            nameToIndex.erase(it);
            rebuildSourcesJson();
        }
    }

    // --- Playback region management (thread-safe) ---

    void addOrUpdatePlaybackRegion(void* key, const std::string& sourceName,
                                    const std::string& regionSequenceName,
                                    double regionStart, double regionDuration,
                                    double playbackStart, double playbackDuration,
                                    float colorR = 0, float colorG = 0, float colorB = 0)
    {
        std::lock_guard<std::mutex> lock(mutex);
        playbackRegions[key] = {sourceName, regionSequenceName,
                                regionStart, regionDuration,
                                playbackStart, playbackDuration,
                                colorR, colorG, colorB};
        rebuildPlaybackRegionsJson();
    }

    void removePlaybackRegion(void* key)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = playbackRegions.find(key);
        if (it == playbackRegions.end())
            return;
        std::string sourceName = it->second.sourceName;
        playbackRegions.erase(it);

        // Check if any other playback regions still reference this source
        bool stillReferenced = false;
        for (const auto& [k, entry] : playbackRegions)
        {
            if (entry.sourceName == sourceName)
            {
                stillReferenced = true;
                break;
            }
        }

        // If no playback regions reference this source, remove it from the pool
        if (!stillReferenced)
        {
            auto sit = nameToIndex.find(sourceName);
            if (sit != nameToIndex.end() && sit->second < sources.size())
            {
                sources[sit->second].removed = true;
                nameToIndex.erase(sit);
                rebuildSourcesJson();
            }
        }

        rebuildPlaybackRegionsJson();
    }

    size_t getPlaybackRegionCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return playbackRegions.size();
    }

    // --- Musical context management ---

    void addOrUpdateMusicalContext(void* key, const std::string& name,
                                    int orderIndex, float colorR, float colorG, float colorB)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = musicalContexts.find(key);
        if (it == musicalContexts.end())
        {
            // New entry - assign index based on current size
            musicalContextIndex[key] = static_cast<int>(musicalContexts.size());
        }
        musicalContexts[key] = {name, orderIndex, colorR, colorG, colorB};
        rebuildMusicalContextsJson();
    }

    int getMusicalContextIndex(void* key) const
    {
        auto it = musicalContextIndex.find(key);
        return (it != musicalContextIndex.end()) ? it->second : 0;
    }

    void removeMusicalContext(void* key)
    {
        std::lock_guard<std::mutex> lock(mutex);
        musicalContexts.erase(key);
        rebuildMusicalContextsJson();
    }

    size_t getMusicalContextCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return musicalContexts.size();
    }

    // --- Region sequence management ---

    void addOrUpdateRegionSequence(void* key, const std::string& name,
                                    int orderIndex, int musicalContextIndex,
                                    float colorR, float colorG, float colorB)
    {
        std::lock_guard<std::mutex> lock(mutex);
        regionSequences[key] = {name, orderIndex, musicalContextIndex, colorR, colorG, colorB};
        rebuildRegionSequencesJson();
    }

    void removeRegionSequence(void* key)
    {
        std::lock_guard<std::mutex> lock(mutex);
        regionSequences.erase(key);
        rebuildRegionSequencesJson();
    }

    size_t getRegionSequenceCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return regionSequences.size();
    }

    // --- Audio modification management ---

    void addOrUpdateAudioModification(void* key, const std::string& name,
                                       const std::string& persistentId, int audioSourceIndex)
    {
        std::lock_guard<std::mutex> lock(mutex);
        audioModifications[key] = {name, persistentId, audioSourceIndex};
        rebuildAudioModificationsJson();
    }

    void removeAudioModification(void* key)
    {
        std::lock_guard<std::mutex> lock(mutex);
        audioModifications.erase(key);
        rebuildAudioModificationsJson();
    }

    size_t getAudioModificationCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return audioModifications.size();
    }

    // --- JSON state access (thread-safe, read under lock) ---

    nlohmann::json getAraState()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return araState;
    }

    void updateAraState(const std::string& key, const nlohmann::json& value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        araState[key] = value;
    }

    std::mutex mutex;

private:
    ARADataPool() = default;
    ARADataPool(const ARADataPool&) = delete;
    ARADataPool& operator=(const ARADataPool&) = delete;

    void rebuildSourceJson(size_t idx)
    {
        if (idx >= sources.size())
            return;
        const auto& s = sources[idx];
        nlohmann::json obj;
        obj["name"] = s.name;
        obj["sampleCount"] = s.samples;
        obj["channels"] = s.channels;
        obj["sampleRate"] = s.sr;
        obj["duration"] = s.duration;
        obj["regionStartInSamples"] = s.regionStart;
        obj["regionDurationInSamples"] = s.regionDuration;
        obj["regionStart"] = s.regionStartSec;
        obj["regionDuration"] = s.regionDurationSec;
        if (idx < araState["sources"].size())
            araState["sources"][idx] = std::move(obj);
        else
            araState["sources"].push_back(std::move(obj));
        araState["sourceCount"] = static_cast<double>(sources.size());
    }

    void rebuildSourcesJson()
    {
        araState["sources"] = nlohmann::json::array();
        nameToIndex.clear();
        double count = 0;
        for (size_t i = 0; i < sources.size(); ++i)
        {
            if (sources[i].removed)
                continue;
            nlohmann::json obj;
            obj["name"] = sources[i].name;
            obj["sampleCount"] = sources[i].samples;
            obj["channels"] = sources[i].channels;
            obj["sampleRate"] = sources[i].sr;
            obj["duration"] = sources[i].duration;
            obj["regionStartInSamples"] = sources[i].regionStart;
            obj["regionDurationInSamples"] = sources[i].regionDuration;
            obj["regionStart"] = sources[i].regionStartSec;
            obj["regionDuration"] = sources[i].regionDurationSec;
            araState["sources"].push_back(std::move(obj));
            nameToIndex[sources[i].name] = i;
            count++;
        }
        araState["sourceCount"] = count;
    }

    void rebuildPlaybackRegionsJson()
    {
        araState["playbackRegions"] = nlohmann::json::array();
        for (const auto& [key, entry] : playbackRegions)
        {
            nlohmann::json obj;
            obj["name"] = entry.sourceName;
            obj["regionSequenceName"] = entry.regionSequenceName;
            obj["regionStartInSamples"] = entry.regionStart;
            obj["regionDurationInSamples"] = entry.regionDuration;
            obj["playbackStart"] = entry.playbackStart;
            obj["playbackDuration"] = entry.playbackDuration;
            obj["colorR"] = entry.colorR;
            obj["colorG"] = entry.colorG;
            obj["colorB"] = entry.colorB;
            araState["playbackRegions"].push_back(std::move(obj));
        }
        araState["playbackRegionCount"] = static_cast<double>(playbackRegions.size());
    }

    void rebuildMusicalContextsJson()
    {
        araState["musicalContexts"] = nlohmann::json::array();
        for (const auto& [key, entry] : musicalContexts)
        {
            nlohmann::json obj;
            obj["name"] = entry.name;
            obj["orderIndex"] = entry.orderIndex;
            obj["colorR"] = entry.colorR;
            obj["colorG"] = entry.colorG;
            obj["colorB"] = entry.colorB;
            araState["musicalContexts"].push_back(std::move(obj));
        }
        araState["musicalContextCount"] = static_cast<double>(musicalContexts.size());
    }

    void rebuildRegionSequencesJson()
    {
        araState["regionSequences"] = nlohmann::json::array();
        for (const auto& [key, entry] : regionSequences)
        {
            nlohmann::json obj;
            obj["name"] = entry.name;
            obj["orderIndex"] = entry.orderIndex;
            obj["musicalContextIndex"] = entry.musicalContextIndex;
            obj["colorR"] = entry.colorR;
            obj["colorG"] = entry.colorG;
            obj["colorB"] = entry.colorB;
            araState["regionSequences"].push_back(std::move(obj));
        }
        araState["regionSequenceCount"] = static_cast<double>(regionSequences.size());
    }

    void rebuildAudioModificationsJson()
    {
        araState["audioModifications"] = nlohmann::json::array();
        for (const auto& [key, entry] : audioModifications)
        {
            nlohmann::json obj;
            obj["name"] = entry.name;
            obj["persistentId"] = entry.persistentId;
            obj["audioSourceIndex"] = entry.audioSourceIndex;
            araState["audioModifications"].push_back(std::move(obj));
        }
        araState["audioModificationCount"] = static_cast<double>(audioModifications.size());
    }

    nlohmann::json araState = {
        {"currentIndex", -1.0},
        {"update", 0.0},
        {"lastEvent", ""},
        {"sourceCount", 0.0},
        {"sources", nlohmann::json::array()},
        {"playbackRegionCount", 0.0},
        {"playbackRegions", nlohmann::json::array()},
        {"musicalContextCount", 0.0},
        {"musicalContexts", nlohmann::json::array()},
        {"regionSequenceCount", 0.0},
        {"regionSequences", nlohmann::json::array()},
        {"audioModificationCount", 0.0},
        {"audioModifications", nlohmann::json::array()},
        {"editorView", {{"selectedRegions", nlohmann::json::array()},
                         {"hiddenSequenceCount", 0.0}}}
    };

    std::vector<SourceEntry> sources;
    std::unordered_map<std::string, size_t> nameToIndex;
    std::unordered_map<void*, PlaybackRegionEntry> playbackRegions;
    std::unordered_map<void*, MusicalContextEntry> musicalContexts;
    std::unordered_map<void*, int> musicalContextIndex;  // void* key -> stable array index
    std::unordered_map<void*, RegionSequenceEntry> regionSequences;
    std::unordered_map<void*, AudioModificationEntry> audioModifications;
};

} // namespace cabbage
