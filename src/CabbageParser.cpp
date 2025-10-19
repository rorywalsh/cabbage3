// CabbageParser.cpp

#include "CabbageParser.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <numeric>

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

std::vector<nlohmann::json> Parser::parseCsdForWidgets(const std::string &csdFile)
{
    std::vector<nlohmann::json> widgets;

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
            parseContent(cabbageContent, widgets);
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

    return widgets;
}

void Parser::parseContent(const std::string &content, std::vector<nlohmann::json> &widgets)
{
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
        lattice::logInfo << "JSON parse error: " << e.what();
    }
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
                    // Validate required fields in populate object
                    if (!value.contains("directory") || !value["directory"].is_string()) {
                        lattice::logError << "populate object missing required 'directory' string field for widget type: " << widgetType;
                        continue;
                    }
                    
                    if (!value.contains("fileType") || !value["fileType"].is_string()) {
                        lattice::logError << "populate object missing required 'fileType' string field for widget type: " << widgetType;
                        continue;
                    }
                    
                    std::string directory = value["directory"].get<std::string>();
                    std::string fileType = cabbage::Utils::sanitisePath(value["fileType"].get<std::string>());
                    
                    lattice::logInfo << "Populating widget from directory: " << directory << " with file type: " << fileType;
                    
                    std::vector<std::string> files = File::getFilesOfType(directory, fileType);
                    
                    if (files.empty()) {
                        lattice::logWarning << "No files found in directory '" << directory << "' with type '" << fileType << "'";
                    }
                    
                    jsonObj[key]["directory"] = directory;
                    jsonObj[key]["fileType"] = fileType;
                    jsonObj["channelType"] = "string";
                    
                    
                    if (!files.empty()) {
                        const std::string items =
                            std::accumulate(std::next(files.begin()), files.end(), files[0],
                                            [](std::string a, const std::string &b) { return std::move(a) + ", " + b; });
                        jsonObj["items"] = files;
                        lattice::logDebug << "Found " << files.size() << " files for populate operation";
                    } else {
                        std::vector<std::string> tmp;
                        jsonObj["items"] = tmp;
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
            else if (key == "colour")
            {
                if (value.is_object())
                {
                    parseColourProperties(value, jsonObj[key]);
//                        lattice::logDebug << "Parsed colour object for widget type: " << widgetType;
                }
                else if (value.is_string() || value.is_number())
                {
                    jsonObj[key] = parseColorValue(value);
//                        lattice::logDebug << "Parsed colour value for widget type: " << widgetType;
                }
                else
                {
                    lattice::logDebug << "colour property must be an object, string, or number for widget type: " << widgetType;
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
                // For all other properties, just assign the value directly
                jsonObj[key] = value;
//                    lattice::logDebug << "Set property '" << key << "' for widget type: " << widgetType;
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
    if (strokeValue.contains("colour"))
    {
        target["colour"] = parseColorValue(strokeValue["colour"]);
    }
    if (strokeValue.contains("width") && strokeValue["width"].is_number())
    {
        target["width"] = strokeValue["width"];
    }
}

void Parser::parseColourProperties(const nlohmann::json &value, nlohmann::json &target)
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
            parseColourProperties(val, target[key]);
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
        
        // Check for CSS rgb() and rgba() syntax
        std::regex rgbRegex(R"(^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$)");
        std::regex rgbaRegex(R"(^rgba\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*([0-1]?\.?\d+)\s*\)$)");
        
        std::smatch match;
        if (std::regex_match(colorStr, match, rgbaRegex))
        {
            // Parse rgba(r, g, b, a)
            int r = std::stoi(match[1].str());
            int g = std::stoi(match[2].str());
            int b = std::stoi(match[3].str());
            double a = std::stod(match[4].str());
            
            // Convert alpha to 0-255 range and create RGBA hex
            int alpha = static_cast<int>(a * 255);
            return rgbaToHex(r, g, b, alpha);
        }
        else if (std::regex_match(colorStr, match, rgbRegex))
        {
            // Parse rgb(r, g, b)
            int r = std::stoi(match[1].str());
            int g = std::stoi(match[2].str());
            int b = std::stoi(match[3].str());
            
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
    std::ostringstream hex;
    hex << "#" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[0]) << std::setw(2)
        << std::setfill('0') << static_cast<int>(rgb[1]) << std::setw(2) << std::setfill('0')
        << static_cast<int>(rgb[2]);
    return hex.str();
}

std::string Parser::rgbaToHex(int r, int g, int b, int a)
{
    std::ostringstream hex;
    hex << "#" << std::hex << std::setw(2) << std::setfill('0') << r << std::setw(2)
        << std::setfill('0') << g << std::setw(2) << std::setfill('0') << b << std::setw(2)
        << std::setfill('0') << a;
    return hex.str();
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

} // namespace cabbage
