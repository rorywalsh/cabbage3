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

#include <fstream>
#include <string>
#include <array>
#include <algorithm>
#include <complex>
#include <cstring>
#include <iostream>

#include "CabbageOpcodes.h"
//==================================================================================
struct CabbageSetValue : csnd::InPlug<3>, CabbageOpcodes<3>
{
    MYFLT *value;
    int init() { return setValue(CabbageOpcodeData::PassType::Init); };
    int kperf() { return setValue(CabbageOpcodeData::PassType::Perf); };
    int setValue(int init);
    int kCycles = 0;
};

struct CabbageSetInitString : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init() { return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int setIdentifier(int init);
};

struct CabbageSetPerfString : csnd::InPlug<64>, CabbageOpcodes<64>
{
    std::string lastValue = ""; // Sentinel for string comparison
    int init() { return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf() { return setIdentifier(CabbageOpcodeData::PassType::Perf); };
    int setIdentifier(int init);
};

struct CabbageSetInitMYFLT : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init() { return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int setIdentifier(int init);
};

struct CabbageSetPerfMYFLT : csnd::InPlug<64>, CabbageOpcodes<64>
{
    MYFLT lastValue = -1.0; // Sentinel value
    int init() { return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf() { return setIdentifier(CabbageOpcodeData::PassType::Perf); };
    int setIdentifier(int init);
};

struct CabbageSetPerfMYFLTArray : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init() { return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf() { return setIdentifier(CabbageOpcodeData::PassType::Perf); };
    int setIdentifier(int init);
};

struct CabbageSetInitMYFLTArray : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init() { return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int setIdentifier(int init);
};
