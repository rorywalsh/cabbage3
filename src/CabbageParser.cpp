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
                        updateJson(j, item, widgets.size());
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


void Parser::updateJson(nlohmann::json &jsonObj, const nlohmann::json &incomingJson, size_t numWidgets)
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
        
        if (widgetType != "form")
        {
            if (incomingJson.contains("channel") && incomingJson["channel"].is_string() &&
                incomingJson["channel"].get<std::string>().empty())
            {
                std::string autoChannel = widgetType + std::to_string(static_cast<int>(numWidgets));
                jsonObj["channel"] = autoChannel;
                lattice::logWarning << "Widget type '" << widgetType << "' is missing a channel property. "
                                  << "Automatically assigned: \"" << autoChannel
                                  << "\". Assign your own channel property to avoid unexpected behavior.";
            }
        }

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
                            jsonObj["items"] = items;
                            lattice::logDebug << "Found " << files.size() << " files for populate operation";
                        } else {
                            jsonObj["items"] = "";
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
                            const std::string items =
                                std::accumulate(std::next(value.begin()), value.end(), value[0].get<std::string>(),
                                                [](std::string a, const std::string &b) { return std::move(a) + ", " + b; });
                            jsonObj["items"] = items;
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
                        std::string escapedText = escapeJSON(value.get<std::string>());
                        jsonObj["channel"] = escapedText;
                    }
                    else if (value.is_object())
                    {
                        lattice::logDebug << "multi-channel support detected";
                        for (auto &[innerKey, val] : value.items())
                        {
                            if (val.is_string())
                            {
                                jsonObj["channel"][innerKey] = escapeJSON(val.get<std::string>());
                            }
                            else
                            {
                                lattice::logDebug << "channel object property '" << innerKey
                                                  << "' must be an object for widget type: " << widgetType;
                            }
                        }
                    }

                }
                else if (key == "id")
                {
                    if (value.is_string())
                    {
                        std::string escapedText = escapeJSON(value.get<std::string>());
                        jsonObj["id"] = escapedText;
                    }
                    else
                    {
                        lattice::logDebug << "id property must be a string for widget type: " << widgetType;
                    }
                }
                else if (key == "channels")
                {
                    if (value.is_array())
                    {
                        nlohmann::json processedChannels = nlohmann::json::array();
                        for (auto& ch : value)
                        {
                            if (ch.is_object())
                            {
                                nlohmann::json processedCh = ch;
                                if (ch.contains("id") && ch["id"].is_string())
                                {
                                    processedCh["id"] = escapeJSON(ch["id"].get<std::string>());
                                }
                                processedChannels.push_back(processedCh);
                            }
                            else
                            {
                                lattice::logDebug << "channels array element must be an object for widget type: " << widgetType;
                            }
                        }
                        jsonObj["channels"] = processedChannels;
                    }
                    else
                    {
                        lattice::logDebug << "channels property must be an array for widget type: " << widgetType;
                    }
                }
                else if (key == "text")
                {
                    if (value.is_string())
                    {
                        std::string escapedText = escapeJSON(value.get<std::string>());
                        if (choc::text::toLowerCase(widgetType).find("button") != std::string::npos)
                        {
                            jsonObj["text"]["on"] = escapedText;
                            jsonObj["text"]["off"] = escapedText;
//                            lattice::logDebug << "Set button text (on/off): " << escapedText;
                        }
                        else
                        {
                            jsonObj["text"] = escapedText;
//                            lattice::logDebug << "Set text: " << escapedText << " for widget type: " << widgetType;
                        }
                    }
                    else if (value.is_object())
                    {
                        for (auto &[innerKey, val] : value.items())
                        {
                            if (val.is_string()) {
                                jsonObj["text"][innerKey] = escapeJSON(val.get<std::string>());
                            } else {
                                lattice::logDebug << "text object property '" << innerKey << "' must be a string for widget type: " << widgetType;
                            }
                        }
                        lattice::logDebug << "Processed text object for widget type: " << widgetType;
                    }
                    else
                    {
                        lattice::logDebug << "text property must be a string or object for widget type: " << widgetType;
                    }
                }
                else if (key == "children")
                {
                    // Handle children array - recursively process each child widget
                    // This is needed to ensure each child gets default properties from their widget descriptors
                    // (same as top-level widgets)
                    if (value.is_array())
                    {
                        nlohmann::json processedChildren = nlohmann::json::array();
                        
                        for (const auto &childJson : value)
                        {
                            if (childJson.is_object() && childJson.contains("type") && childJson["type"].is_string())
                            {
                                std::string childType = childJson["type"].get<std::string>();
                                
                                // Get the default descriptor for this child widget type (just like top-level widgets)
                                auto childDescriptor = WidgetDescriptors::get(childType);
                                
                                if (!childDescriptor.is_null())
                                {
                                    // Recursively merge the child JSON with its descriptor defaults
                                    updateJson(childDescriptor, childJson, numWidgets);
                                    processedChildren.push_back(childDescriptor);
                                    lattice::logDebug << "Processed child widget of type: " << childType << " for parent: " << widgetType;
                                }
                                else
                                {
                                    lattice::logError << "Invalid child widget type: " << childType << " in parent: " << widgetType;
                                }
                            }
                            else
                            {
                                lattice::logError << "Child widget in " << widgetType << " is missing 'type' field or is not an object";
                            }
                        }
                        
                        jsonObj["children"] = processedChildren;
                        lattice::logDebug << "Processed " << processedChildren.size() << " children for widget: " << widgetType;
                    }
                    else
                    {
                        lattice::logWarning << "children property must be an array for widget type: " << widgetType;
                    }
                }
                else if (key == "font")
                {
                    // Handle font object - merge nested properties instead of replacing
                    if (value.is_object())
                    {
                        for (auto &[fontKey, fontVal] : value.items())
                        {
                            if (fontKey == "colour" && fontVal.is_object())
                            {
                                // Deep merge font.colour object
                                for (auto &[colourKey, colourVal] : fontVal.items())
                                {
                                    jsonObj["font"]["colour"][colourKey] = colourVal;
                                }
                            }
                            else
                            {
                                jsonObj["font"][fontKey] = fontVal;
                            }
                        }
                        lattice::logDebug << "Merged font properties for widget type: " << widgetType;
                    }
                    else
                    {
                        lattice::logWarning << "font property must be an object for widget type: " << widgetType;
                    }
                }
                else
                {
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
    catch (const nlohmann::json::exception &e)
    {
        lattice::logError << "JSON exception in updateJson: " << e.what();
    }
    catch (const std::exception &e)
    {
        lattice::logError << "Unexpected exception in updateJson: " << e.what();
    }

    // Assign widget id from first channel if not provided
    if (!jsonObj.contains("id") && jsonObj.contains("channels") && jsonObj["channels"].is_array() && !jsonObj["channels"].empty())
    {
        auto& firstChannel = jsonObj["channels"][0];
        if (firstChannel.contains("id") && firstChannel["id"].is_string())
        {
            jsonObj["id"] = firstChannel["id"];
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
        return validateHexString(value.get<std::string>());
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
