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

#include <map>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <string>
#include <memory>

/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
// #undef _CR
#include <plugin.h>

class ProfilerTimer
{
  public:
    void start() { startPoint = std::chrono::high_resolution_clock::now(); }

    void stop()
    {
        auto endPoint = std::chrono::high_resolution_clock::now();

        long long start =
            std::chrono::time_point_cast<std::chrono::microseconds>(startPoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endPoint).time_since_epoch().count();
        auto duration = end - start;
        average = average + duration;
        count++;
    }

    float getAverage()
    {
        auto a = average / count;
        return a;
    }

  private:
    float average = 0.f;
    float count = 1.f;
    std::chrono::time_point<std::chrono::high_resolution_clock> startPoint;
};
//====================================================================================================

class Profiler
{
  public:
    std::map<std::string, std::unique_ptr<ProfilerTimer>> timer;
};

struct CabbageProfilerStart : csnd::InPlug<2>
{
    int init();
    int kperf();
    Profiler **profiler = nullptr;
};

struct CabbageProfilerStop : csnd::Plugin<1, 2>
{
    int kperf();
    Profiler **profiler = nullptr;
};

struct CabbageProfilerPrint : csnd::InPlug<2>
{
    int kperf();
    Profiler **profiler = nullptr;
};
