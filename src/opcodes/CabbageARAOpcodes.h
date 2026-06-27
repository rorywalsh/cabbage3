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
#undef OK
#undef _CR

#include <plugin.h>
#include <nlohmann/json.hpp>

#if LATTICE_HAS_ARA || defined(CabbageApp)

// ============================================================================
// Consolidated ARA get opcodes — read from ARADataPool::araState JSON
// ============================================================================

// iVal cabbageAraGet "property"
// iVal cabbageAraGet "property", iSourceIdx
// kVal cabbageAraGet "property"
// kVal cabbageAraGet "property", kSourceIdx
struct CabbageAraGetNum : csnd::Plugin<1, 2>
{
    nlohmann::json stateCopy;
    int init();
    int kperf();
};

// SVal cabbageAraGet "property"
// SVal cabbageAraGet "property", iSourceIdx
struct CabbageAraGetString : csnd::Plugin<1, 2>
{
    nlohmann::json stateCopy;
    int init();
};

// ============================================================================
// Update trigger opcode — keeps k-rate trigger output
// ============================================================================

struct CabbageAraGetUpdate : csnd::Plugin<1, 0>
{
    int kperf();
};

struct CabbageAraGetUpdateEvent : csnd::Plugin<2, 0>
{
    int kperf();
};

// ============================================================================
// PCM sample access — kept separate (a-rate performance)
// ============================================================================

struct CabbageAraGetSourceSamplesAudio : csnd::Plugin<1, 3>
{
    std::shared_ptr<std::vector<std::vector<float>>> pcmData;
    int sourceIndex = 0;
    int channelIndex = 0;
    int numSamples = 0;

    int init();
    int aperf();
};

struct CabbageAraGetSourceSamplesK : csnd::Plugin<1, 3>
{
    std::shared_ptr<std::vector<std::vector<float>>> pcmData;
    int sourceIndex = 0;
    int channelIndex = 0;
    int numSamples = 0;

    int init();
    int kperf();
};

struct CabbageAraGetSourceSamplesArray : csnd::Plugin<1, 4>
{
    int init();
};

struct CabbageAraGetSourceSamplesIArray : csnd::Plugin<1, 4>
{
    int init();
};

// ============================================================================
// Full JSON state — returns entire ARA data pool as JSON string
// ============================================================================

struct CabbageAraGetStateJson : csnd::Plugin<1, 2>
{
    int init();
    int kperf();
private:
    MYFLT prevTrig = 0;
};

// ============================================================================
// Diagnostic dump — prints entire ARA state to Csound output
// ============================================================================

struct CabbageAraDump : csnd::InPlug<1>
{
    int init();
    int kperf();
private:
    void araDumpState();
    MYFLT prevTrig = 0;
};
#endif // LATTICE_HAS_ARA || CabbageApp
