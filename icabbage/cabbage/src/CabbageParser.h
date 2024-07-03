#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <json.hpp>
#include "../CabbageWidgetDescriptors.h"
#include "CabbageUtils.h"


class CabbageParser {
public:
    struct StringArgument {
        std::vector<std::string> values;
    };

    struct NumericArgument {
        std::vector<double> values;
    };

    struct Identifier {
        std::string name;
        std::vector<double> numericArgs;
        std::vector<std::string> stringArgs;
        bool hasStringArgs() const {
            return !stringArgs.empty();
        }
    };
    
    
    
    static std::string removeQuotes(const std::string& str) {
        std::string result = str;
        result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return c == '\"'; }), result.end());
        return result;
    }
    

    
    static bool isWidget(const std::string& target) {
        std::vector<std::string> widgetTypes = CabbageWidgetDescriptors::getWidgetTypes();
        // Check if the target string is in the vector
        auto it = std::find(widgetTypes.begin(), widgetTypes.end(), target);
        return it != widgetTypes.end();
    }
    
    static std::vector<nlohmann::json> parseCsdForWidgets(std::string csdFile)
    {
        std::vector<nlohmann::json> widgets;
        std::ifstream file(csdFile);
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string csdContents = oss.str();
        
        std::istringstream iss(csdContents);
        std::string line;
        while (std::getline(iss, line))
        {
            try{
                nlohmann::json j;
                if(CabbageParser::isWidget(CabbageParser::getWidgetType(line)))
                {
                    j = CabbageWidgetDescriptors::get(CabbageParser::getWidgetType(line));
                    CabbageParser::updateJsonFromSyntax(j, line);
                    widgets.push_back(j);
                }
            }
            catch (nlohmann::json::exception& e) {
                    cabAssert(false, e.what());
            }
        }
        
        return widgets;
    }


    static void updateJsonWithValue(nlohmann::json& jsonObj, const double value)
    {
        jsonObj["value"] = value;
    }
    
    static void updateJsonFromSyntax(nlohmann::json& jsonObj, const std::string& syntax)
    {
        try
        {
            std::vector<Identifier> tokens = tokeniseLine(syntax);

            // Parse tokens and update JSON object
            for (const auto& token : tokens)
            {
                if (token.name == "bounds")
                {
                    if (token.numericArgs.size() == 4)
                    {
                        const int left = token.numericArgs[0];
                        const int top = token.numericArgs[1];
                        const int width = token.numericArgs[2];
                        const int height = token.numericArgs[3];
                        jsonObj["left"] = left;
                        jsonObj["top"] = top;
                        jsonObj["width"] = width;
                        jsonObj["height"] = height;
                    }
                    else
                    {
                        std::cout << "The bounds() identifier takes 4 parameters" << std::endl;
                    }
                }
                else if (token.name == "range")
                {
                    if (token.numericArgs.size() >= 3)
                    {
                        const double min = token.numericArgs[0];
                        const double max = token.numericArgs[1];
                        const double value = token.numericArgs[2];
                        jsonObj["min"] = min;
                        jsonObj["max"] = max;
                        jsonObj["value"] = value;

                        if (token.numericArgs.size() == 5)
                        {
                            const double skew = token.numericArgs[3];
                            const double increment = token.numericArgs[4];
                            jsonObj["skew"] = skew;
                            jsonObj["increment"] = increment;
                        }
                    }
                    else
                    {
                        std::cout << "The range() identifier takes at least 3 parameters" << std::endl;
                    }
                }
                else if (token.name == "size")
                {
                    if (token.numericArgs.size() == 2)
                    {
                        const int width = token.numericArgs[0];
                        const int height = token.numericArgs[1];
                        jsonObj["width"] = width;
                        jsonObj["height"] = height;
                    }
                    else
                    {
                        std::cout << "The size() identifier takes 2 parameters" << std::endl;
                    }
                }
                else if (token.name == "sampleRange")
                {
                    if (token.numericArgs.size() == 2)
                    {
                        const int start = token.numericArgs[0];
                        const int end = token.numericArgs[1];
                        jsonObj["startSample"] = start;
                        jsonObj["endSample"] = end;
                    }
                    else
                    {
                        std::cout << "The sampleRange() identifier takes 2 parameters" << std::endl;
                    }
                }
                else if (token.name == "items")
                {
                    if (token.hasStringArgs())
                    {
                        const std::string items = std::accumulate(
                            std::next(token.stringArgs.begin()), token.stringArgs.end(), token.stringArgs[0],
                            [](std::string a, std::string b) { return a + ", " + b; }
                        );
                        jsonObj["items"] = items;
                    }
                    else
                    {
                        std::cout << "The items() identifier takes string parameters" << std::endl;
                    }
                }
                else if (token.name == "samples")
                {
                    if (!token.numericArgs.empty())
                    {
                        std::vector<double> samples(token.numericArgs.begin(), token.numericArgs.end());
                        jsonObj["samples"] = samples;
                    }
                    else
                    {
                        std::cout << "The samples() identifier takes numeric parameters" << std::endl;
                    }
                }
                else if (token.name == "channel")
                {
                    if (token.hasStringArgs())
                    {
                        jsonObj["channel"] = token.stringArgs[0];
                    }
                    else
                    {
                        std::cout << "The channel() identifier takes a string parameter" << std::endl;
                    }
                }
                else if (token.name == "text")
                {
                    //text needs to be escaped properly before we fire it off to the webview..
                    jsonObj[token.name] = escapeJSON(token.stringArgs[0]);
                }
                else
                {
                    if (token.hasStringArgs())
                    {
                        jsonObj[token.name] = token.stringArgs[0];
                    }
                    else if (!token.numericArgs.empty())
                    {
                        jsonObj[token.name] = token.numericArgs[0];
                    }
                    else
                    {
                        std::cout << "The " << token.name << " identifier has no valid arguments" << std::endl;
                    }
                }
            }
        }
        catch (nlohmann::json::exception& e)
        {
            cabAssert(false, e.what());
        }
    }

    static std::string escapeJSON(const std::string& input)
    {
        std::ostringstream oss;
        for (char c : input)
        {
            switch (c) {
                case '\\': oss << "\\\\"; break;
                case '"': oss << "\\\""; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default: oss << c; break;
            }
        }
        return oss.str();
    }
    
    static std::vector<std::string> splitCommaDelimitedArgs(const std::string& input) {
        std::vector<std::string> result;
        std::string item;
        size_t start = 0;
        size_t end = input.find(',');

        while (end != std::string::npos) 
        {
            item = input.substr(start, end - start);
            result.push_back(item);
            start = end + 1;
            end = input.find(',', start);
        }
        // Add the last substring
        item = input.substr(start);
        result.push_back(item);

        return result;
    }
    
    static std::vector<Identifier> tokeniseLine(const std::string& syntax) 
    {
        std::vector<Identifier> identifiers;

        // Regular expressions for different parts of the syntax
        std::regex identifierRegex("\\s*([a-zA-Z]+)\\s*\\(([^)]*)\\)");
        std::regex numericArgRegex("-?\\d*\\.?\\d+");
        std::regex stringArgRegex("\"([^\"]*)\"");

        // Find all identifiers in the syntax
        auto words_begin = std::sregex_iterator(syntax.begin(), syntax.end(), identifierRegex);
        auto words_end = std::sregex_iterator();

        for (auto it = words_begin; it != words_end; ++it) 
        {
            std::smatch match = *it;
            Identifier identifier;
            identifier.name = match[1].str();

            // Parse numeric arguments
            std::string numericArgsStr = match[2].str();
            auto numArgIter = std::sregex_iterator(numericArgsStr.begin(), numericArgsStr.end(), numericArgRegex);
            auto numArgEnd = std::sregex_iterator();
            while (numArgIter != numArgEnd) 
            {
                identifier.numericArgs.push_back(std::stod(numArgIter->str()));
                ++numArgIter;
            }

            // Parse string arguments
            auto strArgIter = std::sregex_iterator(numericArgsStr.begin(), numericArgsStr.end(), stringArgRegex);
            auto strArgEnd = std::sregex_iterator();
            while (strArgIter != strArgEnd) 
            {
                std::string strArgValue = (*strArgIter)[1].str(); // Capture group 1 is inside the quotes
                identifier.stringArgs.push_back(strArgValue);
                ++strArgIter;
            }

            identifiers.push_back(identifier);
        }

        return identifiers;
    }

    static std::string getWidgetType(const std::string& line)
    {
        std::istringstream iss(line);
        std::string firstWord;
        iss >> std::ws >> firstWord;
        return firstWord;
    }
        
};
