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
#include <plugin.h>

struct CabbageSaveState : csnd::InPlug<1>
{
    int init();
    int kperf();
    int writeDataToDisk();
};

struct CabbageLoadState : csnd::InPlug<1>
{
    int init();
    int kperf();
    int readDataFromDisk();
};
