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

/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <plugin.h>
#include "CabbageSetOpcodes.h"

struct CabbageDump : csnd::InPlug<2>
{
    int init() { return dump(CabbageOpcodeData::PassType::Init); };
    int dump(int init);
};

struct CabbageDumpWithTrigger : csnd::InPlug<3>
{
    int kperf() { return dump(CabbageOpcodeData::PassType::Perf); };
    int dump(int init);
};

struct CabbageGetValue : csnd::Plugin<1, 1>
{
    MYFLT *value;
    int init() { return getValue(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getValue(CabbageOpcodeData::PassType::Perf); };
    int getValue(int init);
};

struct CabbageGetValueWithTrigger : csnd::Plugin<2, 1>
{
    MYFLT *value;
    MYFLT currentValue = 0;
    int numberOfPasses = 0;
    int triggerOnPerfPass = 0;
    int init() { return getValue(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getValue(CabbageOpcodeData::PassType::Perf); };
    int getValue(int init);
};

struct CabbageGetValueString : csnd::Plugin<1, 1>
{
    char *currentString = {};
    MYFLT *value;
    int init() { return getValue(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getValue(CabbageOpcodeData::PassType::Perf); };
    int getValue(int init);
};

struct CabbageGetValueStringWithTrigger : csnd::Plugin<2, 1>
{
    char *currentString = {};
    MYFLT *value;
    int init() { return getValue(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getValue(CabbageOpcodeData::PassType::Perf); };
    int getValue(int init);
};

struct CabbageGetMYFLT : csnd::Plugin<1, 2>, CabbageOpcodes<2>
{
    MYFLT *value;
    int init() { return getIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getIdentifier(CabbageOpcodeData::PassType::Perf); };
    int getIdentifier(int init);
};

struct CabbageGetString : csnd::Plugin<1, 2>, CabbageOpcodes<2>
{
    int init() { return getIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getIdentifier(CabbageOpcodeData::PassType::Perf); };
    int getIdentifier(int init);
};

struct CabbageGetStringArray : csnd::Plugin<1, 2>, CabbageOpcodes<2>
{
    int init() { return getIdentifier(CabbageOpcodeData::PassType::Init); };
    int kperf() { return getIdentifier(CabbageOpcodeData::PassType::Perf); };
    int getIdentifier(int init);
};

struct CabbageGetStringWithTrigger : csnd::Plugin<2, 2>, CabbageOpcodes<2>
{
    std::string currentString = {};
    int kperf() { return getIdentifier(CabbageOpcodeData::PassType::Perf); };
    int getIdentifier(int init);
};
