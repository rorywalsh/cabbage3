/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include <sstream>
#include "Cabbage.h"
#include "CabbageCreateOpcode.h"
#include "CabbageParser.h"
#include "CabbageUtils.h"

//=====================================================================================
// cabbageDump "channel" [, iIndent]
int CabbageCreate::init()
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const int argIndex = 0;
    auto data = getIdentData(csound, args, true, 0, argIndex);
    data.type = CabbageOpcodeData::MessageType::Widget;
    
        
    if (!testForValidNumberOfInputs(in_count(), 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::String);

    hostData->opcodeData.enqueue(data);

    return IS_OK;
}
