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

struct SharedData
{
    Window parentWindow;
    bool ready;
};

// Callback function for reparenting after the GTK window is realized
void on_realize(GtkWidget* window, gpointer user_data)
{
    SharedData* shared = (SharedData*)user_data;

    GdkWindow* gdkWindow = gtk_widget_get_window(window);
    if (!gdkWindow)
    {
        std::cerr << "Failed to get GdkWindow\n";
        return;
    }

    Window childWindow = gdk_x11_window_get_xid(gdkWindow);
    Display* display = gdk_x11_get_default_xdisplay();

    std::cout << "Reparenting to parent window: " << shared->parentWindow << "\n";
    XReparentWindow(display, childWindow, shared->parentWindow, 0, 0);
}

// Entry point for child process
extern "C" int webview_process_start(int argc, char* argv[])
{
    if (argc != 3 || strcmp(argv[1], "webview-process") != 0)
    {
        std::cerr << "Invalid arguments to webview_process_start\n";
        return 1;
    }

    // Connect to shared memory
    const char* shmName = argv[2];
    int fd = shm_open(shmName, O_RDWR, 0660);
    if (fd < 0)
    {
        std::cerr << "Failed to open shared memory\n";
        return 1;
    }

    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData),
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED)
    {
        std::cerr << "Failed to mmap shared memory\n";
        close(fd);
        return 1;
    }

    close(fd); // No longer needed after mmap

    // Initialize GTK
    gtk_init(&argc, &argv);

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webView));

    g_signal_connect(window, "realize", G_CALLBACK(on_realize), shared);

    gtk_widget_show_all(window);
    webkit_web_view_load_uri(webView, "https://google.com");

    // Main loop
    gtk_main();
    return 0;
}

// Function to open the WebView in the plugin
void* IWebView::OpenWebView(void* pParent, float x, float y, float width, float height, float scale, bool isTransparent)
{
    const char* shmName = "/webview-12345";

    // Create shared memory
    int fd = shm_open(shmName, O_CREAT | O_RDWR, 0660);
    if (fd < 0)
    {
        std::cerr << "Failed to create shared memory\n";
        return nullptr;
    }

    if (ftruncate(fd, sizeof(SharedData)) < 0)
    {
        std::cerr << "Failed to set shared memory size\n";
        close(fd);
        return nullptr;
    }

    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData),
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED)
    {
        std::cerr << "Failed to mmap shared memory\n";
        close(fd);
        return nullptr;
    }

    shared->parentWindow = reinterpret_cast<Window>(pParent);
    close(fd); // No longer needed after mmap

    // Get the binary path of the current executable
    char selfPath[1024];
    ssize_t len = readlink("/proc/self/exe", selfPath, sizeof(selfPath) - 1);
    if (len == -1)
    {
        std::cerr << "Failed to determine executable path\n";
        return nullptr;
    }
    selfPath[len] = '\0';

    // Fork and exec the subprocess
    pid_t pid = fork();
    if (pid == -1)
    {
        std::cerr << "Failed to fork process\n";
        return nullptr;
    }
    else if (pid == 0)
    {
        // Child process: execute the same binary with a different entry point
        const char* args[] = {selfPath, "webview-process", shmName, nullptr};
        execv(selfPath, (char* const*)args);

        // If execv fails
        std::cerr << "execv failed\n";
        exit(1);
    }

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