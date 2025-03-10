//
// Created by Paul Walker on 2/23/25.
//

#include "FactoryImpl.h"
#include "../cabsProcessor.h"
#include "Plugin.h"
#include <iostream>

namespace impl
{

    static constexpr const char* features[4] =
    {
        CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
        CLAP_PLUGIN_FEATURE_UTILITY,
        CLAP_PLUGIN_FEATURE_STEREO,
        nullptr
    };

    static constexpr clap_plugin_descriptor descriptor =
    {
        .clap_version = CLAP_VERSION,
        .id = "your.reversed.domain.name.PluginName",
        .name = "cawPlugin",
        .vendor = "CabbageAudio",
        .url = "https://cabbageaudio.com",
        .manual_url = "",
        .support_url = "",
        .version = "1.0.0",
        .description = "Cabbage Audio Plugin",
        .features = features
    };

    bool init(const char* /*plugin_path*/)
    {        
        return true;
    }

    void deinit()
    {}

    uint32_t getPluginCount(const clap_plugin_factory* /*factory*/)
    {
        return 1;
    }

    const clap_plugin_descriptor* getPluginDescriptor(const clap_plugin_factory* /*factory*/, uint32_t index)
    {
        static clap_plugin_descriptor desc; // Static variable to persist across calls

        desc.id = cabs::pluginDescriptor.uniqueId;
        desc.name = cabs::pluginDescriptor.name;
        desc.vendor = cabs::pluginDescriptor.vendor;
        desc.url = cabs::pluginDescriptor.url;
        desc.manual_url = cabs::pluginDescriptor.manualUrl;
        desc.support_url = cabs::pluginDescriptor.supportUrl;
        desc.version = cabs::pluginDescriptor.version;
        desc.description = cabs::pluginDescriptor.description;
        desc.features = cabs::pluginDescriptor.features;

        if (index == 0)
            return &desc;

        return nullptr;
    }

    const clap_plugin* createPluginInstance(const clap_plugin_factory* /*factory*/, const clap_host* host, const char* plugin_id)
    {
        if (strcmp(plugin_id, descriptor.id))
        {
            std::cerr << "Error: plugin_id '" << plugin_id << "' not found!" << std::endl;
            return nullptr;
        }

        // Host will own 'plugin'
        auto plugin = CawProcessorPluginFactory::createPlugin(host); // No need for user to specify inputs/outputs
                
        return plugin->clapPlugin();
    }

    const clap_plugin_factory factoryStruct =
    {
        .get_plugin_count = getPluginCount,
        .get_plugin_descriptor = getPluginDescriptor,
        .create_plugin = createPluginInstance,
    };

    const void* getFactory(const char* factory_id)
    {
      if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        return &factoryStruct;
        }
        return nullptr;
    }

}
