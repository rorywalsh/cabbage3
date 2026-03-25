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

#include "Cabbage.h"
#include "CabbageSetOpcodes.h"
#include "CabbageStateOpcodes.h"
#include "../CabbageParser.h"
#include "CabbageUtils.h"
#include "../CabbageProcessor.h"

//=====================================================================================
int CabbageSaveState::init()
{
    return writeDataToDisk();
}

int CabbageSaveState::kperf()
{
    return writeDataToDisk();
};

int CabbageSaveState::writeDataToDisk()
{
    const std::string stateFile = args.str_data(0).data;
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    std::thread([hostData, stateFile]() {
        nlohmann::json stateJson = hostData->saveWidgetStateValuesOnly({}, true);
        cabbage::File::writeToFile(stateFile, stateJson.dump(4));
        lattice::logInfo << "Widget state (values only) saved to: " << stateFile;
    }).detach();

    return IS_OK;
}


//=====================================================================================
int CabbageSaveStateSelected::init()
{
    const std::string stateFile = args.str_data(0).data;
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    // Extract widget IDs from the string array argument (those that get full JSON stored)
    std::unordered_set<std::string> fullJsonIds;
    csnd::Vector<STRINGDAT>& idArray = args.template vector_data<STRINGDAT>(1);
    for (size_t i = 0; i < idArray.len(); ++i)
    {
        if (idArray[i].data != nullptr && idArray[i].data[0] != '\0')
            fullJsonIds.insert(std::string(idArray[i].data));
    }

    std::thread([hostData, stateFile, fullJsonIds]() {
        nlohmann::json stateJson = hostData->saveWidgetStateValuesOnly(fullJsonIds, false);
        cabbage::File::writeToFile(stateFile, stateJson.dump(4));
        lattice::logInfo << "Widget state saved to: " << stateFile;
    }).detach();

    return IS_OK;
}

//=====================================================================================
int CabbageLoadState::init()
{
    return readDataFromDisk();
}

int CabbageLoadState::kperf()
{
    return readDataFromDisk();
};

int CabbageLoadState::readDataFromDisk()
{
    const std::string stateFile = args.str_data(0).data;
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    
    // Read asynchronously to avoid blocking audio thread
    cabbage::File::readFromFileAsync(stateFile, [hostData, stateFile](const std::string& fileContents) {
        if (fileContents.empty()) {
            lattice::logError << "Failed to read state file: " << stateFile;
            return;
        }
        
        try {
            // Parse the JSON
            nlohmann::json stateJson = nlohmann::json::parse(fileContents);
            
            // Use the utility function to load the widget state
            // This handles: widgets array update, Csound channels, parameters, and UI queue
            hostData->loadWidgetState(stateJson);
            
            lattice::logInfo << "State loaded successfully from: " << stateFile;
            
        } catch (const nlohmann::json::exception &e) {
            lattice::logError << "Failed to parse state file: " << e.what();
        }
    });
    
    return IS_OK;
}
