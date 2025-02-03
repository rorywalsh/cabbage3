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
#include "CabbageUtils.h"

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
#include <iostream>
#include <sys/mman.h>    // For mmap, munmap
#include <sys/stat.h>    // For shm_open
#include <fcntl.h>       // For O_RDWR, O_CREAT
#include <unistd.h>      // For close
class MessagePipeHost {
public:
    enum MessageType{
        LoadUrl = 0,
        EvaluateJS
    };

    MessagePipeHost() : pipe_fd(-1) {
    }

    ~MessagePipeHost() {
        if (pipe_fd != -1) {
            close(pipe_fd);
        }
    }

    void createPipe(const char* name) {
        pipeName = name;
        // Create the named pipe (FIFO) if it doesn't exist
        if (mkfifo(name, 0666) == -1) {
            if (errno != EEXIST) {
                perror("mkfifo failed");
                exit(1);
            } else {
                cabbage::logInfo << "Pipe already exists: " << pipeName;
            }
        } else {
            cabbage::logInfo << "Pipe created: " << pipeName;
        }
    }

    bool isOpenForWriting(bool shouldWait = false) 
    {
        int count = 0;
        if (!openForWriting) 
        {
            // In some cases we simply have to wait for the child pipe to be
            // ready - when we first load the UI for example
            if(shouldWait)
            {
                while (count<1000) 
                {
                    pipe_fd = open(pipeName, O_WRONLY | O_NONBLOCK);

                    if (pipe_fd == -1) 
                    {
                        cabbage::logInfo << "Failed to open pipe for writing, retrying...";
                        count++;
                        sleep(.2);
                    } 
                    else 
                    {
                        cabbage::logInfo << "Pipe opened for writing: " << pipeName;
                        openForWriting = true;
                        return true;
                    }
                }
                cabbage::logDebug << "Pipe couldn't be opened for writing: " << pipeName;
            }
            else
            {
                pipe_fd = open(pipeName, O_WRONLY | O_NONBLOCK);

                if (pipe_fd == -1)
                {
                    cabbage::logInfo << "Failed to open pipe for writing, retrying...";
                } 
                else 
                {
                    cabbage::logInfo << "Pipe opened for writing: " << pipeName;
                    openForWriting = true;
                    return true;
                }
            }
        }

        return true;  // If already open, return true
    }

    void send(MessageType type, const std::string& message) 
    {
        if (pipe_fd == -1) 
        {
            cabbage::logInfo << "Pipe is not open, cannot send message!";
            return;
        }

        nlohmann::json jsonMessage;
        std::string formattedMessage;

        if (type == MessageType::LoadUrl)
        {
          jsonMessage["command"] = "LoadUrl";
          jsonMessage["data"] = message;
        } 
        else if (type == MessageType::EvaluateJS) 
        {
          jsonMessage["command"] = "EvaluateJS";
          jsonMessage["data"] = message;
        }

        cabbage::logInfo << "Sending message: " << jsonMessage.dump(4);

        ssize_t bytesWritten = write(pipe_fd, jsonMessage.dump().c_str(), jsonMessage.dump().length());
        if (bytesWritten == -1) 
        {
            perror("write to pipe failed");
        } 
        else 
        {
            cabbage::logInfo << "Message sent to pipe: " << formattedMessage;
        }
    }

private:
    const char* pipeName;
    int pipe_fd;
    bool openForWriting = false;
};

#include <fstream>
#include <memory>
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
    void EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func = nullptr);


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
    pid_t pid = 1;
    MessagePipeHost messagePipe;
#endif
};

END_IPLUG_NAMESPACE
