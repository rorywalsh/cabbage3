
/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <readerwriterqueue.h>
#include <plugin.h>
#include "json.hpp"

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
    MessageType type;
    MYFLT value = 0;

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
    bool containsIllegalCharsWithinParentheses(const std::string& str) {
        size_t openParenthesis = str.find('(');
        if (openParenthesis == std::string::npos) {
            return false;  // No opening parenthesis found
        }

        size_t closeParenthesis = str.rfind(')');
        if (closeParenthesis == std::string::npos || closeParenthesis < openParenthesis) {
            return false;  // No closing parenthesis found or closing parenthesis is before opening parenthesis
        }

        // Check for () character within the parentheses
        for (size_t i = openParenthesis + 1; i < closeParenthesis; ++i) {
            if (str[i] == ')' || str[i] == '(')
                return true;
        }
        return false;
    }
    
    //this check for brackets within strings, when argument is in form of "this is a string", i.e, no identifier
    bool containsIllegalChars(const std::string& str) {
        // Check for both '(' and ')' using ||
        if (str.find('(') != std::string::npos || str.find(')') != std::string::npos) {
            return true;
        }
        return false;
    }
    
    nlohmann::json parseAndFormatJson(const std::string& jsonString) {
        // Wrap the input string with braces to form a complete JSON object
        std::string wrappedJson = "{" + jsonString + "}";

        try {
            // Attempt to parse the wrapped JSON string
            return nlohmann::json::parse(wrappedJson);
        } catch (const nlohmann::json::parse_error&) {
            // If parsing fails, create a new JSON object with an empty key
            nlohmann::json fallbackJson;
            fallbackJson[jsonString] = nullptr; 
            return fallbackJson;
        }
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