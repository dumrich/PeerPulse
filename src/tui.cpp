#include <string>
#include <ncurses.h>
#include <locale.h>
#include <arpa/inet.h>  // For inet_ntoa and related functions
#include <time.h>       // For clock_gettime
#include <errno.h>      // For ETIMEDOUT
#include "tui.h"
#include "server.h"
#include "client.h"


TUI::TUI() : intro_win_(nullptr), main_win_(nullptr), client_win_(nullptr), 
             status_win_(nullptr), intro_panel_(nullptr), main_panel_(nullptr) {
    // Lock the mutex before initializing ncurses
    pthread_mutex_lock(&ncurses_mutex);
    init_ncurses();
    pthread_mutex_unlock(&ncurses_mutex);
    
    socket_address_ = "0.0.0.0:8080";
    
    // Thread-safe initialization of client list with just the server
    pthread_mutex_lock(&clients_mutex);
    clients_.push_back("Main Server (192.168.1.100:8080)");
    pthread_mutex_unlock(&clients_mutex);
}

TUI::~TUI() {
    // Stop the client monitor thread if running
    if (monitor_running) {
        // Signal to the monitor thread that it should exit
        monitor_running = false;
        
        // Wake up the monitor thread if it's waiting on the condition variable
        if (server_clients_mutex && server_clients_cond) {
            pthread_mutex_lock(server_clients_mutex);
            pthread_cond_signal(server_clients_cond);
            pthread_mutex_unlock(server_clients_mutex);
        }
        
        // Wait for the monitor thread to exit
        pthread_join(client_monitor_thread, nullptr);
    }
    
    // Lock mutex for ncurses cleanup
    pthread_mutex_lock(&ncurses_mutex);
    
    // Clean up ncurses resources
    if (intro_panel_) del_panel(intro_panel_);
    if (main_panel_) del_panel(main_panel_);
    if (intro_win_) delwin(intro_win_);
    if (main_win_) delwin(main_win_);
    if (client_win_) delwin(client_win_);
    if (status_win_) delwin(status_win_);
    endwin();
    
    // Unlock before destroying mutex
    pthread_mutex_unlock(&ncurses_mutex);
    
    // Destroy the mutexes
    pthread_mutex_destroy(&ncurses_mutex);
    pthread_mutex_destroy(&clients_mutex);
}

void TUI::set_server_ref(PeerServer* server, pthread_mutex_t* clients_mutex, 
                       pthread_cond_t* clients_cond, std::vector<Client>* clients) {
    server_ref = server;
    server_clients_mutex = clients_mutex;
    server_clients_cond = clients_cond;
    server_clients = clients;
    
    // Start the client monitor thread
    monitor_running = true;
    pthread_create(&client_monitor_thread, nullptr, &TUI::client_monitor_thread_fn, this);
}

void* TUI::client_monitor_thread_fn(void* arg) {
    TUI* tui = static_cast<TUI*>(arg);
    tui->monitor_clients();
    return nullptr;
}

void TUI::monitor_clients() {
    // Check that we have valid server references
    if (!server_clients_mutex || !server_clients_cond || !server_clients) {
        printf("Error: Server references not properly set for client monitoring\n");
        return;
    }
    
    while (monitor_running) {
        // Lock the server's clients mutex
        pthread_mutex_lock(server_clients_mutex);
        
        // Wait for the condition to be signaled, but with a timeout to check monitor_running
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1; // 1 second timeout
        
        // Wait for condition with timeout to periodically check monitor_running
        while (monitor_running && server_clients->size() == last_client_count) {
            int ret = pthread_cond_timedwait(server_clients_cond, server_clients_mutex, &timeout);
            if (ret == ETIMEDOUT) {
                // Timeout occurred, refresh timeout and continue waiting if still running
                if (!monitor_running) break;
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_sec += 1;
            } else {
                // Condition was signaled, break the wait loop
                break;
            }
        }
        
        // Check if we should exit
        if (!monitor_running) {
            pthread_mutex_unlock(server_clients_mutex);
            break;
        }
        
        // Process any new clients
        for (size_t i = last_client_count; i < server_clients->size(); ++i) {
            const Client& client = (*server_clients)[i];
            
            // Format client info
            char client_info[100];
            snprintf(client_info, sizeof(client_info), "Client %zu (%s:%d)", 
                     i, 
                     inet_ntoa(client.address.sin_addr), 
                     ntohs(client.address.sin_port));
            
            // Add the client to the TUI
            add_client(client_info);
        }
        
        // Update our count
        last_client_count = server_clients->size();
        
        // Unlock the server's clients mutex
        pthread_mutex_unlock(server_clients_mutex);
    }
}

void TUI::init_ncurses() {
    // Set up locale for UTF-8 support
    setlocale(LC_ALL, "");
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_BLACK, COLOR_WHITE);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    }
    
    getmaxyx(stdscr, term_height_, term_width_);
}

void TUI::create_intro_screen() {
    // Increase window size to fit the banner plus controls
    int height = 22;  // Increased for the banner plus controls
    int width = 60;   // Increased width to better fit the banner
    int y = (term_height_ - height) / 2;
    int x = (term_width_ - width) / 2;
    
    intro_win_ = newwin(height, width, y, x);
    intro_panel_ = new_panel(intro_win_);
    box(intro_win_, 0, 0);
    
    render_intro_screen();
}

void TUI::render_intro_screen() {
    // Lock the mutex before accessing ncurses
    pthread_mutex_lock(&ncurses_mutex);
    
    // Get window width directly
    int win_width = getmaxx(intro_win_);
    
    // Simple intro message
    const char* welcome_msg = "Welcome to PeerPulse";
    const char* tagline = "Distributed Computing Platform";
    
    // Clear the window before drawing
    wclear(intro_win_);
    box(intro_win_, 0, 0);
    
    // Draw the welcome message
    int welcome_y = 5;
    wattron(intro_win_, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(intro_win_, welcome_y, (win_width - strlen(welcome_msg)) / 2, "%s", welcome_msg);
    wattroff(intro_win_, COLOR_PAIR(3) | A_BOLD);
    
    // Draw the tagline
    wattron(intro_win_, COLOR_PAIR(3));
    mvwprintw(intro_win_, welcome_y + 2, (win_width - strlen(tagline)) / 2, "%s", tagline);
    wattroff(intro_win_, COLOR_PAIR(3));
    
    // Continue button
    wattron(intro_win_, COLOR_PAIR(2));
    mvwprintw(intro_win_, welcome_y + 5, (win_width - 10) / 2, " CONNECT ");
    wattroff(intro_win_, COLOR_PAIR(2));
    
    mvwprintw(intro_win_, welcome_y + 7, (win_width - 23) / 2, "Press Enter to continue");
    
    // Add window title
    wattron(intro_win_, A_BOLD);
    mvwprintw(intro_win_, 0, (win_width - 12) / 2, " PeerPulse ");
    wattroff(intro_win_, A_BOLD);
    
    // Ensure changes are shown
    wrefresh(intro_win_);
    update_panels();
    doupdate();
    
    // Unlock the mutex after finishing ncurses operations
    pthread_mutex_unlock(&ncurses_mutex);
}

void TUI::destroy_intro_screen() {
    del_panel(intro_panel_);
    delwin(intro_win_);
    intro_win_ = nullptr;
    intro_panel_ = nullptr;
}

void TUI::create_main_interface() {
    // Make the main window slightly bigger if possible
    int height = term_height_ - 2;
    int width = term_width_ - 2;  // Fixed width calculation
    
    main_win_ = newwin(height, width, 1, 1);
    main_panel_ = new_panel(main_win_);
    box(main_win_, 0, 0);
    
    // Client window (left 30%)
    int client_width = (width - 2) * 0.3;
    client_win_ = newwin(height - 4, client_width - 1, 2, 2);
    box(client_win_, 0, 0);
    
    // Status window (right 70%)
    int status_width = (width - 2) * 0.7 - 1;
    status_win_ = newwin(height - 4, status_width, 2, client_width + 1);
    box(status_win_, 0, 0);
    
    render_main_interface();
}

void TUI::render_main_interface() {
    // Lock the mutex before accessing ncurses
    pthread_mutex_lock(&ncurses_mutex);
    
    // Get window dimensions
    int win_width = getmaxx(main_win_);
    
    // Simple ASCII art banner without escape characters
    const char* banner[] = {
        " _____                _____      _          ",
        "|  __ \\              |  __ \\    | |         ",
        "| |__) |__  ___ _ __| |__) |   | |___  ___ ",
        "|  ___/ _ \\/ _ \\ '__|  ___/ |  | / __|/ _ \\",
        "| |  |  __/  __/ |  | |   | |__| \\__ \\  __/",
        "|_|   \\___|\\___|_|  |_|    \\____/|___/\\___|",
        "                                            ",
        "       DISTRIBUTED COMPUTING PLATFORM       ",
        "                                            "
    };

    // Calculate centering for the banner
    int banner_height = 9;
    int banner_width = 44; 
    int start_y = 2;
    int start_x = (win_width - banner_width) / 2;
    
    // Clear the window before drawing
    wclear(main_win_);
    box(main_win_, 0, 0);
    
    // Draw the banner
    wattron(main_win_, COLOR_PAIR(3) | A_BOLD);
    for (int i = 0; i < banner_height; i++) {
        // Ensure we don't write beyond window boundaries
        if (start_y + i < getmaxy(main_win_) - 1) {
            mvwprintw(main_win_, start_y + i, start_x, "%.*s", 
                      win_width - 2*start_x, banner[i]);
        }
    }
    wattroff(main_win_, COLOR_PAIR(3) | A_BOLD);
    
    // Client window (left 30%) - adjusted position to be below the banner
    int client_start_y = start_y + banner_height + 2;
    int client_width = (win_width - 4) * 0.3;
    int client_height = getmaxy(main_win_) - client_start_y - 4;
    
    // Check if there's enough space for client window
    if (client_height < 5) {
        // Not enough vertical space, use simpler layout
        client_height = 5;
        client_start_y = getmaxy(main_win_) - client_height - 3;
    }
    
    // Recreate client window with new position
    if (client_win_) delwin(client_win_);
    client_win_ = derwin(main_win_, client_height, client_width, client_start_y, 2);
    box(client_win_, 0, 0);
    
    // Status window (right 70%) - adjusted position to be below the banner
    int status_width = (win_width - 4) * 0.7 - 2;
    if (status_win_) delwin(status_win_);
    status_win_ = derwin(main_win_, client_height, status_width, client_start_y, client_width + 4);
    box(status_win_, 0, 0);
    
    // Client list
    wattron(client_win_, COLOR_PAIR(3));
    mvwprintw(client_win_, 1, 1, "Connected Clients:");
    wattroff(client_win_, COLOR_PAIR(3));
    
    // Thread-safe access to clients_ vector
    pthread_mutex_lock(&clients_mutex);
    for (size_t i = 0; i < clients_.size() && i < (size_t)client_height - 4; ++i) {
        mvwprintw(client_win_, 3 + i, 1, "%s", clients_[i].c_str());
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Status window
    wattron(status_win_, COLOR_PAIR(3));
    mvwprintw(status_win_, 1, 1, "Server Socket: %s", socket_address_.c_str());
    wattroff(status_win_, COLOR_PAIR(3));
    
    // Help text
    mvwprintw(main_win_, getmaxy(main_win_) - 2, 2, "Press 'a' to add client, 's' to view script, 'q' to quit");
    
    // Main window title
    wattron(main_win_, A_BOLD);
    mvwprintw(main_win_, 0, (win_width - 12) / 2, " PeerPulse ");
    wattroff(main_win_, A_BOLD);
    
    wrefresh(main_win_);
    wrefresh(client_win_);
    wrefresh(status_win_);
    update_panels();
    doupdate();
    
    // Unlock the mutex after finishing ncurses operations
    pthread_mutex_unlock(&ncurses_mutex);
}

void TUI::add_dummy_client() {
    static int counter = 1;
    std::string new_client = "Worker Node " + std::to_string(counter) + 
                         " (192.168.1." + std::to_string(100 + counter) + 
                         ":808" + std::to_string(counter) + ")";
    
    // Use the add_client method to ensure thread safety
    add_client(new_client);
    counter++;
}

void TUI::add_client(const std::string& client_info) {
    // Thread-safe access to clients_ vector
    pthread_mutex_lock(&clients_mutex);
    clients_.push_back(client_info);
    pthread_mutex_unlock(&clients_mutex);
    
    // If we're on the main interface, update it to show the new client
    if (in_main) {
        render_main_interface();
    }
}

void TUI::handle_input() {
    // Lock mutex for ncurses operations
    pthread_mutex_lock(&ncurses_mutex);
    
    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '\n':
            case KEY_ENTER:
                if (intro_win_) {
                    // Temporarily unlock mutex for complex operation
                    pthread_mutex_unlock(&ncurses_mutex);
                    destroy_intro_screen();
                    create_main_interface();
                    pthread_mutex_lock(&ncurses_mutex);
                    in_main=true;
                }
                break;
            case 'a':
                if (!intro_win_) {
                    // Temporarily unlock mutex for complex operation
                    pthread_mutex_unlock(&ncurses_mutex);
                    render_main_interface();
                    pthread_mutex_lock(&ncurses_mutex);
                }
                break;
            case 's':
                if (!intro_win_) {
                    // Temporarily unlock mutex for complex operation
                    pthread_mutex_unlock(&ncurses_mutex);
                    show_script_viewer();
                    pthread_mutex_lock(&ncurses_mutex);
                    in_main=false;
                }
                break;
        }
    }
    
    // User pressed 'q' to quit
    printf("Quitting application...\n");
    
    // Unlock mutex before exit
    pthread_mutex_unlock(&ncurses_mutex);
}

void TUI::show_script_viewer() {
    // Save current main window state
    PANEL* saved_panel = main_panel_;
    WINDOW* saved_win = main_win_;
    
    // Create new full-screen window for script view
    int height = term_height_ - 2;
    int width = term_width_ - 2;
    WINDOW* script_win = newwin(height, width, 1, 1);
    PANEL* script_panel = new_panel(script_win);
    
    box(script_win, 0, 0);
    
    // Sample script content
    std::vector<std::string> script_lines = {
        "#!/bin/bash",
        "",
        "# PeerPulse Network Setup Script",
        "# Configures the distributed computing environment",
        "",
        "echo \"Setting up PeerPulse distributed network...\"",
        "",
        "# Start the main server",
        "echo \"Starting main server on port 8080...\"",
        "peerpulse_server --port 8080 --workers 4 &",
        "",
        "# Initialize worker nodes",
        "for i in {1..3}; do",
        "    echo \"Starting worker node $i on port 808$i...\"",
        "    peerpulse_worker --master 192.168.1.100:8080 --port 808$i --id worker$i &",
        "done",
        "",
        "echo \"PeerPulse network setup complete!\"",
        "echo \"Main server: 192.168.1.100:8080\"",
        "echo \"Workers: 3 nodes initialized\"",
        "",
        "# Monitor network status",
        "peerpulse_monitor --interval 5",
    };
    
    // Display the script content
    wattron(script_win, COLOR_PAIR(3));
    mvwprintw(script_win, 1, 2, "PeerPulse Network Setup Script");
    wattroff(script_win, COLOR_PAIR(3));
    
    for (size_t i = 0; i < script_lines.size() && i < (size_t)height - 5; ++i) {
        mvwprintw(script_win, 3 + i, 2, "%s", script_lines[i].c_str());
    }
    
    // Add title
    wattron(script_win, A_BOLD);
    mvwprintw(script_win, 0, (width - 18) / 2, " Script Viewer ");
    wattroff(script_win, A_BOLD);
    
    // Add footer
    mvwprintw(script_win, height - 2, 2, "Press any key to return");
    
    wrefresh(script_win);
    update_panels();
    doupdate();
    
    // Wait for a key press
    getch();
    
    // Clean up and restore original window
    del_panel(script_panel);
    delwin(script_win);
    
    // Restore main interface
    top_panel(saved_panel);
    update_panels();
    wrefresh(saved_win);
    doupdate();
}

void TUI::run() {
    create_intro_screen();
    handle_input();
}
