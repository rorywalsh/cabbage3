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
#include <winrt/Windows.System.h>
#include <dispatcherqueue.h>
#include <winrt/base.h>  // For winrt::com_ptr
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
    
    /** Load an HTML string into the webview */
    void LoadHTML(const char* html);
    
    /** Instruct the webview to load an external URL */
    void LoadURL(const char* url);
    
    /** Load a file on disk into the web view
     * @param fileName On windows this should be an absolute path to the file you want to load. On macOS/iOS it can just be the file name if the file is packaged into a subfolder "web" of the bundle resources
     * @param bundleID The NSBundleID of the macOS/iOS bundle, not required on Windows */
    void LoadFile(const char* fileName, const char* bundleID = "");
    
    /** Runs some JavaScript in the webview
     * @param scriptStr UTF8 encoded JavaScript code to run
     * @param func A function conforming to completionHandlerFunc that should be called on successful execution of the script */
#ifdef _WIN32
    void EvaluateJavaScriptOnMainThread(const char* scriptStr, completionHandlerFunc func);
    void EvaluateJavaScript(const std::string& script);
#else
    void EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func = nullptr);
#endif // _WIN32

    
    /** Enable scrolling on the webview. NOTE: currently only implemented for iOS */
    void EnableScroll(bool enable);
    
    /** Sets whether the webview is interactive */
    void EnableInteraction(bool enable);
    
    /** Set the bounds of the webview in the parent window. xywh are specifed in relation to a 1:1 non retina screen */
    void SetWebViewBounds(float x, float y, float w, float h, float scale = 1.);
    
    /** Called when the web view is ready to receive navigation instructions*/
    virtual void OnWebViewReady() {}
    
    /** Called after navigation instructions have been exectued and e.g. a page has loaded */
    virtual void OnWebContentLoaded() {}
    
    /** When a script in the web view posts a message, it will arrive as a UTF8 json string here */
    virtual void OnMessageFromWebView(const char* json) {}
    
private:
    bool mOpaque = true;
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
#endif
};

END_IPLUG_NAMESPACE
