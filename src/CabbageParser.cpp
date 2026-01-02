// CabbageParser.cpp

#include "CabbageParser.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <unordered_set>

// choc string utility class
#include <choc/text/choc_StringUtilities.h>

namespace cabbage
{

std::string Parser::removeQuotes(const std::string &str)
{
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '\"'), result.end());
    return result;
}

bool Parser::isWidget(const std::string &target)
{
    std::vector<std::string> widgetTypes = cabbage::WidgetDescriptors::getWidgetTypes();
    return std::find(widgetTypes.begin(), widgetTypes.end(), target) != widgetTypes.end();
}

std::vector<nlohmann::json> Parser::parseCsdForWidgets(const std::string &csdFile, std::string *outError)
{
    std::vector<nlohmann::json> widgets;
    std::string jsonError;

    std::ifstream file(csdFile);
    if (!file.is_open())
    {
        lattice::logInfo << "Error opening CSD file: " << csdFile;
        return widgets;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::regex cabbageRegex(R"(<Cabbage>([\s\S]*?)</Cabbage>)");
    std::smatch cabbageMatch;

    if (std::regex_search(content, cabbageMatch, cabbageRegex))
    {
        std::string cabbageContent = cabbageMatch[1].str();

        std::regex formRegex(R"("type"\s*:\s*"form")");
        bool foundFormWidget = std::regex_search(cabbageContent, formRegex);

        if (foundFormWidget)
        {
            jsonError = parseContent(cabbageContent, widgets);
        }
        else
        {
            std::regex includeRegex(R"(#include\s*\"([^\"]+\.json)\")");
            std::smatch includeMatch;

            if (std::regex_search(cabbageContent, includeMatch, includeRegex))
            {
                std::string includeFilename = includeMatch[1].str();
                std::filesystem::path includePath(includeFilename);

                if (!includePath.is_absolute())
                {
                    std::filesystem::path csdPath(csdFile);
                    includePath = csdPath.parent_path() / includePath;
                }

                parseJsonFile(includePath.string(), widgets);
            }
            else
            {
                std::string fallbackJsonFile = csdFile.substr(0, csdFile.find_last_of('.')) + ".json";
                parseJsonFile(fallbackJsonFile, widgets);
            }
        }
    }
    else
    {
        lattice::logInfo << "No <Cabbage> section found in the file: " << csdFile;
    }

    if (outError && !jsonError.empty())
    {
        *outError = jsonError;
    }

    return widgets;
}

std::string Parser::parseContent(const std::string &content, std::vector<nlohmann::json> &widgets)
{
    std::string errorMessage;
    try
    {
        auto jsonArray = nlohmann::json::parse(content);
        if (jsonArray.is_array())
        {
            for (auto &item : jsonArray)
            {
                if (item.is_object())
                {
                    // Check if "type" field exists and is a string
                    if (!item.contains("type") || !item["type"].is_string())
                    {
                        lattice::logError << "Widget is missing valid 'type' field - Skipping this widget and continuing...";
                        lattice::logDebug << "Invalid widget JSON: " << item.dump();
                        continue;
                    }
                    
                    std::string widgetType = item["type"].get<std::string>();
                    auto j = WidgetDescriptors::get(widgetType);
                    if (!j.is_null())
                    {
                        initialiseWidgetJson(j, item, widgets.size());
                        widgets.push_back(j);                        
                    }
                    else
                    {
                        lattice::logError << "Widget type is not valid: " << widgetType 
                                         << " - Skipping this widget and continuing...";
                        // Continue processing other widgets instead of crashing
                    }
                }
            }
        }
    }
    catch (const nlohmann::json::parse_error &e)
    {
        errorMessage = "JSON Parse Error: " + std::string(e.what());
        lattice::logError << errorMessage;
    }
    return errorMessage;
}

void Parser::parseJsonFile(const std::string &filename, std::vector<nlohmann::json> &widgets)
{
    std::ifstream jsonFile(filename);
    if (!jsonFile.is_open())
    {
        lattice::logDebug << "Error opening JSON file:" << filename;
        return;
    }

    std::stringstream buffer;
    buffer << jsonFile.rdbuf();
    parseContent(buffer.str(), widgets);
}


void Parser::initialiseWidgetJson(nlohmann::json &jsonObj, const nlohmann::json &incomingJson, size_t numWidgets)
{
    try
    {
        // Validate input JSON objects
        if (!jsonObj.is_object()) {
            lattice::logDebug << jsonObj.dump(4);
            lattice::logError << "Target jsonObj is not a valid JSON object";
            return;
        }
        
        if (!incomingJson.is_object()) {
            lattice::logError << "Incoming JSON is not a valid JSON object";
            return;
        }
        
        // Check for required 'type' field in target JSON
        if (!jsonObj.contains("type") || !jsonObj["type"].is_string()) {
            lattice::logError << "Target JSON object is missing required 'type' field or it's not a string";
            return;
        }
        
        const std::string widgetType = jsonObj["type"].get<std::string>();
        
        // Assign widget id from first channel if not provided in incoming JSON, otherwise auto-assign
        // Form is a special case with a fixed "MainForm" id
        if (widgetType != "form")
        {
            if (!incomingJson.contains("id"))
            {
                bool assigned = false;
                if (incomingJson.contains("channels") && incomingJson["channels"].is_array() && !incomingJson["channels"].empty())
                {
                    auto& firstChannel = incomingJson["channels"][0];
                    if (firstChannel.contains("id") && firstChannel["id"].is_string())
                    {
                        jsonObj["id"] = escapeJSON(firstChannel["id"].get<std::string>());
                        assigned = true;
                    }
                }
                if (!assigned)
                {
                    std::string autoId = widgetType + std::to_string(static_cast<int>(numWidgets));
                    jsonObj["id"] = autoId;
                    // Also set the first channel's id if channels exist
                    if (incomingJson.contains("channels") && incomingJson["channels"].is_array() && !incomingJson["channels"].empty())
                    {
                        jsonObj["channels"][0]["id"] = autoId;
                    }
                    lattice::logDebug << incomingJson.dump(4);
                    lattice::logWarning << "Widget type '" << widgetType << "' is missing an id property. "
                                      << "Automatically assigned: \"" << autoId
                                      << "\". Assign your own id property to avoid unexpected behavior.";
                }
            }
        }
        else
        {
            jsonObj["id"] = "MainForm";
        }

        mergeJsonProperties(jsonObj, incomingJson);
        
        // Assign default ranges to channels that don't have them
        assignDefaultRangesToChannels(jsonObj);
    }
    catch (const nlohmann::json::exception &e)
    {
        lattice::logError << "JSON exception in initialiseWidgetJson: " << e.what();
    }
    catch (const std::exception &e)
    {
        lattice::logError << "Unexpected exception in initialiseWidgetJson: " << e.what();
    }

}

void Parser::mergeJsonProperties(nlohmann::json &jsonObj, const nlohmann::json &incomingJson)
{
    // Check for required 'type' field in target JSON
    if (!jsonObj.contains("type") || !jsonObj["type"].is_string()) {
        lattice::logError << "Target JSON object is missing required 'type' field or it's not a string";
        return;
    }
    
    const std::string widgetType = jsonObj["type"].get<std::string>();
    
    // Known boolean properties that may be encoded as numeric 1/0 by Csound.
    // We include both top-level keys and common nested property names (eg. "logarithmic").
    static const std::unordered_set<std::string> booleanProperties = {
        "visible", "automatable", "active", "popup", "presetIgnore", "identChannel",
        "svgElement", "valueTextBox", "moveBehind", "filmStrip", "logarithmic"
    };

    // Iterate through incoming JSON properties
    for (auto it = incomingJson.begin(); it != incomingJson.end(); ++it)
    {
        const std::string &key = it.key();
        const auto &value = it.value();

        try
        {
            if (key == "bounds" || key == "range" || key == "size")
            {
                if (value.is_object())
                {
                    for (auto &[propKey, val] : value.items())
                    {
                        jsonObj[key][propKey] = val;
                    }
                }
                else
                {
                    lattice::logWarning << "Property '" << key << "' should be an object for widget type: " << widgetType;
                }
            }
            else if (key == "sampleRange")
            {
                if (value.is_array() && value.size() == 2)
                {
                    if (value[0].is_number() && value[1].is_number()) {
                        jsonObj["startSample"] = value[0].get<int>();
                        jsonObj["endSample"] = value[1].get<int>();
//                            lattice::logDebug << "Set sample range for widget type: " << widgetType;
                    } else {
                        lattice::logWarning << "sampleRange array elements must be numbers for widget type: " << widgetType;
                    }
                }
                else
                {
                    lattice::logWarning << "sampleRange must be an array with exactly 2 elements for widget type: " << widgetType;
                }
            }
            else if (key == "populate")
            {
                if (value.is_object())
                {
                    // Merge with existing populate object if it exists
                    nlohmann::json mergedPopulate = jsonObj.contains("populate") && jsonObj["populate"].is_object() 
                        ? jsonObj["populate"] 
                        : nlohmann::json::object();
                    
                    // Update with incoming fields
                    for (auto& [k, v] : value.items()) {
                        mergedPopulate[k] = v;
                    }
                    
                    // Validate required fields in the merged populate object
                    if (!mergedPopulate.contains("directory") || !mergedPopulate["directory"].is_string()) {
                        lattice::logError << "populate object missing required 'directory' string field for widget type: " << widgetType;
                        continue;
                    }
                    
                    if (!mergedPopulate.contains("fileType") || !mergedPopulate["fileType"].is_string()) {
                        lattice::logError << "populate object missing required 'fileType' string field for widget type: " << widgetType;
                        continue;
                    }
                    
                    std::string directory = mergedPopulate["directory"].get<std::string>();
                    std::string fileType = cabbage::Utils::sanitisePath(mergedPopulate["fileType"].get<std::string>());
                    
                    lattice::logInfo << "Populating widget from directory: " << directory << " with file type: " << fileType;
                    
                    std::vector<std::string> files = File::getFilesOfType(directory, fileType);

                    if (files.empty()) {
                        lattice::logWarning << "No files found in directory '" << directory << "' with type '" << fileType << "'";
                    }

                    // Apply sorting based on populate.order property
                    std::string orderType = "alphanumeric"; // default
                    if (mergedPopulate.contains("order") && mergedPopulate["order"].is_string()) {
                        orderType = mergedPopulate["order"].get<std::string>();
                    }

                    if (orderType == "date") {
                        // Sort by modification time (oldest first)
                        std::sort(files.begin(), files.end(),
                            [](const std::string &a, const std::string &b) {
                                namespace fs = std::filesystem;
                                try {
                                    auto timeA = fs::last_write_time(a);
                                    auto timeB = fs::last_write_time(b);
                                    return timeA < timeB; // oldest first
                                } catch (...) {
                                    return a < b; // fallback to alphanumeric
                                }
                            });
                    } else if (orderType == "size") {
                        // Sort by file size (largest first)
                        std::sort(files.begin(), files.end(),
                            [](const std::string &a, const std::string &b) {
                                namespace fs = std::filesystem;
                                try {
                                    auto sizeA = fs::file_size(a);
                                    auto sizeB = fs::file_size(b);
                                    return sizeA > sizeB; // largest first
                                } catch (...) {
                                    return a < b; // fallback to alphanumeric
                                }
                            });
                    }
                    // else "alphanumeric" - already sorted by getFilesOfType

                    jsonObj[key]["directory"] = directory;
                    jsonObj[key]["fileType"] = fileType;
                    if (!orderType.empty()) {
                        jsonObj[key]["order"] = orderType;
                    }
                    // Set channel type to string for populate
                    if (jsonObj.contains("channels") && jsonObj["channels"].is_array() && !jsonObj["channels"].empty()) {
                        jsonObj["channels"][0]["type"] = "string";
                    }
                    jsonObj["automatable"] = false;

                    // Optionally return only filename stems (no directory, no extension)
                    bool fullPath = false;
                    if (value.contains("fullFileAndPath") && value["fullFileAndPath"].is_boolean()) {
                        fullPath = value["fullFileAndPath"].get<bool>();
                    }

                    std::vector<std::string> items;
                    if (!files.empty()) {
                        if (!fullPath) {
                            items.reserve(files.size());
                            for (const auto &fp : files) {
                                items.push_back(std::filesystem::path(fp).filename().stem().string());
                            }
                        } else {
                            items = files;
                        }
                        jsonObj["items"] = items;
                        lattice::logDebug << "Found " << files.size() << " files for populate operation (order: " << orderType << ")";
                    } else {
                        jsonObj["items"] = items; // empty
                    }
                }
                else
                {
                    lattice::logDebug << "populate property must be an object for widget type: " << widgetType;
                }
            }
            else if (key == "items")
            {
                if (value.is_array())
                {
                    // Validate array contains strings
                    bool allStrings = std::all_of(value.begin(), value.end(),
                                                [](const nlohmann::json& item) { return item.is_string(); });
                    
                    if (allStrings) {
//                            const std::string items =
//                                std::accumulate(std::next(value.begin()), value.end(), value[0].get<std::string>(),
//                                                [](std::string a, const std::string &b) { return std::move(a) + ", " + b; });
                        jsonObj["items"] = value;
                        jsonObj["min"] = 0;
                        jsonObj["max"] = value.size() - 1;
//                            lattice::logDebug << "Set items array with " << value.size() << " elements for widget type: " << widgetType;
                    } else {
                        lattice::logDebug << "items array must contain only string values for widget type: " << widgetType;
                    }
                }
                else
                {
                    lattice::logDebug << "items property must be an array for widget type: " << widgetType;
                }
            }
            else if (key == "samples")
            {
                if (value.is_array())
                {
                    bool allNumbers = std::all_of(value.begin(), value.end(),
                                                [](const nlohmann::json& item) { return item.is_number(); });
                    
                    if (allNumbers) {
                        jsonObj["samples"] = value.get<std::vector<double>>();
//                            lattice::logDebug << "Set samples array with " << value.size() << " numeric values";
                    } else {
                        lattice::logDebug << "samples array must contain only numeric values for widget type: " << widgetType;
                    }
                }
                else
                {
                    lattice::logDebug << "samples property must be an array for widget type: " << widgetType;
                }
            }
            else if (key == "color")
            {
                if (value.is_object())
                {
                    parseColorProperties(value, jsonObj[key]);
//                        lattice::logDebug << "Parsed color object for widget type: " << widgetType;
                }
                else if (value.is_string() || value.is_number())
                {
                    jsonObj[key] = parseColorValue(value);
//                        lattice::logDebug << "Parsed color value for widget type: " << widgetType;
                }
                else
                {
                    lattice::logDebug << "color property must be an object, string, or number for widget type: " << widgetType;
                }
            }
            else if (key == "file")
            {
                if (value.is_string())
                {
                    std::string filePath = cabbage::Utils::sanitisePath(value.get<std::string>());
                    jsonObj["file"] = filePath;
//                        lattice::logDebug << "Set file path: " << filePath << " for widget type: " << widgetType;
                }
                else
                {
                    lattice::logDebug << "file property must be a string for widget type: " << widgetType;
                }
            }
            else if (key == "channel")
            {
                if (value.is_string())
                {
                    jsonObj["channel"] = value.get<std::string>();
//                        lattice::logDebug << "Set channel: " << value.get<std::string>() << " for widget type: " << widgetType;
                }
                else if (value.is_object())
                {
                    // Handle channel object with id and possibly other properties
                    for (auto &[chKey, chVal] : value.items())
                    {
                        jsonObj["channel"][chKey] = chVal;
                    }
//                        lattice::logDebug << "Set channel object for widget type: " << widgetType;
                }
                else
                {
                    lattice::logDebug << "channel property must be a string or object for widget type: " << widgetType;
                }
            }
            else if (key == "channels")
            {
                if (value.is_array())
                {
                    // Handle channels array
                    for (size_t i = 0; i < value.size(); ++i)
                    {
                        if (value[i].is_object())
                        {
                            for (auto &[chKey, chVal] : value[i].items())
                            {
                                jsonObj["channels"][i][chKey] = chVal;
                            }
                        }
                        else
                        {
                            lattice::logDebug << "channels array elements must be objects for widget type: " << widgetType;
                        }
                    }
//                        lattice::logDebug << "Set channels array with " << value.size() << " elements for widget type: " << widgetType;
                }
                else
                {
                    lattice::logDebug << "channels property must be an array for widget type: " << widgetType;
                }
            }
            else
            {
                // For nested objects (like label, thumb, track, valueText), merge properties
                if (value.is_object() && jsonObj.contains(key) && jsonObj[key].is_object())
                    {
                        // Recursively merge nested object properties. When nested properties
                        // come from Csound they are sometimes encoded as numeric 1/0 —
                        // convert those to real booleans for known boolean properties
                        for (auto &[nestedKey, nestedVal] : value.items())
                        {
                            if (booleanProperties.find(nestedKey) != booleanProperties.end() && nestedVal.is_number())
                            {
                                jsonObj[key][nestedKey] = (nestedVal.get<double>() != 0);
                            }
                            else
                            {
                                jsonObj[key][nestedKey] = nestedVal;
                            }
                        }
    //                    lattice::logDebug << "Merged nested object property '" << key << "' for widget type: " << widgetType;
                    }
                else
                {
                    // Check if this is a known boolean property and the value is numeric (from Csound)
                    if (booleanProperties.find(key) != booleanProperties.end() && value.is_number())
                    {
                        // Convert numeric 1/0 from Csound to boolean true/false for JavaScript
                        jsonObj[key] = (value.get<double>() != 0);
//                        lattice::logDebug << "Converted numeric " << value << " to boolean for property '" << key << "'";
                    }
                    else
                    {
                        // For all other properties, just assign the value directly
                        jsonObj[key] = value;
//                        lattice::logDebug << "Set property '" << key << "' for widget type: " << widgetType;
                    }
                }
            }
        }
        catch (const nlohmann::json::exception &e)
        {
            lattice::logDebug << "JSON processing error for key '" << key << "' in widget type '" << widgetType << "': " << e.what();
        }
        catch (const std::exception &e)
        {
            lattice::logDebug << "Unexpected error processing key '" << key << "' in widget type '" << widgetType << "': " << e.what();
        }
    }
}

void Parser::parseStroke(const nlohmann::json &strokeValue, nlohmann::json &target)
{
    if (strokeValue.contains("color"))
    {
        target["color"] = parseColorValue(strokeValue["color"]);
    }
    if (strokeValue.contains("width"))
    {
        target["width"] = strokeValue["width"];
    }
}

void Parser::parseColorProperties(const nlohmann::json &value, nlohmann::json &target)
{
    for (auto &[key, val] : value.items())
    {
        if (key == "fill" || key == "background")
        {
            target[key] = parseColorValue(val);
        }
        else if (key == "stroke" && val.is_object())
        {
            parseStroke(val, target[key]);
        }
        else if (key == "width" && val.is_number())
        {
            // Width is a numeric property, not a color - assign directly
            target[key] = val;
        }
        else if (val.is_object())
        {
            parseColorProperties(val, target[key]);
        }
        else if (val.is_number())
        {
            // Other numeric properties in color objects (like opacity) - assign directly
            target[key] = val;
        }
        else
        {
            target[key] = parseColorValue(val);
        }
    }
}

std::string Parser::parseColorValue(const nlohmann::json &value)
{
    if (value.is_array() && value.size() >= 3)
    {
        return rgbToHex(value.get<std::vector<double>>());
    }
    else if (value.is_string())
    {
        std::string colorStr = value.get<std::string>();
        
        // Improved CSS rgb()/rgba() parsing:
        // Accepts integer (0-255), decimal (0-1) and percentage (0% - 100%) values.
        std::regex rgbaRegex(R"(^rgba\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^\)]+)\s*\)$)", std::regex::icase);
        std::regex rgbRegex(R"(^rgb\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^\)]+)\s*\)$)", std::regex::icase);

        std::smatch match;
        auto parseComponent = [](const std::string &s) -> int {
            std::string str = s;
            // trim
            str.erase(0, str.find_first_not_of(" \t\n\r"));
            str.erase(str.find_last_not_of(" \t\n\r") + 1);

            bool isPercent = false;
            if (!str.empty() && str.back() == '%')
            {
                isPercent = true;
                str = str.substr(0, str.size() - 1);
            }

            double val = 0.0;
            try
            {
                val = std::stod(str);
            }
            catch (...) { val = 0.0; }

            int out;
            if (isPercent)
            {
                // percentage -> 0..255
                out = static_cast<int>(std::round(std::clamp(val, 0.0, 100.0) * 255.0 / 100.0));
            }
            else
            {
                // if value looks like 0..1 use that scale, otherwise assume 0..255
                if (val >= 0.0 && val <= 1.0)
                    out = static_cast<int>(std::round(val * 255.0));
                else
                    out = static_cast<int>(std::round(std::clamp(val, 0.0, 255.0)));
            }

            return std::clamp(out, 0, 255);
        };

        auto parseAlpha = [](const std::string &s) -> double {
            std::string str = s;
            // trim
            str.erase(0, str.find_first_not_of(" \t\n\r"));
            str.erase(str.find_last_not_of(" \t\n\r") + 1);

            bool isPercent = false;
            if (!str.empty() && str.back() == '%')
            {
                isPercent = true;
                str = str.substr(0, str.size() - 1);
            }

            double val = 1.0;
            try { val = std::stod(str); } catch (...) { val = 1.0; }

            if (isPercent)
            {
                val = std::clamp(val / 100.0, 0.0, 1.0);
            }
            else
            {
                // If given as 0..255 (unlikely for alpha) treat >1 as 1
                if (val > 1.0)
                    val = 1.0;
                val = std::clamp(val, 0.0, 1.0);
            }
            return val;
        };

        if (std::regex_match(colorStr, match, rgbaRegex))
        {
            int r = parseComponent(match[1].str());
            int g = parseComponent(match[2].str());
            int b = parseComponent(match[3].str());
            double a = parseAlpha(match[4].str());

            int alpha = static_cast<int>(std::round(a * 255.0));
            return rgbaToHex(r, g, b, alpha);
        }
        else if (std::regex_match(colorStr, match, rgbRegex))
        {
            int r = parseComponent(match[1].str());
            int g = parseComponent(match[2].str());
            int b = parseComponent(match[3].str());

            std::vector<double> rgb = {static_cast<double>(r), static_cast<double>(g), static_cast<double>(b)};
            return rgbToHex(rgb);
        }
        else
        {
            // Fall back to existing hex/named color validation
            return validateHexString(colorStr);
        }
    }
    return "#000000";
}

std::string Parser::rgbToHex(const std::vector<double> &rgb)
{
    auto clampByte = [](int v) { return std::clamp(v, 0, 255); };
    int r = clampByte(static_cast<int>(std::round(rgb.size() > 0 ? rgb[0] : 0.0)));
    int g = clampByte(static_cast<int>(std::round(rgb.size() > 1 ? rgb[1] : 0.0)));
    int b = clampByte(static_cast<int>(std::round(rgb.size() > 2 ? rgb[2] : 0.0)));

    std::ostringstream hex;
    hex << "#" << std::hex << std::setw(2) << std::setfill('0') << std::nouppercase
        << (r >> 4 & 0xF) << (r & 0xF); // placeholder - we'll format properly below
    // Proper formatting using stringstream with manipulators for each byte
    std::ostringstream out;
    out << "#";
    out << std::hex << std::setw(2) << std::setfill('0') << std::nouppercase << std::uppercase;
    // we need to ensure two hex digits per byte, so cast to unsigned and mask
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)(r & 0xFF);
        out << tmp.str();
    }
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)(g & 0xFF);
        out << tmp.str();
    }
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)(b & 0xFF);
        out << tmp.str();
    }
    std::string result = out.str();
    // make lowercase to match existing style
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Parser::rgbaToHex(int r, int g, int b, int a)
{
    auto clampByte = [](int v) { return std::clamp(v, 0, 255); };
    unsigned rr = static_cast<unsigned>(clampByte(r)) & 0xFF;
    unsigned gg = static_cast<unsigned>(clampByte(g)) & 0xFF;
    unsigned bb = static_cast<unsigned>(clampByte(b)) & 0xFF;
    unsigned aa = static_cast<unsigned>(clampByte(a)) & 0xFF;

    std::ostringstream out;
    out << "#";
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)rr; out << tmp.str();
    }
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)gg; out << tmp.str();
    }
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)bb; out << tmp.str();
    }
    {
        std::ostringstream tmp; tmp << std::hex << std::setw(2) << std::setfill('0') << (unsigned)aa; out << tmp.str();
    }

    std::string result = out.str();
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Parser::validateHexString(const std::string &str)
{
    std::regex hexRegex("^#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})$");

    if (std::regex_match(str, hexRegex))
    {
        return str;
    }
    else
    {
        return Colours::getColour(str);
    }
}

std::string Parser::escapeJSON(const std::string &str)
{
    std::string escaped;

    for (char c : str)
    {
        switch (c)
        {
        case '\"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += c;
            break;
        }
    }

    return escaped;
}

void Parser::assignDefaultRangesToChannels(nlohmann::json &jsonObj)
{
    try
    {
        // Handle channels array case
        if (jsonObj.contains("channels") && jsonObj["channels"].is_array())
        {
            // Iterate through channels and ensure each has a complete range object
            for (auto& channel : jsonObj["channels"])
            {
                if (channel.is_object())
                {
                    // Determine interaction type based on widget type
                    std::string widgetType = jsonObj.contains("type") && jsonObj["type"].is_string() 
                        ? jsonObj["type"].get<std::string>() : "";
                    
                    // Default to 'drag' interaction, but use 'click' for certain widget types
                    std::string interaction = "drag";
                    if (widgetType == "button" || widgetType == "checkbox" || widgetType == "optionButton" || 
                        widgetType == "radioGroup" || widgetType == "checkBox")
                    {
                        interaction = "click";
                    }
                    
                    // Create default range with all required properties
                    nlohmann::json defaultRange = {
                        {"min", 0.0},
                        {"max", 1.0},
                        {"value", 0.0},
                        {"defaultValue", 0.0},
                        {"skew", 1.0},
                        {"increment", interaction == "click" ? 1.0 : 0.001}
                    };
                    
                    // If channel doesn't have a range object at all, assign the default
                    if (!channel.contains("range") || !channel["range"].is_object())
                    {
                        channel["range"] = defaultRange;
                        lattice::logDebug << "Assigned default range to channel in widget type: " << widgetType;
                    }
                    else
                    {
                        // Ensure all required properties exist in the existing range object
                        auto& range = channel["range"];
                        if (!range.contains("min") || !range["min"].is_number())
                            range["min"] = defaultRange["min"];
                        if (!range.contains("max") || !range["max"].is_number())
                            range["max"] = defaultRange["max"];
                        if (!range.contains("value") || !range["value"].is_number())
                            range["value"] = defaultRange["value"];
                        if (!range.contains("defaultValue") || !range["defaultValue"].is_number())
                            range["defaultValue"] = defaultRange["defaultValue"];
                        if (!range.contains("skew") || !range["skew"].is_number())
                            range["skew"] = defaultRange["skew"];
                        if (!range.contains("increment") || !range["increment"].is_number())
                            range["increment"] = defaultRange["increment"];

                    }
                }
            }
        }
    }
    catch (const nlohmann::json::exception &e)
    {
        lattice::logError << "JSON exception in assignDefaultRangesToChannels: " << e.what();
    }
    catch (const std::exception &e)
    {
        lattice::logError << "Unexpected exception in assignDefaultRangesToChannels: " << e.what();
    }
}

} // namespace cabbage
