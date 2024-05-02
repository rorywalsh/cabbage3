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
struct CabbageSetValue : csnd::InPlug<2>, CabbageOpcodes<2>
{
    int init(){ return setAttribute(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setAttribute(CabbageOpcodeData::PassType::Perf); };
    int setAttribute(int init);
};

struct CabbageSetIdentifier : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setAttribute(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setAttribute(CabbageOpcodeData::PassType::Init); };
    int setAttribute(int init);
};
