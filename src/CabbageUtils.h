/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include <lattice/LatticeUtils.h>
// choc classes for reading audio files
#include <audio/choc_AudioFileFormat.h>
#include <audio/choc_AudioFileFormat_Ogg.h>
#include <audio/choc_AudioFileFormat_WAV.h>
#include <audio/choc_AudioFileFormat_FLAC.h>
#include <audio/choc_AudioFileFormat_MP3.h>
#include <audio/choc_SampleBuffers.h>


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
    
    static void check(bool condition, const std::string& message)
    {
        if (condition) {
            throw std::runtime_error("Assertion failed: " + message);
        }
    }
    
    // Quickly find a property defined in a user's "form" object
    template <typename T>
    static std::optional<T> findPropertyInForm(const nlohmann::json &json, const std::string &propertyName)
    {
        for (const auto &item : json)
        {
            if (item.contains("type") && item["type"] == "form" && item.contains(propertyName))
            {
                return item[propertyName].get<T>();
            }
        }
        return std::nullopt; // Return empty optional if not found
    }
    
};

class File : public lattice::File
{
public:
    static std::string getCsdFileAndPath();
    // Reads and parses the cabbage section from the file
    static std::optional<nlohmann::json> parseCabbageSection(const std::string &csdFile);
    // Function to get the number of input channels (nchnls_i)
    static int getNumberOfInputChannels(const std::string &csdFile);
    // Function to get the number of output channels (nchnls)
    static int getNumberOfOutputChannels(const std::string &csdFile);
    // Function to get the Cabbage section fo text from a csd file
    static std::string getCabbageSection(const std::string &csdFile);
    // Return the csd path
    static std::string getCsdPath(const std::string& file = "");
    // Extract widget properties from corresponding JS file
    static nlohmann::json extractPropsFromJS(const std::string &jsContent);
    
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
    static std::vector<std::string> getWidgetTypes()
    {
        std::vector<std::string> widgetTypes;
        std::string widgetPath = cabbage::File::getCsdPath() + "/widgets"; // Folder containing widget files
        
        // Check if the directory exists
        if (!std::filesystem::exists(widgetPath) || !std::filesystem::is_directory(widgetPath))
        {
            std::cerr << "Error: Directory " << widgetPath << " does not exist or is not a directory." << std::endl;
            return widgetTypes; // Return an empty vector if directory is not found
        }
        
        // Iterate through the directory and extract the filenames without the extension
        for (const auto &entry : std::filesystem::directory_iterator(widgetPath))
        {
            if (entry.is_regular_file())
            {                                                            // Only process regular files
                std::string filename = entry.path().filename().string(); // Get filename
                std::string extension = entry.path().extension().string();
                
                // Remove extension from filename
                if (!extension.empty())
                {
                    filename = filename.substr(0, filename.length() - extension.length());
                }
                
                widgetTypes.push_back(filename); // Add filename to the vector
            }
        }
        
        return widgetTypes;
    }
    
    // returns a widget descriptor object for a given widget type
    static nlohmann::json get(const std::string &widgetType)
    {
        
        std::vector<std::string> widgetTypes;
#ifdef CabbageApp
        // this folder will be different for plugins than for the vscode extension
        std::string widgetPath =
        cabbage::File::getSettingsProperty("currentConfig", "jsSourceDir") + "/cabbage/widgets";
        ;
#else
        std::string widgetPath = cabbage::File::getCsdPath() + "/cabbage/widgets"; // Folder containing widget files
#endif
        
        if (!cabbage::File::directoryExists(widgetPath))
        {
            lattice::logDebug << "Invalid widget JS files path:" << widgetPath;
            return {};
        }
        
        auto jsFileContents = cabbage::File::loadJSFile(widgetPath + "/" + widgetType + ".js");
        if (!jsFileContents.empty())
        {
            return cabbage::File::extractPropsFromJS(jsFileContents);
        }
        
        lattice::logDebug << "Invalid widget type:" << widgetType;
        cabbage::Utils::check(false, "Invalid widget type:");
        return {};
    }
};

}
