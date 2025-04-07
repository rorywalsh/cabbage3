#include "CabbageUtils.h"
#include <text/choc_Files.h>

namespace cabbage {

std::string Utils::sanitisePath(const std::string &path)
{
    std::string sanitisedPath = path;
    
    // Remove trailing backslashes
    while (!sanitisedPath.empty() && sanitisedPath.back() == '\\')
    {
        sanitisedPath.pop_back();
    }
    // Replace backslashes with forward slashes
    for (char &c : sanitisedPath)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    return sanitisedPath;
}

std::string Utils::getChannelConfig(const std::string &csdFile)
{
    if (auto json = cabbage::File::parseCabbageSection(csdFile))
    {
        if (auto channelConfig = cabbage::Utils::findPropertyInForm<std::string>(*json, "channelConfig"))
        {
            return *channelConfig;
        }
    }
    // Default value if not found or error occurs
    return "2-2";
}

bool Utils::validateChannelConfig(const std::string &channelConfig, int maxInputs, int maxOutputs)
{
    std::istringstream ss(channelConfig);
    std::string pair;
    
    while (ss >> pair)
    {
        size_t dashPos = pair.find('-');
        size_t dotPos = pair.find('.');
        
        int inputs = 0;
        int outputs = 0;
        
        if (dotPos != std::string::npos)
        {
            std::string inputPart = pair.substr(0, dashPos);
            std::string outputPart = pair.substr(dashPos + 1);
            
            inputs = std::stoi(inputPart.substr(0, dotPos)) + std::stoi(inputPart.substr(dotPos + 1));
            outputs = std::stoi(outputPart);
        }
        else
        {
            inputs = std::stoi(pair.substr(0, dashPos));
            outputs = std::stoi(pair.substr(dashPos + 1));
        }
        
        if (inputs > maxInputs || outputs > maxOutputs)
        {
            std::cout << "Error: Channel configuration exceeds the maximum limits. Inputs: " << inputs
            << ", MaxInputs: " << maxInputs << ", Outputs: " << outputs << ", MaxOutputs: " << maxOutputs
            << std::endl;
            return false; // Invalid configuration
        }
    }
    
    return true; // Valid configuration
}

std::string Utils::getJsonWithLineNumbers(const nlohmann::json &j)
{
    std::string json_str = j.dump(4); // 4 is the indent for pretty-printing
    std::istringstream stream(json_str);
    std::string line;
    int line_number = 1;
    std::ostringstream result;
    
    while (std::getline(stream, line))
    {
        result << line_number << ": " << line << "\n";
        line_number++;
    }
    
    return result.str();
}

std::string Utils::getJsonWithLineNumbers(const std::string &json_str)
{
    try
    {
        auto j = nlohmann::json::parse(json_str);
        return getJsonWithLineNumbers(j);
    }
    catch (nlohmann::json::parse_error &e)
    {
        std::stringstream error_output;
        error_output << "Error: Invalid JSON - " << e.what() << "\nOffending JSON:\n";
        std::istringstream json_stream(json_str);
        std::string line;
        int line_number = 1;
        
        while (std::getline(json_stream, line))
        {
            error_output << line_number << ": " << line << "\n";
            line_number++;
        }
        
        return error_output.str();
    }
}

//======================================================================================================
std::string File::getBinaryWithoutExtension()
{
    std::string binaryFileName = getBinaryFileName(); // Get the full filename
    size_t pos = binaryFileName.find_last_of(".");    // Find the last period (.)
    
    if (pos != std::string::npos)
    {
        // Return the substring before the last period
        return binaryFileName.substr(0, pos);
    }
    
    // If there's no period (i.e., no extension), return the full filename
    return binaryFileName;
}

std::string File::getCabbageSection(const std::string &csdFilePath)
{
    
    auto csdFile = (!csdFilePath.empty() && lattice::File::exists(csdFilePath)) ? csdFilePath : getCsdFileAndPath();
    auto csdText = choc::file::loadFileAsString(csdFile);
    
    std::regex cabbageRegex(R"(<Cabbage>([\s\S]*?)</Cabbage>)");
    std::smatch match;
    
    // Search for the content using the regex
    if (std::regex_search(csdText, match, cabbageRegex) && match.size() > 1)
    {
        return match[1].str(); // Return the captured group
    }
    
    return "";
}
// Reads and parses the cabbage section from the file
std::optional<nlohmann::json> File::parseCabbageSection(const std::string &csdFile)
{
    try
    {
        // Get the cabbage section from the file
        const std::string cabbageContents = cabbage::File::getCabbageSection(csdFile);
        
        // Parse the cabbageContents as a JSON object
        return nlohmann::json::parse(cabbageContents);
    }
    catch (const nlohmann::json::parse_error &e)
    {
        // Handle JSON parsing error
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
    
    return std::nullopt; // Return empty optional on failure
}

// Function to get the number of input channels (nchnls_i)
int File::getNumberOfInputChannels(const std::string &csdFile)
{
    
    auto csdFilePath = csdFile.empty() ? getCsdFileAndPath() : csdFile;
    auto input = choc::file::loadFileAsString(csdFilePath);
    
    // Define the regex for inputs (nchnls_i)
    std::regex inputRegex(R"(^\s*nchnls_i\s*=\s*(\d+)\s*$)", std::regex_constants::icase);
    std::smatch match;
    
    // Search for each line individually using regex
    std::istringstream stream(input);
    std::string line;
    
    while (std::getline(stream, line))
    {
        if (std::regex_match(line, match, inputRegex))
        {
            // Convert the matched number to an integer
            return std::stoi(match[1].str());
        }
    }
    
    // return 2 if nchnls_i is not found
    return -1;
}

// Function to get the number of output channels (nchnls)
int File::getNumberOfOutputChannels(const std::string &csdFile)
{
    auto csdFilePath = csdFile.empty() ? getCsdFileAndPath() : csdFile;
    auto input = choc::file::loadFileAsString(csdFilePath);
    
    // Define the regex for outputs (nchnls)
    std::regex outputRegex(R"(^\s*nchnls\s*=\s*(\d+)\s*$)", std::regex_constants::icase);
    std::smatch match;
    
    // Search for each line individually using regex
    std::istringstream stream(input);
    std::string line;
    
    while (std::getline(stream, line))
    {
        if (std::regex_match(line, match, outputRegex))
        {
            // Convert the matched number to an integer
            return std::stoi(match[1].str());
        }
    }
    
    return 2;
}

std::string File::getCabbageResourceDir()
{
    if (lattice::File::usesBundledResources())
        return lattice::File::getResourceDirFromBundle();
    
#if defined(_WIN32)
    return getWindowsProgramDataDir();
#elif defined(__APPLE__)
    return getMacCabbageResourceDir();
#elif defined(__linux__)
    return getLinuxHomeDir() + "/.config/CabbageAudio";
#else
    return "";
#endif
}

std::string File::getCsdPath(const std::string& file)
{
    // If loading resources from plugin bundle..
    if (usesBundledResources())
        return getResourceDirFromBundle();
    
    // Otherwise figure out path to .csd file..
    if (file.empty())
    {
        std::string resourceDir = getResourceDirFromBundle();
        
        std::string binaryFileName = getBinaryFileName();
        size_t pos = binaryFileName.find_last_of(".");
        if (pos != std::string::npos)
            binaryFileName = binaryFileName.substr(0, pos);
        const std::string newPath = joinPath(resourceDir, binaryFileName);
        return newPath;
    }
    else
    {
        std::filesystem::path path(file);
        return path.parent_path().string();
    }
}

std::string File::getCsdFileAndPath(std::string csdFile)
{
    if(lattice::File::exists(csdFile))
        return csdFile;
    
    std::string resourceDir = lattice::File::getResourceDirFromBundle();
    std::string binaryFileName = lattice::File::getBinaryFileName();
    size_t pos = binaryFileName.find_last_of(".");
    
    if (pos != std::string::npos)
        binaryFileName = binaryFileName.substr(0, pos);
    
    if (usesBundledResources())
    {
        const std::string newPath = joinPath(resourceDir, binaryFileName + std::string(".csd"));
        return newPath;
    }
    
    const std::string newPath = joinPath(resourceDir, binaryFileName);
    
    if(!lattice::File::exists(newPath))
    {
        auto fullPath = lattice::File::joinPath(cabbage::File::getCabbageResourceDir(), binaryFileName, binaryFileName + std::string(".csd"));
        return fullPath;
    }
    
    
    return joinPath(newPath, binaryFileName + ".csd");
}

// Function to crudely extract the props object from a corresponding JS file...
// this could be rewritten using ducktapeJS or some other JS parser...
// Note that this won't work with classes that extend other class as the prop
// object might not be found...
nlohmann::json File::extractPropsFromJS(const std::string &jsContent)
{
    std::string propsKey = "this.props =";
    size_t propsPos = jsContent.find(propsKey);
    
    if (propsPos != std::string::npos)
    {
        // Start of the actual props object (after "this.props =")
        size_t start = jsContent.find('{', propsPos);
        if (start == std::string::npos)
        {
            std::cerr << "No opening brace for props found." << std::endl;
            return {};
        }
        
        // Manual brace matching
        int braceCount = 1;
        size_t end = start + 1;
        
        while (end < jsContent.size() && braceCount > 0)
        {
            if (jsContent[end] == '{')
            {
                ++braceCount;
            }
            else if (jsContent[end] == '}')
            {
                --braceCount;
            }
            ++end;
        }
        
        // If we exited and braceCount is not zero, something went wrong
        if (braceCount != 0)
        {
            std::cerr << "Mismatched braces in the props object." << std::endl;
            return {};
        }
        
        // Extract the props object string
        std::string propsString = jsContent.substr(start, end - start);
        
        // Parse the props string into a JSON object using nlohmann::json
        try
        {
            return nlohmann::json::parse(propsString);
        }
        catch (const nlohmann::json::parse_error &e)
        {
            lattice::logInfo << "JSON parse error: " << e.what() << "\nOffending JSON:\n"
            << cabbage::Utils::getJsonWithLineNumbers(propsString);
            return {};
        }
    }
    else
    {
        std::cerr << "No props object found in the JavaScript file." << std::endl;
    }
    
    return {};
}

std::string File::getSettingsFile()
{
    // if in CabbageApp mode, the widget src dir is set by the Cabbage .ini settings
    std::stringstream settingsPath;
#if defined WIN32
    cabbage::Utils::check(false, "fix this");
    /* TCHAR strPath[2048];
     SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath);
     std::string settingsPath = std::string(strPath) + "\\Cabbage\\settings.json";
     return settingsPath;*/
    CHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
    {
        std::stringstream settingsPath;
        settingsPath << path << "\\Cabbage\\settings.json";
        return settingsPath.str();
    }
    
#elif defined __APPLE__
    settingsPath << getenv("HOME") << "/Library/Application Support/Cabbage/settings.json";
    return settingsPath.str();
#else
    cabbage::Utils::check(false, "fix this");
    iniPath.SetFormatted(2048, "%s/.config/%s/", getenv("HOME"), "Cabbage");
    iniPath.Append("settings.json"); // add file name to path
    return iniPath.Get();
#endif
    
    return {};
}

std::string File::getSettingsProperty(const std::string &section, const std::string &key)
{
    // Open the settings file in binary mode
    std::ifstream file(getSettingsFile(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open the file " << getSettingsFile() << std::endl;
        return "";
    }
    
    // Read file contents
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string fileContent = oss.str();
    file.close();
    
    // Parse JSON data
    nlohmann::json jsonData;
    
    try
    {
        jsonData = nlohmann::json::parse(fileContent);
    }
    catch (const nlohmann::json::parse_error &e)
    {
        lattice::logInfo << "Parse error : " << e.what() << " at byte position " << e.byte;
        return "";
    }
    
    // Validate section and key existence
    if (jsonData.contains(section) && jsonData[section].contains(key))
    {
        return jsonData[section][key].get<std::string>();
    }
    
    lattice::logInfo << "Error: Section '" << section << "' or key '" << key << "' not found.";
    return "";
}


std::string File::getCsOptions(const std::string& csdFilePath)
{
    std::ifstream file(csdFilePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + csdFilePath);
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    size_t startPos = content.find("<CsOptions>");
    size_t endPos = content.find("</CsOptions>");
    
    if (startPos == std::string::npos || endPos == std::string::npos)
    {
        return ""; // No CsOptions tag found
    }
    
    startPos += 11; // Move past "<CsOptions>"
    return content.substr(startPos, endPos - startPos);
}

} // end of namespace
