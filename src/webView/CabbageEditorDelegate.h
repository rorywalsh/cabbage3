/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "IPlugEditorDelegate.h"
#include "IPlugWebView.h"
#include "wdl_base64.h"
#include "json.hpp"
#include <functional>
#include "CabbageUtils.h"



/** This Editor Delegate allows using a platform native web view as the UI for an iPlug plugin */
class CabbageEditorDelegate : public iplug::IEditorDelegate
, public iplug::IWebView
{
    static constexpr int kDefaultMaxJSStringLength = 1024;
    
public:
    CabbageEditorDelegate(int nParams);
    virtual ~CabbageEditorDelegate();
    
    //IEditorDelegate
    void* OpenWindow(void* pParent) override;
    void CloseWindow() override
    {
        CloseWebView();
        if (editorCloseCallback)
            editorCloseCallback();
    }
    
    void OpenFileBrowser();
    void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override
    {
        WDL_String str;
        str.SetFormatted(mMaxJSStringLength, "SCVFD(%i, %f)", ctrlTag, normalizedValue);
        EvaluateJavaScript(str.Get());
    }

    void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override
    {
    }

    void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override
    {
        WDL_String str;
        str.SetFormatted(mMaxJSStringLength, "if (typeof sendParameterValueFromEditor !== \"undefined\") sendParameterValueFromEditor(%i, %f);", paramIdx, value);
        EvaluateJavaScript(str.Get());
    }

    void SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
    {
    }

    void SendMidiMsgFromDelegate(const iplug::IMidiMsg& msg) override
    {
        std::string message =  cabbage::StringFormatter::format(R"(
        
          // Create a custom event with the MIDI message details
          let customEvent = new CustomEvent('midiEvent', {
            detail: {
              data: JSON.stringify({
                status: <>,
                data1: <>,
                data2: <>
              })
            }
          });

          // Dispatch the custom event
          document.dispatchEvent(customEvent);
        
        )", std::to_string(msg.mStatus), std::to_string(msg.mData1), std::to_string(msg.mData1));
        EvaluateJavaScript(message.c_str());
    }

    /* this method is called from the webview. It will trigger various callback
     functions which are hosted from the CabbageProcessor class*/
    void OnMessageFromWebView(const char* jsonStr) override
    {
        auto incomingJson = nlohmann::json::parse(jsonStr, nullptr, false);
        const std::string command = incomingJson["command"];
        

        //this is called whenever a UI parameter is updated
        if(command == "parameterChange")
        {
            auto jsonContent = nlohmann::json::parse(incomingJson["obj"].get<std::string>());
            
            if(jsonContent["channelType"] == "string")
                updateStringChannelCallback(jsonContent["channel"], jsonContent["value"].get<std::string>());
            else
                SendParameterValueFromUI(jsonContent["paramIdx"], jsonContent["value"]);
            
        }
        
        //this is called to trigger a native OS file browser
        else if(command == "fileOpen")
        {
            auto jsonContent = nlohmann::json::parse(incomingJson["obj"].get<std::string>());
            OpenFileBrowser();
            updateStringChannelCallback(jsonContent["channel"], selectedFilePath);
        }
        
        //called whenever the state of a widget is updated in the UI - the C++ widget array
        //and the JS widget array should always be in sync
        else if(command == "widgetStateUpdate")
        {
            auto jsonContent = nlohmann::json::parse(incomingJson["obj"].get<std::string>());
            updateWidgetStateCallback(jsonContent);
        }
        
        else if(command == "cabbageSetupComplete")
        {
            interfaceHasLoadedCallback();
        }
        
        else if(command == "cabbageIsReadyToLoad")
        {
            cabbageIsReadyToLoadCsdCallback();
        }
    //    else if(json["msg"] == "fileRead")
    //    {
    //        readAudioFileCallback(json["channel"], selectedFilePath);
    //    }
    //    else if (json["msg"] == "BPCFUI")
    //    {
    //        BeginInformHostOfParamChangeFromUI(json["paramIdx"]);
    //    }
    //    else if (json["msg"] == "EPCFUI")
    //    {
    //        EndInformHostOfParamChangeFromUI(json["paramIdx"]);
    //    }
    //    else if (json["msg"] == "SAMFUI")
    //    {
    //        std::vector<unsigned char> base64;
    //
    //        if(json.count("data") > 0 && json["data"].is_string())
    //        {
    //            auto dStr = json["data"].get<std::string>();
    //            int dSize = static_cast<int>(dStr.size());
    //
    //            // calculate the exact size of the decoded base64 data
    //            int numPaddingBytes = 0;
    //
    //            if(dSize >= 2 && dStr[dSize-2] == '=')
    //                numPaddingBytes = 2;
    //            else if(dSize >= 1 && dStr[dSize-1] == '=')
    //                numPaddingBytes = 1;
    //
    //
    //            base64.resize((dSize * 3) / 4 - numPaddingBytes);
    //            wdl_base64decode(dStr.c_str(), base64.data(), static_cast<int>(base64.size()));
    //        }
    //
    //        SendArbitraryMsgFromUI(json["msgTag"], json["ctrlTag"], static_cast<int>(base64.size()), base64.data());
    //    }
        else if(command == "midiMessage")
        {
            auto jsonContent = nlohmann::json::parse(incomingJson["obj"].get<std::string>());
            iplug::IMidiMsg msg {0, jsonContent["statusByte"].get<uint8_t>(),
                jsonContent["dataByte1"].get<uint8_t>(),
                jsonContent["dataByte2"].get<uint8_t>()};
            SendMidiMsgFromUI(msg);
        }
    }


    void OnMidiMsgUI(const iplug::IMidiMsg& msg) override
    {
        cabAssert(false, "false");
    }
    
    void Resize(int width, int height);
    
    void OnWebViewReady() override
    {
        if (editorInitFuncCallback)
            editorInitFuncCallback();
    }
    
    void OnWebContentLoaded() override {
        OnUIOpen();
        if (editorOnLoadCallback)
            editorOnLoadCallback();
    }
    
    void SetMaxJSStringLength(int length)
    {
        mMaxJSStringLength = length;
    }
    
    void SetEnableDevTools(bool enable)
    {
        mEnableDevTools = enable;
    }
    
    bool GetEnableDevTools() const { return mEnableDevTools; }

    
protected:
    
    int GetBase64Length(int dataSize)
    {
        return static_cast<int>(4. * std::ceil((static_cast<double>(dataSize) / 3.)));
    }
    
    int mMaxJSStringLength = kDefaultMaxJSStringLength;
    std::function<void()> editorInitFuncCallback = nullptr;
    std::function<void()> editorDeleteFuncCallback = nullptr;
    std::function<void()> editorCloseCallback = nullptr;
    std::function<void()> editorOnLoadCallback = nullptr;
    std::function<void(std::string, std::string)> readAudioFileCallback = nullptr;
    std::function<void(std::string, std::string)> updateStringChannelCallback = nullptr;
    std::function<void(nlohmann::json)> updateWidgetStateCallback = nullptr;
    std::function<void()> interfaceHasLoadedCallback = nullptr;
    std::function<void()> cabbageIsReadyToLoadCsdCallback = nullptr;
    std::function<void(std::string, float)> updateChannelCallback = nullptr;
    void* mHelperView = nullptr;
    std::string selectedFilePath;
    
private:
    bool mEnableDevTools = false;

};


