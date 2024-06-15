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
    }
    
    void OpenFileBrowser();
    void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
    void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override;
    
    void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
    void SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData) override;
    
    void SendMidiMsgFromDelegate(const iplug::IMidiMsg& msg) override;
    void OnMessageFromWebView(const char* jsonStr) override;

    void OnMidiMsgUI(const iplug::IMidiMsg& msg) override
    {
        cabAssert(false, "false");
    }
    
    void Resize(int width, int height);
    
    void OnWebViewReady() override
    {
        if (editorInitFunc)
            editorInitFunc();
    }
    
    void OnWebContentLoaded() override
    {
        OnUIOpen();
        std::string result = StringFormatter::format(R"(

                                                     console.log("dfghjkl;';lkjhgfdfghjklkjhgfvjkl;")
 const cabbageCode = `<>`;
setTimeout(function(){
 window.postMessage({ command: "onFileChanged", text: cabbageCode });
}, 100);

                                                     )", CabbageFile::getFileAsString());
        EvaluateJavaScript(result.c_str());
        Resize(1800,800);
        if (editorOnLoad)
            editorOnLoad();
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
    std::function<void()> editorInitFunc = nullptr;
    std::function<void()> editorDeleteFunc = nullptr;
    std::function<void()> editorOnLoad = nullptr;
    std::function<void(std::string, std::string)> updateStringChannel = nullptr;
    std::function<void(std::string, float)> updateChannel = nullptr;
    void* mHelperView = nullptr;
    std::string selectedFilePath;
    
private:
    bool mEnableDevTools = false;

};


