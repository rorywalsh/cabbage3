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
        double regionStart = 0;    // modification start in samples
        double regionDuration = 0; // modification duration in samples
    };

    static ARADataPool& instance()
    {
        static ARADataPool pool;
        return pool;
    }

    std::shared_ptr<SourceEntry> getByIndex(size_t index)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (index >= sources.size())
            return nullptr;
        return std::make_shared<SourceEntry>(sources[index]);
    }

    std::shared_ptr<SourceEntry> getByName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = nameToIndex.find(name);
        if (it == nameToIndex.end() || it->second >= sources.size())
            return nullptr;
        return std::make_shared<SourceEntry>(sources[it->second]);
    }

    size_t getSourceCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return sources.size();
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
            return idx;
        }
        size_t idx = sources.size();
        sources.push_back({name, samples, channels, sr, duration, std::move(pcmData), data});
        nameToIndex[name] = idx;
        return idx;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        sources.clear();
        nameToIndex.clear();
    }

    void updateRegion(size_t index, double start, double duration)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (index < sources.size())
        {
            sources[index].regionStart = start;
            sources[index].regionDuration = duration;
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
        }
    }

    std::mutex mutex;

private:
    ARADataPool() = default;
    ARADataPool(const ARADataPool&) = delete;
    ARADataPool& operator=(const ARADataPool&) = delete;

    std::vector<SourceEntry> sources;
    std::unordered_map<std::string, size_t> nameToIndex;
};

} // namespace cabbage
