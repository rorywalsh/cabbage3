/*
 * Copyright (C) the iPlug 2 developers, Rory Walsh (c) 2024
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 * 
 * Modifications made by Rory Walsh in 2024.
 * 
 * This file is based on the iPlug 2 library, which is licensed under the
 * [iPlug 2 License Information]. The original copyright notice and license
 * must remain intact in the portions of the code that have not been modified.
 */

#pragma once
#undef OK

#include "IPlugPlatform.h"
#include "wdlstring.h"
#include <functional>
#include <atomic>

#if defined OS_MAC
#define PLATFORM_VIEW NSView
#define PLATFORM_RECT NSRect
#define MAKERECT NSMakeRect
#elif defined OS_IOS
#define PLATFORM_VIEW UIView
#define PLATFORM_RECT CGRect
#define MAKERECT CGRectMake
#elif defined OS_WIN
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"

#else //__linux__
struct WebViewData;
typedef WebViewData* WebViewHandle;
typedef void (*WebViewMessageCallback)(void* arg, char* msg);
#endif

BEGIN_IPLUG_NAMESPACE

using completionHandlerFunc = std::function<void(const char* result)>;

/** IWebView is a base interface for hosting a platform web view inside an IPlug plug-in's UI */
class IWebView
{
public:
    IWebView(bool opaque = true);
    virtual ~IWebView();
    
    void* OpenWebView(void* pParent, float x, float y, float w, float h, float scale = 1.0f, bool enableDevTools = true);
    void CloseWebView();
    void HideWebView(bool hide);
    
    void LoadHTML(const char* html);
    void LoadURL(const char* url);
    void LoadFile(const char* fileName, const char* bundleID = "");
    void EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func = nullptr);
    void EnableScroll(bool enable);
    void EnableInteraction(bool enable);
    void SetWebViewBounds(float x, float y, float w, float h, float scale = 1.);
    void ProcessEvents();

    virtual void OnWebViewReady() {}
    virtual void OnWebContentLoaded() {}
    virtual void OnMessageFromWebView(const char* json) {}

private:
    bool mOpaque = true;
    std::atomic<bool> should_exit_{false};

#if defined OS_MAC || defined OS_IOS
    void* mWKWebView = nullptr;
    void* mWebConfig = nullptr;
    void* mScriptHandler = nullptr;
#elif defined OS_WIN
    HWND mParentWnd = NULL;
    wil::com_ptr<ICoreWebView2Controller> mWebViewCtrlr;
    wil::com_ptr<ICoreWebView2> mWebViewWnd;
    EventRegistrationToken mWebMessageReceivedToken;
    EventRegistrationToken mNavigationCompletedToken;
    EventRegistrationToken mContextMenuRequestedToken;
    bool mShowOnLoad = true;
#else //__linux__
//    WebKitWebContext* webviewContext = {};
//    GtkWidget* webview = {};
//    WebKitUserContentManager* manager = {};
//    unsigned long signalHandlerID = 0;
#endif
};

END_IPLUG_NAMESPACE
