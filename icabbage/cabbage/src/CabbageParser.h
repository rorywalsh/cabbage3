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
    
    static std::vector<std::string> getWidgetTypes(){
        return {"form", "rslider", "combobox", "button", "checkbox", "gentable", "label", "hslider", "vslider", "checkbox", "keyboard"};
    }
    
    static bool isWidget(const std::string& target) {
        std::vector<std::string> widgetTypes = getWidgetTypes();
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


    static void updateJsonFromSyntax(nlohmann::json& jsonObj, const std::string& syntax) {
        try{
            std::vector<Identifier> tokens = tokeniseLine(syntax);
            
            // Parse tokens and update JSON object
            for (size_t i = 0; i < tokens.size(); ++i)
            {
                if (tokens[i].name == "bounds")
                {
                    if(tokens[i].numericArgs.size() == 4)
                    {
                        const int top = tokens[i].numericArgs[0];
                        const int left = tokens[i].numericArgs[1];
                        const int width = tokens[i].numericArgs[2];
                        const int height = tokens[i].numericArgs[3];
                        jsonObj["top"] = top;
                        jsonObj["left"] = left;
                        jsonObj["width"] = width;
                        jsonObj["height"] = height;
                    }
                    else
                    {
                        std::cout << "The bounds() identifier takes 4 parameters" << std::endl;
                    }
                }

                else if (tokens[i].name == "channel")
                {
                    const std::string channel = tokens[i].stringArgs[0];
                    jsonObj["channel"] = channel;
                }
                else if (tokens[i].name == "range")
                {
                    if(tokens[i].numericArgs.size()>=3)
                    {
                        const double min = tokens[i].numericArgs[0];
                        const double max = tokens[i].numericArgs[1];
                        const double value = tokens[i].numericArgs[2];
                        jsonObj["min"] = min;
                        jsonObj["max"] = max;
                        jsonObj["defaultValue"] = value;
                        
                        if(tokens[i].numericArgs.size()==5)
                        {
                            double sliderSkew = tokens[i].numericArgs[3];
                            double increment = tokens[i].numericArgs[4];
                            jsonObj["skew"] = sliderSkew;
                            jsonObj["increment"] = increment;
                        }
                    }
                    else
                    {
                        std::cout << "The range() identifier takes at least 3 parameters" << std::endl;
                    }
                }
                else
                {
                    
                    if(tokens[i].hasStringArgs())
                    {
//                        std::cout << "Identifier: " << tokens[i].name << " String:" << tokens[i].stringArgs[0] << std::endl;
                        jsonObj[tokens[i].name] = tokens[i].stringArgs[0];
                    }
                    else
                    {
//                        std::cout << "Identifier: " << tokens[i].name << " Value:" << tokens[i].numericArgs[0] << std::endl;
                        jsonObj[tokens[i].name] = tokens[i].numericArgs[0];
                    }
                }
            }
        }
        catch (nlohmann::json::exception& e) {
                cabAssert(false, e.what());
        }
    }

    static std::vector<CabbageParser::Identifier> tokeniseLine(const std::string& syntax)
    {
        std::vector<CabbageParser::Identifier> identifiers;

        // Replace all occurrences of \" with "
        std::string sanitizedSyntax = syntax;
        std::replace(sanitizedSyntax.begin(), sanitizedSyntax.end(), '\\', '\"');

        // Regular expressions for different parts of the syntax
        std::regex identifierRegex("\\s*([a-zA-Z]+)\\s*\\(([^)]*)\\)");
        std::regex numericArgRegex("-?\\d*\\.?\\d+");
        std::regex stringArgRegex("\"([^\"]*)\"");

        std::smatch match;
        std::string::const_iterator searchStart(sanitizedSyntax.cbegin());

        // Find all identifiers in the sanitized syntax
        while (std::regex_search(searchStart, sanitizedSyntax.cend(), match, identifierRegex)) {
            Identifier identifier;
            identifier.name = match[1].str();

            // Parse numeric arguments
            std::string numericArgsStr = match[2].str();
            std::sregex_iterator numArgIter(numericArgsStr.begin(), numericArgsStr.end(), numericArgRegex);
            std::sregex_iterator end;
            while (numArgIter != end) {
                identifier.numericArgs.push_back(std::stod(numArgIter->str()));
                ++numArgIter;
            }

            // Parse string arguments
            std::sregex_iterator strArgIter(numericArgsStr.begin(), numericArgsStr.end(), stringArgRegex);
            while (strArgIter != end) {
                StringArgument strArg;
                std::string strArgValue = strArgIter->str();
                // Remove quotes
                strArgValue.erase(std::remove(strArgValue.begin(), strArgValue.end(), '\"'), strArgValue.end());
                identifier.stringArgs.push_back(strArgValue);
                ++strArgIter;
            }

            identifiers.push_back(identifier);
            searchStart = match.suffix().first;
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
