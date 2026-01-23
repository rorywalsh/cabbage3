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
#include "CabbagePluginInfo.h"
#include <unordered_map>
#include <mutex>
#include <choc/text/choc_Files.h>
#include <sstream>
#include <cstdio>
#ifdef CabbagePro
#include "encrypt.h"
#include <choc/containers/choc_ZipFile.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ctime>
#endif

namespace cabbage {

std::string File::getManufacturerName()
{
    return LATTICE_MANUFACTURER_NAME;
}

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

    // Prevent crash during static initialization by checking if file exists
    if (!lattice::File::exists(csdFile))
    {
        lattice::logDebug << "CSD file does not exist: " << csdFile;
        return "";
    }

#ifdef CabbagePro
    // Pro version: Check if file is encrypted
    if (Decrypt::isEncrypted(csdFile))
    {
        try {
            csdText = Decrypt::getCsdText(csdFile);
            lattice::logDebug << "Successfully decrypted CSD file for Cabbage section extraction";
        }
        catch (const std::exception& e) {
            lattice::logError << "Failed to decrypt CSD file: " << e.what();
            return "";
        }
    }
    else
    {
        // Regular unencrypted CSD file
        try {
            csdText = choc::file::loadFileAsString(csdFile);
        }
        catch (const choc::file::Error& e) {
            lattice::logDebug << "Couldn't parse "<< csdFile << " file for text";
            return "";
        }
    }
#else
    // Free version: Only handle unencrypted files
    try {
        csdText = choc::file::loadFileAsString(csdFile);
    }
    catch (const choc::file::Error& e) {
        lattice::logDebug << "Couldn't parse "<< csdFile << " file for text";
        return "";
    }
#endif
    
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

#ifdef CabbagePro
    // Pro version: Check if file is encrypted
    if (Decrypt::isEncrypted(csdFilePath))
    {
        try {
            input = Decrypt::getCsdText(csdFilePath);
        }
        catch (const std::exception& e) {
            lattice::logError << "Failed to decrypt CSD file: " << e.what();
            return 2;
        }
    }
    else
    {
        try {
            input = choc::file::loadFileAsString(csdFilePath);
        }
        catch (const choc::file::Error& e) {
            lattice::logDebug << "Couldn't parse " << csdFilePath << " file for text";
            return 2;
        }
    }
#else
    try {
        input = choc::file::loadFileAsString(csdFilePath);
    }
    catch (const choc::file::Error& e) {
        lattice::logDebug << "Couldn't parse " << csdFilePath << " file for text";
        return 2;
    }
#endif
    
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

#ifdef CabbagePro
    // Pro version: Check if file is encrypted
    if (Decrypt::isEncrypted(csdFilePath))
    {
        try {
            input = Decrypt::getCsdText(csdFilePath);
        }
        catch (const std::exception& e) {
            lattice::logError << "Failed to decrypt CSD file: " << e.what();
            return 2;
        }
    }
    else
    {
        try {
            input = choc::file::loadFileAsString(csdFilePath);
        }
        catch (const choc::file::Error& e) {
            lattice::logDebug << "Couldn't parse " << csdFilePath << " file for text";
            return 2;
        }
    }
#else
    try {
        input = choc::file::loadFileAsString(csdFilePath);
    }
    catch (const choc::file::Error& e) {
        lattice::logDebug << "Couldn't parse " << csdFilePath << " file for text";
        return 2;
    }
#endif
    
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
    return getLinuxHomeDir() + "/.config/" + getManufacturerName();
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

// Static cache for CSD file paths - keyed by binary name to support multiple plugin types
// Each plugin binary (e.g., different .vst3 files) gets its own cached CSD path and temp dir data
namespace {
    // Struct to hold all cached data for a plugin binary
    struct CachedPluginData {
        std::string csdPath;           // The cached CSD file path
        std::string tempDir;           // Temp directory path (empty if no extraction)
        int tempDirRefCount;           // Reference count for temp dir (0 if tempDir.empty())
    };
    
    // Cache struct to hold the map and mutex
    struct Cache {
        std::unordered_map<std::string, CachedPluginData> data;
        std::mutex mutex;
    };
    
    // Function to get the cache, ensuring lazy initialization
    Cache& getCache() {
        static Cache cache;
        static bool useMutex = false;
        useMutex = true; // Set after cache is initialized
        return cache;
    }
    
    // Helper to get lock if safe
    std::unique_lock<std::mutex> getLock() {
        static bool useMutex = false;
        if (useMutex) {
            return std::unique_lock<std::mutex>(getCache().mutex);
        }
        return std::unique_lock<std::mutex>(); // Empty lock
    }
}

void File::setCsdFileAndPath(const std::string& csdFile)
{
    auto lock = getLock(); // Thread-safe access to global cache
    std::string binaryName = lattice::File::getBinaryFileName(); // Get unique key for this plugin binary
    
    if(!csdFile.empty() && lattice::File::exists(csdFile))
    {
        // Get or create the cached data for this binary
        auto& data = getCache().data[binaryName];
        data.csdPath = csdFile; // Cache CSD path
        lattice::logInfo << "setCsdFileAndPath: Set CSD path for " << binaryName << " to: " << csdFile;
    }
    else
    {
        lattice::logWarning << "setCsdFileAndPath: Path does not exist or is empty: " << csdFile;
    }
}

std::pair<std::string, std::string> File::setupRootDirectory(const std::string& csdFile)
{
    std::string finalCsdPath;
    std::string rootPath;

    // Determine the final CSD path
    if (!csdFile.empty() && lattice::File::exists(csdFile))
    {
        finalCsdPath = csdFile;
    }
    else
    {
        // Use default path lookup
        finalCsdPath = cabbage::File::getCsdFileAndPath();
    }

    // Set the initial CSD path
    setCsdFileAndPath(finalCsdPath);

#ifndef CabbagePro
    // Non-Pro mode: root path is parent of CSD file
    rootPath = cabbage::File::getParentDirectory(finalCsdPath);
    return {rootPath, ""};
#else
    // Pro mode: root path is parent of parent of CSD file
    rootPath = cabbage::File::getParentDirectory(cabbage::File::getParentDirectory(finalCsdPath));
    
    // Check if this binary already has an extracted temp directory cached
    std::string binaryName = lattice::File::getBinaryFileName();
    std::string cabzTempDir;
    
    {
        auto lock = getLock();
        auto it = getCache().data.find(binaryName);
        if (it != getCache().data.end() && !it->second.tempDir.empty() && it->second.tempDirRefCount >= 0)
        {
            // Already extracted for this binary, reuse without incrementing here
            cabzTempDir = it->second.tempDir;
            lattice::logInfo << "Reusing existing temp dir for " << binaryName << ": " << cabzTempDir
                           << " (current ref count: " << it->second.tempDirRefCount << ")";
        }
    }

    if (!cabzTempDir.empty())
    {
        // Use cached temp directory
        finalCsdPath = cabzTempDir + "/" + cabbage::File::getBinaryWithoutExtension() + ".ecsd";
        setCsdFileAndPath(finalCsdPath);
        return {cabzTempDir, cabzTempDir}; // Return temp dir as both root and temp dir
    }
    
    // First time extraction for this binary
    cabzTempDir = cabbage::File::extractCabzArchive(rootPath);
    
    if (!cabzTempDir.empty())
    {
        // Log extracted files for debugging
        auto files = lattice::File::getFilesOfType(cabzTempDir, "*");
        for (auto& f : files)
        {
            lattice::logInfo << "File: " << f;
        }

        // Store temp dir and set initial reference count
        {
            auto lock = getLock();
            auto& data = getCache().data[binaryName];
            data.tempDir = cabzTempDir;
            data.tempDirRefCount = 0; // Will be incremented by CabbageProcessor constructor
            lattice::logInfo << "Set temp dir for " << binaryName << " to: " << cabzTempDir
                           << " (initial ref count: " << data.tempDirRefCount << ")";
        }

        // Override with the encrypted CSD file from the cabz archive
        finalCsdPath = cabzTempDir + "/" + cabbage::File::getBinaryWithoutExtension() + ".ecsd";
        setCsdFileAndPath(finalCsdPath);

        return {cabzTempDir, cabzTempDir}; // Return temp dir as both root and temp dir
    }
    else
    {
        // No cabz archive found, return the calculated root path
        return {rootPath, ""}; // No temp dir used
    }
#endif
}

std::string File::getCsdFileAndPath(std::string csdFile)
{
    // DESIGN: Single Source of Truth for CSD Path per Plugin Binary
    // ============================================
    // The CSD file path is cached ONCE per plugin binary in setCsdFileAndPath().
    // All subsequent calls for the same binary use the cached value.
    // This ensures cabz temp directories work correctly and supports multiple plugin types.

    auto lock = getLock(); // Thread-safe access
    std::string binaryName = lattice::File::getBinaryFileName(); // Get unique key for this plugin binary

    // If csdFile is provided and exists, return it (but don't cache - use setCsdFileAndPath for that)
    if(!csdFile.empty() && lattice::File::exists(csdFile))
    {
        return csdFile;
    }

    // Use the cached path for this binary
    auto it = getCache().data.find(binaryName);
    if(it != getCache().data.end() && !it->second.csdPath.empty() && lattice::File::exists(it->second.csdPath))
    {
        lattice::logDebug << "getCsdFileAndPath: Using cached CSD path for " << binaryName << ": " << it->second.csdPath;
        return it->second.csdPath;
    }

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
    std::string content;

#ifdef CabbagePro
    // Pro version: Check if file is encrypted
    if (Decrypt::isEncrypted(csdFilePath))
    {
        try {
            content = Decrypt::getCsdText(csdFilePath);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to decrypt CSD file: " + std::string(e.what()));
        }
    }
    else
    {
        std::ifstream file(csdFilePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + csdFilePath);
        }
        content = std::string((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    }
#else
    std::ifstream file(csdFilePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + csdFilePath);
    }
    content = std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
#endif
    
    size_t startPos = content.find("<CsOptions>");
    size_t endPos = content.find("</CsOptions>");
    
    if (startPos == std::string::npos || endPos == std::string::npos)
    {
        return ""; // No CsOptions tag found
    }
    
    startPos += 11; // Move past "<CsOptions>"
    return content.substr(startPos, endPos - startPos);
}

#ifdef CabbagePro
std::string File::extractCabzArchive(const std::string &resourceDir)
{
    // Look for .cabz file in the resource directory
    std::string cabzPath;
    std::string binaryName = lattice::File::getBinaryFileName();
    size_t pos = binaryName.find_last_of(".");
    if (pos != std::string::npos)
        binaryName = binaryName.substr(0, pos);

    cabzPath = lattice::File::joinPath(resourceDir, binaryName + ".cabz");

    if (!lattice::File::exists(cabzPath))
    {
        lattice::logDebug << "No .cabz archive found at: " << cabzPath;
        return ""; // No archive, use regular resources
    }

    lattice::logInfo << "Found .cabz archive: " << cabzPath;

    try
    {
        // Read encrypted archive
        std::ifstream file(cabzPath, std::ios::binary);
        if (!file.is_open())
        {
            lattice::logError << "Failed to open .cabz file: " << cabzPath;
            return "";
        }

        // Read entire file
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> encryptedData(fileSize);
        file.read(reinterpret_cast<char*>(encryptedData.data()), fileSize);
        file.close();

        lattice::logInfo << "Read encrypted archive: " << fileSize << " bytes";

        // Decrypt the archive using the company-wide encryption key
        // NOTE: All Pro plugins use the same encryption key for simplicity.
        // This avoids the chicken-and-egg problem of needing to decrypt files to find the key.
        lattice::logInfo << "Decrypting archive...";
        std::vector<uint8_t> zipData = Decrypt::decryptData(encryptedData);
        lattice::logInfo << "Decrypted archive: " << zipData.size() << " bytes";

        // Create temp directory
        std::string tempDir = std::filesystem::temp_directory_path().string();
        tempDir = lattice::File::joinPath(tempDir, "cabbage_" + binaryName + "_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(tempDir);
        lattice::logInfo << "Created temp directory: " << tempDir;

        // Create an input stream from decrypted zip data
        auto zipStream = std::make_shared<std::istringstream>(
            std::string(reinterpret_cast<const char*>(zipData.data()), zipData.size()),
            std::ios::binary
        );

        // Extract zip archive using choc
        choc::zip::ZipFile archive(zipStream);
        lattice::logInfo << "Found " << archive.items.size() << " files in archive";

        // Extract all files
        bool success = archive.uncompressToFolder(tempDir, true, false);
        if (!success)
        {
            lattice::logError << "Failed to extract some files from archive";
            cleanupCabzTempDir(tempDir);
            return "";
        }

        lattice::logInfo << "Successfully extracted .cabz archive to: " << tempDir;
        return tempDir;
    }
    catch (const std::exception& e)
    {
        lattice::logError << "Failed to extract .cabz archive: " << e.what();
        return "";
    }
}

void File::cleanupCabzTempDir(const std::string &tempDir)
{
    if (tempDir.empty() || tempDir.find("cabbage_") == std::string::npos)
        return;

    try
    {
        std::filesystem::remove_all(tempDir);
        lattice::logInfo << "Cleaned up temp directory: " << tempDir;
    }
    catch (const std::exception& e)
    {
        lattice::logError << "Failed to cleanup temp directory: " << e.what();
    }
}

// Decrement reference count for temp directory and clean up if no more references
// Called by CabbageProcessor destructor to ensure temp dirs are cleaned up when last instance is destroyed
void File::decrementTempDirRef(const std::string& tempDir)
{
    if (tempDir.empty())
        return;

    auto lock = getLock(); // Thread-safe access
    
    // Find the binary that has this temp dir
    for (auto& pair : getCache().data)
    {
        auto& data = pair.second;
        if (data.tempDir == tempDir)
        {
            data.tempDirRefCount--; // Decrement reference count
            lattice::logInfo << "Decremented ref count for temp dir: " << tempDir 
                           << " (binary: " << pair.first << ") to " << data.tempDirRefCount;
            
            if (data.tempDirRefCount <= 0)
            {
                // No more references, safe to clean up
                std::string tempDirToClean = data.tempDir;
                data.tempDir.clear(); // Clear the temp dir path
                data.tempDirRefCount = 0; // Reset count
                cleanupCabzTempDir(tempDirToClean); // Actually remove the directory
            }
            return; // Found and handled
        }
    }
    
    lattice::logWarning << "Attempted to decrement ref count for unknown temp dir: " << tempDir;
}

// Increment reference count for temp directory
// Called by CabbageProcessor constructor after setupRootDirectory
void File::incrementTempDirRef(const std::string& tempDir)
{
    if (tempDir.empty())
        return;

    auto lock = getLock(); // Thread-safe access
    
    // Find the binary that has this temp dir
    for (auto& pair : getCache().data)
    {
        auto& data = pair.second;
        if (data.tempDir == tempDir)
        {
            data.tempDirRefCount++; // Increment reference count
            lattice::logInfo << "Incremented ref count for temp dir: " << tempDir 
                           << " (binary: " << pair.first << ") to " << data.tempDirRefCount;
            return; // Found and handled
        }
    }
    
    lattice::logWarning << "Attempted to increment ref count for unknown temp dir: " << tempDir;
}
#else
// Stub implementations for non-Pro builds
std::string File::extractCabzArchive(const std::string &)
{
    return "";
}

void File::cleanupCabzTempDir(const std::string &)
{
}
#endif

} // end of namespace
