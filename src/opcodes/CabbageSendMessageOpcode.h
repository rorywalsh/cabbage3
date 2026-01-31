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
#include "CabbageOpcodes.h"

/**
 * cabbageSendMessage opcode - Send arbitrary JSON data to the frontend
 *
 * Usage:
 *   cabbageSendMessage "json string"          ; Send at i-time
 *   cabbageSendMessage kTrig, "json string"   ; Send at k-rate when triggered
 *
 * The JSON can be any valid JSON object/array. It will be sent to the frontend
 * as-is, and the frontend developer is responsible for handling the message format.
 */
struct CabbageSendMessage : csnd::InPlug<1>, CabbageOpcodes<1>
{
    int init() { return sendMessageInit(CabbageOpcodeData::PassType::Init); };
    int kperf() { return sendMessagePerf(CabbageOpcodeData::PassType::Perf); };
    int sendMessageInit(CabbageOpcodeData::PassType passType);
    int sendMessagePerf(CabbageOpcodeData::PassType passType);
};

