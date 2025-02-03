#include "IPlugWebView.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>  // Include GDK X11 header
#include <webkit2/webkit2.h>
#include <syscall.h>
#include <linux/futex.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <X11/extensions/shape.h>
#include <unistd.h>
#include <sys/wait.h>


using namespace iplug;


IWebView::IWebView(bool opaque)
{
}

IWebView::~IWebView()
{
    CloseWebView();
}

// Function to handle X11 errors
int x11_error_handler(Display *display, XErrorEvent *event) {
    char error_text[256];
    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "X11 Error: %s\n", error_text);
    return 0;
}

// Function to reparent the GTK window
gboolean reparent_window(gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    Display *xdisplay = GDK_WINDOW_XDISPLAY(gdk_window);
    Window gtk_x11_window = GDK_WINDOW_XID(gdk_window);

    // Extract the plugin X11 window ID from the command-line arguments
    Window x11_window_id = (Window)atol(g_getenv("X11_WINDOW_ID"));

    // Reparent the GTK window into the plugin's X11 window
    XReparentWindow(xdisplay, gtk_x11_window, x11_window_id, 0, 0);

    // Flush the X11 connection to ensure the reparenting is applied
    XSync(xdisplay, False);

    return FALSE; // Run only once
}


void* IWebView::OpenWebView(void* pParent, float x, float y, float width, float height, float scale, bool isTransparent)
{
    if(pParent == NULL) {
        cabbage::logDebug << "Invalid parent";
        return nullptr;
    }

    // Create a unique queue
    std::string messageQueueNameStr = "/tmp/cabbagePipe_" + std::to_string(getpid());

    // Create the message queue before forking
    messagePipe.createPipe(messageQueueNameStr.c_str());

    // Convert parameters to strings
    std::ostringstream x11WindowIdStr, xStr, yStr, widthStr, heightStr, scaleStr;
    x11WindowIdStr << reinterpret_cast<unsigned long>(pParent);
    xStr << x;
    yStr << y;
    widthStr << width;
    heightStr << height;
    scaleStr << scale;
    std::string isTransparentStr = isTransparent ? "true" : "false";
    std::string enableDevToolsStr = "true";

    std::vector<const char*> args = {
        "/home/rory/sourcecode/cabbage3/src/webView/linux_process/webviewLaunch",
        x11WindowIdStr.str().c_str(),
        messageQueueNameStr.c_str(),
        xStr.str().c_str(),
        yStr.str().c_str(),
        widthStr.str().c_str(),
        heightStr.str().c_str(),
        scaleStr.str().c_str(),
        isTransparentStr.c_str(),
        enableDevToolsStr.c_str(),
        nullptr // Null terminator for exec
    };

    // Fork process
    pid = fork();
    if (pid == 0) {
        usleep(10 * 1000);
        execv(args[0], const_cast<char* const*>(args.data()));
        perror("execv failed");  // Print error if exec fails
        //now kill the process that started the webview...
        exit(1);
    }
    else if (pid < 0) {
        cabbage::logDebug << "Fork failed";
        return nullptr;
    }

    OnWebViewReady();


    return nullptr;
}



void IWebView::CloseWebView() {
//    fclose(namedPipe);
}

void IWebView::HideWebView(bool hide) {
    // Implement if needed
}

void IWebView::LoadHTML(const char* html) {
    // Implement if needed
}

void IWebView::LoadURL(const char* url) {
    if(messagePipe.isOpenForWriting(true))
        messagePipe.send(MessagePipeHost::MessageType::LoadUrl, url);
}

void IWebView::LoadFile(const char* fileName, const char* bundleID) {
    // Implement if needed
}

void IWebView::EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func) {
    if(messagePipe.isOpenForWriting())
        messagePipe.send(MessagePipeHost::MessageType::EvaluateJS, scriptStr);
}

void IWebView::EnableScroll(bool enable) {
    // Implement if needed
}

void IWebView::EnableInteraction(bool enable) {
    // Implement if needed
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale) {
    // Implement if needed
}

