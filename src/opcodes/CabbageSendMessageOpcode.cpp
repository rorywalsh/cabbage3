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

#include <sstream>
#include "Cabbage.h"
#include "CabbageSendMessageOpcode.h"
#include "CabbageParser.h"
#include "CabbageUtils.h"

//=====================================================================================
// cabbageSendMessage "json string"
//=====================================================================================
int CabbageSendMessage::sendMessageInit(CabbageOpcodeData::PassType passType)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    if (!testForValidNumberOfInputs(in_count(), 1))
    {
        csound->init_error("cabbageSendMessage: Not enough input arguments\n");
        return NOTOK;
    }

    // Get the JSON string from the first argument
    std::string jsonStr = getSafeString(args, 0);

    // Create opcode data with the arbitrary JSON
    CabbageOpcodeData data;
    data.type = CabbageOpcodeData::MessageType::Generic;
    data.channel = "cabbageSendMessage";  // Use generic channel name for arbitrary messages

    // Try to parse the JSON to validate it
    try {
        data.cabbageJson = nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error &e) {
        std::string errorMsg = "cabbageSendMessage: Invalid JSON - " + std::string(e.what()) + "\n";
        csound->message(errorMsg.c_str());
        return NOTOK;
    }

    // Send directly to opcodeData queue (bypass channel cache for generic messages)
    hostData->opcodeData.enqueue(data);

    return IS_OK;
}

//=====================================================================================
// cabbageSendMessage kTrig, "json string"
//=====================================================================================
int CabbageSendMessage::sendMessagePerf(CabbageOpcodeData::PassType passType)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    if (!testForValidNumberOfInputs(in_count(), 2))
    {
        csound->init_error("cabbageSendMessage: Not enough input arguments\n");
        return NOTOK;
    }

    const int trigger = int(args[0]);

    // Only send when triggered
    if (trigger == 0)
    {
        return IS_OK;
    }

    // Get the JSON string from the second argument
    std::string jsonStr = getSafeString(args, 1);

    // Create opcode data with the arbitrary JSON
    CabbageOpcodeData data;
    data.type = CabbageOpcodeData::MessageType::Generic;
    data.channel = "cabbageSendMessage";  // Use generic channel name for arbitrary messages

    // Try to parse the JSON to validate it
    try {
        data.cabbageJson = nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error &e) {
        std::string errorMsg = "cabbageSendMessage: Invalid JSON - " + std::string(e.what()) + "\n";
        csound->message(errorMsg.c_str());
        return NOTOK;
    }

    // Send directly to opcodeData queue (bypass channel cache for generic messages)
    hostData->opcodeData.enqueue(data);

    return IS_OK;
}
