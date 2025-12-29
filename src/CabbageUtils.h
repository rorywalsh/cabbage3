/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include <lattice/LatticeProcessor.h>
#include <lattice/LatticeUtils.h>
// choc classes for reading audio files
#include <choc/audio/choc_AudioFileFormat.h>
#include <choc/audio/choc_AudioFileFormat_Ogg.h>
#include <choc/audio/choc_AudioFileFormat_WAV.h>
#include <choc/audio/choc_AudioFileFormat_FLAC.h>
#include <choc/audio/choc_AudioFileFormat_MP3.h>
#include <choc/audio/choc_SampleBuffers.h>
#include <set>
#include <filesystem>
#ifdef LATTICE_WINDOWS
#include <shlobj.h>
#endif
#pragma once

namespace cabbage
{


class Utils
{
public:
    static std::string sanitisePath(const std::string &path);
    static std::string getChannelConfig(const std::string &csdFile);
    static bool validateChannelConfig(const std::string &channelConfig, int maxInputs, int maxOutputs);
    static std::string getJsonWithLineNumbers(const nlohmann::json &j);
    static std::string getJsonWithLineNumbers(const std::string &json_str);
    
    static void check(bool condition, const std::string& message = "")
    {
        if (!condition) {
            throw std::runtime_error(message.empty() ? "" : "Assertion failed: " + message);
        }
    }
    
    // Quickly find a property defined in a user's "form" object - sppourts flat and nested properties
    template <typename T>
    static std::optional<T> findPropertyInForm(const nlohmann::json &json, const std::string &propertyName)
    {
        auto getNestedValue = [](const nlohmann::json &jsonObj,
                                 const std::string &path) -> std::optional<nlohmann::json>
        {
            nlohmann::json current = jsonObj;
            std::stringstream ss(path);
            std::string segment;

            // Split on '.' to navigate through nested properties
            while (std::getline(ss, segment, '.'))
            {
                if (!current.contains(segment))
                    return std::nullopt;
                current = current[segment];
            }
            return current;
        };

        for (const auto &item : json)
        {
            if (item.contains("type") && item["type"] == "form")
            {
                auto valueOpt = getNestedValue(item, propertyName);
                if (valueOpt.has_value())
                    return valueOpt->template get<T>();
            }
        }
        return std::nullopt; // Return empty optional if not found
    }

    
};

class File : public lattice::File
{
public:
    // Returns the csd file. If csdFile is emtpy it willdeduct the location
    static std::string getCsdFileAndPath(std::string csdFile = "");
    // Reads and parses the cabbage section from the file
    static std::optional<nlohmann::json> parseCabbageSection(const std::string &csdFilePath);
    // Returns the CsOptions
    static std::string getCsOptions(const std::string& csdFilePath);    
    // Function to get the number of input channels (nchnls_i)
    static int getNumberOfInputChannels(const std::string &csdFilePath);
    // Function to get the number of output channels (nchnls)
    static int getNumberOfOutputChannels(const std::string &csdFilePath);
    // Function to get the Cabbage section fo text from a csd file
    static std::string getCabbageSection(const std::string &csdFilePath);
    // Return the csd path
    static std::string getCsdPath(const std::string& file = "");
    // Extract widget properties from corresponding JS file
    static nlohmann::json extractPropsFromJS(const std::string &jsContent);
    // Returns path to Cabbage specific resources folder
    static std::string getCabbageResourceDir();
    // Return the user's Cabbage file
    static std::string getSettingsFile();
    // Returns a property from the settings file
    static std::string getSettingsProperty(const std::string &section, const std::string &key);
    // Retrun binary without extension
    static std::string getBinaryWithoutExtension();
    // Find path to Cabbage widgets JS source dir
    static std::string findCabbageJSWidgetPath();
    // Browse for a file using native dialog
    static std::string browseForFile(const std::string& title = "Choose a file", const std::string& initialDir = "", const std::string& filters = "*");
    // Write std::string to file
    static bool writeToFile(const std::string &filePath, const std::string &content);
    // Read from file
    static std::string readFromFile(const std::string &filePath);

#if defined(_WIN32)
    static std::string getWindowsProgramDataDir()
    {
           char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path)))
           {
               return std::string(path) + "\\CabbageAudio";
           }
              
           else
               return "";
    }
#elif defined(__APPLE__)
    static std::string getMacCabbageResourceDir()
    {
        const char *homeDir = getenv("HOME");
        if (homeDir)
            return std::string(homeDir) + "/Library/CabbageAudio";
        else
        {
            struct passwd *pw = getpwuid(getuid());
            if (pw)
                return std::string(pw->pw_dir) + "/Library/CabbageAudio";
            else
                return "";
        }
    }
#elif defined(__linux__)
    static std::string getLinuxHomeDir()
    {
        const char *homeDir = getenv("HOME");
        if (homeDir)
            return std::string(homeDir);
        else
        {
            struct passwd *pw = getpwuid(getuid());
            if (pw)
                return std::string(pw->pw_dir);
            else
                return "";
        }
    }
#endif
    //===========================================================================================
    template <typename T>
    static File::Soundfile<T> readAudioFile(const std::string &filePath, int targetSampleRate)
    {
        if (!cabbage::File::exists(filePath))
        {
            lattice::logDebug << "reader is not valid";
            return {};
        }

        choc::audio::AudioFileFormatList formats;
        formats.addFormat<choc::audio::WAVAudioFileFormat<false>>();
        formats.addFormat<choc::audio::OggAudioFileFormat<false>>();
        formats.addFormat<choc::audio::MP3AudioFileFormat>();
        formats.addFormat<choc::audio::FLACAudioFileFormat<false>>();
        auto reader = formats.createReader(filePath);

        if (!reader.get())
        {
            lattice::logDebug << "reader is not valid";
            return {};
        }

        auto &p = reader->getProperties();
        try
        {
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
        catch (std::exception &e)
        {
            lattice::logDebug << e.what();
            return {};
        }
    }
};

/*
 Utility class to get widget descriptors from widget JS files
 */
class WidgetDescriptors
{
public:
    // Utility function to get full list of widget types contained in widgets directory
    // Scans ALL configured widget directories (built-in + custom)
    static std::vector<std::string> getWidgetTypes()
    {
        std::vector<std::string> widgetTypes;
        std::vector<std::string> widgetPaths;
        
        // Get all configured widget directories from settings
        try
        {
            std::ifstream file(cabbage::File::getSettingsFile(), std::ios::binary);
            if (file.is_open())
            {
                std::ostringstream oss; oss << file.rdbuf(); file.close();
                auto jsonData = nlohmann::json::parse(oss.str());
                
                if (jsonData.contains("currentConfig") && jsonData["currentConfig"].contains("jsSourceDir"))
                {
                    auto &val = jsonData["currentConfig"]["jsSourceDir"];
                    std::vector<std::string> dirs;
                    
                    if (val.is_array())
                    {
                        for (auto &v : val)
                            if (v.is_string()) dirs.emplace_back(v.get<std::string>());
                    }
                    else if (val.is_string())
                    {
                        dirs.emplace_back(val.get<std::string>());
                    }
                    
                    // Build list of all widget directories
                    for (const auto &baseDir : dirs)
                    {
                        std::string candidate = lattice::File::joinPath(baseDir, "cabbage", "widgets");
                        if (cabbage::File::directoryExists(candidate))
                            widgetPaths.push_back(candidate);
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            lattice::logDebug << "Error reading jsSourceDir from settings: " << e.what();
        }
        
        // If no paths found, use the fallback
        if (widgetPaths.empty())
        {
            std::string fallbackPath = cabbage::File::findCabbageJSWidgetPath();
            if (!fallbackPath.empty())
                widgetPaths.push_back(fallbackPath);
        }
        
        // Scan all widget directories and collect unique widget types
        std::set<std::string> uniqueTypes; // Use set to avoid duplicates
        
        lattice::logDebug << "Scanning " << widgetPaths.size() << " widget directories";
        
        for (const auto &widgetPath : widgetPaths)
        {
            lattice::logDebug << "Checking widget path: " << widgetPath;
            
            if (!std::filesystem::exists(widgetPath) || !std::filesystem::is_directory(widgetPath))
            {
                lattice::logDebug << "Path does not exist or is not a directory: " << widgetPath;
                continue;
            }
            
            // Iterate through the directory and extract the filenames without the extension
            for (const auto &entry : std::filesystem::directory_iterator(widgetPath))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    std::string extension = entry.path().extension().string();
                    
                    // Remove extension from filename
                    if (!extension.empty())
                    {
                        filename = filename.substr(0, filename.length() - extension.length());
                    }
                    
                    //lattice::logDebug << "Found widget type: " << filename;
                    uniqueTypes.insert(filename); // Add to set (automatically deduplicates)
                }
            }
        }
        
        lattice::logDebug << "Total unique widget types found: " << uniqueTypes.size();
        
        // Convert set to vector
        widgetTypes.assign(uniqueTypes.begin(), uniqueTypes.end());
        
        return widgetTypes;
    }
    
    // returns a widget descriptor object for a given widget type
    static nlohmann::json get(const std::string &widgetType)
    {
        std::vector<std::string> widgetPaths;
        
#ifndef CabbageApp
        // In plugin mode, look for widgets relative to the CSD file
        const auto resourceDir = lattice::File::getParentDirectory(cabbage::File::getCsdFileAndPath());
        std::string pluginWidgetPath = lattice::File::joinPath(resourceDir, "cabbage", "widgets");
        if (cabbage::File::directoryExists(pluginWidgetPath))
        {
            widgetPaths.push_back(pluginWidgetPath);
            //lattice::logDebug << "Plugin mode: Added CSD-relative widget path: " << pluginWidgetPath;
        }
#else
        // In CabbageApp mode, get all configured widget directories from settings
        try
        {
            std::ifstream file(cabbage::File::getSettingsFile(), std::ios::binary);
            if (file.is_open())
            {
                std::ostringstream oss; oss << file.rdbuf(); file.close();
                auto jsonData = nlohmann::json::parse(oss.str());
                
                if (jsonData.contains("currentConfig") && jsonData["currentConfig"].contains("jsSourceDir"))
                {
                    auto &val = jsonData["currentConfig"]["jsSourceDir"];
                    std::vector<std::string> dirs;
                    
                    if (val.is_array())
                    {
                        for (auto &v : val)
                            if (v.is_string()) dirs.emplace_back(v.get<std::string>());
                    }
                    else if (val.is_string())
                    {
                        dirs.emplace_back(val.get<std::string>());
                    }
                    
                    // Build list of all widget directories
                    for (const auto &baseDir : dirs)
                    {
                        std::string candidate = lattice::File::joinPath(baseDir, "cabbage", "widgets");
                        if (cabbage::File::directoryExists(candidate))
                            widgetPaths.push_back(candidate);
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            lattice::logDebug << "Error reading jsSourceDir from settings: " << e.what();
        }
        
        // If no paths found, use the fallback
        if (widgetPaths.empty())
        {
            std::string fallbackPath = cabbage::File::findCabbageJSWidgetPath();
            if (!fallbackPath.empty())
                widgetPaths.push_back(fallbackPath);
        }
#endif
        
        // Search all widget directories for the widget file
        for (const auto &widgetPath : widgetPaths)
        {
            std::string fullPath = widgetPath + "/" + widgetType + ".js";
            //lattice::logDebug << "Searching " << widgetPath << " for widget classes..";
            if (cabbage::File::exists(fullPath))
            {
                auto jsFileContents = cabbage::File::loadJSFile(fullPath);
                if (!jsFileContents.empty())
                {
                    //lattice::logDebug << "Found widget descriptor for '" << widgetType << "' in: " << widgetPath;
                    return cabbage::File::extractPropsFromJS(jsFileContents);
                }
            }
        }
        
        lattice::logInfo << "Unknown widget type: " << widgetType << " - skipping widget";
        return {};
    }
};

}
