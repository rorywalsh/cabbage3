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
                    CabbageParser::updateJsonFromSyntax(j, line, widgets.size()+1);
                    widgets.push_back(j);
                    
                }
            }
            catch (nlohmann::json::exception& e) {
                    cabAssert(false, e.what());
            }
        }
        
        return widgets;
    }

    // Function to convert a number to a two-character hex string
    static std::string toHex(int num) {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << std::hex << num;
        return ss.str();
    }

    // Function to convert RGB(A) values to a hex string
    static std::string rgbToHex(const std::vector<double>& numericArgs) {
        int r = static_cast<int>(numericArgs[0]);
        int g = static_cast<int>(numericArgs[1]);
        int b = static_cast<int>(numericArgs[2]);
        int a = numericArgs.size() == 4 ? static_cast<int>(numericArgs[3]) : 255; // Default alpha to 255 if not provided

        return "#" + toHex(r) + toHex(g) + toHex(b) + toHex(a);
    }

    // Function to validate and possibly adjust a hex string
    static std::string validateHexString(const std::string& hexStr) {
        // Mapping of CSS color names to their hex values
        static const std::unordered_map<std::string, std::string> cssColorMap = {
            {"black", "#000000"}, {"white", "#ffffff"}, {"blue", "#0000ff"}, {"grey", "#808080"},
            {"gray", "#808080"}, {"green", "#008000"}, {"red", "#ff0000"}, {"yellow", "#ffff00"},
            {"aliceblue", "#f0f8ff"}, {"antiquewhite", "#faebd7"}, {"aqua", "#00ffff"}, {"aquamarine", "#7fffd4"},
            {"azure", "#f0ffff"}, {"beige", "#f5f5dc"}, {"bisque", "#ffe4c4"}, {"blanchedalmond", "#ffebcd"},
            {"blueviolet", "#8a2be2"}, {"brown", "#a52a2a"}, {"burlywood", "#deb887"}, {"cadetblue", "#5f9ea0"},
            {"chartreuse", "#7fff00"}, {"chocolate", "#d2691e"}, {"coral", "#ff7f50"}, {"cornflowerblue", "#6495ed"},
            {"cornsilk", "#fff8dc"}, {"crimson", "#dc143c"}, {"cyan", "#00ffff"}, {"darkblue", "#00008b"},
            {"darkcyan", "#008b8b"}, {"darkgoldenrod", "#b8860b"}, {"darkgrey", "#a9a9a9"}, {"darkgray", "#a9a9a9"},
            {"darkgreen", "#006400"}, {"darkkhaki", "#bdb76b"}, {"darkmagenta", "#8b008b"}, {"darkolivegreen", "#556b2f"},
            {"darkorange", "#ff8c00"}, {"darkorchid", "#9932cc"}, {"darkred", "#8b0000"}, {"darksalmon", "#e9967a"},
            {"darkseagreen", "#8fbc8f"}, {"darkslateblue", "#483d8b"}, {"darkslategrey", "#2f4f4f"}, {"darkslategray", "#2f4f4f"},
            {"darkturquoise", "#00ced1"}, {"darkviolet", "#9400d3"}, {"deeppink", "#ff1493"}, {"deepskyblue", "#00bfff"},
            {"dimgrey", "#696969"}, {"dimgray", "#696969"}, {"dodgerblue", "#1e90ff"}, {"firebrick", "#b22222"},
            {"floralwhite", "#fffaf0"}, {"forestgreen", "#228b22"}, {"fuchsia", "#ff00ff"}, {"gainsboro", "#dcdcdc"},
            {"ghostwhite", "#f8f8ff"}, {"gold", "#ffd700"}, {"goldenrod", "#daa520"}, {"greenyellow", "#adff2f"},
            {"honeydew", "#f0fff0"}, {"hotpink", "#ff69b4"}, {"indianred", "#cd5c5c"}, {"indigo", "#4b0082"},
            {"ivory", "#fffff0"}, {"khaki", "#f0e68c"}, {"lavender", "#e6e6fa"}, {"lavenderblush", "#fff0f5"},
            {"lawngreen", "#7cfc00"}, {"lemonchiffon", "#fffacd"}, {"lightblue", "#add8e6"}, {"lightcoral", "#f08080"},
            {"lightcyan", "#e0ffff"}, {"lightgoldenrodyellow", "#fafad2"}, {"lightgreen", "#90ee90"}, {"lightgrey", "#d3d3d3"},
            {"lightgray", "#d3d3d3"}, {"lightpink", "#ffb6c1"}, {"lightsalmon", "#ffa07a"}, {"lightseagreen", "#20b2aa"},
            {"lightskyblue", "#87cefa"}, {"lightslategrey", "#778899"}, {"lightslategray", "#778899"}, {"lightsteelblue", "#b0c4de"},
            {"lightyellow", "#ffffe0"}, {"lime", "#00ff00"}, {"limegreen", "#32cd32"}, {"linen", "#faf0e6"},
            {"magenta", "#ff00ff"}, {"maroon", "#800000"}, {"mediumaquamarine", "#66cdaa"}, {"mediumblue", "#0000cd"},
            {"mediumorchid", "#ba55d3"}, {"mediumpurple", "#9370db"}, {"mediumseagreen", "#3cb371"}, {"mediumslateblue", "#7b68ee"},
            {"mediumspringgreen", "#00fa9a"}, {"mediumturquoise", "#48d1cc"}, {"mediumvioletred", "#c71585"}, {"midnightblue", "#191970"},
            {"mintcream", "#f5fffa"}, {"mistyrose", "#ffe4e1"}, {"moccasin", "#ffe4b5"}, {"navajowhite", "#ffdead"},
            {"navy", "#000080"}, {"oldlace", "#fdf5e6"}, {"olive", "#808000"}, {"olivedrab", "#6b8e23"},
            {"orange", "#ffa500"}, {"orangered", "#ff4500"}, {"orchid", "#da70d6"}, {"palegoldenrod", "#eee8aa"},
            {"palegreen", "#98fb98"}, {"paleturquoise", "#afeeee"}, {"palevioletred", "#db7093"}, {"papayawhip", "#ffefd5"},
            {"peachpuff", "#ffdab9"}, {"peru", "#cd853f"}, {"pink", "#ffc0cb"}, {"plum", "#dda0dd"},
            {"powderblue", "#b0e0e6"}, {"purple", "#800080"}, {"rebeccapurple", "#663399"}, {"rosybrown", "#bc8f8f"},
            {"royalblue", "#4169e1"}, {"saddlebrown", "#8b4513"}, {"salmon", "#fa8072"}, {"sandybrown", "#f4a460"},
            {"seagreen", "#2e8b57"}, {"seashell", "#fff5ee"}, {"sienna", "#a0522d"}, {"silver", "#c0c0c0"},
            {"skyblue", "#87ceeb"}, {"slateblue", "#6a5acd"}, {"slategrey", "#708090"}, {"slategray", "#708090"},
            {"snow", "#fffafa"}, {"springgreen", "#00ff7f"}, {"steelblue", "#4682b4"}, {"tan", "#d2b48c"},
            {"teal", "#008080"}, {"thistle", "#d8bfd8"}, {"tomato", "#ff6347"}, {"turquoise", "#40e0d0"},
            {"violet", "#ee82ee"}, {"wheat", "#f5deb3"}, {"whitesmoke", "#f5f5f5"}, {"yellowgreen", "#9acd32"}
        };

        // Check if the input is a CSS color name
        auto it = cssColorMap.find(toLowerCase(hexStr));
        if (it != cssColorMap.end()) {
            std::string hexValue = it->second;
            return hexValue + "ff"; // Add default alpha
        }

        // Check if the input is a hex color code
        if (hexStr[0] != '#' || (hexStr.length() != 7 && hexStr.length() != 9)) {
            cabAssert(false, " is not a valid hex colour");
        }

        if (hexStr.length() == 7) {
            return hexStr + "ff"; // Add default alpha if not provided
        }

        return hexStr;
    }
    
    static void updateJsonWithValue(nlohmann::json& jsonObj, const double value)
    {
        jsonObj["value"] = value;
    }
    
    static void updateJsonFromSyntax(nlohmann::json& jsonObj, const std::string& syntax, size_t numWidgets)
    {
        try
        {
            std::vector<Identifier> tokens = tokeniseLine(syntax);
            //set default channel - every channel must be unique. This will be overwritten if a channel is passed. The channel name of the main form cannot be changed..
            if(jsonObj["type"].get<std::string>() != "form")
                jsonObj["channel"] = jsonObj["type"].get<std::string>()+std::to_string(static_cast<int>(numWidgets));
            
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
                else if (token.name == "populate")
                {
                    if (token.stringArgs.size() == 2)
                    {
                        const std::string dir = token.stringArgs[0];
                        const std::string fileTypes = token.stringArgs[1];
                        jsonObj["currentDirectory"] = dir;
                        jsonObj["fileType"] = fileTypes;
                        //get a sorted list of files from a given directory
                        const auto files = CabbageFile::getFilesOfType( dir, CabbageFile::sanitisePath(fileTypes));
                        
                        jsonObj["channelType"] = "string";
                        jsonObj["files"] = files;

                        // Use std::accumulate to concatenate filenames into a comma-delimited string
                        const std::string items = std::accumulate(
                            std::next(files.begin()), files.end(), CabbageFile::getFileName(files[0]),
                            [](std::string a, const std::string& b) { return std::move(a) + ", " + CabbageFile::getFileName(b); }
                        );

                        jsonObj["items"] = items;
                    }
                    else
                    {
                        std::cout << "The populate() identifier takes 2 parameters" << std::endl;
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
                        jsonObj["min"] = 0;
                        jsonObj["max"] = token.stringArgs.size()-1;
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
                else if (toLowerCase(token.name).find("colour") != std::string::npos)
                {
                    std::string colourString;

                    if (!token.numericArgs.empty())
                        colourString = rgbToHex(token.numericArgs);

                    else if (token.hasStringArgs()){
                        colourString = validateHexString(token.stringArgs[0]);
                    }

                    else
                        cabAssert(false, "token does not contain valid colour information");


                    jsonObj[token.name] = colourString;

                }
                else if (token.name == "channel")
                {
                    if (token.hasStringArgs())
                    {
                        if(token.stringArgs[0].length() == 0)
                        {
                            jsonObj["channel"] = jsonObj["type"];
                        }
                        else
                            jsonObj["channel"] = token.stringArgs[0];
                    }
                    else
                    {
                        std::cout << "The channel() identifier takes a string parameter" << std::endl;
                    }
                }
                else if (token.name == "file")
                {
                    if(token.hasStringArgs())
                    {
                        auto file = CabbageFile::sanitisePath(token.stringArgs[0]);
                        jsonObj[token.name] = file;
                    }
                }
                else if (token.name == "text")
                {
                    if(token.hasStringArgs())
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

    static std::string toLowerCase(const std::string& str) {
        std::string lowerCaseStr = str;
        std::transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return lowerCaseStr;
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
    
    static void replaceCharWithinParentheses(std::string& str, char charToReplace, const std::string& replacementStr) {
        size_t openParenthesisStart = str.find('(');
        if (openParenthesisStart == std::string::npos) {
            std::cerr << "No opening parenthesis found.\n";
            return;
        }

        size_t openParenthesisEnd = str.rfind(')');
        if (openParenthesisEnd == std::string::npos) {
            std::cerr << "No closing parenthesis found.\n";
            return;
        }

        // Only process the text between the first ( and the last )
        std::string innerText = str.substr(openParenthesisStart + 1, openParenthesisEnd - openParenthesisStart - 1);

        // Replace the specified character within the inner text
        size_t pos = 0;
        while ((pos = innerText.find(charToReplace, pos)) != std::string::npos) {
            innerText.replace(pos, 1, replacementStr);
            pos += replacementStr.length();  // Move past the replaced string
        }

        // Update the original string with the modified inner text
        str.replace(openParenthesisStart + 1, openParenthesisEnd - openParenthesisStart - 1, innerText);
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
