/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
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
            out[index].size = file.size();
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
   auto existingFiles = lattice::File::getFilesOfType(directory, "*" + extension);

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
    outargs.str_data(0).size = int(strlen(fileName.c_str()) + 1);
    outargs.str_data(0).data = csound->strdup(fileName.data());

    return IS_OK;
}

