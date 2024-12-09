// CabbageParser.cpp

#include "CabbageParser.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <numeric>

namespace cabbage {

std::string Parser::removeQuotes(const std::string& str)
{
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '\"'), result.end());
    return result;
}

bool Parser::isWidget(const std::string& target)
{
    std::vector<std::string> widgetTypes = WidgetDescriptors::getWidgetTypes();
    return std::find(widgetTypes.begin(), widgetTypes.end(), target) != widgetTypes.end();
}

std::vector<nlohmann::json> Parser::parseCsdForWidgets(const std::string& csdFile)
{
    std::vector<nlohmann::json> widgets;

    std::ifstream file(csdFile);
    if (!file.is_open())
    {
        LOG_INFO("Error opening CSD file: ", csdFile);
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
        LOG_INFO("No <Cabbage> section found in the file: ", csdFile);
    }

    return widgets;
}

void Parser::parseContent(const std::string& content, std::vector<nlohmann::json>& widgets)
{
    try
    {
        auto jsonArray = nlohmann::json::parse(content);
        if (jsonArray.is_array())
        {
            for (auto& item : jsonArray)
            {
                if (item.is_object())
                {
                    auto j = WidgetDescriptors::get(item["type"]);
                    if (!j.is_null())
                    {
                        updateJson(j, item, widgets.size());
                        widgets.push_back(j);
                    }
                }
            }
        }
    }
    catch (const nlohmann::json::parse_error& e)
    {
        LOG_INFO("JSON parse error: ", e.what());
    }
}

void Parser::parseJsonFile(const std::string& filename, std::vector<nlohmann::json>& widgets)
{
    std::ifstream jsonFile(filename);
    if (!jsonFile.is_open())
    {
        LOG_INFO("Error opening JSON file:" " <<", filename);
        return;
    }

    std::stringstream buffer;
    buffer << jsonFile.rdbuf();
    parseContent(buffer.str(), widgets);
}

void Parser::updateJson(nlohmann::json& jsonObj, const nlohmann::json& incomingJson, size_t numWidgets)
{
    try
    {
        if (jsonObj["type"].get<std::string>() != "form")
        {
            if (jsonObj.contains("channel") && jsonObj["channel"].is_string() && jsonObj["channel"].get<std::string>().empty())
            {
                jsonObj["channel"] = jsonObj["type"].get<std::string>() + std::to_string(static_cast<int>(numWidgets));
            }
        }

        for (auto it = incomingJson.begin(); it != incomingJson.end(); ++it)
        {
            const std::string& key = it.key();
            const auto& value = it.value();

            if (key == "bounds" || key == "range" || key == "size")
            {
                if (value.is_object())
                {
                    for (auto& [propKey, val] : value.items())
                    {
                        jsonObj[key][propKey] = val;
                    }
                }
            }
            else if (key == "sampleRange")
            {
                if (value.is_array() && value.size() == 2)
                {
                    jsonObj["startSample"] = value[0].get<int>();
                    jsonObj["endSample"] = value[1].get<int>();
                }
            }
            else if (key == "populate")
            {
                if (value.is_object())
                {
                    jsonObj[key]["directory"] = value["directory"].get<std::string>();
                    jsonObj[key]["fileType"] = value["fileType"].get<std::string>();

                    std::vector<std::string> files = File::getFilesOfType(value["directory"].get<std::string>(), File::sanitisePath(value["fileType"].get<std::string>()));

                    jsonObj["channelType"] = "string";

                    const std::string items = std::accumulate(
                        std::next(files.begin()), files.end(), files[0],
                        [](std::string a, const std::string& b) { return std::move(a) + ", " + b; }
                    );

                    jsonObj["items"] = items;
                }
            }
            else if (key == "items")
            {
                if (value.is_array())
                {
                    const std::string items = std::accumulate(
                        std::next(value.begin()), value.end(), value[0].get<std::string>(),
                        [](std::string a, const std::string& b) { return std::move(a) + ", " + b; }
                    );
                    jsonObj["items"] = items;
                    jsonObj["min"] = 0;
                    jsonObj["max"] = value.size() - 1;
                }
            }
            else if (key == "samples")
            {
                if (value.is_array())
                {
                    jsonObj["samples"] = value.get<std::vector<double>>();
                }
            }
            else if (key == "colour")
            {
                if (value.is_object())
                {
                    parseColourProperties(value, jsonObj[key]);
                }
                else
                {
                    jsonObj[key] = parseColorValue(value);
                }
            }
            else if (key == "file")
            {
                if (value.is_string())
                {
                    jsonObj["file"] = File::sanitisePath(value.get<std::string>());
                }
            }
            else if (key == "text")
            {
                if (value.is_string())
                {
                    if (Utils::toLower(jsonObj["type"].get<std::string>()).find("button") != std::string::npos)
                    {
                        jsonObj["text"]["on"] = escapeJSON(value.get<std::string>());
                        jsonObj["text"]["off"] = escapeJSON(value.get<std::string>());
                    }
                    else
                    {
                        jsonObj["text"] = escapeJSON(value.get<std::string>());
                    }
                }
                else if (value.is_object())
                {
                    for (auto& [innerKey, val] : value.items())
                    {
                        jsonObj["text"][innerKey] = escapeJSON(val.get<std::string>());
                    }
                }
            }
            else
            {
                jsonObj[key] = value;
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        LOG_INFO("JSON exception: ", e.what());
    }
}

void Parser::parseStroke(const nlohmann::json& strokeValue, nlohmann::json& target)
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

void Parser::parseColourProperties(const nlohmann::json& value, nlohmann::json& target)
{
    for (auto& [key, val] : value.items())
    {
        if (key == "fill" || key == "background")
        {
            target[key] = parseColorValue(val);
        }
        else if (key == "stroke" && val.is_object())
        {
            parseStroke(val, target[key]);
        }
        else if (val.is_object())
        {
            parseColourProperties(val, target[key]);
        }
        else
        {
            target[key] = parseColorValue(val);
        }
    }
}

std::string Parser::parseColorValue(const nlohmann::json& value)
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

std::string Parser::rgbToHex(const std::vector<double>& rgb)
{
    std::ostringstream hex;
    hex << "#" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[0])
        << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[1])
        << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[2]);
    return hex.str();
}

std::string Parser::validateHexString(const std::string& str)
{
    std::regex hexRegex("^#([0-9A-Fa-f]{6})$");

    if (std::regex_match(str, hexRegex))
    {
        return str;
    }
    else
    {
        return Colours::getColour(str);
    }
}

std::string Parser::escapeJSON(const std::string& str)
{
    std::string escaped;

    for (char c : str)
    {
        switch (c)
        {
            case '\"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }

    return escaped;
}

} // namespace cabbage