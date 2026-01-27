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

#pragma once
#include <clap/clap.h>
#include "CabbageProcessor.h"

#if !defined(CabbagePluginSynthAUv2) || defined(CabbagePluginEffectAUv2)

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
#endif


#if defined(CabbagePluginSynthVST3) || defined(CabbagePluginSynthAUv2)
    static constexpr const char* features[] = {
        CLAP_PLUGIN_FEATURE_INSTRUMENT,
        CLAP_PLUGIN_FEATURE_SYNTHESIZER,
        CLAP_PLUGIN_FEATURE_STEREO,
        nullptr
    };
#elif defined(CabbagePluginMidiFxAUv2)
    static constexpr const char* features[] = {
        CLAP_PLUGIN_FEATURE_NOTE_EFFECT,
        CLAP_PLUGIN_FEATURE_INSTRUMENT,
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

#define LATTICE_AU_SUBTYPE "Cp47"
#if defined(CabbagePluginEffectAUv2)
    #define LATTICE_AU_TYPE "aufx"
#elif defined(CabbagePluginMidiFxAUv2)
    #define LATTICE_AU_TYPE "aumi"
#else
    #define LATTICE_AU_TYPE "aumu"
#endif
#define LATTICE_MANUFACTURER_NAME "CabbageAudio"
#define LATTICE_MANUFACTURER_CODE "Cabb"


// Function to safely access the descriptor (ensures initialization order)
inline const clap_plugin_descriptor* getDescriptor() {
    static const clap_plugin_descriptor descriptor = {
        .clap_version = CLAP_VERSION,
    #if defined(CabbagePluginSynthAUv2) || defined(CabbagePluginEffectAUv2)
        .id = "com.cabbageaudio.1d47",
        .name = "Cabbage1d47Plugin",
    #else
        .id = pluginInfo.id.c_str(),
        .name = pluginInfo.name.c_str(),       
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
