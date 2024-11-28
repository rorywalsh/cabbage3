/*
 * Copyright (c) 2024 Rory Walsh
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
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
    MYFLT* value;
    int init(){ return setValue(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setValue(CabbageOpcodeData::PassType::Perf); };
    int setValue(int init);
    int kCycles = 0;
};

struct CabbageSetInitString : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int setIdentifier(int init);
};

struct CabbageSetPerfString : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setIdentifier(CabbageOpcodeData::PassType::Perf); };
    int setIdentifier(int init);
};

struct CabbageSetInitMYFLT : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int setIdentifier(int init);
};

struct CabbageSetPerfMYFLT : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setIdentifier(CabbageOpcodeData::PassType::Perf); };
    int setIdentifier(int init);
};
