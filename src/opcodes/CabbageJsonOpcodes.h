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

#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>

/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <plugin.h>
#include "CabbageOpcodes.h"

// SVal cabbageJsonGet SJson, SPath
// Query a JSON document with dot notation ("nodes.0.params.rate").
// Strings come back verbatim, other values as compact JSON. Missing -> "".
struct CabbageJsonGetString : csnd::Plugin<1, 2>
{
    std::string lastDoc;
    std::string lastPath;
    std::string lastResult;
    int init() { return get(false); };
    int kperf() { return get(true); };
    int get(bool perf);
};

// SVal, kTrig cabbageJsonGet SJson, SPath
// As above, with kTrig firing 1 when the result string changes.
struct CabbageJsonGetStringWithTrigger : csnd::Plugin<2, 2>
{
    std::string lastDoc;
    std::string lastPath;
    std::string lastResult;
    int kperf();
};

// kval cabbageJsonGet SJson, SPath / ival cabbageJsonGet SJson, SPath
// Numbers direct, numeric strings parsed, booleans -> 1/0, else 0.
struct CabbageJsonGetNumber : csnd::Plugin<1, 2>
{
    std::string lastDoc;
    std::string lastPath;
    double lastResult = 0.0;
    int init() { return get(); };
    int kperf() { return get(); };
    int get();
};

// SArr[] cabbageJsonGet SJson, SPath (i-time only)
// Per-element string rules. Missing/non-array -> empty array.
struct CabbageJsonGetStringArray : csnd::Plugin<1, 2>
{
    int init() { return get(); };
    int get();
};

// kArr[] cabbageJsonGet SJson, SPath (i-time only)
// Per-element numeric rules. Missing/non-array -> empty array.
struct CabbageJsonGetNumberArray : csnd::Plugin<1, 2>
{
    int init() { return get(); };
    int get();
};

// iHas/kHas cabbageJsonHas SJson, SPath
// 1 when the path exists (even if null), else 0.
struct CabbageJsonHas : csnd::Plugin<1, 2>
{
    int init() { return check(); };
    int kperf() { return check(); };
    int check();
};

// iLen/kLen cabbageJsonLen SJson, SPath
// Array length / object key count. Missing or scalar -> 0.
struct CabbageJsonLen : csnd::Plugin<1, 2>
{
    int init() { return check(); };
    int kperf() { return check(); };
    int check();
};

// SType cabbageJsonType SJson, SPath
// "string" | "number" | "boolean" | "array" | "object" | "null" | "missing".
struct CabbageJsonType : csnd::Plugin<1, 2>
{
    std::string lastDoc;
    std::string lastPath;
    std::string lastResult;
    int init() { return get(false); };
    int kperf() { return get(true); };
    int get(bool perf);
};

// SJson cabbageJsonSet SJson, SPath, SVal / SJson cabbageJsonSet SJson, SPath, kVal
// Returns a new document with the path set (intermediate objects created).
// Array elements replaceable by index; out-of-range or type clash -> "" + warning.
struct CabbageJsonSetString : csnd::Plugin<1, 3>
{
    int init() { return set(); };
    int kperf() { return set(); };
    int set();
};

struct CabbageJsonSetNumber : csnd::Plugin<1, 3>
{
    int init() { return set(); };
    int kperf() { return set(); };
    int set();
};
