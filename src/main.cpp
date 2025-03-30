#include <iostream>
#include <tui.h>
#include <server.h>
#include <signal.h>

// Global pointer for cleanup in signal handler
TUI* g_tui = nullptr;

// Signal handler for proper cleanup
void cleanup_handler(int signum) {
    if (g_tui) {
        delete g_tui;
        g_tui = nullptr;
    }
    exit(signum);
}

int main(int argc, char** argv) {
    // Register signal handlers for proper cleanup
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);

    std::string fileName;
    
    try {
        TUI tui;
        g_tui = &tui;  // Store for signal handler
        
        PeerServer server(tui);
        fileName = argv[1];
        server.addFile(fileName);
        puts(server._script_buf);
        server.run();  // This will now run the TUI in the main thread
        //
        g_tui = nullptr;  // Clear before normal exit
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
