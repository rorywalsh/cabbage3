/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#if __has_feature(objc_arc)
#error This file must be compiled without Arc. Don't use -fobjc-arc flag!
#endif



#include "CabbageEditorDelegate.h"

#ifdef OS_MAC
#import <AppKit/AppKit.h>
#elif defined(OS_IOS)
#import <UIKit/UIKit.h>
#endif

using namespace iplug;

@interface HELPER_VIEW : PLATFORM_VIEW
{
    CabbageEditorDelegate* mDelegate;
}
- (void) removeFromSuperview;
- (id) initWithEditorDelegate: (CabbageEditorDelegate*) pDelegate;
@end

@implementation HELPER_VIEW
{
}

- (id) initWithEditorDelegate: (CabbageEditorDelegate*) pDelegate;
{
    mDelegate = pDelegate;
    CGFloat w = pDelegate->GetEditorWidth();
    CGFloat h = pDelegate->GetEditorHeight();
    CGRect r = CGRectMake(0, 0, w, h);
    self = [super initWithFrame:r];
    
    void* pWebView = pDelegate->OpenWebView(self, 0, 0, w, h, 1.0f, pDelegate->GetEnableDevTools());
    
    [self addSubview: (PLATFORM_VIEW*) pWebView];
    
    return self;
}

- (void) removeFromSuperview
{
#ifdef AU_API
    //For AUv2 this is where we know about the window being closed, close via delegate
    mDelegate->CloseWindow();
#endif
    [super removeFromSuperview];
}

@end

CabbageEditorDelegate::CabbageEditorDelegate(int nParams)
: IEditorDelegate(nParams)
, IWebView()
{
}

CabbageEditorDelegate::~CabbageEditorDelegate()
{
    CloseWindow();
    
    if (editorDeleteFunc)
        editorDeleteFunc();
    
    PLATFORM_VIEW* pHelperView = (PLATFORM_VIEW*) mHelperView;
    [pHelperView release];
    mHelperView = nullptr;
}

void* CabbageEditorDelegate::OpenWindow(void* pParent)
{
    PLATFORM_VIEW* pParentView = (PLATFORM_VIEW*) pParent;
    
    HELPER_VIEW* pHelperView = [[HELPER_VIEW alloc] initWithEditorDelegate: this];
    mHelperView = (void*) pHelperView;
    
    if (pParentView) {
        [pParentView addSubview: pHelperView];
    }
    
    if (editorInitFunc)
        editorInitFunc();
 
        
    return mHelperView;
    
}

void CabbageEditorDelegate::Resize(int width, int height)
{
    CGFloat w = static_cast<float>(width);
    CGFloat h = static_cast<float>(height);
    HELPER_VIEW* pHelperView = (HELPER_VIEW*) mHelperView;
    [pHelperView setFrame:CGRectMake(0, 0, w, h)];
    SetWebViewBounds(0, 0, w, h);
    EditorResizeFromUI(width, height, true);
}

void CabbageEditorDelegate::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
    WDL_String str;
    str.SetFormatted(mMaxJSStringLength, "SCVFD(%i, %f)", ctrlTag, normalizedValue);
    EvaluateJavaScript(str.Get());
}

void CabbageEditorDelegate::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
    //  WDL_String str;
    //  std::vector<char> base64;
    //  base64.resize(GetBase64Length(dataSize) + 1);
    //  wdl_base64encode(reinterpret_cast<const unsigned char*>(pData), base64.data(), dataSize);
    //  str.SetFormatted(mMaxJSStringLength, "SCMFD(%i, %i, %i, '%s')", ctrlTag, msgTag, dataSize, base64.data());
    //  EvaluateJavaScript(str.Get());
}

void CabbageEditorDelegate::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{   
    WDL_String str;
    str.SetFormatted(mMaxJSStringLength, "if (typeof sendParameterValueFromEditor !== \"undefined\") sendParameterValueFromEditor(%i, %f);", paramIdx, value);
    EvaluateJavaScript(str.Get());
}

void CabbageEditorDelegate::SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
    //  WDL_String str;
    //  std::vector<char> base64;
    //  if (dataSize)
    //  {
    //    base64.resize(GetBase64Length(dataSize) + 1);
    //    wdl_base64encode(reinterpret_cast<const unsigned char*>(pData), base64.data(), dataSize);
    //  }
    //  str.SetFormatted(mMaxJSStringLength, "SAMFD(%i, %i, '%s')", msgTag, static_cast<int>(base64.size()), base64.data());
    //  EvaluateJavaScript(str.Get());
}

void CabbageEditorDelegate::SendMidiMsgFromDelegate(const iplug::IMidiMsg& msg)
{
      WDL_String str;
      str.SetFormatted(mMaxJSStringLength, "SMMFD(%i, %i, %i)", msg.mStatus, msg.mData1, msg.mData2);
      EvaluateJavaScript(str.Get());
}

void CabbageEditorDelegate::OnMessageFromWebView(const char* jsonStr)
{
    auto json = nlohmann::json::parse(jsonStr, nullptr, false);
    
    if(json["msg"] == "parameterUpdate")
    {
        SendParameterValueFromUI(json["paramIdx"], json["value"]);
    }
    else if (json["msg"] == "BPCFUI")
    {
        BeginInformHostOfParamChangeFromUI(json["paramIdx"]);
    }
    else if (json["msg"] == "EPCFUI")
    {
        EndInformHostOfParamChangeFromUI(json["paramIdx"]);
    }
    else if (json["msg"] == "SAMFUI")
    {
        std::vector<unsigned char> base64;
        
        if(json.count("data") > 0 && json["data"].is_string())
        {
            auto dStr = json["data"].get<std::string>();
            int dSize = static_cast<int>(dStr.size());
            
            // calculate the exact size of the decoded base64 data
            int numPaddingBytes = 0;
            
            if(dSize >= 2 && dStr[dSize-2] == '=')
                numPaddingBytes = 2;
            else if(dSize >= 1 && dStr[dSize-1] == '=')
                numPaddingBytes = 1;
            
            
            base64.resize((dSize * 3) / 4 - numPaddingBytes);
            wdl_base64decode(dStr.c_str(), base64.data(), static_cast<int>(base64.size()));
        }
        
        SendArbitraryMsgFromUI(json["msgTag"], json["ctrlTag"], static_cast<int>(base64.size()), base64.data());
    }
    else if(json["msg"] == "SMMFUI")
    {
        iplug::IMidiMsg msg {0, json["statusByte"].get<uint8_t>(),
            json["dataByte1"].get<uint8_t>(),
            json["dataByte2"].get<uint8_t>()};
        SendMidiMsgFromUI(msg);
    }
}

