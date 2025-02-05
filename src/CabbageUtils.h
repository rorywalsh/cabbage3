/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */


#pragma once

#define cabAssert(exp, msg) assert(((void)msg, exp))

#include <optional>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include "json.hpp"
#include "wdltypes.h"
#include "wdlstring.h"
#include <filesystem>

#if defined(_WIN32)
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
#include <dlfcn.h>
#include <sys/mman.h>    // For mmap, munmap
#include <sys/stat.h>    // For shm_open
#include <fcntl.h>       // For O_RDWR, O_CREAT
#include <unistd.h>      // For close

#endif

#include <algorithm> // for std::sort

//choc classes for reading audio files
#include "choc/audio/choc_AudioFileFormat.h"
#include "choc/audio/choc_AudioFileFormat_Ogg.h"
#include "choc/audio/choc_AudioFileFormat_WAV.h"
#include "choc/audio/choc_AudioFileFormat_FLAC.h"
#include "choc/audio/choc_AudioFileFormat_MP3.h"
#include "choc/audio/choc_SampleBuffers.h"

#include <mutex>

namespace cabbage {

// Function to handle debug output in Visual Studio
inline void logToDebug(const std::string& message) {
#ifdef _WIN32
    OutputDebugStringA(message.c_str());
#endif
}

class Logger {
public:
    static Logger& getInstance();
    void setLogFile(const std::string& filePath);
    void closeLogFile();
    void logMessage(const std::string& message);

private:
    Logger() = default;
    ~Logger() {
        closeLogFile();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile;
    std::mutex fileMutex;
};

// Stream class to allow chaining with << operator
class LogStream {
public:
    LogStream(const char* logLevel, bool includeContext)
        : logLevel(logLevel), includeContext(includeContext) {}

    // Move constructor and move assignment operator
    LogStream(LogStream&& other) noexcept
        : logLevel(other.logLevel),
          includeContext(other.includeContext),
          message(std::move(other.message)),
          file(other.file),
          line(other.line),
          function(other.function) {}

    LogStream& operator=(LogStream&& other) noexcept {
        if (this != &other) {
            logLevel = other.logLevel;
            includeContext = other.includeContext;
            message = std::move(other.message);
            file = other.file;
            line = other.line;
            function = other.function;
        }
        return *this;
    }

    // Destructor to log the message
    ~LogStream() {
        std::ostringstream oss;
        oss << "Cabbage " << logLevel << ": ";
        oss << message.str();

        if (includeContext) {
            oss << " " << std::filesystem::path(file).filename().string() << " (" << line << ") " << function
                << " [Thread ID: " << std::this_thread::get_id() << "]";
        }

        Logger::getInstance().logMessage(oss.str());
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        message << value;
        return *this;
    }

    void setContext(const char* file, int line, const char* function) {
        this->file = file;
        this->line = line;
        this->function = function;
    }

private:
    const char* logLevel;
    bool includeContext;
    std::ostringstream message;
    const char* file = nullptr;
    int line = 0;
    const char* function = nullptr;

    // Disable copy operations
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
};

// Stream-like logger functions for different log levels
inline LogStream LogInfo() {
    return LogStream("INFO", false); // Info doesn't include file/line info
}

inline LogStream LogVerbose(const char* file, int line, const char* function) {
    LogStream log("VERBOSE", true);
    log.setContext(file, line, function);
    return log;
}

inline LogStream LogWarning(const char* file, int line, const char* function) {
    LogStream log("WARNING", true);
    log.setContext(file, line, function);
    return log;
}

inline LogStream LogError(const char* file, int line, const char* function) {
    LogStream log("ERROR", true);
    log.setContext(file, line, function);
    return log;
}


// To use the logging functions like std::ostream:
#define logInfo LogInfo()
#define logDebug LogVerbose(__FILE__, __LINE__, __FUNCTION__)
#define logWarning LogWarning(__FILE__, __LINE__, __FUNCTION__)
#define logError LogError(__FILE__, __LINE__, __FUNCTION__)

#if defined(LINUX)
// Message Pipe Host for sending messages to and from webview on Linux
class MessagePipeHost {
public:
    enum MessageType{
        LoadUrl = 0,
        EvaluateJS = 1,
        KillProcess
    };

    MessagePipeHost() : pipe_fd(-1) {
    }

    ~MessagePipeHost() {
        closePipe();
    }

    void closePipe(){
        if (pipe_fd != -1) {
            close(pipe_fd);
        }
        if (unlink(pipeName) == -1) {
            perror("Error removing pipe");
        }
    }
    void createPipe(const char* name);
    bool isOpenForWriting(bool shouldWait = false);
    void send(MessageType type, const std::string& message = "");

private:
    const char* pipeName;
    int pipe_fd;
    bool openForWriting = false;
};
#endif

class Utils 
{
public:
    static std::string sanitisePath(const std::string& path);
    static bool validateChannelConfig(const std::string& channelConfig, int maxInputs, int maxOutputs);
    static std::string getJsonWithLineNumbers(const nlohmann::json& j);
    static std::string getJsonWithLineNumbers(const std::string& json_str);
    static std::string toLower(const std::string& str);
    static std::string getChannelConfig(const std::string& csdFile);
    static int getDebounceInterval(const std::string& csdFile);
    static bool getEnableDevTools(const std::string& csdFile);
    
    template <typename T>
    static std::optional<T> findPropertyInForm(const nlohmann::json& json, const std::string& propertyName) 
    {
        for (const auto& item : json) {
            if (item.contains("type") && item["type"] == "form" && item.contains(propertyName)) {
                return item[propertyName].get<T>();
            }
        }
        return std::nullopt; // Return empty optional if not found
    }
};

class StringFormatter 
{
public:
    // Implementations of StringFormatter class methods
    template <typename... Args>
    static std::string format(const std::string& templateStr, Args&&... args) 
    {
        std::vector<std::string> arguments{ toString(std::forward<Args>(args))... };
        return processTemplate(templateStr, arguments);
    }
    
    static void removeBackticks(std::string& str);
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
        for (size_t i = 0; i < templateStr.size(); ++i) 
        {
            if (templateStr[i] == '<' && i + 1 < templateStr.size() && templateStr[i + 1] == '>' && argIndex < args.size())
            {
                result += args[argIndex++];
                ++i;  // Skip the '>'
            } else 
            {
                result += templateStr[i];
            }
        }

        return result;
    }
};

class File 
{
public:
    template <typename T>
    struct Soundfile{
        std::vector<T> audioData;
        int numChannels;
        int numSamples;
        Soundfile(std::vector<T> data = {}, int numChans = 0, int numSamps = 0):
        audioData(data),
        numSamples(numSamps),
        numChannels(numChans)
        {}        
    };

    //===========================================================================================
    template <typename T>
    static File::Soundfile<T> readAudioFile(const std::string &filePath, int targetSampleRate)
    {
        if(!cabbage::File::fileExists(filePath))
        {
            cabbage::logDebug << "reader is not valid";
            return {};
        }
        
        choc::audio::AudioFileFormatList formats;
        formats.addFormat<choc::audio::WAVAudioFileFormat<false>>();
        formats.addFormat<choc::audio::OggAudioFileFormat<false>>();
        formats.addFormat<choc::audio::MP3AudioFileFormat>();
        formats.addFormat<choc::audio::FLACAudioFileFormat<false>>();
        auto reader = formats.createReader (filePath);
        
        if(!reader.get())
        {
            cabbage::logDebug << "reader is not valid";
            return {};
        }
        
        auto& p = reader->getProperties();
        try{
            // pass a targetSampleRate in case resampling is needed
            auto samples = reader->loadFileContent(targetSampleRate, p.numFrames * p.numChannels);
            auto bufferView = samples.frames.getView();
            int numFrames = bufferView.getChannel(0).getNumFrames();
            int numChannels = bufferView.getNumChannels();
            int totalSamples = numFrames * numChannels;
            
            // Create a vector of the appropriate size
            std::vector<T> audioData(totalSamples);
            
            for (int frame = 0; frame < numFrames; ++frame)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    audioData[frame * numChannels + channel] = static_cast<T>(bufferView.getSample(channel, frame));
                }
            }
            
            Soundfile<T> soundfile(audioData, numChannels, totalSamples);
            
            return soundfile;
        }
        catch (std::exception& e) {
            cabbage::logDebug << e.what();
            return {};
        }
    }

    // Returns a new file path with the specified file extension
    static std::string withExtension(const std::string& filePath, const std::string& newExtension);

    // Retrieves the name of the binary file
    static std::string getBinaryFileName();

    // Gets a list of files of a specific type in a directory
    static std::vector<std::string> getFilesOfType(const std::string& dirPath, const std::string& fileTypes);

    // Extracts properties from a given JavaScript content
    static nlohmann::json extractPropsFromJS(const std::string& jsContent);

    // Gets the full path of the .csd file
    static std::string getCsdPath(const std::string file = "");

    // Gets the .csd file name without its extension
    static std::string getCsdWithoutExtension();

    // Joins a directory path and a file name into a single path
    static std::string joinPath(const std::string& dirPath, const std::string& fileName);

    // Retrieves the path to the current binary
    static std::string getBinaryPath();

    // Checks if a file exists at the given path
    static bool fileExists(const std::string& filePath);

    // Checks if a directory exists at the given path
    static bool directoryExists(const std::string& dirPath);

    // Retrieves the directory for Cabbage resources
    static std::string getCabbageResourceDir();

    // Loads the content of a JavaScript file as a string
    static std::string loadJSFile(const std::string& filePath);

    // Extracts the Cabbage section from a given .csd file
    static std::string getCabbageSection(const std::string& csdFile = "");

    // Reads the entire content of a file into a string
    static std::string getFileAsString(std::string csdFile = "");

    // Retrieves the number of input channels (nchnls_i) from the .csd file
    static int getNumberOfInputChannels(const std::string& csdFile);

    // Retrieves the number of output channels (nchnls) from the .csd file
    static int getNumberOfOutputChannels(const std::string& csdFile);

    // Formats a file path to a consistent style
    static std::string formatPath(const std::string& path);

    // Retrieves the full path and name of the .csd file - mostly used in plugin wrapper as path is known
    // when loading instruments from VS-Code
    static std::string getCsdFileAndPath();

    // Reads and parses the Cabbage section from the specified .csd file
    static std::optional<nlohmann::json> parseCabbageSection(const std::string& csdFile);

    // Retrieves the path to the settings file
    static std::string getSettingsFile();

    // Retrieves a specific property from the settings file by section and key
    static std::string getSettingsProperty(const std::string& section, const std::string& key);

    
private:
#if defined(_WIN32)
    static std::string getWindowsBinaryPath()
    {
        //char dllPath[MAX_PATH];
        //GetModuleFileNameA(NULL, dllPath, MAX_PATH); // Use the 'A' version for ANSI
        //std::string fileName(dllPath); // Convert char array to std::string
        //return std::string(fileName);
        char dllPath[MAX_PATH] = { 0 };
        HMODULE hModule = NULL;

        // Get the handle to the module containing this function
        if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&getWindowsBinaryPath),
            &hModule)) {
            GetModuleFileNameA(hModule, dllPath, sizeof(dllPath));
        }
        else {
            // Handle the error
            std::cerr << "Error retrieving module handle: " << GetLastError() << std::endl;
        }

        return std::string(dllPath);
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
    static std::string getSharedLibraryPath()
    {
        Dl_info dl_info;
        if (dladdr(reinterpret_cast<void*>(&getSharedLibraryPath), &dl_info) != 0)
        {
            return std::string(dl_info.dli_fname);
        }
        return {};
    }

    static std::string getLinuxBinaryPath()
    {
        return getSharedLibraryPath();
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
class WidgetDescriptors 
{
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
    static nlohmann::json get(const std::string& widgetType)
    {
        
        std::vector<std::string> widgetTypes;
#ifdef CabbageApp
        //this folder will be different for plugins than for the vscode extension
        std::string widgetPath = cabbage::File::getSettingsProperty("currentConfig", "jsSourceDir") + "/cabbage/widgets";;
#else
        std::string widgetPath = cabbage::File::getCsdPath() + "/cabbage/widgets"; // Folder containing widget files
#endif
        
        if(!cabbage::File::directoryExists(widgetPath))
        {
            cabbage::logDebug << "Invalid widget JS files path:" << widgetType;
            return {};
        }
            
        
        auto jsFileContents = cabbage::File::loadJSFile(widgetPath + "/" + widgetType + ".js");
        if (!jsFileContents.empty())
        {
            return cabbage::File::extractPropsFromJS(jsFileContents);
        }
        
        cabbage::logDebug << "Invalid widget type:" << widgetType;
        cabAssert(false, "Invalid widget type:");
        return {};
    }
};


} // namespace cabbage
