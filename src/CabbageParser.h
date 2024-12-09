// CabbageParser.h

#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <json.hpp>
#include "CabbageUtils.h"
#include "CabbageColours.h"

namespace cabbage {

class Parser
{
public:
    static std::string removeQuotes(const std::string& str);
    static bool isWidget(const std::string& target);
    static std::vector<nlohmann::json> parseCsdForWidgets(const std::string& csdFile);

    // Helper functions
    static void parseContent(const std::string& content, std::vector<nlohmann::json>& widgets);
    static void parseJsonFile(const std::string& filename, std::vector<nlohmann::json>& widgets);


    static void updateJson(nlohmann::json& jsonObj, const nlohmann::json& incomingJson, size_t numWidgets);
    static void parseStroke(const nlohmann::json& strokeValue, nlohmann::json& target);
    static void parseColourProperties(const nlohmann::json& value, nlohmann::json& target);
private:

    static std::string parseColorValue(const nlohmann::json& value);
    static std::string rgbToHex(const std::vector<double>& rgb);
    static std::string validateHexString(const std::string& str);
    static std::string escapeJSON(const std::string& str);
};

} // namespace cabbage