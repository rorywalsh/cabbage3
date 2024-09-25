#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <json.hpp>
#include "../CabbageWidgetDescriptors.h"
#include "CabbageUtils.h"


class CabbageParser
{
public:

    static std::string removeQuotes(const std::string& str)
    {
        std::string result = str;
        result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return c == '\"'; }), result.end());
        return result;
    }

    static bool isWidget(const std::string& target)
    {
        std::vector<std::string> widgetTypes = CabbageWidgetDescriptors::getWidgetTypes();
        for( auto& w : widgetTypes )
            _log(w);
        auto it = std::find(widgetTypes.begin(), widgetTypes.end(), target);
        return it != widgetTypes.end();
    }

    static std::vector<nlohmann::json> parseCsdForWidgets(const std::string& csdFile)
    {
        std::vector<nlohmann::json> widgets;

        // Read the content of the CSD file
        std::ifstream file(csdFile);
        if (!file.is_open())
        {
            std::cerr << "Error opening Csd file: " << csdFile << std::endl;
            return widgets;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Regular expression to extract the content between <Cabbage> and </Cabbage>
        std::regex cabbageRegex(R"(<Cabbage>([\s\S]*?)</Cabbage>)");
        std::smatch cabbageMatch;

        if (std::regex_search(content, cabbageMatch, cabbageRegex))
        {
            std::string cabbageContent = cabbageMatch[1].str();

            // Check for form widget
            std::regex formRegex(R"("type"\s*:\s*"form")");
            bool foundFormWidget = std::regex_search(cabbageContent, formRegex);

            if (foundFormWidget)
            {
                parseContent(cabbageContent, widgets);
            }
            else
            {
                // Check for #include "filename.json"
                std::regex includeRegex(R"(#include\s*\"([^\"]+\.json)\")");
                std::smatch includeMatch;

                if (std::regex_search(cabbageContent, includeMatch, includeRegex))
                {
                    std::string includeFilename = includeMatch[1].str();
                    std::filesystem::path includePath(includeFilename);

                    // If the path is not absolute, construct a new path based on the CSD file's directory
                    if (!includePath.is_absolute())
                    {
                        std::filesystem::path csdPath(csdFile);
                        includePath = csdPath.parent_path() / includePath;
                    }

                    // Convert the path to a string and parse the JSON file
                    parseJsonFile(includePath.string(), widgets);
                }
                else
                {
                    // Use a .json file with the same base name as the CSD file
                    std::string fallbackJsonFile = csdFile.substr(0, csdFile.find_last_of('.')) + ".json";
                    parseJsonFile(fallbackJsonFile, widgets);
                }
            }
        }
        else
        {
            std::cerr << "No Cabbage section found in the file: " << csdFile << std::endl;
        }

        return widgets;
    }

    // Helper function to parse content as JSON and add to widgets
    static void parseContent(const std::string& content, std::vector<nlohmann::json>& widgets)
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
                        auto j = CabbageWidgetDescriptors::get(item["type"]);
                        updateJson(j, item, widgets.size());
                        widgets.push_back(j);
                    }
                    else
                    {
                        std::cerr << "Encountered non-object in JSON array" << std::endl;
                    }
                }
            }
            else
            {
                std::cerr << "JSON content is not an array" << std::endl;
            }
        }
        catch (const nlohmann::json::parse_error& e)
        {
            _log("JSON parse error: " << e.what() << std::endl);
        }
    }

    // Helper function to parse a JSON file and add to widgets
    static void parseJsonFile(const std::string& filename, std::vector<nlohmann::json>& widgets)
    {
        std::ifstream jsonFile(filename);
        if (!jsonFile.is_open())
        {
            std::cerr << "Error opening JSON file: " << filename << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << jsonFile.rdbuf();
        parseContent(buffer.str(), widgets);
    }

    static void updateJson(nlohmann::json& jsonObj, const nlohmann::json& incomingJson, size_t numWidgets)
    {
        try
        {
            if (jsonObj["type"].get<std::string>() != "form")
            {
                //if channel is empty, assign it a unique name
                if (jsonObj.contains("channel") && jsonObj["channel"].is_string() && jsonObj["channel"].get<std::string>().empty())
                {
                    jsonObj["channel"] = jsonObj["type"].get<std::string>() + std::to_string(static_cast<int>(numWidgets));
                }
            }

            //parse multi argument identifiers here into separate keys
            for (auto it = incomingJson.begin(); it != incomingJson.end(); ++it)
            {
                const std::string& key = it.key();
                const auto& value = it.value();
                
                if (key == "bounds")
                {
                    if (value.is_array() && value.size() == 4)
                    {
                        jsonObj["left"] = value[0].get<int>();
                        jsonObj["top"] = value[1].get<int>();
                        jsonObj["width"] = value[2].get<int>();
                        jsonObj["height"] = value[3].get<int>();
                    }
                    else
                    {
                        std::cerr << "The bounds identifier takes 4 parameters" << std::endl;
                    }
                }
                else if (key == "range")
                {
                    if (value.is_array() && value.size() >= 3)
                    {
                        jsonObj["min"] = value[0].get<double>();
                        jsonObj["max"] = value[1].get<double>();
                        jsonObj["value"] = value[2].get<double>();

                        if (value.size() == 5)
                        {
                            jsonObj["skew"] = value[3].get<double>();
                            jsonObj["increment"] = value[4].get<double>();
                        }
                    }
                    else
                    {
                        std::cerr << "The range identifier takes at least 3 parameters" << std::endl;
                    }
                }
                else if (key == "size")
                {
                    if (value.is_array() && value.size() == 2)
                    {
                        jsonObj["width"] = value[0].get<int>();
                        jsonObj["height"] = value[1].get<int>();
                    }
                    else
                    {
                        std::cerr << "The size identifier takes 2 parameters" << std::endl;
                    }
                }
                else if (key == "sampleRange")
                {
                    if (value.is_array() && value.size() == 2)
                    {
                        jsonObj["startSample"] = value[0].get<int>();
                        jsonObj["endSample"] = value[1].get<int>();
                    }
                    else
                    {
                        std::cerr << "The sampleRange identifier takes 2 parameters" << std::endl;
                    }
                }
                else if (key == "populate")
                {
                    if (value.is_array() && value.size() == 2)
                    {
                        jsonObj["currentDirectory"] = value[0].get<std::string>();
                        jsonObj["fileType"] = value[1].get<std::string>();

                        std::vector<std::string> files = CabbageFile::getFilesOfType(value[0].get<std::string>(), CabbageFile::sanitisePath(value[1].get<std::string>()));

                        jsonObj["channelType"] = "string";
                        jsonObj["files"] = files;

                        const std::string items = std::accumulate(
                            std::next(files.begin()), files.end(), files[0],
                            [](std::string a, const std::string& b) { return std::move(a) + ", " + b; }
                        );

                        jsonObj["items"] = items;
                    }
                    else
                    {
                        std::cerr << "The populate identifier takes 2 parameters" << std::endl;
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
                    else
                    {
                        std::cerr << "The items identifier takes string parameters" << std::endl;
                    }
                }
                else if (key == "samples")
                {
                    if (value.is_array())
                    {
                        jsonObj["samples"] = value.get<std::vector<double>>();
                    }
                    else
                    {
                        std::cerr << "The samples identifier takes numeric parameters" << std::endl;
                    }
                }
                else if (key.find("colour") != std::string::npos)
                {
                    std::string colourString;

                    if (value.is_array())
                    {
                        colourString = rgbToHex(value.get<std::vector<double>>());
                    }
                    else if (value.is_string())
                    {
                        colourString = validateHexString(value.get<std::string>());
                    }
                    else
                    {
                        std::cerr << "Token does not contain valid colour information" << std::endl;
                    }

                    jsonObj[key] = colourString;
                }
//                else if (key == "channel")
//                {
//                    if (value.is_string() && value.get<std::string>().empty())
//                    {
//
//                    }
//                    else
//                    {
//                        std::cerr << "The channel identifier takes a string parameter" << std::endl;
//                    }
//                }
                else if (key == "file")
                {
                    if (value.is_string())
                    {
                        jsonObj["file"] = CabbageFile::sanitisePath(value.get<std::string>());
                    }
                }
                else if (key == "text")
                {
                    if (value.is_string())
                    {
                        jsonObj["text"] = escapeJSON(value.get<std::string>());
                        jsonObj["textOn"] = escapeJSON(value.get<std::string>());
                        jsonObj["textOff"] = escapeJSON(value.get<std::string>());
                    }
                    else if (value.is_array() && value.size() == 2)
                    {
                        jsonObj["textOff"] = value[0].get<std::string>();
                        jsonObj["textOn"] = value[1].get<std::string>();
                    }
                }
                else
                {
                    
                    if (value.is_string())
                    {
//                        _log("key is " << key << ", value is " << value.get<std::string>());
                        jsonObj[key] = value.get<std::string>();
                    }
                    else if (value.is_number())
                    {
                        jsonObj[key] = value;
                    }
                    else
                    {
                        std::cerr << "The " << key << " identifier has no valid arguments" << std::endl;
                    }
                }
            }
//            _log("channel(\"" << jsonObj["channel"].get<std::string>() << "\") bounds(" << jsonObj["left"].get<int>() << ", " << jsonObj["top"].get<int>() << ")");

        }
        catch (const nlohmann::json::exception& e)
        {
            std::cerr << "JSON exception: " << e.what() << std::endl;
        }
        
    }

private:

    static std::string rgbToHex(const std::vector<double>& rgb)
    {
        std::ostringstream hex;
        hex << "#" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[0])
            << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[1])
            << std::setw(2) << std::setfill('0') << static_cast<int>(rgb[2]);
        return hex.str();
    }

    static std::string validateHexString(const std::string& str)
    {
        std::regex hexRegex("^#([0-9A-Fa-f]{6})$");

        if (std::regex_match(str, hexRegex))
        {
            return str;
        }
        else
        {
            std::cerr << "Invalid hex colour string: " << str << std::endl;
            return "#000000"; // Default to black
        }
    }

    static std::string escapeJSON(const std::string& str)
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
};
