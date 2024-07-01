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
    int init(){ return setValue(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setValue(CabbageOpcodeData::PassType::Perf); };
    int setValue(int init);
};

struct CabbageSetInitStrings : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int setIdentifier(int init);
};

struct CabbageSetPerfStrings : csnd::InPlug<64>, CabbageOpcodes<64>
{
    int init(){ return setIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf(){ return setIdentifier(CabbageOpcodeData::PassType::Perf); };
    int setIdentifier(int init);
};
