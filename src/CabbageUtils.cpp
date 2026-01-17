/*
 * Copyright (c) 2024 Rory Walsh
 *
 * This file is part of Cabbage3
 *
 * Cabbage3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cabbage3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cabbage3.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CabbageUtils.h"
#include <choc/text/choc_Files.h>
#include <sstream>
#include <cstdio>

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
bool File::writeToFile(const std::string &filePath, const std::string &content)
{
    try
    {
        std::ofstream outFile(filePath, std::ios::binary);
        if (!outFile)
        {
            lattice::logDebug << "Failed to open file for writing: " << filePath;
            return false;
        }
        outFile << content;
        outFile.close();
        return true;
    }
    catch (const std::exception &e)
    {
        lattice::logDebug << "Exception writing to file " << filePath << ": " << e.what();
        return false;
    }
}

void File::writeToFileAsync(const std::string &filePath, const std::string &content)
{
    // Launch detached thread for true fire-and-forget (no blocking on destruction)
    std::thread([filePath, content]() {
        writeToFile(filePath, content);
    }).detach();
}

std::string File::readFromFile(const std::string &filePath)
{
    try
    {
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile)
        {
            lattice::logDebug << "Failed to open file for reading: " << filePath;
            return "";
        }
        std::ostringstream ss;
        ss << inFile.rdbuf();
        inFile.close();
        return ss.str();
    }
    catch (const std::exception &e)
    {
        lattice::logDebug << "Exception reading from file " << filePath << ": " << e.what();
        return "";
    }
}

void File::readFromFileAsync(const std::string &filePath, std::function<void(const std::string&)> callback)
{
    // Static vector to keep futures alive (prevents immediate blocking)
    static std::vector<std::future<void>> activeFutures;
    
    // Clean up completed futures to avoid unbounded growth
    activeFutures.erase(
        std::remove_if(activeFutures.begin(), activeFutures.end(),
            [](std::future<void>& f) {
                return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        activeFutures.end()
    );
    
    // Launch async with explicit policy to ensure new thread
    activeFutures.push_back(std::async(std::launch::async, [filePath, callback]() {
        std::string content = readFromFile(filePath);
        if (callback) {
            callback(content);
        }
    }));
}

//======================================================================================================
std::string File::getBinaryWithoutExtension()
{
    std::string binaryFileName = getBinaryFileName(); // Full path with extension

    size_t pos = binaryFileName.find_last_of(".");
    std::string withoutExtension = (pos != std::string::npos) ? binaryFileName.substr(0, pos) : binaryFileName;

    // Check if the file without extension exists
    std::ifstream f(withoutExtension);
    if (f.good())
    {
        return withoutExtension;
    }

    // If not, and it ends with 'd', try removing the 'd' and check again
    if (!withoutExtension.empty() && withoutExtension.back() == 'd')
    {
        std::string stripped = withoutExtension.substr(0, withoutExtension.length() - 1);
        std::ifstream f2(stripped);
        if (f2.good())
        {
            return stripped;
        }
    }

    // Fallback: return original version
    return withoutExtension;
}

std::string File::findCabbageJSWidgetPath()
{
    std::string widgetPath;

#if defined(CabbageApp) || defined(CabbageTests)
    // Primary: Get path(s) from settings. jsSourceDir may be a string or an array of strings.
    try
    {
        std::ifstream file(getSettingsFile(), std::ios::binary);
        if (file.is_open())
        {
            std::ostringstream oss; oss << file.rdbuf(); file.close();
            auto jsonData = nlohmann::json::parse(oss.str());
            std::vector<std::string> dirs;
            if (jsonData.contains("currentConfig") && jsonData["currentConfig"].contains("jsSourceDir"))
            {
                auto &val = jsonData["currentConfig"]["jsSourceDir"];
                if (val.is_array())
                {
                    for (auto &v : val)
                        if (v.is_string()) dirs.emplace_back(v.get<std::string>());
                }
                else if (val.is_string())
                {
                    dirs.emplace_back(val.get<std::string>());
                }
            }

            // Iterate all configured dirs and return the first existing widgets path
            for (const auto &baseDir : dirs)
            {
                std::string candidate = lattice::File::joinPath(baseDir, "cabbage", "widgets");
                if (cabbage::File::directoryExists(candidate))
                    return candidate;
            }
        }
    }
    catch (const std::exception &e)
    {
        lattice::logDebug << "Error reading jsSourceDir from settings: " << e.what();
    }
    // Fallback to extension source directory if settings not available
#else
    const auto resourceDir = lattice::File::getParentDirectory(cabbage::File::getCsdFileAndPath());
    widgetPath = lattice::File::joinPath(resourceDir, "cabbage", "widgets");
#endif

    if (cabbage::File::directoryExists(widgetPath))
        return widgetPath;

    // Try fallback in VSCode extensions
    std::vector<std::string> vscodePaths;

#if defined(_WIN32)
    const char *homeDrive = std::getenv("HOMEDRIVE");
    const char *homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath)
        vscodePaths.emplace_back(std::string(homeDrive) + homePath + "\\.vscode\\extensions");
#elif defined(__APPLE__) || defined(__linux__)
    const char *home = std::getenv("HOME");
    if (home)
        vscodePaths.emplace_back(std::string(home) + "/.vscode/extensions");
#endif

    for (const auto &path : vscodePaths)
    {
        if (!std::filesystem::exists(path))
            continue;

        std::regex cabbagePattern(R"(cabbageaudio\.vscabbage-[^/\\]+)");
        for (const auto &dirEntry : std::filesystem::directory_iterator(path))
        {
            const auto &dirPath = dirEntry.path();
            if (dirEntry.is_directory())
            {
                const std::string dirname = dirPath.filename().string();
                if (std::regex_match(dirname, cabbagePattern))
                {
                    std::filesystem::path candidate = dirPath / "src" / "cabbage" / "widgets";
                    if (std::filesystem::exists(candidate))
                    {
                        return candidate.string();
                    }
                }
            }
        }
    }

    // If all else fails
    lattice::logDebug << "Could not locate widget JS files in settings or fallback path.";
    return {};
}

std::string File::getCabbageSection(const std::string &csdFilePath)
{
    auto csdFile = (!csdFilePath.empty() && lattice::File::exists(csdFilePath)) ? csdFilePath : getCsdFileAndPath();
    std::string csdText = {};
    
    try {
        // Attempt to load the file as a string
        csdText = choc::file::loadFileAsString(csdFile);
    }
    catch (const choc::file::Error& e) {
        lattice::logDebug << "Couldn't parse "<< csdFile << " file for text";
        return "";
    }
    
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
    
    std::string input = {};
    auto csdFilePath = csdFile.empty() ? getCsdFileAndPath() : csdFile;
    
    try {
        // Attempt to load the file as a string
        input = choc::file::loadFileAsString(csdFile);
    }
    catch (const choc::file::Error& e) {
        lattice::logDebug << "Couldn't parse " << csdFilePath << " file for text";
        return 2;
    }
    
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
    
    // return number of output channels if nchnls_i is not found
    return getNumberOfOutputChannels(csdFile);
}

// Function to get the number of output channels (nchnls)
int File::getNumberOfOutputChannels(const std::string &csdFile)
{
    auto csdFilePath = csdFile.empty() ? getCsdFileAndPath() : csdFile;
    std::string input = {};
    
    try {
        // Attempt to load the file as a string
        input = choc::file::loadFileAsString(csdFilePath);
    }
    catch (const choc::file::Error& e) {
        lattice::logDebug << "Couldn't parse " << csdFilePath << " file for text";
        return 2;
    }
    
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
    return getLinuxHomeDir() + "/.config/" + std::string(LATTICE_MANUFACTURER_NAME);
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
    std::stringstream settingsPath;
#if defined WIN32
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
