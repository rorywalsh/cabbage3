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
// CabbageAraGetSourceCount
// ============================================================================

int CabbageAraGetSourceCount::init()
{
    outargs[0] = static_cast<MYFLT>(cabbage::ARADataPool::instance().getSourceCount());
    return IS_OK;
}

int CabbageAraGetSourceCount::kperf()
{
    outargs[0] = static_cast<MYFLT>(cabbage::ARADataPool::instance().getSourceCount());
    return IS_OK;
}

// ============================================================================
// CabbageAraGetCurrentSourceIndex
// ============================================================================

int CabbageAraGetCurrentSourceIndex::init()
{
    auto* engine = getEngine(csound);
    if (engine)
        outargs[0] = static_cast<MYFLT>(engine->getProcessor().getAraCurrentSourceIndex());
    else
        outargs[0] = -1;
    return IS_OK;
}

int CabbageAraGetCurrentSourceIndex::kperf()
{
    auto* engine = getEngine(csound);
    if (engine)
        outargs[0] = static_cast<MYFLT>(engine->getProcessor().getAraCurrentSourceIndex());
    else
        outargs[0] = -1;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetCurrentSourceName
// ============================================================================

int CabbageAraGetCurrentSourceName::init()
{
    auto* engine = getEngine(csound);
    if (engine)
    {
        auto& proc = engine->getProcessor();
        auto entry = cabbage::ARADataPool::instance().getByIndex(
            static_cast<size_t>(proc.getAraCurrentSourceIndex()));
        if (entry)
        {
            outargs.str_data(0).data = csound->strdup(const_cast<char*>(entry->name.c_str()));
            outargs.str_data(0).size = static_cast<int32_t>(entry->name.size()) + 1;
            return IS_OK;
        }
    }
    outargs.str_data(0).data = csound->strdup(const_cast<char*>(""));
    outargs.str_data(0).size = 1;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetUpdate
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
// CabbageAraGetSourceName
// ============================================================================

int CabbageAraGetSourceName::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs.str_data(0).data = csound->strdup(const_cast<char*>(entry->name.c_str()));
        outargs.str_data(0).size = static_cast<int32_t>(entry->name.size()) + 1;
        return IS_OK;
    }
    csound->message("cabbageAraGetSourceName: source index out of range");
    outargs.str_data(0).data = csound->strdup(const_cast<char*>(""));
    outargs.str_data(0).size = 1;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceChannels
// ============================================================================

int CabbageAraGetSourceChannels::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->channels);
        return IS_OK;
    }
    csound->message("cabbageAraGetSourceChannels: source index out of range");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetSourceChannels::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->channels);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceSampleCount
// ============================================================================

int CabbageAraGetSourceSampleCount::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->samples);
        return IS_OK;
    }
    csound->message("cabbageAraGetSourceSampleCount: source index out of range");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetSourceSampleCount::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->samples);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceSr
// ============================================================================

int CabbageAraGetSourceSr::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->sr);
        return IS_OK;
    }
    csound->message("cabbageAraGetSourceSr: source index out of range");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetSourceSr::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->sr);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceDuration
// ============================================================================

int CabbageAraGetSourceDuration::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->duration);
        return IS_OK;
    }
    csound->message("cabbageAraGetSourceDuration: source index out of range");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetSourceDuration::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->duration);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceSamplesAudio — aOut cabbageAraGetSourceSamples aPos, iChan, iSourceIndex
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
        {
            out[i] = 0;
        }
        else
        {
            out[i] = static_cast<MYFLT>(channelData[static_cast<size_t>(pos)]);
        }
    }
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceSamplesK — kOut cabbageAraGetSourceSamples kPos, iChan, iSourceIndex
// ============================================================================

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
    {
        outargs[0] = 0;
    }
    else
    {
        outargs[0] = static_cast<MYFLT>(channelData[static_cast<size_t>(pos)]);
    }
    return IS_OK;
}

// ============================================================================
// CabbageAraGetRegionSampleStart — iStart cabbageAraGetRegionSampleStart iSourceIndex
// ============================================================================

int CabbageAraGetRegionSampleStart::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->regionStart);
        return IS_OK;
    }
    csound->message("cabbageAraGetRegionSampleStart: source index out of range");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetRegionSampleStart::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->regionStart);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetRegionSampleCount — iSamples cabbageAraGetRegionSampleCount iSourceIndex
// ============================================================================

int CabbageAraGetRegionSampleCount::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->regionDuration);
        return IS_OK;
    }
    csound->message("cabbageAraGetRegionSampleCount: source index out of range");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetRegionSampleCount::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry)
    {
        outargs[0] = static_cast<MYFLT>(entry->regionDuration);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetRegionDuration — iDur cabbageAraGetRegionDuration iSourceIndex
// ============================================================================

int CabbageAraGetRegionDuration::init()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry && entry->sr > 0.0)
    {
        outargs[0] = static_cast<MYFLT>(entry->regionDuration / entry->sr);
        return IS_OK;
    }
    csound->message("cabbageAraGetRegionDuration: source index out of range or sr is zero");
    outargs[0] = 0;
    return IS_OK;
}

int CabbageAraGetRegionDuration::kperf()
{
    int idx = static_cast<int>(inargs[0]);
    auto entry = cabbage::ARADataPool::instance().getByIndex(static_cast<size_t>(idx));
    if (entry && entry->sr > 0.0)
    {
        outargs[0] = static_cast<MYFLT>(entry->regionDuration / entry->sr);
        return IS_OK;
    }
    outargs[0] = 0;
    return IS_OK;
}

// ============================================================================
// CabbageAraGetSourceSamplesArray — kSamples[] cabbageAraGetSourceSamples iStart, iCount, iChan, iSourceIndex
// ============================================================================

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

// ============================================================================
// CabbageAraGetSourceSamplesIArray — iSamples[] cabbageAraGetSourceSamples iStart, iCount, iChan, iSourceIndex
// ============================================================================

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
