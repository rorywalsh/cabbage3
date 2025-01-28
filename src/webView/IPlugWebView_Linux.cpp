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
#include <sys/mman.h>    // For mmap, munmap
#include <sys/stat.h>    // For shm_open
#include <fcntl.h>       // For O_RDWR, O_CREAT
#include <unistd.h>      // For close
#include "CabbageUtils.h"

using namespace iplug;

IWebView::IWebView(bool opaque)
{
}

IWebView::~IWebView()
{
    CloseWebView();
}

struct SharedData {
    Window parentWindow;
    bool ready;
};

// Entry point for child process
extern "C" int webview_process_start(int argc, char* argv[]) {
    if (argc != 3 || strcmp(argv[1], "webview-process") != 0)
        return 1;

    // Connect to shared memory
    const char* shmName = argv[2];
    int fd = shm_open(shmName, O_RDWR, 0666);
    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData),
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd, 0);
    
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webView));
    
    gtk_widget_show_all(window);
    
    webkit_web_view_load_uri(webView, "https://google.com");
    
    // Reparent into plugin window
    GdkWindow* gdkWindow = gtk_widget_get_window(window);
    Window childWindow = gdk_x11_window_get_xid(gdkWindow);
    Display* display = gdk_x11_get_default_xdisplay();
    XReparentWindow(display, childWindow, shared->parentWindow, 0, 0);
    
    // Main loop
    gtk_main();
    return 0;
}

// Function to open the WebView in the plugin
void* IWebView::OpenWebView(void* pParent, float x, float y, float width, float height, float scale, bool isTransparent) {
    // Create shared memory
    const char* shmName = "/webview-12345";
    int fd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData));
    
    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData),
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd, 0);
    shared->parentWindow = reinterpret_cast<Window>(pParent);
    
    // Launch child process using ld-linux
    auto self = cabbage::File::getBinaryPath();


    const char* ldPath = "/lib/ld-linux.so.2"; // Adjust this path based on your system
    const char* args[] = {ldPath, self.c_str(), "webview-process", shmName, nullptr}; // Use selfPath as the shared object path


    const char* newArgs[] = {ldPath, self.c_str(), nullptr};
    execv(newArgs[0], (char* const*)newArgs);

    return pParent;
}

void IWebView::CloseWebView()
{

}

void IWebView::HideWebView(bool hide)
{

}

void IWebView::LoadHTML(const char* html)
{

}

void IWebView::LoadURL(const char* url)
{

}

void IWebView::LoadFile(const char* fileName, const char* bundleID)
{

}

void IWebView::EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func)
{

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

}