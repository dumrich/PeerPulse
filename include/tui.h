#pragma once

#include <ncurses.h>
#include <panel.h>
#include <vector>
#include <string>
#include <pthread.h>

// Forward declaration
class PeerServer;
class Client;

class TUI {
public:
    TUI();
    ~TUI();
    
    void run();
    
    // Add a new client to the TUI's client list
    void add_client(const std::string& client_info);
    
    // Add a message to the status window
    void add_status_message(const std::string& message);
    
    // Set the server reference for client monitoring
    void set_server_ref(PeerServer* server, pthread_mutex_t* clients_mutex, 
                      pthread_cond_t* clients_cond, std::vector<Client>* clients);
    
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
    
    // Status messages for the right panel
    std::vector<std::string> status_messages_;
    
    // Dimensions
    int term_height_;
    int term_width_;
    
    // Thread synchronization
    pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Server references for client checking
    PeerServer* server_ref = nullptr;
    pthread_mutex_t* server_clients_mutex = nullptr;
    pthread_cond_t* server_clients_cond = nullptr;
    std::vector<Client>* server_clients = nullptr;
    size_t last_client_count = 0;
    
    // Method to check for new clients on demand
    void check_for_new_clients();
    
    // Private methods
    void init_ncurses();
    void create_intro_screen();
    void create_main_interface();
    void destroy_intro_screen();
    void render_intro_screen();
    void render_main_interface();
    void handle_input();
    void show_script_viewer();

    void distribute_network();
};
