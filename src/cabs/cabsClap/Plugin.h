#pragma once

#include <clap/helpers/plugin.hh>
#include "gui/choc_WebView.h"

class Processor;
class ClapPlugin;

using pluginType = ClapPlugin;
using pluginDescriptor = clap_plugin_descriptor;

class ClapPlugin : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Ignore,
                                            clap::helpers::CheckingLevel::Maximal>
{
public:
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

    ClapPlugin(const clap_host* host, int numInputs, int numOutputs);
    ~ClapPlugin() override;

    bool implementsAudioPorts() const noexcept override
    {
        return true;
    }

    uint32_t audioPortsCount(bool /*isInput*/) const noexcept override
    {
        return 1;
    }

    bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info* info) const noexcept override;

    bool implementsParams() const noexcept override
    {
        return true;
    }

    bool isValidParamId(clap_id paramId) const noexcept override
    {
        return paramId == gainPrmId_;
    }

    uint32_t paramsCount() const noexcept override
    {
        return 1;
    }

    bool paramsInfo(uint32_t paramIndex, clap_param_info* info) const noexcept override;

    bool paramsValue(clap_id paramId, double* value) noexcept override;

    bool paramsValueToText(clap_id paramId, double value, char* display, uint32_t size) noexcept override;
    bool paramsTextToValue(clap_id paramId, const char* display, double* value) noexcept override;
    bool activate(double sampleRate, uint32_t, uint32_t) noexcept override;

    clap_process_status process(const clap_process* process) noexcept override;

    // Add GUI implementation
    bool implementsGui() const noexcept override { return true; }
    bool guiIsApiSupported(const char* api, bool isFloating) noexcept override;
    bool guiCreate(const char* api, bool isFloating) noexcept override;
    void guiDestroy() noexcept override;
    bool guiSetScale(double scale) noexcept override;
    bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
    bool guiGetSize(uint32_t* width, uint32_t* height) noexcept override;
    bool guiShow() noexcept override;
    bool guiHide() noexcept override;
    bool guiSetParent(const clap_window* window) noexcept override;

private:
    static constexpr clap_id gainPrmId_ = 2345;

    double gain_ = 0.0;
    double targetGain_ = 0.0;
    double stepSizeToTargetGain_ = 0.0;
    int fadeLengthInSamples_ = 0;
    int currentFadeIndex_ = fadeLengthInSamples_;

    // Add GUI members
    std::unique_ptr<choc::ui::WebView> webview;
    uint32_t currentWidth_ = 800;  // Default width
    uint32_t currentHeight_ = 600; // Default height

    // Utility functions for parameter handling
    void sendParameterValueToHost(clap_id paramId, double value) noexcept;
    void beginParamAdjust(clap_id paramId) noexcept;
    void endParamAdjust(clap_id paramId) noexcept;

    Processor *CabsProcessor; // Pointer to CawProcessor processor
};

class CawProcessorPluginFactory {
public:
    static ClapPlugin* createPlugin(const clap_host* host);
};
