#include <iostream>
#include <regex>
#include <string>
#include <vector>

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
        std::vector<StringArgument> stringArgs;
        bool hasStringArgs() const {
            return !stringArgs.empty();
        }
    };
    
    static std::string removeQuotes(const std::string& str) {
        std::string result = str;
        result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return c == '\"'; }), result.end());
        return result;
    }
    
    // Function to tokenize the syntax using regular expressions
    static std::vector<Identifier> tokeniseLineOfCabbageCode(const std::string& syntax) 
    {
        std::vector<Identifier> identifiers;
        
        // Regular expressions for different parts of the syntax
        std::regex identifierRegex("\\s*([a-zA-Z]+)\\s*\\(([^)]*)\\)");
        std::regex numericArgRegex("-?\\d*\\.?\\d+");
        std::regex stringArgRegex("\"([^\"]*)\"");
        
        std::smatch match;
        std::string::const_iterator searchStart(syntax.cbegin());
        
        // Find all identifiers in the syntax
        while (std::regex_search(searchStart, syntax.cend(), match, identifierRegex)) {
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

                std::istringstream iss(strArgValue);
                std::string token;
                while (iss >> token) {
                    strArg.values.push_back(token);
                }
                identifier.stringArgs.push_back(strArg);
                ++strArgIter;
            }
            
            identifiers.push_back(identifier);
            searchStart = match.suffix().first;
        }
        
        return identifiers;
    }
    
    
};
