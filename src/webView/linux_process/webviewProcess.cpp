#include "json.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <fcntl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <webkit2/webkit2.h>

// g++ -o webviewLaunch webviewProcess.cpp $(pkg-config --cflags --libs gtk+-3.0
// webkit2gtk-4.1 x11)

class WebViewApp
{
  public:
    WebViewApp(int argc, char *argv[])
    {
        // Initialize GTK
        gtk_init(&argc, &argv);

        // Parse command-line arguments
        if (argc != 10)
        {
            g_printerr("Not enough args:%d Usage: %s <x11WindowId> <x> <y> <width> "
                       "<height> <scale> <isTransparent>\n",
                       argc, argv[0]);
            exit(1);
        }

        // Extract arguments
        x11WindowId = (Window)atol(argv[1]);
        outgoingPipeName = std::string(argv[2]) + "_outgoing";
        incomingPipeName = std::string(argv[2]) + "_incoming";
        x = atof(argv[3]);
        y = atof(argv[4]);
        width = atof(argv[5]);
        height = atof(argv[6]);
        scale = atof(argv[7]);
        isTransparent = (strcmp(argv[8], "true") == 0);
        enableDevTools = (strcmp(argv[9], "true") == 0);

        // Debug: Print X11 window IDs
        std::cout << "Plugin X11 Window ID: " << x11WindowId << std::endl;

        // Open named pipe for outgoing message
        openNamedPipe(true);
        // Open named pipe for incoming message
        openNamedPipe(false);

        // Create the GTK window and WebView
        createWindow();
        createWebView(enableDevTools);

        // Set transparency if needed
        if (isTransparent)
        {
            setTransparency();
        }

        // Load a default webpage
        webkit_web_view_load_uri(webview, "https://www.example.com");

        // Show the window
        gtk_widget_show_all(window);

        // Connect the destroy signal to exit the application
        g_signal_connect(window, "destroy", G_CALLBACK(onDestroy), this);

        // Periodically check the named pipe for new JavaScript code
        g_timeout_add(10, readFromPipe, this);

        // Make sure our window gets properly destroyed
        g_signal_connect(window, "delete-event", G_CALLBACK(onDestroy), this);
    }

    ~WebViewApp()
    {
        logMessage("Destructor");
        // Close the named pipe
        closePipe();
    }

    void run()
    {
        // Start the GTK main loop
        gtk_main();
    }

  private:

    char buffer[4096 * 256];
    GtkWidget *window;
    WebKitWebView *webview;
    Window x11WindowId;
    float x, y, width, height, scale;
    bool isTransparent;
    bool enableDevTools;
    int incomingPipeFd = -1;
    int outgoingPipeFd = -1;
    std::string incomingPipeName = {};
    std::string outgoingPipeName = {};

    static void logMessage(std::string_view message)
    {
        std::cout << "WebViewProc:" << message << std::endl;
    }

    void openNamedPipe(bool isOutput)
    {
        if (isOutput)
        {
            outgoingPipeFd = open(outgoingPipeName.c_str(), O_WRONLY | O_NONBLOCK);
            if (outgoingPipeFd == -1)
            {
                perror("open");
                logMessage("Failed to open named pipe for output:" + outgoingPipeName
                          + ". Ensure the host application created it.");
                exit(1);
            }
            else
            {
                logMessage("Opened named pipe for output:" + outgoingPipeName);
            }
        }
        else
        {
            incomingPipeFd = open(incomingPipeName.c_str(), O_RDONLY | O_NONBLOCK);
            if (incomingPipeFd == -1)
            {
                perror("open");
                logMessage("Failed to open named pipe for input:" + incomingPipeName
                          + ". Ensure the host application created it.");
                exit(1);
            }
            else
            {
                logMessage("Opened named pipe input:" + incomingPipeName);
            }
        }
    }

    void createWindow()
    {
        logMessage("Creating window");

        // Create a GTK plug (embedded window) using the specified X11 window ID
        window = gtk_plug_new(x11WindowId);

        // Set the default size of the window
        gtk_window_set_default_size(GTK_WINDOW(window), (int)width, (int)height);

        // Move the window to the specified position
        gtk_window_move(GTK_WINDOW(window), (int)x, (int)y);

        // Remove window decorations
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    }

    void createWebView(bool enableDebug)
    {
        logMessage("Creating webview widget");
        // Create a user content manager to inject scripts
        WebKitUserContentManager *userContentManager = webkit_user_content_manager_new();

        // Create the WebView with the content manager
        webview = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(userContentManager));

        // Define the JavaScript function to be injected
        const char *js_code = "function IPlugSendMsg(m) {"
                              "   window.webkit.messageHandlers.callback.postMessage(m);"
                              "}";

        // Create a WebKit user script that runs at document start
        WebKitUserScript *script =
            webkit_user_script_new(js_code,
                                   WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,        // Inject into top-level frame
                                   WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, // Run before page loads
                                   NULL, NULL);

        // Add the script to the WebView
        webkit_user_content_manager_add_script(userContentManager, script);

        // Register the message handler and connect it to a callback
        webkit_user_content_manager_register_script_message_handler(userContentManager, "callback");
        g_signal_connect(userContentManager, "script-message-received::callback", G_CALLBACK(onWebMessageReceived),
                         this);

        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

        // Add the WebView to the GTK window
        gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webview));

        // Enable Developer Tools if requested
        if (enableDebug)
        {
            WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
            webkit_settings_set_enable_developer_extras(settings, TRUE);
        }
    }

    static void onWebMessageReceived(WebKitUserContentManager *manager, WebKitJavascriptResult *result,
                                     gpointer user_data)
    {
        WebViewApp *app = static_cast<WebViewApp *>(user_data);
        JSCValue *value = webkit_javascript_result_get_js_value(result);

        // Print the raw value as JSON (for other types)
        gchar *json = jsc_value_to_json(value, 0);

        ssize_t bytesWritten = write(app->outgoingPipeFd, std::string(json).c_str(), std::string(json).length());

        if (bytesWritten == -1)
        {
            perror("Write to pipe failed in child process");
        }
        else
        {
            logMessage("Message received: " +  std::string(json) + " and sent to pipe: " + app->outgoingPipeName);
        }

        logMessage("Received message from WebView (Raw Value in JSON): " + std::string(json));
        g_free(json);

    }

    void setTransparency()
    {
        GdkScreen *screen = gtk_widget_get_screen(window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);

        if (visual && gdk_screen_is_composited(screen))
        {
            gtk_widget_set_visual(window, visual);
            gtk_widget_set_app_paintable(window, TRUE);
        }
    }

    static gboolean reparent_window(gpointer data)
    {
        logMessage("Reparenting window");
        GtkWidget *window = GTK_WIDGET(data);
        GdkWindow *gdk_window = gtk_widget_get_window(window);
        Display *xdisplay = GDK_WINDOW_XDISPLAY(gdk_window);
        Window gtk_x11_window = GDK_WINDOW_XID(gdk_window);

        // Extract the plugin X11 window ID from the environment
        Window x11WindowId = (Window)atol(g_getenv("X11_WINDOW_ID"));

        // Reparent the GTK window into the plugin's X11 window
        XReparentWindow(xdisplay, gtk_x11_window, x11WindowId, 0, 0);

        // Flush the X11 connection to ensure the reparenting is applied
        XSync(xdisplay, False);

        return FALSE; // Run only once
    }

    static gboolean readFromPipe(gpointer data)
    {
        WebViewApp *app = static_cast<WebViewApp *>(data);        

        // Read data from the pipe
        ssize_t bytes_read = read(app->incomingPipeFd,app-> buffer, sizeof(app->buffer) - 1);
        if (bytes_read > 0)
        {
            app->buffer[bytes_read] = '\0'; // Null-terminate the buffer

            // Check if the incoming message is larger than the buffer size
            if (bytes_read == sizeof(app->buffer) - 1)
            {
                std::cerr << "Warning: Incoming message is larger than the buffer size (" << sizeof(app->buffer)
                          << " bytes). Some data may have been lost." << std::endl;
            }

            try
            {
                // Parse the buffer as a JSON array
                nlohmann::json messages = nlohmann::json::parse("[" + std::string(app->buffer) + "]");

                // Ensure we received an array
                if (!messages.is_array())
                {
                    std::cerr << "Error: Expected a JSON array but received something else." << std::endl;
                    return TRUE;
                }

                // Process each object in the array
                for (const auto &message : messages)
                {
                    if (!message.is_object() || !message.contains("data") || !message.contains("command"))
                    {
                        std::cerr << "Invalid JSON format, missing 'data' or 'command'." << std::endl;
                        continue;
                    }

                    std::string msgData = message["data"];
                    std::string command = message["command"];

                    // Process the message based on the "command" field
                    if (command == "LoadUrl")
                    {
                        logMessage("Loading URL: " + msgData);
                        webkit_web_view_load_uri(app->webview, msgData.c_str());
                    }
                    else if (command == "EvaluateJS")
                    {
                        logMessage("Evaluating JavaScript: " + msgData);
                        webkit_web_view_evaluate_javascript(app->webview, msgData.c_str(), -1, NULL, NULL, NULL, NULL,
                                                            NULL);
                    }
                    else if (command == "KillProcess")
                    {
                        logMessage("KillProcess");
                        g_idle_add(
                            [](gpointer data) -> gboolean {
                                WebViewApp *app = static_cast<WebViewApp *>(data);
                                if (app->window)
                                {
                                    logMessage("OnIdle - closing pipe");
                                    app->closePipe();
                                    gtk_widget_destroy(app->window);
                                    logMessage("OnIdle - gtk_main_quit");
                                    gtk_main_quit();
                                }
                                return FALSE; // Run only once
                            },
                            nullptr);
                    }
                    else
                    {
                        std::cerr << "Unknown command: " << command << std::endl;
                    }
                }
            }
            catch (const nlohmann::json::parse_error &e)
            {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
            }
        }

        return TRUE; // Continue listening for more data
    }


    static std::string reformatJsonInput(const std::string& input)
    {
        if (input.empty())
        {
            return "[]"; // Return empty JSON array if input is empty
        }

        try
        {
            // Try to parse the input as a JSON array
            nlohmann::json parsed = nlohmann::json::parse(input);

            if (parsed.is_array())
            {
                return input; // Already a valid JSON array, return as-is
            }
            else
            {
                return "[" + input + "]"; // Single object, wrap it in an array
            }
        }
        catch (const nlohmann::json::parse_error& e)
        {
            // If parsing fails, we assume multiple concatenated objects.
            // Fix concatenated objects by inserting commas between them.
            
            std::string fixed_json = "[";
            bool first = true;

            for (size_t i = 0; i < input.size(); ++i)
            {
                // Check for starting curly brace (possible start of a new object)
                if (input[i] == '{')
                {
                    if (!first)
                    {
                        fixed_json += ","; // Insert a comma if it's not the first object
                    }
                    first = false;
                }
                fixed_json += input[i]; // Append character

                // Detect end of an object or array and stop adding commas (if needed)
                if (input[i] == '}')
                {
                    // After processing the whole input, add closing array bracket
                    if (i == input.size() - 1) {
                        fixed_json += "]";
                    }
                }
            }

            return fixed_json; // Return the formatted array-wrapped JSON
        }
    }

    static void handleSigterm(int signum)
    {
        logMessage("Received SIGTERM, shutting down");

        g_idle_add(
            [](gpointer data) -> gboolean {
                WebViewApp *app = static_cast<WebViewApp *>(data);
                if (app->window)
                {
                    logMessage("OnIdle - closing pipe");
                    app->closePipe();
                    gtk_widget_destroy(app->window); // This will trigger "destroy" and
                                                     // call onDestroy()
                    logMessage("OnIdle - gtk_main_quit");
                    gtk_main_quit(); // Quit GTK main loop
                }
                return FALSE; // Run only once
            },
            nullptr);
    }

    static void onDestroy(GtkWidget *widget, gpointer data)
    {
        logMessage("Destroying WebViewApp");
        WebViewApp *app = static_cast<WebViewApp *>(data);
        app->closePipe();
        gtk_main_quit(); // Quit GTK main loop
    }

    void closePipe()
    {
        if (incomingPipeFd != -1)
        {
            close(incomingPipeFd);
            if (unlink(incomingPipeName.c_str()) == -1)
                perror("Error removing pipe");
        }
        if (outgoingPipeFd != -1)
        {
            close(incomingPipeFd);
            if (unlink(outgoingPipeName.c_str()) == -1)
                perror("Error removing pipe");
        }
    }
};

int main(int argc, char *argv[])
{
    WebViewApp app(argc, argv);
    app.run();
    return 0;
}