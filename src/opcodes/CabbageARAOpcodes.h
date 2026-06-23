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

struct CabbageAraGetSourceCount : csnd::Plugin<1, 0>
{
    int init();
    int kperf();
};

struct CabbageAraGetCurrentSourceIndex : csnd::Plugin<1, 0>
{
    int init();
    int kperf();
};

struct CabbageAraGetCurrentSourceName : csnd::Plugin<1, 0>
{
    int init();
};

struct CabbageAraGetUpdate : csnd::Plugin<1, 0>
{
    int kperf();
};

struct CabbageAraGetUpdateEvent : csnd::Plugin<2, 0>
{
    int kperf();
};

struct CabbageAraGetSourceName : csnd::Plugin<1, 1>
{
    int init();
};

struct CabbageAraGetSourceChannels : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};

struct CabbageAraGetSourceSampleCount : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};

struct CabbageAraGetSourceSr : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};

struct CabbageAraGetSourceDuration : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};

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

struct CabbageAraGetRegionSampleStart : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};

struct CabbageAraGetRegionSampleCount : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};

struct CabbageAraGetRegionDuration : csnd::Plugin<1, 1>
{
    int init();
    int kperf();
};
