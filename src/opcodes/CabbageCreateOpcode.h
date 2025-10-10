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

/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <plugin.h>
#include "CabbageOpcodes.h"

struct CabbageCreate : csnd::InPlug<1>, CabbageOpcodes<1>
{
    int init();
};

