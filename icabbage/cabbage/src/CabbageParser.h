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

//
//class CabbageParser2 {
//public:
//    struct StringArgument {
//        std::vector<std::string> values;
//    };
//
//    struct NumericArgument {
//        std::vector<double> values;
//    };
//
//    struct Identifier {
//        std::string name;
//        std::vector<double> numericArgs;
//        std::vector<std::string> stringArgs;
//        bool hasStringArgs() const {
//            return !stringArgs.empty();
//        }
//    };
//    
//    
//    static std::vector<nlohmann::json> parseCsdForWidgets(std::string csdFile)
//    {
////        std::vector<nlohmann::json> widgets;
////        std::ifstream file(csdFile);
////        std::ostringstream oss;
////        oss << file.rdbuf();
////        std::string csdContents = oss.str();
////        
////        std::istringstream iss(csdContents);
////        std::string line;
////        while (std::getline(iss, line))
////        {
////            try{
////                nlohmann::json j;
////                if(CabbageParser::isWidget(CabbageParser::getWidgetType(line)))
////                {
////                    j = CabbageWidgetDescriptors::get(CabbageParser::getWidgetType(line));
////                    CabbageParser::updateJsonFromSyntax(j, line, widgets.size()+1);
////                    widgets.push_back(j);
////                    
////                }
////            }
////            catch (nlohmann::json::exception& e) {
////                    cabAssert(false, e.what());
////            }
////        }
////        
////        return widgets;
//    }
//
//    
//    
//    static void updateJsonWithValue(nlohmann::json& jsonObj, const double value)
//    {
//        jsonObj["value"] = value;
//    }
//    
//    static void updateJsonFromSyntax(nlohmann::json& jsonObj, const std::string& syntax, size_t numWidgets)
//    {
//        try
//        {
//            std::vector<Identifier> tokens = tokeniseLine(syntax);
//            //set default channel - every channel must be unique. This will be overwritten if a channel is passed. The channel name of the main form cannot be changed..
//            if(jsonObj["type"].get<std::string>() != "form")
//                jsonObj["channel"] = jsonObj["type"].get<std::string>()+std::to_string(static_cast<int>(numWidgets));
//            
//            // Parse tokens and update JSON object
//            for (const auto& token : tokens)
//            {
//                if (token.name == "bounds")
//                {
//                    if (token.numericArgs.size() == 4)
//                    {
//                        const int left = token.numericArgs[0];
//                        const int top = token.numericArgs[1];
//                        const int width = token.numericArgs[2];
//                        const int height = token.numericArgs[3];
//                        jsonObj["left"] = left;
//                        jsonObj["top"] = top;
//                        jsonObj["width"] = width;
//                        jsonObj["height"] = height;
//                    }
//                    else
//                    {
//                        std::cout << "The bounds() identifier takes 4 parameters" << std::endl;
//                    }
//                }
//                else if (token.name == "range")
//                {
//                    if (token.numericArgs.size() >= 3)
//                    {
//                        const double min = token.numericArgs[0];
//                        const double max = token.numericArgs[1];
//                        const double value = token.numericArgs[2];
//                        jsonObj["min"] = min;
//                        jsonObj["max"] = max;
//                        jsonObj["value"] = value;
//
//                        if (token.numericArgs.size() == 5)
//                        {
//                            const double skew = token.numericArgs[3];
//                            const double increment = token.numericArgs[4];
//                            jsonObj["skew"] = skew;
//                            jsonObj["increment"] = increment;
//                        }
//                    }
//                    else
//                    {
//                        std::cout << "The range() identifier takes at least 3 parameters" << std::endl;
//                    }
//                }
//                else if (token.name == "size")
//                {
//                    if (token.numericArgs.size() == 2)
//                    {
//                        const int width = token.numericArgs[0];
//                        const int height = token.numericArgs[1];
//                        jsonObj["width"] = width;
//                        jsonObj["height"] = height;
//                    }
//                    else
//                    {
//                        std::cout << "The size() identifier takes 2 parameters" << std::endl;
//                    }
//                }
//                else if (token.name == "sampleRange")
//                {
//                    if (token.numericArgs.size() == 2)
//                    {
//                        const int start = token.numericArgs[0];
//                        const int end = token.numericArgs[1];
//                        jsonObj["startSample"] = start;
//                        jsonObj["endSample"] = end;
//                    }
//                    else
//                    {
//                        std::cout << "The sampleRange() identifier takes 2 parameters" << std::endl;
//                    }
//                }
//                else if (token.name == "populate")
//                {
//                    if (token.stringArgs.size() == 2)
//                    {
//                        const std::string dir = token.stringArgs[0];
//                        const std::string fileTypes = token.stringArgs[1];
//                        jsonObj["currentDirectory"] = dir;
//                        jsonObj["fileType"] = fileTypes;
//                        //get a sorted list of files from a given directory
//                        const auto files = CabbageFile::getFilesOfType( dir, CabbageFile::sanitisePath(fileTypes));
//                        
//                        jsonObj["channelType"] = "string";
//                        jsonObj["files"] = files;
//
//                        // Use std::accumulate to concatenate filenames into a comma-delimited string
//                        const std::string items = std::accumulate(
//                            std::next(files.begin()), files.end(), CabbageFile::getFileName(files[0]),
//                            [](std::string a, const std::string& b) { return std::move(a) + ", " + CabbageFile::getFileName(b); }
//                        );
//
//                        jsonObj["items"] = items;
//                    }
//                    else
//                    {
//                        std::cout << "The populate() identifier takes 2 parameters" << std::endl;
//                    }
//                }
//                else if (token.name == "items")
//                {
//                    if (token.hasStringArgs())
//                    {
//                        const std::string items = std::accumulate(
//                            std::next(token.stringArgs.begin()), token.stringArgs.end(), token.stringArgs[0],
//                            [](std::string a, std::string b) { return a + ", " + b; }
//                        );
//                        jsonObj["items"] = items;
//                        jsonObj["min"] = 0;
//                        jsonObj["max"] = token.stringArgs.size()-1;
//                    }
//                    else
//                    {
//                        std::cout << "The items() identifier takes string parameters" << std::endl;
//                    }
//                }
//                else if (token.name == "samples")
//                {
//                    if (!token.numericArgs.empty())
//                    {
//                        std::vector<double> samples(token.numericArgs.begin(), token.numericArgs.end());
//                        jsonObj["samples"] = samples;
//                    }
//                    else
//                    {
//                        std::cout << "The samples() identifier takes numeric parameters" << std::endl;
//                    }
//                }
//                else if (toLowerCase(token.name).find("colour") != std::string::npos)
//                {
//                    std::string colourString;
//
//                    if (!token.numericArgs.empty())
//                        colourString = rgbToHex(token.numericArgs);
//
//                    else if (token.hasStringArgs()){
//                        colourString = validateHexString(token.stringArgs[0]);
//                    }
//
//                    else
//                        cabAssert(false, "token does not contain valid colour information");
//
//
//                    jsonObj[token.name] = colourString;
//
//                }
//                else if (token.name == "channel")
//                {
//                    if (token.hasStringArgs())
//                    {
//                        if(token.stringArgs[0].length() == 0)
//                        {
//                            jsonObj["channel"] = jsonObj["type"];
//                        }
//                        else
//                            jsonObj["channel"] = token.stringArgs[0];
//                    }
//                    else
//                    {
//                        std::cout << "The channel() identifier takes a string parameter" << std::endl;
//                    }
//                }
//                else if (token.name == "file")
//                {
//                    if(token.hasStringArgs())
//                    {
//                        auto file = CabbageFile::sanitisePath(token.stringArgs[0]);
//                        jsonObj[token.name] = file;
//                    }
//                }
//                else if (token.name == "text")
//                {
//                    if(token.hasStringArgs())
//                        jsonObj[token.name] = escapeJSON(token.stringArgs[0]);
//                }
//                else
//                {
//                    if (token.hasStringArgs())
//                    {
//                        jsonObj[token.name] = token.stringArgs[0];
//                    }
//                    else if (!token.numericArgs.empty())
//                    {
//                        jsonObj[token.name] = token.numericArgs[0];
//                    }
//                    else
//                    {
//                        std::cout << "The " << token.name << " identifier has no valid arguments" << std::endl;
//                    }
//                }
//            }
//        }
//        catch (nlohmann::json::exception& e)
//        {
//            cabAssert(false, e.what());
//        }
//    }
//
//    static std::string toLowerCase(const std::string& str) {
//        std::string lowerCaseStr = str;
//        std::transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(),
//                       [](unsigned char c){ return std::tolower(c); });
//        return lowerCaseStr;
//    }
//    
//    static std::string escapeJSON(const std::string& input)
//    {
//        std::ostringstream oss;
//        for (char c : input)
//        {
//            switch (c) {
//                case '\\': oss << "\\\\"; break;
//                case '"': oss << "\\\""; break;
//                case '\n': oss << "\\n"; break;
//                case '\r': oss << "\\r"; break;
//                case '\t': oss << "\\t"; break;
//                default: oss << c; break;
//            }
//        }
//        return oss.str();
//    }
//    
//    static std::vector<std::string> splitCommaDelimitedArgs(const std::string& input) {
//        std::vector<std::string> result;
//        std::string item;
//        size_t start = 0;
//        size_t end = input.find(',');
//
//        while (end != std::string::npos) 
//        {
//            item = input.substr(start, end - start);
//            result.push_back(item);
//            start = end + 1;
//            end = input.find(',', start);
//        }
//        // Add the last substring
//        item = input.substr(start);
//        result.push_back(item);
//
//        return result;
//    }
//    
//    static void replaceCharWithinParentheses(std::string& str, char charToReplace, const std::string& replacementStr) {
//        size_t openParenthesisStart = str.find('(');
//        if (openParenthesisStart == std::string::npos) {
//            std::cerr << "No opening parenthesis found.\n";
//            return;
//        }
//
//        size_t openParenthesisEnd = str.rfind(')');
//        if (openParenthesisEnd == std::string::npos) {
//            std::cerr << "No closing parenthesis found.\n";
//            return;
//        }
//
//        // Only process the text between the first ( and the last )
//        std::string innerText = str.substr(openParenthesisStart + 1, openParenthesisEnd - openParenthesisStart - 1);
//
//        // Replace the specified character within the inner text
//        size_t pos = 0;
//        while ((pos = innerText.find(charToReplace, pos)) != std::string::npos) {
//            innerText.replace(pos, 1, replacementStr);
//            pos += replacementStr.length();  // Move past the replaced string
//        }
//
//        // Update the original string with the modified inner text
//        str.replace(openParenthesisStart + 1, openParenthesisEnd - openParenthesisStart - 1, innerText);
//    }
//
//    
//    static std::vector<Identifier> tokeniseLine(const std::string& syntax)
//    {
//        std::vector<Identifier> identifiers;
//        
//        // Regular expressions for different parts of the syntax
//        std::regex identifierRegex("\\s*([a-zA-Z]+)\\s*\\(([^)]*)\\)");
//        std::regex numericArgRegex("-?\\d*\\.?\\d+");
//        std::regex stringArgRegex("\"([^\"]*)\"");
//
//        // Find all identifiers in the syntax
//        auto words_begin = std::sregex_iterator(syntax.begin(), syntax.end(), identifierRegex);
//        auto words_end = std::sregex_iterator();
//
//        for (auto it = words_begin; it != words_end; ++it)
//        {
//            std::smatch match = *it;
//            Identifier identifier;
//            identifier.name = match[1].str();
//
//            // Parse numeric arguments
//            std::string numericArgsStr = match[2].str();
//            auto numArgIter = std::sregex_iterator(numericArgsStr.begin(), numericArgsStr.end(), numericArgRegex);
//            auto numArgEnd = std::sregex_iterator();
//            while (numArgIter != numArgEnd)
//            {
//                identifier.numericArgs.push_back(std::stod(numArgIter->str()));
//                ++numArgIter;
//            }
//
//            // Parse string arguments
//            auto strArgIter = std::sregex_iterator(numericArgsStr.begin(), numericArgsStr.end(), stringArgRegex);
//            auto strArgEnd = std::sregex_iterator();
//            while (strArgIter != strArgEnd)
//            {
//                std::string strArgValue = (*strArgIter)[1].str(); // Capture group 1 is inside the quotes
//                identifier.stringArgs.push_back(strArgValue);
//                ++strArgIter;
//            }
//
//            identifiers.push_back(identifier);
//        }
//
//        return identifiers;
//    }
//    
//    static std::string getWidgetType(const std::string& line)
//    {
//        std::istringstream iss(line);
//        std::string firstWord;
//        iss >> std::ws >> firstWord;
//        return firstWord;
//    }
//        
//};
