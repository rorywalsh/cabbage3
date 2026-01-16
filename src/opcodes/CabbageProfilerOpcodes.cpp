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

#include <sstream>
#include "CabbageProfilerOpcodes.h"

int CabbageProfilerStart::init()
{
    std::string identifier(args.str_data(0).data);
    std::string block(args.str_data(1).data);

    profiler = (Profiler **)csound->query_global_variable(identifier.c_str());
    Profiler *profilerData;

    if (profiler != nullptr)
    {
        profilerData = *profiler;
        profilerData->timer[block].reset(new ProfilerTimer());
        profilerData->timer[block]->start();
    }
    else
    {
        csound->create_global_variable(identifier.c_str(), sizeof(Profiler *));
        profiler = (Profiler **)csound->query_global_variable(identifier.c_str());
        *profiler = new Profiler();
        profilerData = *profiler;
        profilerData->timer[block].reset(new ProfilerTimer());
        profilerData->timer[block]->start();
    }

    return OK;
}

int CabbageProfilerStart::kperf()
{
    std::string identifier(args.str_data(0).data);
    std::string block(args.str_data(1).data);

    profiler = (Profiler **)csound->query_global_variable(identifier.c_str());
    Profiler *profilerData;

    if (profiler != nullptr)
    {
        profilerData = *profiler;
    }
    else
        return NOTOK;

    profilerData->timer[block]->start();
    return OK;
}

int CabbageProfilerStop::kperf()
{
    std::string identifier(inargs.str_data(0).data);
    std::string block(inargs.str_data(1).data);

    profiler = (Profiler **)csound->query_global_variable(identifier.c_str());
    Profiler *profilerData;

    if (profiler != nullptr)
    {
        profilerData = *profiler;
    }
    else
        return NOTOK;

    if (profilerData->timer[block])
    {
        profilerData->timer[block]->stop();
        outargs[0] = profilerData->timer[block]->getAverage();
    }

    return OK;
}

int CabbageProfilerPrint::kperf()
{
    std::string identifier(args.str_data(0).data);
    int trig = args[1];

    profiler = (Profiler **)csound->query_global_variable(identifier.c_str());
    Profiler* profilerData = {};

    if (profiler != nullptr)
    {
        profilerData = *profiler;
    }
    else
        return NOTOK;

    if (trig == 1)
    {
        std::map<std::string, std::unique_ptr<ProfilerTimer>>::iterator it;
        std::stringstream output = {};
        output << identifier << " | ";
        for (auto const &t : profilerData->timer)
        {
             if(t.second.get())
                 output << t.first << ":" << std::string(t.second.get()->getAverage(), 4) << "\t\t";
        }

        csound->message(output.str());
    }

    return OK;
}
