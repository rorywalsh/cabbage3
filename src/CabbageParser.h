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

// CabbageParser.h

#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "CabbageUtils.h"
#include "CabbageColours.h"

namespace cabbage
{

class Parser
{
  public:
    static std::string removeQuotes(const std::string &str);
    static bool isWidget(const std::string &target);
    static std::vector<nlohmann::json> parseCsdForWidgets(const std::string &csdFile, std::string *outError = nullptr);

    // Helper functions
    static std::string parseContent(const std::string &content, std::vector<nlohmann::json> &widgets);
    static void parseJsonFile(const std::string &filename, std::vector<nlohmann::json> &widgets);

    static void initialiseWidgetJson(nlohmann::json &jsonObj, const nlohmann::json &incomingJson, size_t numWidgets);
    static void mergeJsonProperties(nlohmann::json &jsonObj, const nlohmann::json &incomingJson);
    static void assignDefaultRangesToChannels(nlohmann::json &jsonObj);
    static void parseStroke(const nlohmann::json &strokeValue, nlohmann::json &target);
    static void parseColorProperties(const nlohmann::json &value, nlohmann::json &target);
    
    // Async populate processing - returns immediately, processes in background
    static void processPopulateAsync(const std::string& widgetChannel, const nlohmann::json& populateConfig, 
                                     std::function<void(const nlohmann::json&)> callback);

  private:
    static std::string parseColorValue(const nlohmann::json &value);
    static std::string rgbToHex(const std::vector<double> &rgb);
    static std::string rgbaToHex(int r, int g, int b, int a);
    static std::string validateHexString(const std::string &str);
    static std::string escapeJSON(const std::string &str);
};

} // namespace cabbage
