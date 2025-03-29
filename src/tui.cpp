#include <tui.h>
#include <string>
#include <ncurses.h>
#include "tui.h"

TUI::TUI() {
    init_ncurses();
    socket_address_ = "0.0.0.0:8080";
    
    // Add some dummy clients
    clients_.push_back("Main Server (192.168.1.100:8080)");
    clients_.push_back("Worker Node 1 (192.168.1.101:8081)");
    clients_.push_back("Worker Node 2 (192.168.1.102:8082)");
}

TUI::~TUI() {
    del_panel(intro_panel_);
    del_panel(main_panel_);
    delwin(intro_win_);
    delwin(main_win_);
    delwin(client_win_);
    delwin(status_win_);
    endwin();
}

void TUI::init_ncurses() {
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
    int height = 10;
    int width = 40;
    int y = (term_height_ - height) / 2;
    int x = (term_width_ - width) / 2;
    
    intro_win_ = newwin(height, width, y, x);
    intro_panel_ = new_panel(intro_win_);
    box(intro_win_, 0, 0);
    
    render_intro_screen();
}

void TUI::render_intro_screen() {
    wattron(intro_win_, COLOR_PAIR(1));
    mvwprintw(intro_win_, 2, (40 - 9) / 2, "PeerCompute");
    wattroff(intro_win_, COLOR_PAIR(1));
    
    wattron(intro_win_, COLOR_PAIR(2));
    mvwprintw(intro_win_, 5, (40 - 8) / 2, " CONTINUE ");
    wattroff(intro_win_, COLOR_PAIR(2));
    
    mvwprintw(intro_win_, 7, (40 - 20) / 2, "Press Enter to start");
    
    update_panels();
    doupdate();
}

void TUI::destroy_intro_screen() {
    del_panel(intro_panel_);
    delwin(intro_win_);
    intro_win_ = nullptr;
    intro_panel_ = nullptr;
}

void TUI::create_main_interface() {
    main_win_ = newwin(term_height_ - 2, term_width_ - 2, 1, 1);
    main_panel_ = new_panel(main_win_);
    box(main_win_, 0, 0);
    
    // Client window (left 30%)
    int client_width = (term_width_ - 2) * 0.3;
    client_win_ = newwin(term_height_ - 4, client_width - 1, 2, 2);
    box(client_win_, 0, 0);
    
    // Status window (right 70%)
    int status_width = (term_width_ - 2) * 0.7 - 1;
    status_win_ = newwin(term_height_ - 4, status_width, 2, client_width + 1);
    box(status_win_, 0, 0);
    
    render_main_interface();
}

void TUI::render_main_interface() {
    // Main window title
    wattron(main_win_, A_BOLD);
    mvwprintw(main_win_, 0, (term_width_ - 2 - 9) / 2, "PeerCompute");
    wattroff(main_win_, A_BOLD);
    
    // Client list
    wattron(client_win_, COLOR_PAIR(3));
    mvwprintw(client_win_, 1, 1, "Connected Clients:");
    wattroff(client_win_, COLOR_PAIR(3));
    
    for (size_t i = 0; i < clients_.size(); ++i) {
        mvwprintw(client_win_, 3 + i, 1, clients_[i].c_str());
    }
    
    // Status window
    wattron(status_win_, COLOR_PAIR(3));
    mvwprintw(status_win_, 1, 1, "Server Socket: %s", socket_address_.c_str());
    wattroff(status_win_, COLOR_PAIR(3));
    
    // Help text
    mvwprintw(main_win_, term_height_ - 3, 2, "Press 'a' to add client, 'q' to quit");
    
    update_panels();
    doupdate();
}

void TUI::add_dummy_client() {
    static int counter = 3;
    clients_.push_back("Worker Node " + std::to_string(counter) + 
                      " (192.168.1." + std::to_string(100 + counter) + 
                      ":808" + std::to_string(counter) + ")");
    counter++;
    render_main_interface();
}

void TUI::handle_input() {
    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '\n':
            case KEY_ENTER:
                if (intro_win_) {
                    destroy_intro_screen();
                    create_main_interface();
                }
                break;
            case 'a':
                if (!intro_win_) {
                    add_dummy_client();
                }
                break;
        }
    }
}

void TUI::run() {
    create_intro_screen();
    handle_input();
}
