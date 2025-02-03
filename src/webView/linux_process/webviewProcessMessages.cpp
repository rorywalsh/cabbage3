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
#include <mqueue.h> // For POSIX message queue

class WebViewApp {
public:
    WebViewApp(int argc, char *argv[]) {
        // Initialize GTK
        gtk_init(&argc, &argv);

        for( int i = 0 ; i < argc ; i++){
            std::cout << argv[i] << " - ";
        }

        std::cout << std::endl;


        // Parse command-line arguments
        if (argc != 10) {
            g_printerr("Not enough args:%d Usage: %s <x11_window_id> <x> <y> <width> <height> <scale> <isTransparent>\n", argc, argv[0]);
            exit(1);
        }


        // Extract arguments
        x11_window_id = (Window)atol(argv[1]);
        queueName = argv[2];  // Message queue name
        x = atof(argv[3]);
        y = atof(argv[4]);
        width = atof(argv[5]);
        height = atof(argv[6]);
        scale = atof(argv[7]);
        isTransparent = (strcmp(argv[8], "true") == 0);
        enableDevTools = (strcmp(argv[9], "true") == 0);

        // std::cout << "Message Queue Name: " << queueName << std::endl;

        // Debug: Print X11 window IDs
        std::cout << "Plugin X11 Window ID: " << x11_window_id << std::endl;

        // Set the X11 window ID as an environment variable for the reparent function
        char x11_window_id_str[32];
        snprintf(x11_window_id_str, sizeof(x11_window_id_str), "%lu", x11_window_id);
        std::cout << "x11_window_id_str:" << x11_window_id_str;
        g_setenv("X11_WINDOW_ID", x11_window_id_str, TRUE);

        // Open the message queue
        openMessageQueue();

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
        std::cout << "Finished showing window" << std::endl;

        // Schedule the reparenting to occur after the GTK main loop starts
        g_idle_add(reparent_window, window);
        std::cout << "Finished scheduling reparenting" << std::endl;

        // Connect the destroy signal to exit the application
        g_signal_connect(window, "destroy", G_CALLBACK(onDestroy), this);

        // Periodically check the message queue for new JavaScript code
        g_timeout_add(10, readFromQueue, this);
    }

    ~WebViewApp() {
        // Close the message queue
        if (mq != (mqd_t)-1) {
            mq_close(mq);
        }
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
    mqd_t mq = (mqd_t)-1;  // POSIX message queue descriptor
    const char* queueName = {};

    void openMessageQueue() {
        // Open the message queue for reading (non-blocking)
        struct mq_attr attr;
        attr.mq_flags = 0;           // Blocking mode
        attr.mq_maxmsg = 100;         // Max number of messages in queue
        attr.mq_msgsize = 8192;      // Max message size (increase this if needed)
        mq = mq_open(queueName, O_RDONLY | O_NONBLOCK, 0666, &attr);
        if (mq == (mqd_t)-1)
        {
            perror("mq_open failed");
            return;
        }else {
            std::cout << "Child: Successfully opened message queue: " << queueName << " (fd = " << mq << ")" << std::endl;
        }
    }

    void createWindow() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size(GTK_WINDOW(window), (int)width, (int)height);
        gtk_window_move(GTK_WINDOW(window), (int)x, (int)y);
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE); // Remove window decorations
        std::cout << "Finished creating window" << std::endl;
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

        // Add the WebView to the GTK window
        gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(web_view));

        // Enable Developer Tools if requested
        if (enableDebug) {
            WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view));
            webkit_settings_set_enable_developer_extras(settings, TRUE);
        }
        std::cout << "Finished creating webview" << std::endl;
    }

    void setTransparency() {
        GdkScreen *screen = gtk_widget_get_screen(window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual && gdk_screen_is_composited(screen)) {
            gtk_widget_set_visual(window, visual);
            gtk_widget_set_app_paintable(window, TRUE);
        }
        std::cout << "Finished setTransparency" << std::endl;
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
        std::cout << "Finished reparent_window" << std::endl;
        return FALSE; // Run only once
    }

    static gboolean readFromQueue(gpointer data) {
        WebViewApp *app = static_cast<WebViewApp *>(data);

        struct mq_attr attr;
        if (mq_getattr(app->mq, &attr) == -1) 
        {
            perror("mq_getattr failed");
            return TRUE;
        }

        std::cout << "  Max message size: " << attr.mq_msgsize << " bytes" << std::endl;
        
        char buffer[8192];

        // Read data from the message queue
        //sstd::cout << "Child: Message queue descriptor = " << app->mq << std::endl;
        ssize_t bytes_read = mq_receive(app->mq, buffer, sizeof(buffer) - 1, NULL);
        
        if (bytes_read == -1) {
            perror("mq_receive failed in child"); // Print the error
        } else if (bytes_read == 0) {
            std::cerr << "Message queue empty" << std::endl;
        } else {
            buffer[bytes_read] = '\0'; // Null-terminate
            std::cerr << "Received message: " << buffer << std::endl;
        }

        return TRUE; // Continue listening
        // WebViewApp *app = static_cast<WebViewApp *>(data);
        // char buffer[1024];

        // // Read data from the message queue
        // ssize_t bytes_read = mq_receive(app->mq, buffer, sizeof(buffer) - 1, NULL);
        // if (bytes_read > 0) {
        //     buffer[bytes_read] = '\0'; // Null-terminate

        //     std::string input(buffer);
        //     std::istringstream ss(input);
        //     std::string command, argument;
        //     std::cerr << "Command before split: " << input << std::endl;

        //     // Split at the first comma
        //     if (std::getline(ss, command, ',') && std::getline(ss, argument)) {
        //         // Remove any extra spaces
        //         command.erase(command.find_last_not_of(" \n\r\t") + 1);
        //         argument.erase(0, argument.find_first_not_of(" \n\r\t"));

        //         if (command == "LOAD_URL") {
        //             webkit_web_view_load_uri(app->web_view, argument.c_str());
        //         } else if (command == "EVALUATE_JS") {
        //             webkit_web_view_evaluate_javascript(app->web_view, argument.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
        //         } else {
        //             std::cerr << "Unknown command: " << command << std::endl;
        //         }
        //     } else {
        //         std::cerr << "Invalid message format." << std::endl;
        //     }
        // }

        // return TRUE; // Continue listening
    }

    static void onDestroy(GtkWidget *widget, gpointer data) {
        WebViewApp *app = static_cast<WebViewApp *>(data);
        delete app; // Clean up the application
        gtk_main_quit(); // Exit the GTK main loop
    }
};

int main(int argc, char *argv[]) {
    WebViewApp app(argc, argv);
    app.run();
    return 0;
}
