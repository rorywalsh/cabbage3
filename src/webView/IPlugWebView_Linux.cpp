#include "IPlugWebView.h"
#include <webkit2/webkit2.h>
#include <glib-object.h>

using namespace iplug;

IWebView::IWebView(bool opaque)
: mOpaque(opaque)
{
}

IWebView::~IWebView()
{
  CloseWebView();
}

void* IWebView::OpenWebView(void* pParent, float x, float y, float w, float h, float scale, bool enableDevTools)
{
  mParentWnd = (GtkWidget*)pParent;

  float ss = 1;//GetScaleForHWND(mParentWnd);

  x *= ss;
  y *= ss;
  w *= ss;
  h *= ss;

  // Create the WebView
  mWebViewCtrlr = webkit_web_view_new();

  // Get the WebKit settings object
  WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(mWebViewCtrlr));

  // Enable Developer Tools if requested
  if (enableDevTools)
  {
    g_object_set(G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);
  }

  // Set WebView size and make it visible
  gtk_widget_set_size_request(GTK_WIDGET(mWebViewCtrlr), (int)w, (int)h);
  gtk_widget_show(GTK_WIDGET(mWebViewCtrlr));

  // Return the parent window pointer or the WebView handle if required
  return mParentWnd;
}

void IWebView::CloseWebView()
{
  if (mWebViewCtrlr)
  {
    g_object_unref(mWebViewCtrlr);
    mWebViewCtrlr = nullptr;
  }
}

void IWebView::HideWebView(bool hide)
{
  if (mWebViewCtrlr)
  {
    if (hide)
      gtk_widget_hide(GTK_WIDGET(mWebViewCtrlr));
    else
      gtk_widget_show(GTK_WIDGET(mWebViewCtrlr));
  }
  else
  {
    mShowOnLoad = !hide;
  }
}


void IWebView::LoadURL(const char* url)
{
  if (mWebViewCtrlr)
  {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(mWebViewCtrlr), url);
  }
}

// Other methods...
