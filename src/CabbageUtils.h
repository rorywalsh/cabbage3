/*
 * Copyright (c) 2024 Rory Walsh
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */


#pragma once

#define cabAssert(exp, msg) assert(((void)msg, exp))

#include <thread>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include "json.hpp"

#if defined(_WIN32)
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#include <windows.h>
#include <Shlobj.h>
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"
#include <winrt/Windows.System.h>
#include <DispatcherQueue.h>
#include <winrt/base.h>  // For winrt::com_ptr
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dlfcn.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

#include <algorithm> // for std::sort



// Function to write detailed log messages
template<typename... Args>
void writeDetailed(const char* file, int line, const char* function, Args&&... args) {
    std::ostringstream oss;
    oss << "Cabbage DEBUG: " << file << " (" << line << ") "
        << function << ": ";

    // Use fold expression to append all arguments to the string stream
    (oss << ... << std::forward<Args>(args)); // C++17 fold expression

    oss << " [Thread ID: " << std::this_thread::get_id() << "]" << std::endl;

    std::cout << oss.str() << std::endl;
}

template<typename... Args>
void writeBasic(Args&&... args) {
    std::ostringstream oss;
    // Use fold expression to append all arguments to the string stream
    (oss << ... << std::forward<Args>(args)); // C++17 fold expression
    std::cout << oss.str() << std::endl;
}
// Variadic macro to simplify calling the writeDetailed function
#define LOG_VERBOSE(...) writeDetailed(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_INFO(...) writeBasic(__VA_ARGS__)

// Function to handle debug output in VS
inline void logToDebug(const std::string& message) {
#ifdef _WIN32
    OutputDebugStringA(message.c_str());
#endif
}

namespace cabbage {


class Utils {
public:
    
    //print JSON with line numbers
    static std::string getJsonWithLineNumbers(const nlohmann::json& j)
    {
        // Get the pretty-printed JSON string
        std::string json_str = j.dump(4);  // 4 is the indent for pretty-printing
        
        // Use a string stream to break the string into lines
        std::istringstream stream(json_str);
        std::string line;
        int line_number = 1;
        
        // Prepare a string to store the result
        std::ostringstream result;
        
        // Add each line with its line number to the result string
        while (std::getline(stream, line))
        {
            result << line_number << ": " << line << "\n";
            line_number++;
        }
        
        // Return the complete string with line numbers
        return result.str();
    }
    
    static std::string getJsonWithLineNumbers(const std::string& json_str) {
        try {
            // Parse the JSON string into a nlohmann::json object
            auto j = nlohmann::json::parse(json_str);
            
            // Reuse the original function to add line numbers (if valid)
            return getJsonWithLineNumbers(j);
        }
        
        catch (nlohmann::json::parse_error& e) {
            // Create a string stream to format the output
            std::stringstream error_output;
            
            // Add the error message from the exception
            error_output << "Error: Invalid JSON - " << e.what() << "\n";
            
            // Add the offending JSON string with line numbers
            error_output << "Offending JSON:\n";
            
            // Split the JSON string by lines and add line numbers
            std::istringstream json_stream(json_str);
            std::string line;
            int line_number = 1;
            
            while (std::getline(json_stream, line)) {
                error_output << line_number << ": " << line << "\n";
                line_number++;
            }
            
            // Return the formatted string (error message + JSON with line numbers)
            return error_output.str();
        }
    }
    
    static std::string toLower(const std::string& str) {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return lowerStr;
    }
};
/*
 String formatter class
 */
class StringFormatter
{
public:
    template <typename... Args>
    static std::string format(const std::string& templateStr, Args&&... args)
    {
        std::vector<std::string> arguments{ toString(std::forward<Args>(args))... };
        return processTemplate(templateStr, arguments);
    }
    
    static void removeBackticks(std::string& str)
    {
        // Use std::remove to move all backticks to the end of the string
        // and return an iterator to the new end of the string.
        auto new_end = std::remove(str.begin(), str.end(), '`');
        // Erase the characters from the new end to the actual end of the string
        str.erase(new_end, str.end());
    }
    
    static std::string getCabbageSectionAsJSEscapedString(const std::string& input)
    {
        // Find the positions of <Cabbage> and </Cabbage>
        size_t start_pos = input.find("<Cabbage>");
        size_t end_pos = input.find("</Cabbage>");
        std::string cabbageSection;
        // Check if both markers are found
        if (start_pos != std::string::npos && end_pos != std::string::npos)
        {
            start_pos = input.find("<Cabbage>");
            end_pos = input.find("</Cabbage>");
            size_t length = end_pos + strlen("</Cabbage>") - start_pos;
            cabbageSection = input.substr(start_pos, length);
        }
        else
        {
            // If markers are not found, return empty string or handle error as needed
            // For simplicity, returning the entire input if markers are not found
            cabbageSection = input;
        }
        std::string sanitisedString = sanitiseString(cabbageSection);
        
        return sanitisedString + "\n";
    }
    
private:
    template <typename T>
    static std::string toString(T&& value)
    {
        std::ostringstream oss;
        oss << std::forward<T>(value);
        return oss.str();
    }
    
    static std::string processTemplate(const std::string& templateStr, const std::vector<std::string>& args)
    {
        std::string result;
        result.reserve(templateStr.size());
        
        size_t argIndex = 0;
        for (size_t i = 0; i < templateStr.size(); ++i) {
            if (templateStr[i] == '<' && i + 1 < templateStr.size() && templateStr[i + 1] == '>' && argIndex < args.size())
            {
                result += args[argIndex++];
                ++i;  // Skip the '>'
            }
            else
            {
                result += templateStr[i];
            }
        }
        
        return result;
    }
    
    static std::string sanitiseString(const std::string& input)
    {
        std::string sanitized;
        sanitized.reserve(input.size() * 2); // Reserve space to avoid frequent reallocations
        
        for (char c : input)
        {
            switch (c) {
                case '\\': sanitized += "\\\\"; break;
                case '\"': sanitized += "\\\""; break;
                case '\r': sanitized += "\\r"; break;
                case '\n': sanitized += "\\n"; break;
                default: sanitized += c; break;
            }
        }
        
        return sanitized;
    }
};


/*
 Utility class to read and write files, query paths, etc.
 */
class File {
public:
    static std::string getBinaryPath()
    {
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
    
    static bool fileExists(const std::string& filePath)
    {
        std::ifstream file(filePath);
        return file.good();
    }
    
    static bool directoryExists(const std::string& dirPath)
    {
#if defined(_WIN32)
        DWORD attrib = GetFileAttributesA(dirPath.c_str());
        return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat info;
        if (stat(dirPath.c_str(), &info) != 0) return false;
        return (info.st_mode & S_IFDIR);
#endif
    }
    
    static std::string getCabbageResourceDir() {
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
    
    // Function to load JavaScript file into a string
    static std::string loadJSFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        std::string jsContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return jsContent;
    }
    
    static std::string getSettingsFile()
    {
        //if in CabbageApp mode, the widget src dir is set by the Cabbage .ini settings
        WDL_String iniPath;
#if defined OS_WIN
        //TCHAR strPath[2048];
        //SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath);
        //iniPath.SetFormatted(2048, "%s\\%s\\", strPath, "Cabbage");
        char strPath[2048];
        SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath);

#elif defined OS_MAC
        iniPath.SetFormatted(2048, "%s/Library/Application Support/%s/", getenv("HOME"), "Cabbage");
#else
#error NOT IMPLEMENTED
#endif
        iniPath.Append("settings.json"); // add file name to path
        return iniPath.Get();
    }
    
    // Function to crudely extract the props object from a corresponding JS file...
    // this could be rewritten using ducktapeJS or some other JS parser...
    // Note that this won't work with classes that extend other class as the prop
    // object might not be found...
    static nlohmann::json extractPropsFromJS(const std::string& jsContent)
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
    
    
    static std::string getBinaryFileName()
    {
        std::string binaryPath = getBinaryPath();
        size_t pos = binaryPath.find_last_of("/\\");
        if (pos != std::string::npos)
            return binaryPath.substr(pos + 1);
        else
            return binaryPath;
    }
    
    static std::string joinPath(const std::string& dirPath, const std::string& fileName) {
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
    
    static std::string getCsdFileAndPath()
    {
        std::string resourceDir = getCabbageResourceDir();
        std::string binaryFileName = getBinaryFileName();
        size_t pos = binaryFileName.find_last_of(".");
        if (pos != std::string::npos)
            binaryFileName = binaryFileName.substr(0, pos);
        const std::string newPath = joinPath(resourceDir, binaryFileName);
        return joinPath(newPath, binaryFileName + ".csd");
    }
    
    static std::string getCsdPath()
    {
        std::string resourceDir = getCabbageResourceDir();
        std::string binaryFileName = getBinaryFileName();
        size_t pos = binaryFileName.find_last_of(".");
        if (pos != std::string::npos)
            binaryFileName = binaryFileName.substr(0, pos);
        const std::string newPath = joinPath(resourceDir, binaryFileName);
        return newPath;
    }
    
    //return a JS escaped string
    static std::string getCabbageSection()
    {
        auto csdText = getFileAsString();
        return StringFormatter::getCabbageSectionAsJSEscapedString(csdText);
    }
    
    //return the file contents, if the file path is not provided, finds
    //the file based on the curren binary name
    static std::string getFileAsString(std::string csdFile = "")
    {
        if (csdFile.empty())
            csdFile = getCsdFileAndPath();
        
        std::ifstream file(csdFile);
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string csdContents = oss.str();
        return csdContents;
    }
    
    static std::string sanitisePath(const std::string& path)
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
    
    static std::vector<std::string> getFilesOfType(const std::string& dirPath, const std::string& fileTypes)
    {
        std::vector<std::string> result;
        
        // Resolve the absolute path based on the current CSD file location
        std::filesystem::path searchPath = cabbage::File::sanitisePath(dirPath);
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
    
    
    static const char* convertToForwardSlashes(const char* path)
    {
        // Find the length of the input path
        size_t length = std::strlen(path);

        // Dynamically allocate memory for the result
        char* result = new char[length + 1];  // +1 for the null terminator

        // Copy the input path into the result buffer
        std::memcpy(result, path, length + 1);

        // Traverse the result buffer and replace '\\' with '/'
        for (size_t i = 0; i < length; ++i) {
            if (result[i] == '\\') {
                result[i] = '/';
            }
        }

        // Return the modified path
        return result;  // Caller must delete[] the result when done
    }
    
    static std::string getFileName(const std::string& absolutePath)
    {
        std::filesystem::path path(absolutePath);
        return path.filename().string();
    }
    
    static std::string getParentDirectory(const std::string& absolutePath)
    {
        try {
            // Create a path object from the input string
            std::filesystem::path filePath(absolutePath);
            
            // Check if the path is absolute
            if (!filePath.is_absolute())
            {
                throw std::invalid_argument("The path provided is not an absolute path.");
            }
            
            // Check if the path exists
            if (!std::filesystem::exists(filePath))
            {
                throw std::runtime_error("The specified path does not exist.");
            }
            
            // Return the parent directory
            std::filesystem::path parentDir = filePath.parent_path();
            
            // If the parent directory is empty (e.g., root directory), handle that case
            if (parentDir.empty())
            {
                throw std::runtime_error("The path does not have a parent directory.");
            }
            
            // Return the parent directory as a string
            return parentDir.string();
            
        }
        catch (const std::exception& ex) {
            // Handle errors and return the error message
            return std::string("Error: ") + ex.what();
        }
    }
    
    static std::string getSettingsProperty(const std::string& section, const std::string& key)
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
private:
#if defined(_WIN32)
    static std::string getWindowsBinaryPath()
    {
        char   DllPath[MAX_PATH] = { 0 };
        GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), DllPath, _countof(DllPath));
        std::string fileName = DllPath;
        LOG_INFO(fileName);
        return std::string(fileName);
    }
    
    static std::string getWindowsProgramDataDir()
    {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path)))
            return std::string(path) + "\\CabbageAudio";
        else
            return "";
    }
#elif defined(__APPLE__)
    static std::string getMacBinaryPath() {
        Dl_info info;
        if (dladdr((void*)"getMacBinaryPath", &info))
        {
            return std::string(info.dli_fname);
        }
        return "";
    }
    
    static std::string getMacCabbageResourceDir()
    {
        const char* homeDir = getenv("HOME");
        if (homeDir)
            return std::string(homeDir) + "/Library/CabbageAudio";
        else {
            struct passwd* pw = getpwuid(getuid());
            if (pw)
                return std::string(pw->pw_dir) + "/Library/CabbageAudio";
            else
                return "";
        }
    }
    
#elif defined(__linux__)
    static std::string getLinuxBinaryPath()
    {
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        return std::string(path, (count > 0) ? count : 0);
    }
    
    static std::string getLinuxHomeDir()
    {
        const char* homeDir = getenv("HOME");
        if (homeDir)
            return std::string(homeDir);
        else
        {
            struct passwd* pw = getpwuid(getuid());
            if (pw)
                return std::string(pw->pw_dir);
            else
                return "";
        }
    }
#endif
};


/*
 Utility class to get widget descriptors from widget JS files
 */
class WidgetDescriptors {
public:
    
    //Utility function to get full list of widget types contained in widgets directory
    static std::vector<std::string> getWidgetTypes()
    {
        std::vector<std::string> widgetTypes;
        std::string widgetPath = cabbage::File::getCsdPath() + "/widgets"; // Folder containing widget files
        
        // Check if the directory exists
        if (!std::filesystem::exists(widgetPath) || !std::filesystem::is_directory(widgetPath))
        {
            std::cerr << "Error: Directory " << widgetPath << " does not exist or is not a directory." << std::endl;
            return widgetTypes;  // Return an empty vector if directory is not found
        }
        
        // Iterate through the directory and extract the filenames without the extension
        for (const auto& entry : std::filesystem::directory_iterator(widgetPath))
        {
            if (entry.is_regular_file())
            {  // Only process regular files
                std::string filename = entry.path().filename().string();  // Get filename
                std::string extension = entry.path().extension().string();
                
                // Remove extension from filename
                if (!extension.empty())
                {
                    filename = filename.substr(0, filename.length() - extension.length());
                }
                
                widgetTypes.push_back(filename);  // Add filename to the vector
            }
        }
        
        return widgetTypes;
    }
    
    //returns a widget descriptor object for a given widget type
    static nlohmann::json get(std::string widgetType)
    {
        
        std::vector<std::string> widgetTypes;
#ifdef CabbageApp
        //this folder will be different for plugins than for the vscode extension
        std::string widgetPath = cabbage::File::getSettingsProperty("currentConfig", "jsSourceDir") + "/cabbage/widgets";;
#else
        std::string widgetPath = cabbage::File::getCsdPath() + "/cabbage/widgets"; // Folder containing widget files
#endif
        
        if(!cabbage::File::directoryExists(widgetPath))
            return {};
        
        auto jsFileContents = cabbage::File::loadJSFile(widgetPath + "/" + widgetType + ".js");
        if (!jsFileContents.empty())
        {
            return cabbage::File::extractPropsFromJS(jsFileContents);
        }
        
        LOG_INFO("Invalid widget type:", widgetType);
        cabAssert(false, "Invalid widget type:");
        return {};
    }
};

} //end of namespace
