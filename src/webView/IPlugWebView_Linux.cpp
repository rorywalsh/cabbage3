#include "IPlugWebView.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h> // Include GDK X11 header
#include <webkit2/webkit2.h>
#include <syscall.h>
#include <linux/futex.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <X11/extensions/shape.h>
#include <unistd.h>
#include <sys/wait.h>

#if !defined(CabbageApp)

#include "webview_binary.h"

using namespace iplug;

IWebView::IWebView(bool opaque)
    : instanceMap(cabbage::SharedMemoryQueue::CreateDefaultInstanceTracker(true)),
      memoryQueue("/cabbage_" + instanceMap.getInstanceId(), 100, 1024)
{
    cabbage::logInfo << "Instance ID:" << instanceMap.getInstanceId();
    webviewProcessPath = createTempFile(std::string("/tmp/cabWV_" + instanceMap.getInstanceId() + "XXXXXX").c_str());
}

IWebView::~IWebView()
{
    unlink(std::string(webviewProcessPath).c_str());

    CloseWebView();
}

// Creates a temporary file and returns the full path
std::string IWebView::createTempFile(const char *path_template)
{
    // Allocate memory for the temporary file name
    char *temp_filename = new char[strlen(path_template) + 1]; // +1 for the null terminator
    std::strcpy(temp_filename, path_template);

    // Create a temporary file
    int fd = mkstemp(temp_filename); // Creates and opens the file
    if (fd == -1)
    {
        delete[] temp_filename; // Clean up the allocated memory
        throw std::runtime_error("Failed to create temporary file");
    }

    // Write binary data to the file
    std::string decoded_binary = cabbage::Base64::decode(webview_binary);
    auto data_size = decoded_binary.size(); // Use the size of the string, not strlen
    if (write(fd, decoded_binary.data(), data_size) != static_cast<ssize_t>(data_size))
    {
        close(fd);
        unlink(temp_filename);  // Clean up
        delete[] temp_filename; // Clean up the allocated memory
        throw std::runtime_error("Failed to write to temporary file");
    }

    // Mark the file as executable
    if (chmod(temp_filename, S_IRWXU) == -1)
    { // Read, write, execute by owner
        close(fd);
        unlink(temp_filename);  // Clean up
        delete[] temp_filename; // Clean up the allocated memory
        throw std::runtime_error("Failed to make file executable");
    }

    // Close the file
    close(fd);

    // Save the full path
    std::string full_path(temp_filename);

    // Clean up the allocated memory
    delete[] temp_filename;

    return full_path;
}

void *IWebView::OpenWebView(void *pParent, float x, float y, float width, float height, float scale, bool isTransparent)
{
    if (pParent == NULL)
    {
        cabbage::logDebug << "Invalid parent";
        return nullptr;
    }

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

    // Fork process
    webviewPid = fork();

    if (webviewPid == 0)
    {

        std::vector<std::string> stringArgs = {webviewProcessPath.c_str(),
                                               x11WindowIdStr.str(),
                                               "/cabbage_" + instanceMap.getInstanceId(),
                                               xStr.str(),
                                               yStr.str(),
                                               widthStr.str(),
                                               heightStr.str(),
                                               scaleStr.str(),
                                               isTransparentStr,
                                               enableDevToolsStr};

        std::vector<const char *> args;
        for (const auto &arg : stringArgs)
        {
            args.push_back(arg.c_str());
        }

        usleep(10000);
        args.push_back(nullptr); // Null terminator for exec
        cabbage::logInfo << "Webview process Name:" << args[0];
        execv(args[0], const_cast<char *const *>(args.data()));
        perror(args[0]); // Print error if exec fails
        // now kill the process that started the webview...
        exit(1);
    }
    else if (webviewPid < 0)
    {
        cabbage::logDebug << "Fork failed";
        return nullptr;
    }

    usleep(100 * 1000);
    OnWebViewReady();
    return nullptr;
}

void IWebView::CloseWebView()
{
    kill(webviewPid, SIGTERM);
}

void IWebView::HideWebView(bool hide)
{
    // Implement if needed
}

void IWebView::LoadHTML(const char *html)
{
    // Implement if needed
}

void IWebView::LoadURL(const char *url)
{
    nlohmann::json message;
    message["command"] = "LoadUrl";
    message["data"] = url;
    memoryQueue.sendToChild(message);
    OnWebContentLoaded();
}

void IWebView::LoadFile(const char *fileName, const char *bundleID)
{
    // Implement if needed
}

void IWebView::EvaluateJavaScript(const char *scriptStr, completionHandlerFunc func)
{
    nlohmann::json message;
    message["command"] = "EvaluateJS";
    message["data"] = scriptStr;
    memoryQueue.sendToChild(message);
}

void IWebView::EnableScroll(bool enable)
{
    // Implement if needed
}

void IWebView::EnableInteraction(bool enable)
{
    // Implement if needed
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
    // Implement if needed
}

// Function to handle X11 errors
int x11_error_handler(Display *display, XErrorEvent *event)
{
    char error_text[256];
    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "X11 Error: %s\n", error_text);
    return 0;
}

// Function to reparent the GTK window
gboolean reparent_window(gpointer data)
{
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

#endif