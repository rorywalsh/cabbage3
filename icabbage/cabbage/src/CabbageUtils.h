#pragma once

#define cabAssert(exp, msg) assert(((void)msg, exp))

#include <thread>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#include <Shlobj.h>
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


// Define a macro to enable/disable debugging
#define DEBUG 1

#if DEBUG
#define _log(message) \
    do { \
        std::ostringstream oss; \
        oss << "Cabbage DEBUG: " << __FILE__ << " (" << __LINE__ << ") " << __FUNCTION__ << ": " << message << std::endl; \
        std::cout << oss.str(); \
    } while (0)
#else
#define DEBUG_LOG(message) \
    do { \
    } while (0)
#endif

// StringFormatter class
class StringFormatter {
public:
    template <typename... Args>
    static std::string format(const std::string& templateStr, Args&&... args) {
        std::vector<std::string> arguments{ toString(std::forward<Args>(args))... };
        return processTemplate(templateStr, arguments);
    }

    static std::string getCabbageSectionAsJSEscapedString(const std::string& input) {
        // Find the positions of <Cabbage> and </Cabbage>
        size_t start_pos = input.find("<Cabbage>");
        size_t end_pos = input.find("</Cabbage>");
        std::string cabbageSection;
            // Check if both markers are found
            if (start_pos != std::string::npos && end_pos != std::string::npos) {
                start_pos = input.find("<Cabbage>");
                end_pos = input.find("</Cabbage>");
                size_t length = end_pos + strlen("</Cabbage>") - start_pos;
                cabbageSection = input.substr(start_pos, length);
            } else {
                // If markers are not found, return empty string or handle error as needed
                // For simplicity, returning the entire input if markers are not found
                cabbageSection = input;
            }
        std::string sanitisedString = sanitiseString(cabbageSection);

        return sanitisedString + "\n";
    }

private:
    template <typename T>
    static std::string toString(T&& value) {
        std::ostringstream oss;
        oss << std::forward<T>(value);
        return oss.str();
    }

    static std::string processTemplate(const std::string& templateStr, const std::vector<std::string>& args) {
        std::string result;
        result.reserve(templateStr.size());

        size_t argIndex = 0;
        for (size_t i = 0; i < templateStr.size(); ++i) {
            if (templateStr[i] == '<' && i + 1 < templateStr.size() && templateStr[i + 1] == '>' && argIndex < args.size()) {
                result += args[argIndex++];
                ++i;  // Skip the '>'
            } else {
                result += templateStr[i];
            }
        }

        return result;
    }

    static std::string sanitiseString(const std::string& input) {
        std::string sanitized;
        sanitized.reserve(input.size() * 2); // Reserve space to avoid frequent reallocations

        for (char c : input) {
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



class CabbageFile {
public:
    static std::string getBinaryPath() {
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

    static bool fileExists(const std::string& filePath) {
        std::ifstream file(filePath);
        return file.good();
    }

    static bool directoryExists(const std::string& dirPath) {
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

    static std::string getBinaryFileName() {
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
        else {
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

    static std::string getCsdFileAndPath() {
        std::string resourceDir = getCabbageResourceDir();
        std::string binaryFileName = getBinaryFileName();
        size_t pos = binaryFileName.find_last_of(".");
        if (pos != std::string::npos)
            binaryFileName = binaryFileName.substr(0, pos);
        const std::string newPath = joinPath(resourceDir, binaryFileName);
        return joinPath(newPath, binaryFileName + ".csd");
    }
    
    static std::string getCsdPath() {
        std::string resourceDir = getCabbageResourceDir();
        std::string binaryFileName = getBinaryFileName();
        size_t pos = binaryFileName.find_last_of(".");
        if (pos != std::string::npos)
            binaryFileName = binaryFileName.substr(0, pos);
        const std::string newPath = joinPath(resourceDir, binaryFileName);
        return newPath;
    }
    
    //return a JS escaped string
    static std::string getCabbageSection(){
        auto csdText = getFileAsString();
        return StringFormatter::getCabbageSectionAsJSEscapedString(csdText);
    }
    
    //return the file contents, if the file path is not provided, finds
    //the file based on the curren binary name
    static std::string getFileAsString(std::string csdFile = ""){
        if(csdFile.empty())
            csdFile = getCsdFileAndPath();
        
        std::ifstream file(csdFile);
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string csdContents = oss.str();
        return csdContents;
    }
    
    static std::string sanitisePath(const std::string& path) {
            std::string sanitizedPath = path;

            // Remove trailing backslashes
            while (!sanitizedPath.empty() && sanitizedPath.back() == '\\') {
                sanitizedPath.pop_back();
            }

            // Replace backslashes with forward slashes
            for (char& c : sanitizedPath) {
                if (c == '\\') {
                    c = '/';
                }
            }

            return sanitizedPath;
        }

private:
    #if defined(_WIN32)
    static std::string getWindowsBinaryPath() {
        char path[MAX_PATH];
        HMODULE hModule = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(&getWindowsBinaryPath), &hModule);
        GetModuleFileName(hModule, path, MAX_PATH);
        return std::string(path);
    }

    static std::string getWindowsProgramDataDir() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path)))
            return std::string(path) + "\\CabbageAudio";
        else
            return "";
    }
    #elif defined(__APPLE__)
    static std::string getMacBinaryPath() {
        Dl_info info;
        if (dladdr((void*)"getMacBinaryPath", &info)){
            return std::string(info.dli_fname);
        }
        return "";
    }

    static std::string getMacCabbageResourceDir() {
        const char *homeDir = getenv("HOME");
        if (homeDir)
            return std::string(homeDir) + "/Library/CabbageAudio";
        else {
            struct passwd *pw = getpwuid(getuid());
            if (pw)
                return std::string(pw->pw_dir) + "/Library/CabbageAudio";
            else
                return "";
        }
    }
    
    #elif defined(__linux__)
    static std::string getLinuxBinaryPath() {
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        return std::string(path, (count > 0) ? count : 0);
    }

    static std::string getLinuxHomeDir() {
        const char *homeDir = getenv("HOME");
        if (homeDir)
            return std::string(homeDir);
        else {
            struct passwd *pw = getpwuid(getuid());
            if (pw)
                return std::string(pw->pw_dir);
            else
                return "";
        }
    }
    #endif
};

class TimerThread {
public:
    TimerThread() : stop(false) {}

    // Start the thread with a member function callback and a timer interval
        template <typename T>
        void Start(T* obj, void (T::*memberFunc)(), int intervalMillis) {
            mThread = std::thread([=]() {
                while (!stop) {
                    // Call the member function on the object instance
                    (obj->*memberFunc)();
                    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMillis)); // Sleep for the specified interval
                    //std::cout << "Timer tick" << std::endl; // Debug output
                }
                //std::cout << "Timer stopped" << std::endl; // Debug output
            });
        }


    // Stop the timer thread
    void Stop() {
        stop = true;
        if (mThread.joinable()) {
            mThread.join();
        }
    }

private:
    std::thread mThread;
    std::atomic<bool> stop;
};


