/*
 * Copyright (c) 2026 Rory Walsh
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

#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <plugin.h>
#include <csound_structs.h>
#include <csound_type_system.h>

// ============================================================================
// Cabbage trigger structs — plugin structs registered with
// csound->RegisterStruct() (csound PR #3356).
//
// The struct overloads of the multi-output trigger opcodes return one of
// these instead of a separate value/trigger pair, e.g.:
//
//   res:CabbageNumTrig cabbageGetValue "channel"   ; res.val, res.trig
//   res:CabbageStrTrig cabbageGetValue "channel"   ; res.val (S), res.trig
//
// The C++ definitions below mirror the orchestra type for documentation
// only. Csound stores each member in its own CS_VAR_MEM block, so the
// members are NOT contiguous in memory and these structs must never be
// overlaid on a CS_STRUCT_VAR — opcode code reads and writes through
// CS_STRUCT_VAR::members[i]->value instead (see cabbageWriteStrMember).
// ============================================================================

struct CabbageNumTrig
{
    cs_float val;  // latest channel value (k)
    cs_float trig; // 0/1 edge trigger (k)
};

struct CabbageStrTrig
{
    STRINGDAT val; // latest string value (S)
    cs_float trig; // 0/1 edge trigger (k)
};

// Per-struct registration outcome. Engine::addOpcodes() registers whichever
// subset succeeded, so one bad member spec can no longer disable both types.
// `apiPresent` is false when the host Csound predates RegisterStruct
// (csound PR #3356). A NULL return with the name already present in the type
// pool (re-init on a live instance) counts as success.
struct CabbageTrigStructStatus
{
    bool apiPresent = false;
    bool numOk = false;
    bool strOk = false;
    const void *numType = nullptr;
    const void *strType = nullptr;
};

inline const CS_TYPE *cabbageFindStructType(CSOUND *csound, const char *name)
{
    if (csound == nullptr || name == nullptr)
        return nullptr;
    TYPE_POOL *pool = csoundGetTypePool(csound);
    if (pool == nullptr)
        return nullptr;
    return csoundGetTypeWithVarTypeName(pool, name);
}

// Registers both Cabbage trigger struct types on a Csound instance. Must be
// called before orchestra compilation (Engine::addOpcodes() does this, which
// runs before Compile()). The legacy multi-output opcodes are unaffected
// either way.
inline CabbageTrigStructStatus registerCabbageTrigStructs(CSOUND *csound)
{
    CabbageTrigStructStatus status;
    if (csound == nullptr)
        return status;
    if (csound->RegisterStruct == nullptr)
        return status;
    status.apiPresent = true;

    static const CSOUND_STRUCT_MEMBER numMembers[] = {{"val", "k"}, {"trig", "k"}};
    static const CSOUND_STRUCT_MEMBER strMembers[] = {{"val", "S"}, {"trig", "k"}};

    const CS_TYPE *numType = csound->RegisterStruct(csound, "CabbageNumTrig", numMembers, 2);
    if (numType == nullptr)
        numType = cabbageFindStructType(csound, "CabbageNumTrig"); // already registered?
    status.numType = numType;
    status.numOk = (numType != nullptr);

    const CS_TYPE *strType = csound->RegisterStruct(csound, "CabbageStrTrig", strMembers, 2);
    if (strType == nullptr)
        strType = cabbageFindStructType(csound, "CabbageStrTrig"); // already registered?
    status.strType = strType;
    status.strOk = (strType != nullptr);

    return status;
}


// Copies text into the string member `index` of a plugin struct output value.
// Follows the engine's STRINGDAT ownership rules (see string_free_internal /
// string_resize_internal in csound_standard_types.c): `size` is the allocated
// capacity, refcount 0 means we own the buffer (free before replacing an
// undersized one), refcount -1 means it is an alias (detach with a fresh
// buffer instead of touching the shared one).
inline void cabbageWriteStrMember(CSOUND *csound, CS_STRUCT_VAR *value, int32_t index, const char *text)
{
    if (csound == nullptr || value == nullptr || text == nullptr || index < 0 || index >= value->memberCount)
        return;

    STRINGDAT *member = reinterpret_cast<STRINGDAT *>(&value->members[index]->value);
    const size_t length = strlen(text) + 1;

    if (member->data != nullptr && member->refcount == 0 && member->size >= length)
    {
        memcpy(member->data, text, length);
        return;
    }

    // Owned but too small: free first. Aliased/shared buffers are left alone.
    if (member->data != nullptr && member->refcount == 0)
        csound->Free(csound, member->data);

    member->data = static_cast<char *>(csound->Calloc(csound, length));
    if (member->data == nullptr)
    {
        member->size = 0;
        member->refcount = 0;
        return;
    }
    memcpy(member->data, text, length);
    member->size = length;
    member->refcount = 0;
}
