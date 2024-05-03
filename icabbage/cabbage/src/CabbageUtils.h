#pragma once

#include <iostream>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <Shlobj.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
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
            return getLinuxHomeDir();
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

private:
    #if defined(_WIN32)
    static std::string getWindowsBinaryPath() {
        char path[MAX_PATH];
        GetModuleFileName(NULL, path, MAX_PATH);
        return std::string(path);
    }

    static std::string getWindowsProgramDataDir() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path)))
            return std::string(path+"/CabbageAudio");
        else
            return "";
    }
    #elif defined(__APPLE__)
    static std::string getMacBinaryPath() {
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
            return std::string(path);
        else
            return "";
    }

    static std::string getMacCabbageResourceDir() {
        return "/Library/Application Support/CabbageAudio";
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
