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

    static std::string getCsdPath() {
        std::string resourceDir = getCabbageResourceDir();
        std::string binaryFileName = getBinaryFileName();
        size_t pos = binaryFileName.find_last_of(".");
        if (pos != std::string::npos)
            binaryFileName = binaryFileName.substr(0, pos);
        const std::string newPath = joinPath(resourceDir, binaryFileName);
        return joinPath(newPath, binaryFileName + ".csd");
    }
    
    static std::string getFileAsString(std::string csdFile = ""){
        if(csdFile.empty())
            csdFile = getCsdPath();
        
        std::ifstream file(csdFile);
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string csdContents = oss.str();
        return csdContents;
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
    TimerThread() : mStop(false) {}

    // Start the thread with a member function callback and a timer interval
        template <typename T>
        void Start(T* obj, void (T::*memberFunc)(), int intervalMillis) {
            mThread = std::thread([=]() {
                while (!mStop) {
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
        mStop = true;
        if (mThread.joinable()) {
            mThread.join();
        }
    }

private:
    std::thread mThread;
    std::atomic<bool> mStop;
};


class StringFormatter {
public:
    template <typename... Args>
    static std::string format(const std::string& templateStr, Args&&... args) {
        std::vector<std::string> arguments{ toString(std::forward<Args>(args))... };
        return processTemplate(templateStr, arguments);
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
                ++i;  // Skip the '}'
            } else {
                result += templateStr[i];
            }
        }

        return result;
    }
};

class CabbageUtils {
public:
    static const char* getWidgetUpdateScript(std::string channel, float value)
    {
        static std::string result;
            result = StringFormatter::format(R"(
                window.postMessage({
                    command: "widgetUpdate",
                    text: JSON.stringify({
                        channel: "<>",
                       value: <>
                    })
                });
            )",
            channel,
            value);
            return result.c_str();
    }
    
    static const char* getWidgetUpdateScript(std::string channel, std::string data)
    {
        static std::string result;
            result = StringFormatter::format(R"(
                window.postMessage({
                    command: "widgetUpdate",
                    text: JSON.stringify({
                        channel: "<>",
                        data: '<>'
                    })
                });
            )",
            channel,
            data);
            return result.c_str();
    }
    
    static const char* getTableUpdateScript(std::string channel, std::vector<double> samples)
    {
        std::string data = "[";
        for(const auto& s : samples)
        {
            data += std::to_string(s) + ",";
        }
        data += "]";
        
        static std::string result;
            result = StringFormatter::format(R"(
                window.postMessage({
                    command: "widgetTableUpdate",
                    text: JSON.stringify({
                        channel: "<>",
                        data: '<>'
                    })
                });
            )",
            channel,
            data);
            return result.c_str();
    }
    
    static const char* getCsoundOutputUpdateScript(std::string output)
    {
        static std::string result;
            result = StringFormatter::format(R"(
             window.postMessage({ command: "csoundOutputUpdate", text: `<>` });
            )",
            output);
            return result.c_str();
    }
    
    static std::vector<std::string> getWidgetTypes(){
        return {"form", "rslider", "combobox", "button", "checkbox", "gentable", "label", "hslider", "vslider", "checkbox", "keyboard"};
    }
    
    static bool isWidget(const std::string& target) {
        std::vector<std::string> widgetTypes = getWidgetTypes();
        // Check if the target string is in the vector
        auto it = std::find(widgetTypes.begin(), widgetTypes.end(), target);
        return it != widgetTypes.end();
    }
};
