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

#include "IPlugWebView.h"
#include "IPlugPaths.h"
#include <string>
#include <windows.h>
#include <shlobj.h>
#include <cassert>

using namespace iplug;
using namespace Microsoft::WRL;

extern float GetScaleForHWND(HWND hWnd);

IWebView::IWebView(bool opaque)
: mOpaque(opaque)
{
}

IWebView::~IWebView()
{
  CloseWebView();
}

typedef HRESULT(*TCCWebView2EnvWithOptions)(
  PCWSTR browserExecutableFolder,
  PCWSTR userDataFolder,
  PCWSTR additionalBrowserArguments,
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environment_created_handler);

void* IWebView::OpenWebView(void* pParent, float x, float y, float w, float h, float scale, bool enableDevTools)
{
  mParentWnd = (HWND)pParent;

  float ss = GetScaleForHWND(mParentWnd);

  x *= ss;
  y *= ss;
  w *= ss;
  h *= ss;

  WDL_String cachePath;
  WebViewCachePath(cachePath);
  WCHAR cachePathWide[IPLUG_WIN_MAX_WIDE_PATH];
  UTF8ToUTF16(cachePathWide, cachePath.Get(), IPLUG_WIN_MAX_WIDE_PATH);

  CreateCoreWebView2EnvironmentWithOptions(
    nullptr, cachePathWide, nullptr,
  Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
    [&, x, y, w, h](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
      env
        ->CreateCoreWebView2Controller(
          mParentWnd,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
          [&, x, y, w, h, enableDevTools](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
            if (controller != nullptr)
            {
              mWebViewCtrlr = controller;
              mWebViewCtrlr->get_CoreWebView2(&mWebViewWnd);
            }

            mWebViewCtrlr->put_IsVisible(mShowOnLoad);

            ICoreWebView2Settings* Settings;
            mWebViewWnd->get_Settings(&Settings);
            Settings->put_IsScriptEnabled(TRUE);
            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            Settings->put_IsWebMessageEnabled(TRUE);
            Settings->put_AreDefaultContextMenusEnabled(enableDevTools);
            Settings->put_AreDevToolsEnabled(enableDevTools);

            // this script adds a function IPlugSendMsg that is used to call the platform webview messaging function in JS
            mWebViewWnd->AddScriptToExecuteOnDocumentCreated(
              L"function IPlugSendMsg(m) {window.chrome.webview.postMessage(m)};",
              Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>([this](HRESULT error,
                                                                                                PCWSTR id) -> HRESULT {
                return S_OK;
              }).Get());

            mWebViewWnd->add_WebMessageReceived(
              Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) {
                  wil::unique_cotaskmem_string jsonString;
                  args->get_WebMessageAsJson(&jsonString);
                  std::wstring jsonWString = jsonString.get();
                  WDL_String cStr;
                  UTF16ToUTF8(cStr, jsonWString.c_str());
                  OnMessageFromWebView(cStr.Get());
                  return S_OK;
                }).Get(), &mWebMessageReceivedToken);

            mWebViewWnd->add_NavigationCompleted(
              Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                  BOOL success;
                  args->get_IsSuccess(&success);
                  if (success)
                  {
                    OnWebContentLoaded();
                  }
                  return S_OK;
                })
              .Get(), &mNavigationCompletedToken);

            if (!mOpaque)
            {
              wil::com_ptr<ICoreWebView2Controller2> controller2 = mWebViewCtrlr.query<ICoreWebView2Controller2>();
              COREWEBVIEW2_COLOR color;
              memset(&color, 0, sizeof(COREWEBVIEW2_COLOR));
              controller2->put_DefaultBackgroundColor(color);
            }

            mWebViewCtrlr->put_Bounds({ (LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h) });
            OnWebViewReady();
            return S_OK;
          }).Get());
      return S_OK;
    }).Get());

  return mParentWnd;
}

void IWebView::CloseWebView()
{
  if (mWebViewCtrlr.get() != nullptr)
  {
    mWebViewCtrlr->Close();
    mWebViewCtrlr = nullptr;
    mWebViewWnd = nullptr;
  }
}

void IWebView::HideWebView(bool hide)
{
  if (mWebViewCtrlr.get() != nullptr)
  {
    mWebViewCtrlr->put_IsVisible(!hide);
  }
  else
  {
    // the controller is set asynchonously, so we store the state 
    // to apply it when the controller is created
    mShowOnLoad = !hide;
  }
}

void IWebView::LoadHTML(const char* html)
{
  if (mWebViewWnd)
  {
    WCHAR htmlWide[IPLUG_WIN_MAX_WIDE_PATH]; // TODO: error check/size
    UTF8ToUTF16(htmlWide, html, IPLUG_WIN_MAX_WIDE_PATH); // TODO: error check/size
    mWebViewWnd->NavigateToString(htmlWide);
  }
}

void IWebView::LoadURL(const char* url)
{
  //TODO: error check url?
  if (mWebViewWnd)
  {
    WCHAR urlWide[IPLUG_WIN_MAX_WIDE_PATH]; // TODO: error check/size
    UTF8ToUTF16(urlWide, url, IPLUG_WIN_MAX_WIDE_PATH); // TODO: error check/size
    mWebViewWnd->Navigate(urlWide);
  }
}

void IWebView::LoadFile(const char* fileName, const char* bundleID)
{
  if (mWebViewWnd)
  {
    WDL_String fullStr;
    fullStr.SetFormatted(MAX_WIN32_PATH_LEN, "file://%s", fileName);
    WCHAR fileUrlWide[IPLUG_WIN_MAX_WIDE_PATH]; // TODO: error check/size
    UTF8ToUTF16(fileUrlWide, fullStr.Get(), IPLUG_WIN_MAX_WIDE_PATH); // TODO: error check/size
    mWebViewWnd->Navigate(fileUrlWide);
  }
}

void IWebView::EvaluateJavaScript(const std::string& script)
{
#if defined(_WIN32)
    if (auto dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread())
    {
        // Dispatch the EvaluateJavaScript call to the main thread
        dispatcherQueue.TryEnqueue([this, script]()
        {
            EvaluateJavaScriptOnMainThread(script.c_str(), nullptr); // Assuming completionHandlerFunc is not needed
        });
    }
    else
    {
        // If no dispatcher is available, log or handle the error
        //std::cerr << "Dispatcher queue not available!" << std::endl;
    }
#else
    EvaluateJavaScript(script.c_str());
#endif
}

void IWebView::EvaluateJavaScriptOnMainThread(const char* scriptStr, completionHandlerFunc func)
{
  if (mWebViewWnd)
  {
    WCHAR scriptWide[IPLUG_WIN_MAX_WIDE_PATH]; // TODO: error check/size
    UTF8ToUTF16(scriptWide, scriptStr, IPLUG_WIN_MAX_WIDE_PATH); // TODO: error check/size

    mWebViewWnd->ExecuteScript(scriptWide, Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
      [func](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
        if (func && resultObjectAsJson) {
          WDL_String str;
          UTF16ToUTF8(str, resultObjectAsJson);
          func(str.Get());
        }
        return S_OK;
      }).Get());
  }
}

void IWebView::EnableScroll(bool enable)
{
  /* NO-OP */
}

void IWebView::EnableInteraction(bool enable)
{
  /* NO-OP */
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
  if (mWebViewCtrlr)
  {
    float ss = GetScaleForHWND(mParentWnd);

    x *= ss;
    y *= ss;
    w *= ss;
    h *= ss;

    mWebViewCtrlr->SetBoundsAndZoomFactor({ (LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h) }, scale);
  }
}
