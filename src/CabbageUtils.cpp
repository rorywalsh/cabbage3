// src/CabbageUtils.cpp
#include "CabbageUtils.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace cabbage {

std::string Utils::sanitisePath(const std::string& path) {
    std::string sanitisedPath = path;
    // Remove trailing backslashes
    while (!sanitisedPath.empty() && sanitisedPath.back() == '\\') {
        sanitisedPath.pop_back();
    }
    // Replace backslashes with forward slashes
    for (char& c : sanitisedPath) {
        if (c == '\\') {
            c = '/';
        }
    }
    return sanitisedPath;
}

bool Utils::validateChannelConfig(const std::string& channelConfig, int maxInputs, int maxOutputs) {
    std::istringstream ss(channelConfig);
    std::string pair;

    while (ss >> pair) {
        size_t dashPos = pair.find('-');
        size_t dotPos = pair.find('.');

        int inputs = 0;
        int outputs = 0;

        if (dotPos != std::string::npos) {
            std::string inputPart = pair.substr(0, dashPos);
            std::string outputPart = pair.substr(dashPos + 1);

            inputs = std::stoi(inputPart.substr(0, dotPos)) + std::stoi(inputPart.substr(dotPos + 1));
            outputs = std::stoi(outputPart);
        } else {
            inputs = std::stoi(pair.substr(0, dashPos));
            outputs = std::stoi(pair.substr(dashPos + 1));
        }

        if (inputs > maxInputs || outputs > maxOutputs) {
            std::cout << "Error: Channel configuration exceeds the maximum limits. Inputs: " 
                      << inputs << ", MaxInputs: " << maxInputs 
                      << ", Outputs: " << outputs << ", MaxOutputs: " << maxOutputs << std::endl;
            return false;  // Invalid configuration
        }
    }

    return true;  // Valid configuration
}

std::string Utils::getJsonWithLineNumbers(const nlohmann::json& j) {
    std::string json_str = j.dump(4);  // 4 is the indent for pretty-printing
    std::istringstream stream(json_str);
    std::string line;
    int line_number = 1;
    std::ostringstream result;

    while (std::getline(stream, line)) {
        result << line_number << ": " << line << "\n";
        line_number++;
    }

    return result.str();
}

std::string Utils::getJsonWithLineNumbers(const std::string& json_str) {
    try {
        auto j = nlohmann::json::parse(json_str);
        return getJsonWithLineNumbers(j);
    } catch (nlohmann::json::parse_error& e) {
        std::stringstream error_output;
        error_output << "Error: Invalid JSON - " << e.what() << "\nOffending JSON:\n";
        std::istringstream json_stream(json_str);
        std::string line;
        int line_number = 1;

        while (std::getline(json_stream, line)) {
            error_output << line_number << ": " << line << "\n";
            line_number++;
        }

        return error_output.str();
    }
}

std::string Utils::toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

std::string Utils::getChannelConfig(const std::string& csdFile) {
    if (auto json = cabbage::File::parseCabbageSection(csdFile)) {
        if (auto channelConfig = cabbage::Utils::findPropertyInForm<std::string>(*json, "channelConfig")) {
            return *channelConfig;
        }
    }
    // Default value if not found or error occurs
    return "2-2";
}

bool Utils::getEnableDevTools(const std::string& csdFile) {
    if (auto json = cabbage::File::parseCabbageSection(csdFile)) {
        if (auto enableDevTools = cabbage::Utils::findPropertyInForm<bool>(*json, "enableDevTools")) {
            return *enableDevTools;
        }
    }
    // Default value if not found or error occurs
    return true;
}

void StringFormatter::removeBackticks(std::string& str) {
    auto new_end = std::remove(str.begin(), str.end(), '`');
    str.erase(new_end, str.end());
}


std::string File::getBinaryPath() {
#if defined(_WIN32)
        return getWindowsBinaryPath();
#elif defined(__APPLE__)
        return getMacBinaryPath();
#elif defined(__linux__)
        return getLinuxBinaryPath();
#else
        return "";
#endif
}

bool File::fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}

bool File::directoryExists(const std::string& dirPath) {
#if defined(_WIN32)
    DWORD attrib = GetFileAttributesA(dirPath.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat info;
    if (stat(dirPath.c_str(), &info) != 0) return false;
    return (info.st_mode & S_IFDIR);
#endif
}

std::string File::getCabbageResourceDir() {
#if defined(_WIN32)
        return getWindowsProgramDataDir();
#elif defined(__APPLE__)
        return getMacCabbageResourceDir();
#elif defined(__linux__)
        return getLinuxHomeDir() + "/CabbageAudio";
#else
        return "";
#endif
}

std::string File::loadJSFile(const std::string& filePath) {
        std::ifstream file(filePath);
        std::string jsContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return jsContent;
}

std::string File::getCabbageSection(const std::string& csdFile) {
    auto csdText = getFileAsString();
    std::regex cabbageRegex(R"(<Cabbage>([\s\S]*?)</Cabbage>)");
    std::smatch match;

    // Search for the content using the regex
    if (std::regex_search(csdText, match, cabbageRegex) && match.size() > 1) {
        return match[1].str(); // Return the captured group
    }
    
    return "";
}

std::string File::getCsdWithoutExtension()
{
    std::string binaryFileName = getBinaryFileName(); // Get the full filename
    size_t pos = binaryFileName.find_last_of("."); // Find the last period (.)
    
    if (pos != std::string::npos)
    {
        // Return the substring before the last period
        return binaryFileName.substr(0, pos);
    }
    
    // If there's no period (i.e., no extension), return the full filename
    return binaryFileName;
}

std::string File::getSettingsFile()
{
    //if in CabbageApp mode, the widget src dir is set by the Cabbage .ini settings
    WDL_String iniPath;
#if defined WIN32
   /* TCHAR strPath[2048];
    SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath);
    std::string settingsPath = std::string(strPath) + "\\Cabbage\\settings.json";
    return settingsPath;*/
    CHAR strPath[256];
    SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath);
    iniPath.SetFormatted(256, "%s\\%s\\", strPath, "Cabbage");
    iniPath.Append("settings.json"); // add file name to path
    return iniPath.Get();

#elif defined __APPLE__
    iniPath.SetFormatted(2048, "%s/Library/Application Support/%s/", getenv("HOME"), "Cabbage");
    iniPath.Append("settings.json"); // add file name to path
    return iniPath.Get();
#else
#error NOT IMPLEMENTED
#endif
    
}

std::string File::getSettingsProperty(const std::string& section, const std::string& key)
{
    // Open the settings file
    std::ifstream file(getSettingsFile());
    if (!file.is_open())
    {
        //std::cerr << "Error: Could not open the file " << getSettingsFile() << std::endl;
        return "";
    }
    
    // Parse the JSON content from the file
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse JSON - " << e.what() << std::endl;
        return "";
    }
    
    // Check if the section exists
    if (jsonData.contains(section))
    {
        // Get the section object
        nlohmann::json sectionObj = jsonData[section];
        
        // Check if the key exists within the section
        if (sectionObj.contains(key))
        {
            try {
                // Return the value as a string
                return sectionObj[key].get<std::string>();
            }
            catch (const std::exception& e) {
                std::cerr << "Error: Failed to retrieve the key '" << key << "' as a string - " << e.what() << std::endl;
                return "";
            }
        }
        else
        {
            std::cerr << "Error: Key '" << key << "' not found in section '" << section << "'." << std::endl;
        }
    }
    else
    {
        std::cerr << "Error: Section '" << section << "' not found in the JSON file." << std::endl;
    }
    
    return "";
}
//===========================================================================================
std::string File::getFileAsString(std::string csdFile) {
    if (csdFile.empty())
        csdFile = getCsdFileAndPath();
        
        std::ifstream file(csdFile);
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string csdContents = oss.str();
        return csdContents;
}

std::string File::getBinaryFileName()
{
    std::string binaryPath = getBinaryPath();
    size_t pos = binaryPath.find_last_of("/\\");
    if (pos != std::string::npos)
        return binaryPath.substr(pos + 1);
    else
        return binaryPath;
}

std::string File::formatPath(const std::string& path)
{
    std::string sanitizedPath = path;
    
    // Remove trailing backslashes
    while (!sanitizedPath.empty() && sanitizedPath.back() == '\\')
    {
        sanitizedPath.pop_back();
    }
    
    // Replace backslashes with forward slashes
    for (char& c : sanitizedPath)
    {
        if (c == '\\') {
            c = '/';
        }
    }
    
    return sanitizedPath;
}


std::string File::getCsdFileAndPath()
{
    std::string resourceDir = getCabbageResourceDir();
    std::string binaryFileName = getBinaryFileName();
    size_t pos = binaryFileName.find_last_of(".");
    if (pos != std::string::npos)
        binaryFileName = binaryFileName.substr(0, pos);
    const std::string newPath = joinPath(resourceDir, binaryFileName);
    return joinPath(newPath, binaryFileName + ".csd");
}

// Reads and parses the cabbage section from the file
std::optional<nlohmann::json> File::parseCabbageSection(const std::string& csdFile)
{
    try {
        // Get the cabbage section from the file
        const std::string cabbageContents = cabbage::File::getCabbageSection(csdFile);

        // Parse the cabbageContents as a JSON object
        return nlohmann::json::parse(cabbageContents);
    } catch (const nlohmann::json::parse_error& e) {
        // Handle JSON parsing error
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
    return std::nullopt; // Return empty optional on failure
}

// Function to get the number of input channels (nchnls_i)
int File::getNumberOfInputChannels(const std::string& csdFile)
{
    auto input = getFileAsString(csdFile);

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
int File::getNumberOfOutputChannels(const std::string& csdFile)
{
    auto input = getFileAsString(csdFile);
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

std::vector<std::string> File::getFilesOfType(const std::string& dirPath, const std::string& fileTypes)
{
    std::vector<std::string> result;
    
    // Resolve the absolute path based on the current CSD file location
    std::filesystem::path searchPath = cabbage::File::formatPath(dirPath);
    if (searchPath.is_relative())
    {
        std::string csdFilePath = getCsdFileAndPath();
        std::filesystem::path csdDirPath = std::filesystem::path(csdFilePath).parent_path();
        searchPath = csdDirPath / searchPath;
    }
    
    // Normalize the path to remove any redundant elements
    searchPath = std::filesystem::canonical(searchPath);
    
    // Split the fileTypes string into individual patterns
    std::vector<std::string> patterns;
    std::stringstream ss(fileTypes);
    std::string pattern;
    while (std::getline(ss, pattern, ';'))
    {
        patterns.push_back(pattern);
    }
    
    // Iterate over the directory and match the patterns
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath))
    {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            for (const auto& p : patterns) {
                if (std::filesystem::path(filePath).filename().string().find(p.substr(1)) != std::string::npos)
                {
                    result.push_back(filePath);
                    break;
                }
            }
        }
    }
    
    std::sort(result.begin(), result.end(), [](const std::string& a, const std::string& b)
              {
        // Extract filenames without extensions
        std::string fileNameA = std::filesystem::path(a).filename().stem().string();
        std::string fileNameB = std::filesystem::path(b).filename().stem().string();
        
        // Convert filenames to integers if possible
        auto convertToInt = [](const std::string& s) -> int {
            try
            {
                return std::stoi(s);
            }
            catch (...)
            {
                return 0; // Return 0 if conversion fails
            }
        };
        
        int numA = convertToInt(fileNameA);
        int numB = convertToInt(fileNameB);
        
        // Compare numeric parts if both filenames are numeric, otherwise use lexicographical comparison
        if (numA != 0 && numB != 0)
        {
            return numA < numB;
        }
        else
        {
            return fileNameA < fileNameB;
        }
    });
    
    return result;
}

// Function to crudely extract the props object from a corresponding JS file...
// this could be rewritten using ducktapeJS or some other JS parser...
// Note that this won't work with classes that extend other class as the prop
// object might not be found...
nlohmann::json File::extractPropsFromJS(const std::string& jsContent)
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
        catch (const nlohmann::json::parse_error& e)
        {
            LOG_INFO("JSON parse error: ", e.what(), "\nOffending JSON:\n", cabbage::Utils::getJsonWithLineNumbers(propsString));
            return {};
        }
    }
    else
    {
        std::cerr << "No props object found in the JavaScript file." << std::endl;
    }
    
    return {};
}

std::string File::getCsdPath(const std::string file)
{
    if(file.empty())
    {
        std::string resourceDir = getCabbageResourceDir();
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

std::string File::joinPath(const std::string& dirPath, const std::string& fileName) {
    if (dirPath.empty())
        return fileName;
    else if (fileName.empty())
        return dirPath;
    else
    {
        char separator =
#if defined(_WIN32)
        '\\';
#else
        '/';
#endif
        if (dirPath.back() == separator || fileName.front() == separator)
            return dirPath + fileName;
        else
            return dirPath + separator + fileName;
    }
}

} // namespace cabbage
