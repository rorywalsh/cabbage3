/*
 * Copyright (c) 2024 Rory Walsh
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#undef _CR
/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */


#include <readerwriterqueue.h>
#include <plugin.h>
#include "json.hpp"
#include <type_traits>

#define IS_OK 0
#define NOT_OK 1

struct CabbageOpcodeData
{
    enum MessageType{
        Value,
        Identifier
    };
    
    enum PassType{
        Init,
        Perf
    };
        
    nlohmann::json cabbageJson = {};
    std::string channel = {};
    std::string identifier = {};
    MessageType type;
};

template <std::size_t NumInputParams>
struct CabbageOpcodes
{
    std::vector<nlohmann::json>** wd = nullptr;
    char* name = NULL;
    char* identifier = NULL;
    MYFLT* value = {};
    MYFLT lastValue = 0;
    MYFLT* str = {};
    
    static bool hasNullTerminator(const char* str, size_t length)
    {
        for (size_t i = 0; i < length; ++i) 
        {
            if (str[i] == '\0') 
            {
                return true;
            }
        }
        return false;
    }
    
    
    static std::string removeNullTerminator(const std::string& str) 
    {
        // Create a copy of the string
        std::string result = str;

        // If the last character is null terminator, remove it
        if (!result.empty() && result.back() == '\0') {
            result.pop_back();
        }

        return result;
    }
    
    bool hasNullTerminator(const std::string& str)
    {
        const char* cStr = str.c_str();
        size_t length = str.size();

        // Check if the character after the last element is a null terminator
        return cStr[length] == '\0';
    }
    
    //run identifier strings through this to make sure things are correctly escaped - they will form part
    //of a JSON string
    static std::string sanitiseString(const std::string& input) 
    {
        std::string sanitized;
        sanitized.reserve(input.size() * 2); // Reserve space to avoid frequent reallocations

        for (char c : input) {
            if (c == '\n') {
                sanitized += "\\n"; // Replace newline with literal "\\n"
            } else {
                switch (c) {
                    case '\\': sanitized += "\\\\"; break;
                    case '\"': sanitized += "\\\""; break;
                    case '\r': sanitized += "\\r"; break;
                    case '\t': sanitized += "\\t"; break;
                    default: sanitized += c; break;
                }
            }
        }

        return sanitized;
    }
    
    CabbageOpcodeData getValueIdentData(csnd::Param<NumInputParams>& args, bool init, int nameIndex, int identIndex)
    {
        CabbageOpcodeData data;
        if(init)
        {
            if(args.str_data(nameIndex).size == 0)
                name = {};
            else
                name = args.str_data(nameIndex).data;
        }

        data.cabbageJson["value"] = 0;
        data.channel = name;
        return data;
    }
    

    //this check for brackets within strings, when argument is in form of text("this is a string")
    bool containsIllegalCharsWithinParentheses(const std::string& str) 
    {
        size_t openParenthesis = str.find('(');
        if (openParenthesis == std::string::npos) 
        {
            return false;  // No opening parenthesis found
        }

        size_t closeParenthesis = str.rfind(')');
        if (closeParenthesis == std::string::npos || closeParenthesis < openParenthesis) 
        {
            return false;  // No closing parenthesis found or closing parenthesis is before opening parenthesis
        }

        // Check for () character within the parentheses
        for (size_t i = openParenthesis + 1; i < closeParenthesis; ++i) 
        {
            if (str[i] == ')' || str[i] == '(')
                return true;
        }
        return false;
    }
    
    //this check for brackets within strings, when argument is in form of "this is a string", i.e, no identifier
    bool containsIllegalChars(const std::string& str) 
    {
        // Check for both '(' and ')' using ||
        if (str.find('(') != std::string::npos || str.find(')') != std::string::npos) 
        {
            return true;
        }
        return false;
    }
    
    // Function to split a dot notation string into a vector of keys
    std::vector<std::string> split(const std::string& str, char delimiter = '.')
    {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) 
        {
            tokens.push_back(token);
        }
        return tokens;
    }

    // Function to set a value in a JSON object using dot notation
    void setJsonValue(nlohmann::json& jsonObj, const std::string& dotNotation, const nlohmann::json& value)
    {
        std::vector<std::string> keys = split(dotNotation, '.');
        nlohmann::json* current = &jsonObj;

        for (size_t i = 0; i < keys.size(); ++i) 
        {
            const std::string& key = keys[i];

            // If we're at the last key, set the value
            if (i == keys.size() - 1) 
            {
                (*current)[key] = value;
            } else 
            {
                // If the key doesn't exist, create a new object
                if (!(*current).contains(key)) 
                {
                    (*current)[key] = nlohmann::json::object();
                }
                // Move deeper into the JSON structure
                current = &(*current)[key];
            }
        }
    }
    
    // Function to access a JSON object using dot notation
    nlohmann::json getJsonValue(const nlohmann::json& jsonObj, const std::string& jsonString)
    {
        if(jsonString.find(".") == std::string::npos)
        {
            if(jsonObj.contains(jsonString))
                return jsonObj[jsonString];
            
            return jsonObj;            
            
        }
        
//        _log(jsonObj.dump(4));
        //else deal with dot notation
        std::vector<std::string> keys = split(jsonString, '.');
        nlohmann::json current = jsonObj;
        for (const auto& key : keys)
        {
            if (current.contains(key)) 
            {
                current = current[key];
            } 
            else
            {
                return nullptr; // Return null if the key does not exist
            }
        }
        return current;
    }
    
    template <typename T>
    void updateWidgetJson(nlohmann::json& jsonObj, csnd::Param<NumInputParams>& args, int argIndex, int numIns, std::string identifier)
    {
        std::vector<T> params;
        if(numIns>argIndex+2) //dealing with array...
        {
            for( int i = argIndex ; i < numIns ; i++)
            {
                if constexpr (std::is_same_v<T, std::string>)
                    params.push_back(args.str_data(i).data);
                else
                    params.push_back(args[i]);
            }
            jsonObj[identifier] = params;
        }
        else
        {
            //check if the identifier is already a JSON object, i.e, as in the case below
            //cabbageSet metro(1), "infoText", sprintf({{"text":"%s"}}, SText)
            auto j = parseAndFormatJson(identifier);
            auto it = j.begin();
            if (it.value().is_null())
            {
                if(identifier.find(".") == std::string::npos)
                {
                    if constexpr (std::is_same_v<T, std::string>)
                        jsonObj[identifier] = args.str_data(argIndex).data;
                    else
                        jsonObj[identifier] = args[argIndex];
                }
                else
                {
                    //dot notation
                    if constexpr (std::is_same_v<T, std::string>)
                        setJsonValue(jsonObj, args.str_data(argIndex-1).data, args.str_data(argIndex).data);
                    else
                        setJsonValue(jsonObj, args.str_data(argIndex-1).data, args[argIndex]);
                }
                LOG_INFO(jsonObj.dump(4));
            }
            else
            {
                //identifier arg was well formed JSON
                jsonObj = j;
            }
        }
    }
    
    bool testForValidNumberOfInputs(int totalInputs, int minInputs)
    {
        return (totalInputs >= minInputs);
    }
    
    nlohmann::json parseAndFormatJson(const std::string& jsonString)
    {
        // Wrap the input string with braces to form a complete JSON object
        std::string wrappedJson = "{" + jsonString + "}";

        try {
            // Attempt to parse the wrapped JSON string
            if (nlohmann::json::accept(wrappedJson))
                return nlohmann::json::parse(wrappedJson);
            else
            {
                // If parsing fails, create a new JSON object with an empty key
                nlohmann::json fallbackJson;
                fallbackJson[split(jsonString)[0]] = nullptr;
                return fallbackJson;
            }
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
        return {};
    }
    
    CabbageOpcodeData getIdentData(csnd::Csound* csound, csnd::Param<NumInputParams>& args, bool init, int channelIndex, int identIndex)
    {
        CabbageOpcodeData data;
        if(init)
        {
            if(args.str_data(channelIndex).size == 0)
                name = {};
            else
            {
                name = args.str_data(channelIndex).data;
            }

            if(args.str_data(identIndex).size == 0)
                identifier = {};
            else
            {
                if(containsIllegalCharsWithinParentheses(args.str_data(identIndex).data))
                {
                    csound->message("Cabbage Warning: Ill-formatted arguments passed to channel:\""+data.channel+"\" Check for brackets within strings..");
                }
                identifier = args.str_data(identIndex).data;
            }
        }
        
        data.identifier = identifier;
        data.channel = name;
        try {
            data.cabbageJson = parseAndFormatJson(identifier);
        }
        catch (const nlohmann::json::parse_error& e){
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }

        return data;
    }
    
};
