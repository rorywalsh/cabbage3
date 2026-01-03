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
int CabbageCreate::init()
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const int argIndex = 0;
    
    
    if (!testForValidNumberOfInputs(in_count(), 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }
    
    // Get the JSON data from the first (and only) argument
    auto data = getIdentData(csound, args, true, 0, argIndex);
    data.type = CabbageOpcodeData::MessageType::Widget;
    
    // For cabbageCreate, we expect a complete JSON object, not a key-value pair
    // So parse it directly instead of using parseAndFormatJson's wrapping behavior
    try {
        std::string jsonStr = args.str_data(argIndex).data;
        data.cabbageJson = nlohmann::json::parse(jsonStr);
        data.identifier = jsonStr;
    } catch (const nlohmann::json::parse_error &e) {
        std::string errorMsg = "cabbageCreate: Invalid JSON - " + std::string(e.what()) + "\n";
        csound->message(errorMsg.c_str());
        lattice::logError << errorMsg;
        csound->init_error(errorMsg.c_str());
        // Signal the error to the Engine so it can display in the webview
        return NOTOK;
    }
    
//    lattice::logDebug << "Parsed JSON: " << data.cabbageJson.dump(4);
    
    // Check if the JSON was parsed successfully and contains required fields
    if (data.cabbageJson.is_null() || !data.cabbageJson.contains("type"))
    {
        std::string errorMsg = "cabbageCreate requires a valid JSON object with at least a 'type' field\n";
        csound->init_error(errorMsg.c_str());
        return NOTOK;
    }
    
    // Extract channel ID for the widget
    if (data.cabbageJson.contains("channels") &&
        data.cabbageJson["channels"].is_array() &&
        !data.cabbageJson["channels"].empty() &&
        data.cabbageJson["channels"][0].contains("id"))
    {
        data.channel = data.cabbageJson["channels"][0]["id"].get<std::string>();
    }
    else if (data.cabbageJson.contains("id") && data.cabbageJson["id"].is_string())
    {
        data.channel = data.cabbageJson["id"].get<std::string>();
    }
    else
    {
        std::string errorMsg = "cabbageCreate requires a widget with either an 'id' or 'channels[0].id' field\n";
        csound->init_error(errorMsg.c_str());
        return NOTOK;
    }
    
    // Create Csound channel(s) for the widget so cabbageSetValue/cabbageGetValue work
    cabbage::Parser::assignDefaultRangesToChannels(data.cabbageJson);
    
    if (data.cabbageJson.contains("channels") && data.cabbageJson["channels"].is_array())
    {
        for (auto &ch : data.cabbageJson["channels"])
        {
            if (!ch.contains("id") || !ch["id"].is_string())
                continue;
                
            const std::string channel = ch["id"].get<std::string>();
            const float defVal = ch["range"]["defaultValue"].get<float>();
            hostData->setControlChannel(channel, defVal);
        }
    }
    
    hostData->opcodeData.enqueue(data);

    return IS_OK;
}
