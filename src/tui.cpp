#include <string>
#include <ncurses.h>
#include <panel.h>
#include <locale.h>
#include <chrono>
#include <thread>
#include <vector>
#include "tui.h"

// Constructor - initializes ncurses and basic setup
TUI::TUI() {
    init_ncurses();
    socket_address_ = "0.0.0.0:8080";
    
    // Add some dummy clients
    clients_.push_back("Main Server (192.168.1.100:8080)");
    clients_.push_back("Worker Node 1 (192.168.1.101:8081)");
    clients_.push_back("Worker Node 2 (192.168.1.102:8082)");
}

// Destructor - cleans up resources
TUI::~TUI() {
    cleanup_resources();
}

// Initialize ncurses library and settings
void TUI::init_ncurses() {
    // Set up locale for UTF-8 support
    setlocale(LC_ALL, "");
    
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();           // Line buffering disabled
    noecho();           // Don't echo input
    keypad(stdscr, TRUE); // Enable function keys
    curs_set(0);        // Hide cursor
    
    // Define color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    
    // Get terminal dimensions
    getmaxyx(stdscr, term_height_, term_width_);
    
    refresh();
}

// Clean up all resources before destruction
void TUI::cleanup_resources() {
    // Clean up ncurses windows and panels
    if (intro_panel_) {
        del_panel(intro_panel_);
        intro_panel_ = nullptr;
    }
    
    if (main_panel_) {
        del_panel(main_panel_);
        main_panel_ = nullptr;
    }
    
    if (intro_win_) {
        delwin(intro_win_);
        intro_win_ = nullptr;
    }
    
    if (main_win_) {
        delwin(main_win_);
        main_win_ = nullptr;
    }
    
    if (client_win_) {
        delwin(client_win_);
        client_win_ = nullptr;
    }
    
    if (status_win_) {
        delwin(status_win_);
        status_win_ = nullptr;
    }
    
    // Shut down ncurses
    endwin();
}

// Create the intro/welcome screen
void TUI::create_intro_screen() {
    // Set size and position for intro window
    int height = 20;
    int width = 60;
    int y = (term_height_ - height) / 2;
    int x = (term_width_ - width) / 2;
    
    // Create window and panel
    intro_win_ = newwin(height, width, y, x);
    intro_panel_ = new_panel(intro_win_);
    
    // Enable special keys for this window
    keypad(intro_win_, TRUE);
    
    // Ensure window has a border
    box(intro_win_, 0, 0);
    
    // Render the intro screen content
    render_intro_screen();
}

// Draw the content of the intro screen
void TUI::render_intro_screen() {
    if (!intro_win_) return;
    
    // Get window width for centering
    int win_width = getmaxx(intro_win_);
    
    // Clear the window
    wclear(intro_win_);
    box(intro_win_, 0, 0);
    
    // Welcome messages
    const char* welcome_msg = "Welcome to PeerPulse";
    const char* tagline = "Distributed Computing Platform";
    
    // Position for the welcome message
    int welcome_y = 5;
    
    // Draw the welcome text
    wattron(intro_win_, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(intro_win_, welcome_y, (win_width - strlen(welcome_msg)) / 2, "%s", welcome_msg);
    wattroff(intro_win_, COLOR_PAIR(3) | A_BOLD);
    
    // Draw the tagline text
    wattron(intro_win_, COLOR_PAIR(3));
    mvwprintw(intro_win_, welcome_y + 2, (win_width - strlen(tagline)) / 2, "%s", tagline);
    wattroff(intro_win_, COLOR_PAIR(3));
    
    // Draw a connect button
    wattron(intro_win_, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(intro_win_, welcome_y + 6, (win_width - 10) / 2, " CONNECT ");
    wattroff(intro_win_, COLOR_PAIR(2) | A_BOLD);
    
    // Add instruction text
    mvwprintw(intro_win_, welcome_y + 9, (win_width - 25) / 2, "Press ENTER to continue");
    
    // Add window title
    wattron(intro_win_, A_BOLD);
    mvwprintw(intro_win_, 0, (win_width - 12) / 2, " PeerPulse ");
    wattroff(intro_win_, A_BOLD);
    
    // Refresh the window and panel
    wrefresh(intro_win_);
    update_panels();
    doupdate();
}

// Remove the intro screen
void TUI::destroy_intro_screen() {
    if (intro_panel_) {
        del_panel(intro_panel_);
        intro_panel_ = nullptr;
    }
    
    if (intro_win_) {
        delwin(intro_win_);
        intro_win_ = nullptr;
    }
}

// Create the main interface screen
void TUI::create_main_interface() {
    // Create a full-screen window for the main interface
    main_win_ = newwin(term_height_, term_width_, 0, 0);
    main_panel_ = new_panel(main_win_);
    
    // Enable special keys for this window
    keypad(main_win_, TRUE);
    
    // Render the main interface content
    render_main_interface();
    
    // Initialize status message
    update_status("PeerPulse started successfully");
    
    // Refresh the window and panel
    update_panels();
    doupdate();
}

// Draw the content of the main interface
void TUI::render_main_interface() {
    if (!main_win_) return;
    
    // Get window dimensions
    int win_width = getmaxx(main_win_);
    
    // Clear the window
    wclear(main_win_);
    box(main_win_, 0, 0);
    
    // ASCII art banner for PeerPulse
    const char* banner[] = {
        "$$$$$$$\\                                $$$$$$$\\            $$\\                 ",
        "$$  __$$\\                               $$  __$$\\           $$ |                ",
        "$$ |  $$ | $$$$$$\\   $$$$$$\\   $$$$$$\\  $$ |  $$ |$$\\   $$\\ $$ | $$$$$$$\\  $$$$$$\\  ",
        "$$$$$$$  |$$  __$$\\ $$  __$$\\ $$  __$$\\ $$$$$$$  |$$ |  $$ |$$ |$$  _____|$$  __$$\\ ",
        "$$  ____/ $$$$$$$$ |$$$$$$$$ |$$ |  \\__|$$  ____/ $$ |  $$ |$$ |\\$$$$$$\\  $$$$$$$$ |",
        "$$ |      $$   ____|$$   ____|$$ |      $$ |      $$ |  $$ |$$ | \\____$$\\ $$   ____|",
        "$$ |      \\$$$$$$$\\ \\$$$$$$$\\ $$ |      $$ |      \\$$$$$$  |$$ |$$$$$$$  |\\$$$$$$$\\ ",
        "\\__|       \\_______| \\_______|\\___|      \\___|       \\______/ \\__|\\_______/  \\_______|",
        "                                                                                  ",
        "                  DISTRIBUTED COMPUTING PLATFORM                                  ",
        "                                                                                  "
    };
    
    // Calculate dimensions and position for the banner
    int banner_height = 11;
    int banner_width = 74;
    int start_y = 2;
    int start_x = (win_width - banner_width) / 2;
    
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
    
    // Calculate positions for client and status windows
    int client_start_y = start_y + banner_height + 2;
    int client_width = (win_width - 4) * 0.3;
    int client_height = getmaxy(main_win_) - client_start_y - 4;
    
    // Ensure minimum height
    if (client_height < 5) {
        client_height = 5;
        client_start_y = getmaxy(main_win_) - client_height - 3;
    }
    
    // Create client window
    if (client_win_) delwin(client_win_);
    client_win_ = derwin(main_win_, client_height, client_width, client_start_y, 2);
    box(client_win_, 0, 0);
    
    // Create status window
    int status_width = (win_width - 4) * 0.7 - 2;
    if (status_win_) delwin(status_win_);
    status_win_ = derwin(main_win_, client_height, status_width, client_start_y, client_width + 4);
    box(status_win_, 0, 0);
    
    // Client list header
    wattron(client_win_, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(client_win_, 1, 1, "Connected Clients:");
    wattroff(client_win_, COLOR_PAIR(3) | A_BOLD);
    
    // List clients
    for (size_t i = 0; i < clients_.size() && i < (size_t)client_height - 4; ++i) {
        mvwprintw(client_win_, 3 + i, 1, "%s", clients_[i].c_str());
    }
    
    // Status window header
    wattron(status_win_, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(status_win_, 1, 1, "Server Socket: %s", socket_address_.c_str());
    wattroff(status_win_, COLOR_PAIR(3) | A_BOLD);
    
    // Help text at bottom
    mvwprintw(main_win_, getmaxy(main_win_) - 2, 2, "Press 's' to view script, 'q' to quit");
    
    // Main window title
    wattron(main_win_, A_BOLD);
    mvwprintw(main_win_, 0, (win_width - 12) / 2, " PeerPulse ");
    wattroff(main_win_, A_BOLD);
    
    // Refresh all windows
    wrefresh(main_win_);
    wrefresh(client_win_);
    wrefresh(status_win_);
}

// Update the status message in the status window
void TUI::update_status(const std::string& status) {
    if (!status_win_) return;
    
    // Clear the second line of the status window
    wmove(status_win_, 2, 1);
    wclrtoeol(status_win_);
    
    // Display the new status message
    mvwprintw(status_win_, 2, 1, "Status: %s", status.c_str());
    wrefresh(status_win_);
}

// Display the script viewer screen
void TUI::show_script_viewer() {
    // Save main window state
    PANEL* saved_panel = main_panel_;
    WINDOW* saved_win = main_win_;
    
    // Create fullscreen window for script viewer
    int height = term_height_ - 2;
    int width = term_width_ - 2;
    WINDOW* script_win = newwin(height, width, 1, 1);
    PANEL* script_panel = new_panel(script_win);
    
    // Enable special keys
    keypad(script_win, TRUE);
    
    // Add border
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
    
    // Display script title
    wattron(script_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(script_win, 1, 2, "PeerPulse Network Setup Script");
    wattroff(script_win, COLOR_PAIR(3) | A_BOLD);
    
    // Display script content
    for (size_t i = 0; i < script_lines.size() && i < (size_t)height - 5; ++i) {
        mvwprintw(script_win, 3 + i, 2, "%s", script_lines[i].c_str());
    }
    
    // Add window title
    wattron(script_win, A_BOLD);
    mvwprintw(script_win, 0, (width - 18) / 2, " Script Viewer ");
    wattroff(script_win, A_BOLD);
    
    // Add footer with instructions
    mvwprintw(script_win, height - 2, 2, "Press any key to return to main interface");
    
    // Refresh the window and panel
    wrefresh(script_win);
    update_panels();
    doupdate();
    
    // Wait for a key press
    wgetch(script_win);
    
    // Clean up
    del_panel(script_panel);
    delwin(script_win);
    
    // Restore main interface
    top_panel(saved_panel);
    update_panels();
    wrefresh(saved_win);
    doupdate();
}

// Main application loop
void TUI::run() {
    // Start with the intro screen
    create_intro_screen();
    
    // Main event loop
    bool should_exit = false;
    while (!should_exit) {
        // Get user input
        int ch;
        if (intro_win_) {
            ch = wgetch(intro_win_);
        } else {
            ch = wgetch(main_win_);
        }
        
        // Handle input
        if (ch == 'q' && !intro_win_) {
            should_exit = true;
        } else if (ch == KEY_ENTER || ch == '\n') {
            if (intro_win_) {
                destroy_intro_screen();
                create_main_interface();
            }
        } else if (ch == 's' && !intro_win_) {
            show_script_viewer();
        }
    }
    
    // Clean up before exiting
    cleanup_resources();
}
