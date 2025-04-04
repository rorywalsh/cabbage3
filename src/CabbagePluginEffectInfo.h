#pragma once
#include <clap/clap.h>
#include "CabbageProcessor.h"

namespace {
    struct PluginInfo {
        std::string name;
        std::string id;
        
        PluginInfo() :
            name(cabbage::File::getBinaryWithoutExtension()),
            id([this](){
                const std::string cabbageJson(cabbage::File::getCabbageSection(cabbage::File::getCsdFileAndPath()));
                if (nlohmann::json::accept(cabbageJson))
                {
                    auto jsonArray = nlohmann::json::parse(cabbageJson);
                    for (const auto& obj : jsonArray) 
                    {
                        if (obj.contains("type") && obj["type"] == "form")
                        {
                            return obj.value("pluginId", "com.cabbageaudio." + name);
                        }
                    }
                }
                return "com.cabbageaudio." + name;
            }())
        {}
    };

    // 2. Global strings with static storage duration
    static const PluginInfo pluginInfo;
}

#if defined(CabbagePluginSynth)
    static constexpr const char* features[] = {
        "CLAP_PLUGIN_FEATURE_INSTRUMENT",
        "CLAP_PLUGIN_FEATURE_SYNTHESIZER",
        "CLAP_PLUGIN_FEATURE_STEREO",
        nullptr
    };
#else
    static constexpr const char* features[] = {
        "CLAP_PLUGIN_FEATURE_AUDIO_EFFECT",
        "CLAP_PLUGIN_FEATURE_UTILITY",
        "CLAP_PLUGIN_FEATURE_STEREO",
        nullptr
    };
#endif

static const clap_plugin_descriptor descriptor = {
    .clap_version = CLAP_VERSION,
    .id = pluginInfo.id.c_str(),       // Safe (static storage)
    .name = pluginInfo.name.c_str(), // Safe (static storage)
    .vendor = "CabbageAudio",
    .url = "https://cabbageaudio.com",
    .manual_url = "https://docs.cabbageaudio.com",
    .support_url = "https://support.cabbageaudio.com",
    .version = "1.0.0",
    .description = "CabbagePluginEffect Plugin",
    .features = features
};


