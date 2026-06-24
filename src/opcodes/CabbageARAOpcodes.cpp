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
// CabbageAraGetNum — iVal cabbageAraGet "property" [, iSourceIdx]
// ============================================================================

int CabbageAraGetNum::init()
{
    stateCopy = cabbage::ARADataPool::instance().getAraState();
    std::string prop(inargs.str_data(0).data);

    // Handle selectedRegion* properties (indexed into editorView.selectedRegions[])
    if (prop == "selectedRegionCount")
    {
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        outargs[0] = regions.is_array() ? static_cast<double>(regions.size()) : 0;
    }
    else if (prop.rfind("selectedRegion", 0) == 0 && in_count() >= 2 && inargs[1] >= 0)
    {
        int selIdx = static_cast<int>(inargs[1]);
        auto& regions = stateCopy["editorView"]["selectedRegions"];
        if (regions.is_array() && selIdx < static_cast<int>(regions.size()))
        {
            std::string key;
            if (prop == "selectedRegionStart")           key = "start";
            else if (prop == "selectedRegionDuration")   key = "durationSec";
            else if (prop == "selectedRegionSampleCount") key = "durationSamples";
            else                                         key = "start";
            auto& obj = regions[selIdx];
            outargs[0] = obj.contains(key) ? obj[key].get<double>() : 0;
        }
        else
        {
            outargs[0] = 0;
        }
    }
    else if (in_count() >= 2 && inargs[1] >= 0)
    {
        int idx = static_cast<int>(inargs[1]);
        auto val = getJson(stateCopy, "sources." + std::to_string(idx) + "." + prop);
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
    std::string prop(inargs.str_data(0).data);
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
