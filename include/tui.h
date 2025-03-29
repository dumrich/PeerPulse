#pragma once

#include <ncurses.h>
#include <panel.h>
#include <vector>
#include <string>
#include <pthread.h>

class TUI {
public:
    TUI();
    ~TUI();
    
    void run();

    bool in_main = false;
    
private:
    // Window and panel pointers
    WINDOW* intro_win_;
    WINDOW* main_win_;
    WINDOW* client_win_;
    WINDOW* status_win_;
    PANEL* intro_panel_;
    PANEL* main_panel_;
    
    // Client data
    std::vector<std::string> clients_;
    std::string socket_address_;
    
    // Dimensions
    int term_height_;
    int term_width_;
    
    // Thread synchronization
    pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Private methods
    void init_ncurses();
    void create_intro_screen();
    void create_main_interface();
    void destroy_intro_screen();
    void render_intro_screen();
    void render_main_interface();
    void add_dummy_client();
    void handle_input();
    void show_script_viewer();
};
