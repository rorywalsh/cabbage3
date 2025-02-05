#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include "json.hpp"

//g++ -o webviewLaunch webviewProcess.cpp $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 x11)

class WebViewApp {
public:
    WebViewApp(int argc, char *argv[]) {
        // Initialize GTK
        gtk_init(&argc, &argv);

        // Parse command-line arguments
        if (argc != 10) {
            g_printerr("Not enought ags:%d Usage: %s <x11_window_id> <x> <y> <width> <height> <scale> <isTransparent>\n", argc, argv[0]);
            exit(1);
        }

        // Extract arguments
        x11_window_id = (Window)atol(argv[1]);
        pipeName = argv[2];
        x = atof(argv[3]);
        y = atof(argv[4]);
        width = atof(argv[5]);
        height = atof(argv[6]);
        scale = atof(argv[7]);
        isTransparent = (strcmp(argv[8], "true") == 0);
        enableDevTools = (strcmp(argv[9], "true") == 0);

        // Debug: Print X11 window IDs
        std::cout << "Plugin X11 Window ID: " << x11_window_id << std::endl;

        // Set the X11 window ID as an environment variable for the reparent function
        char x11_window_id_str[32];
        snprintf(x11_window_id_str, sizeof(x11_window_id_str), "%lu", x11_window_id);
        g_setenv("X11_WINDOW_ID", x11_window_id_str, TRUE);

        // Open the named pipe
        openNamedPipe();

        // Create the GTK window and WebView
        createWindow();
        createWebView(enableDevTools);

        // Set transparency if needed
        if (isTransparent) {
            setTransparency();
        }

        // Load a default webpage
        webkit_web_view_load_uri(web_view, "https://www.example.com");

        // Show the window
        gtk_widget_show_all(window);

        // Schedule the reparenting to occur after the GTK main loop starts
        g_idle_add(reparent_window, window);

        // Connect the destroy signal to exit the application
        g_signal_connect(window, "destroy", G_CALLBACK(onDestroy), this);

        // Periodically check the named pipe for new JavaScript code
        g_timeout_add(10, readFromPipe, this);



    }

    ~WebViewApp() {
        // Close the named pipe
        closePipe();
    }

    void run() {
        // Start the GTK main loop
        gtk_main();
    }

private:
    GtkWidget *window;
    WebKitWebView *web_view;
    Window x11_window_id;
    float x, y, width, height, scale;
    bool isTransparent;
    bool enableDevTools;
    int pipe_fd = -1;
    const char* pipeName = {};

    void openNamedPipe() {
        // Open the FIFO for reading (non-blocking)
        pipe_fd = open(pipeName, O_RDONLY | O_NONBLOCK);
        if (pipe_fd == -1) {
            perror("open");
            std::cerr << "Failed to open named pipe. Ensure the host application created it." << std::endl;
            exit(1);
        }
        else{
            std::cout << "Opened named pipe:" << pipeName <<  std::endl;
        }
    }

    void createWindow() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size(GTK_WINDOW(window), (int)width, (int)height);
        gtk_window_move(GTK_WINDOW(window), (int)x, (int)y);
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE); // Remove window decorations
    }

    void createWebView(bool enableDebug) {
        // Create a user content manager to inject scripts
        WebKitUserContentManager *userContentManager = webkit_user_content_manager_new();
        
        // Create the WebView with the content manager
        web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(userContentManager));
        
        // Define the JavaScript function to be injected
        const char* js_code = 
            "function IPlugSendMsg(m) {"
            "   window.webkit.messageHandlers.callback.postMessage(m);"
            "}";

        // Create a WebKit user script that runs at document start
        WebKitUserScript *script = webkit_user_script_new(
            js_code,
            WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,  // Inject into top-level frame
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,  // Run before page loads
            NULL,
            NULL
        );

        // Add the script to the WebView
        webkit_user_content_manager_add_script(userContentManager, script);

        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

        // Add the WebView to the GTK window
        gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(web_view));

        // Enable Developer Tools if requested
        if (enableDebug) {
            WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view));
            webkit_settings_set_enable_developer_extras(settings, TRUE);
        }
    }


    void setTransparency() {
        GdkScreen *screen = gtk_widget_get_screen(window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual && gdk_screen_is_composited(screen)) {
            gtk_widget_set_visual(window, visual);
            gtk_widget_set_app_paintable(window, TRUE);
        }
    }

    static gboolean reparent_window(gpointer data) {
        GtkWidget *window = GTK_WIDGET(data);
        GdkWindow *gdk_window = gtk_widget_get_window(window);
        Display *xdisplay = GDK_WINDOW_XDISPLAY(gdk_window);
        Window gtk_x11_window = GDK_WINDOW_XID(gdk_window);

        // Extract the plugin X11 window ID from the environment
        Window x11_window_id = (Window)atol(g_getenv("X11_WINDOW_ID"));

        // Reparent the GTK window into the plugin's X11 window
        XReparentWindow(xdisplay, gtk_x11_window, x11_window_id, 0, 0);

        // Flush the X11 connection to ensure the reparenting is applied
        XSync(xdisplay, False);

        return FALSE; // Run only once
    }

    static gboolean readFromPipe(gpointer data) {
        WebViewApp *app = static_cast<WebViewApp *>(data);
        char buffer[1024];
        // Read data from the pipe
        ssize_t bytes_read = read(app->pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            
            buffer[bytes_read] = '\0'; // Null-terminate the buffer

            // Check if the incoming message is larger than the buffer size
            if (bytes_read == sizeof(buffer) - 1) {
                std::cerr << "Warning: Incoming message is larger than the buffer size (" 
                        << sizeof(buffer) << " bytes). Some data may have been lost." << std::endl;
            }

            try {
                // Parse the buffer as a JSON object
                nlohmann::json message = nlohmann::json::parse(buffer);
                std::cout << "Received message: " << message.dump() << std::endl;
                // Check for required fields ("data" and "command")
                if (message.contains("data") && message.contains("command")) {
                    std::string msgData = message["data"];
                    std::string command = message["command"];

                    // Process the message based on the "command" field
                    if (command == "LoadUrl") 
                    {
                        // Handle the LOAD_URL command
                        std::cout << "Loading URL: " << msgData << std::endl;
                        webkit_web_view_load_uri(app->web_view, msgData.c_str());
                    } 
                    else if (command == "EvaluateJs") 
                    {
                        // Handle the EVALUATE_JS command
                        std::cout << "Evaluating JavaScript: " << msgData << std::endl;
                        webkit_web_view_evaluate_javascript(app->web_view, msgData.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
                    } 
                    else {
                        std::cerr << "Unknown command command: " << command << std::endl;
                    }
                } else {
                    std::cerr << "Invalid JSON format, missing 'data' or 'command'" << std::endl;
                }
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            }
        }

        return TRUE; // Continue listening for more data
    }

    static void onDestroy(gpointer data)
    {
        std::cout << "Killing webview process" << std::endl;
        WebViewApp *app = static_cast<WebViewApp *>(data);
        gtk_widget_destroy(app->window); // Destroy GTK Window
        gtk_main_quit(); // Quit the GTK loop
    }

    void closePipe()
    {
        if (pipe_fd != -1) {
            close(pipe_fd);
        }
    }
};

int main(int argc, char *argv[]) {
    WebViewApp app(argc, argv);
    app.run();
    return 0;
}