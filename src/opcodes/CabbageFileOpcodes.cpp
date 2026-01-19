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
#include "CabbageFileOpcodes.h"
#include "CabbageParser.h"
#include "CabbageUtils.h"

//=====================================================================================
int CabbageGetFiles::getFiles()
{
    const std::string directory = inargs.str_data(0).data;
    // Get the second argument (file type filter)
    const std::string fileType = inargs.str_data(1).data;
    
    if(lattice::File::directoryExists(directory))
    {
        
        auto files = lattice::File::getFilesOfType(directory, fileType);
        csnd::Vector<STRINGDAT>& out = outargs.vector_data<STRINGDAT>(0);
        out.init(csound, static_cast<int>(files.size()), this->insdshead);
        int index = 0;
        for( auto& file : files)
        {
            out[index].size = static_cast<int>(file.size() + 1); // +1 to include null terminator
            out[index].data = csound->strdup((char*)file.c_str());
            index++;
        }
    }
    else
    {
        lattice::logDebug << "cabbageGetFiles - Could not find directory at: " + directory;
        return NOT_OK;
    }
    return IS_OK;
}
//========================================================================
int CabbageCreateFileName::createFileName()
{

    const std::filesystem::path directory = inargs.str_data(0).data;
    // Get the second argument (file type filter)
    const std::string extension = inargs.str_data(1).data;
    
   const std::vector<std::string> adjectives = {
       "Red", "Blue", "Swift", "Silent", "Golden", "Bright", "Happy", "Clever",
       "Fierce", "Lucky", "Bold", "Shiny", "Calm", "Misty", "Wild", "Brave"
   };

   const std::vector<std::string> nouns = {
       "Cabbage", "Kale", "Broccoli", "Cauliflower", "Spinach", "Lettuce",
       "BrusselsSprout", "Collard", "Chard", "BokChoy", "Arugula", "Radish",
       "Turnip", "Beetroot", "Celery", "Fennel"
   };

   // --- Get existing files ---
   auto existingFiles = lattice::File::getFilesOfType(directory.string(), "*" + extension);

   // --- Random engine ---
   static std::random_device rd;
   static std::mt19937 gen(rd());
   std::uniform_int_distribution<std::size_t> adjDist(0, adjectives.size() - 1);
   std::uniform_int_distribution<std::size_t> nounDist(0, nouns.size() - 1);

   std::string fileName;
   int suffix = 0;

   // --- Loop until we find a unique name ---
   while (true) {
       std::string label = adjectives[adjDist(gen)] + nouns[nounDist(gen)];
       if (suffix > 0) {
           label += "_" + std::to_string(suffix);
       }

       fileName = label + extension;

       // Check against existing files
       if (std::find(existingFiles.begin(), existingFiles.end(), (directory / fileName).string())
           == existingFiles.end()) {
           break; // unique
       }

       ++suffix;
   }
   
    // Return the full absolute path
    std::string fullPath = (directory / fileName).string();
    outargs.str_data(0).size = int(fullPath.size() + 1);
    outargs.str_data(0).data = csound->strdup((char*)fullPath.c_str());

    return IS_OK;
}


//===========================================================================================
int CabbageJoinPath::joinPath() {
    // Require at least one argument
    if (in_count() < 1)
    {
        outargs.str_data(0).size = 1;
        outargs.str_data(0).data = csound->strdup((char*)"");
        return IS_OK;
    }

    // Start with the first argument as base
    auto &baseArg = inargs.str_data(0);
    std::string baseStr;
    if (baseArg.data != nullptr)
        baseStr = std::string(baseArg.data); // Use null-terminated string constructor

    std::filesystem::path result = baseStr;

    // Append remaining parts
    for (int i = 1; i < int(in_count()); ++i)
    {
        auto &arg = inargs.str_data(i);
        if (arg.data != nullptr)
        {
            std::string part(arg.data); // Use null-terminated string constructor
            
            // If the part starts with a dot, treat it as an extension to append to the filename
            // rather than a separate path component
            if (!part.empty() && part[0] == '.')
            {
                // Append extension directly to the last component
                std::string currentPath = result.string();
                result = currentPath + part;
            }
            else
            {
                // Normal path component - use filesystem path joining
                result /= part;
            }
        }
    }

    std::string finalPath = result.generic_string();

    outargs.str_data(0).size = static_cast<int>(finalPath.size() + 1);
    outargs.str_data(0).data = csound->strdup(finalPath.data());

    return IS_OK;
}
