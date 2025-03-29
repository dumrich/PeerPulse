#pragma once

#include <ncurses.h>
#include <panel.h>
#include <vector>
#include <string>

class TUI {
public:
    TUI();
    ~TUI();
    
    void run();
    
private:
    // Window and panel pointers
    WINDOW* intro_win_ = nullptr;
    WINDOW* main_win_ = nullptr;
    WINDOW* client_win_ = nullptr;
    WINDOW* status_win_ = nullptr;
    PANEL* intro_panel_ = nullptr;
    PANEL* main_panel_ = nullptr;
    
    // Client data
    std::vector<std::string> clients_;
    std::string socket_address_;
    
    // Dimensions
    int term_height_;
    int term_width_;
    
    // Initialization methods
    void init_ncurses();
    
    // Screen setup methods
    void create_intro_screen();
    void render_intro_screen();
    void destroy_intro_screen();
    
    void create_main_interface();
    void render_main_interface();
    
    // Script viewer
    void show_script_viewer();
    
    // UI utilities
    void update_status(const std::string& status);
    
    // Cleanup
    void cleanup_resources();
};
