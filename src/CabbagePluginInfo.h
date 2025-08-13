#pragma once
#include <clap/clap.h>
#include "CabbageProcessor.h"

namespace {
    struct PluginInfo {
        std::string name;
        std::string id;
        
        PluginInfo()
#ifdef CabbageApp
            : name(""), id("")
#else
            : name(getPluginName()), id(getPluginId())
#endif
        {}
        
    private:
        std::string getPluginName() {
            try {
                auto binaryName = cabbage::File::getBinaryWithoutExtension();
                return binaryName.empty() ? "CabbagePluginEffect" : binaryName;
            } catch (...) {
                return "CabbagePluginEffect";
            }
        }
        
        std::string getPluginId() {
            try {
                const std::string cabbageJson(cabbage::File::getCabbageSection(cabbage::File::getCsdFileAndPath()));
                if (nlohmann::json::accept(cabbageJson))
                {
                    auto jsonArray = nlohmann::json::parse(cabbageJson);
                    for (const auto& obj : jsonArray)
                    {
                        if (obj.contains("type") && obj["type"] == "form")
                        {
                            std::string pluginId = obj.value("pluginId", "");
                            if (!pluginId.empty())
                            {
                                // If pluginId is only 4 chars, assume it's a short code and prepend domain
                                if (pluginId.length() == 4)
                                {
                                    return "com.cabbageaudio." + pluginId;
                                }
                                // If more than 4 chars, assume it's already a valid reversed domain
                                else if (pluginId.length() > 4)
                                {
                                    return pluginId;
                                }
                            }
                            // Fallback to default
                            return "com.cabbageaudio." + name;
                        }
                    }
                }
                return "com.cabbageaudio." + name;
            } catch (...) {
                return "com.cabbageaudio." + name;
            }
        }
    };

    // Global PluginInfo with static storage duration
    static const PluginInfo pluginInfo;
}

#if defined(CabbagePluginSynth)
    static constexpr const char* features[] = {
        CLAP_PLUGIN_FEATURE_INSTRUMENT,
        CLAP_PLUGIN_FEATURE_SYNTHESIZER,
        CLAP_PLUGIN_FEATURE_STEREO,
        nullptr
    };
#else
    static constexpr const char* features[] = {
        CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
        CLAP_PLUGIN_FEATURE_UTILITY,
        CLAP_PLUGIN_FEATURE_STEREO,
        nullptr
    };
#endif

// Function to safely access the descriptor (ensures initialization order)
inline const clap_plugin_descriptor* getDescriptor() {
    static const clap_plugin_descriptor descriptor = {
        .clap_version = CLAP_VERSION,
    #if VST3_SDK
        .id = pluginInfo.id.c_str(),
        .name = pluginInfo.name.c_str(),
    #else
        .id = "com.cabbageaudio.1d47",
        .name = "Cabbage1d47Plugin",
    #endif
        .vendor = "CabbageAudio",
        .url = "https://cabbageaudio.com",
        .manual_url = "https://docs.cabbageaudio.com",
        .support_url = "https://support.cabbageaudio.com",
        .version = "1.0.0",
        .description = "CabbagePlugin",
        .features = features
    };
    return &descriptor;
}
