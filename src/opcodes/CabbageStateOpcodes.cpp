/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
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
    
    // Use the utility function to get the complete widget state
    nlohmann::json stateJson = hostData->saveWidgetState();

//    cabbage::File::writeToFile(stateFile, stateJson.dump(-1, ' ', false));
    cabbage::File::writeToFile(stateFile, stateJson.dump(4));
    
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
    
    // Read the JSON file
    std::string fileContents = cabbage::File::readFromFile(stateFile);
    
    try {
        // Parse the JSON
        nlohmann::json stateJson = nlohmann::json::parse(fileContents);
        
        // Use the utility function to load the widget state
        // This handles: widgets array update, Csound channels, parameters, and UI queue
        hostData->loadWidgetState(stateJson);
        
        lattice::logInfo << "State loaded successfully from: " << stateFile;
        return IS_OK;
        
    } catch (const nlohmann::json::exception &e) {
        lattice::logError << "Failed to parse state file: " << e.what();
        return NOT_OK;
    }
}
