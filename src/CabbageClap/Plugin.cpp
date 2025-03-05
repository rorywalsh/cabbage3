#include "Plugin.h"
#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hxx>
#include <clap/ext/params.h>
#include "Utils.h"
#include "gui/choc_WebView.h"
#include "../Cumhdach.h"

#define CABBAGE_WINDOWS 1

#if CABBAGE_WINDOWS
#include <windows.h>
#elif CABBAGE_MACOS
extern "C"
{
    bool attachViewToParent(void *childView, void *parentView); // Forward declaration
}
#elif CABBAGE_LINUX
#include <X11/Xlib.h>
#endif



ClapPlugin::ClapPlugin(const clap_host* host, int numInputs, int numOutputs)
: clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>(
    &descriptor, host)
{
    cumhdachProcessor = new Cumhdach(numInputs, numOutputs);
}

ClapPlugin::~ClapPlugin()
{
    
}


bool ClapPlugin::audioPortsInfo(uint32_t index, bool /*isInput*/, clap_audio_port_info* info) const noexcept
{
    if (index != 0)
        return false;


    info->id = 0;
    info->in_place_pair = CLAP_INVALID_ID;
    strncpy(info->name, "main", sizeof(info->name));
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = cumhdachProcessor->getNumOutputs();
    info->port_type = CLAP_PORT_STEREO;

    return true;
}

bool ClapPlugin::paramsInfo(uint32_t paramIndex, clap_param_info* info) const noexcept
{
    auto numParameters = cumhdachProcessor->getParameters().size();
    
    if (paramIndex >= numParameters)
        return false;


    const auto p = cumhdachProcessor->getParameters()[paramIndex];

    info->id = paramIndex;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
    strncpy(info->name, p.name, CLAP_NAME_SIZE);
    strncpy(info->module, "", CLAP_NAME_SIZE);
    info->min_value = p.min;
    info->max_value = p.min;
    info->default_value = p.value;//utils::decibelsToGain(0.0);


    return true;
}

bool ClapPlugin::paramsValue(clap_id paramId, double* value) noexcept
{
    if (paramId != gainPrmId_)
        return false;

//    *value = utils::toLinearCurve(gain_);
    return true;
}

bool ClapPlugin::paramsValueToText(clap_id paramId, double value, char* display, uint32_t size) noexcept
{
    if (paramId != gainPrmId_)
        return false;

    const auto valueIndB = utils::gainToDecibels(utils::toExponentialCurve(value));

    if (valueIndB <= utils::minusInfinitydB)
    {
        snprintf(display, size, "-inf dB");
    }
    else
    {
        snprintf(display, size, "%.2f dB", valueIndB);
    }

    return true;
}

bool ClapPlugin::paramsTextToValue(clap_id paramId, const char* display, double* value) noexcept
{
    if (paramId != gainPrmId_)
        return false;

    const double value_ = strtod(display, nullptr);
    *value = utils::toLinearCurve(utils::decibelsToGain(value_));

    return true;
}

bool ClapPlugin::activate(double sampleRate, uint32_t /*minFrameCount*/, uint32_t /*maxFrameCount*/) noexcept {
    return true;
}

clap_process_status ClapPlugin::process(const clap_process* process) noexcept 
{
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_CONTINUE;

    // Call the CabbageProcessor's process method
    float** inputs = process->audio_inputs[0].data32;
    float** outputs = process->audio_outputs[0].data32;
    std::size_t blockSize = process->frames_count;

    cumhdachProcessor->process(inputs, outputs, blockSize);
    return CLAP_PROCESS_CONTINUE;



    // Handle parameter changes
    // auto event = process->in_events;
    // for (uint32_t i = 0; i < event->size(event); ++i) 
    // {
    //     auto nextEvent = event->get(event, i);
    //     if (nextEvent->space_id == CLAP_CORE_EVENT_SPACE_ID && 
    //         nextEvent->type == CLAP_EVENT_PARAM_VALUE) 
    //     {
    //         auto gainValue = reinterpret_cast<const clap_event_param_value*>(nextEvent);
    //         if (gainValue->param_id == gainPrmId_) 
    //         {
    //             gain_ = utils::toExponentialCurve(gainValue->value);
    //             if (webview)
    //                 webview->evaluateJavascript("updateGainFromHost(" + 
    //                     std::to_string(utils::gainToDecibels(gain_)) + ");");
    //         }
    //     }
    // }

    // // Process audio
    // float** input = process->audio_inputs[0].data32;
    // float** output = process->audio_outputs[0].data32;
    // const auto channels = process->audio_outputs->channel_count;
    
    // for (uint32_t i = 0; i < process->frames_count; i++) {
    //     for (uint32_t ch = 0; ch < channels; ++ch)
    //         output[ch][i] = input[ch][i] * gain_;
    // }

    // return CLAP_PROCESS_CONTINUE;

}

bool ClapPlugin::guiIsApiSupported(const char* api, bool isFloating) noexcept {
    // We support embedded and floating windows
    return strcmp(api, CLAP_WINDOW_API_WIN32) == 0 ||
           strcmp(api, CLAP_WINDOW_API_COCOA) == 0 ||
           strcmp(api, CLAP_WINDOW_API_X11) == 0;
}

bool ClapPlugin::guiCreate(const char* api, bool isFloating) noexcept {
    try {
        choc::ui::WebView::Options options;
        options.enableDebugMode = true;
        
        webview = std::make_unique<choc::ui::WebView>(options);
        if (!webview)
            return false;

        // Add JavaScript interface for parameter control
        webview->bind("setGainParameter", [this](const choc::value::ValueView& args) -> choc::value::Value {
            auto value = args[0]["value"].getWithDefault<double>(0.0);
            sendParameterValueToHost(gainPrmId_, utils::decibelsToGain(value));
            return {};
        });

        // Load HTML content
        webview->setHTML(R"(
            <!DOCTYPE html>
            <html>
            <head>
                <style>
                    body { 
                        background: #2d2d2d;
                        color: white;
                        font-family: Arial, sans-serif;
                        margin: 0;
                        padding: 20px;
                        box-sizing: border-box;
                        display: flex;
                        flex-direction: column;
                        align-items: center;
                    }
                    .control-group {
                        margin: 20px;
                        text-align: center;
                    }
                    .slider {
                        width: 200px;
                        margin: 10px;
                    }
                    .value-display {
                        font-family: monospace;
                    }
                </style>
            </head>
            <body>
                <h1>Gain Control</h1>
                <div class="control-group">
                    <input type="range" class="slider" id="gainSlider"
                           min="-70" max="12" step="0.1" value="0">
                    <div class="value-display">
                        <span id="gainValue">0.0</span> dB
                    </div>
                </div>

                <script>
                    const gainSlider = document.getElementById('gainSlider');
                    const gainValue = document.getElementById('gainValue');

                    gainSlider.addEventListener('input', function() {
                        const value = parseFloat(this.value);
                        gainValue.textContent = value.toFixed(1);
                        window.setGainParameter({
                            param: "gain",
                            value: value,
                            unit: "dB"
                        });
                    });

                    function updateGainFromHost(value) {
                        gainSlider.value = value;
                        gainValue.textContent = value.toFixed(1);
                    }
                </script>
            </body>
            </html>
        )");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in guiCreate: " << e.what() << std::endl;
        return false;
    }
}

void ClapPlugin::guiDestroy() noexcept {
    webview.reset();
}

bool ClapPlugin::guiSetScale(double) noexcept {
    return true;
}

bool ClapPlugin::guiSetSize(uint32_t width, uint32_t height) noexcept {
    currentWidth_ = width;
    currentHeight_ = height;
    return webview != nullptr;
}

bool ClapPlugin::guiGetSize(uint32_t* width, uint32_t* height) noexcept {
    *width = currentWidth_;
    *height = currentHeight_;
    return true;
}

bool ClapPlugin::guiShow() noexcept {
    return webview != nullptr;
}

bool ClapPlugin::guiHide() noexcept {
    return webview != nullptr;
}

bool ClapPlugin::guiSetParent(const clap_window *window) noexcept
{
    if (!webview)
    {
        utils::DebugLog("WebView not created when setting parent");
        return false;
    }

    try
    {
#if CABBAGE_WINDOWS
        if (strcmp(window->api, CLAP_WINDOW_API_WIN32) == 0)
        {
            auto *child = static_cast<HWND>(webview->getViewHandle());
            auto *parent = static_cast<::HWND>(window->win32);
            ::InvalidateRect(child, NULL, false);
            ::SetWindowLongPtrW(child, GWL_STYLE, WS_CHILD);
            ::SetParent(child, parent);
            ::ShowWindow(child, SW_SHOW);

            return true;
        }
#elif CABBAGE_MACOS
        utils::DebugLog("Setting parent for API: " + std::string(window->api));
        if (strcmp(window->api, CLAP_WINDOW_API_COCOA) == 0)
        {
            void *parent = window->cocoa;
            void *child = viewHandle;
            utils::DebugLog("Parent handle: " + std::to_string(reinterpret_cast<uintptr_t>(parent)) +
                            ", Child handle: " + std::to_string(reinterpret_cast<uintptr_t>(child)));
            bool result = attachViewToParent(child, parent);
            utils::DebugLog("Parent attachment result: " + std::to_string(result));
            return result;
        }
#elif CABBAGE_LINUX
        if (strcmp(window->api, CLAP_WINDOW_API_X11) == 0)
        {
            XReparentWindow(XOpenDisplay(nullptr), (Window)viewHandle, (Window)window->x11, 0, 0);
            return true;
        }
#endif

        return false;
    }
    catch (const std::exception &e)
    {
        utils::DebugLog("Exception in guiSetParent: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        utils::DebugLog("Unknown exception in guiSetParent");
        return false;
    }
}

// Utility function to send parameter changes to host
void ClapPlugin::sendParameterValueToHost(clap_id paramId, double value) noexcept {
    if (auto* host = _host.host()) {
        if (auto* params = (const clap_host_params*) host->get_extension(host, CLAP_EXT_PARAMS)) {
            gain_ = value;
            params->request_flush(host);
        }
    }
}
