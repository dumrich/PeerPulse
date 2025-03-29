#include <iostream>
#include <tui.h>
#include <server.h>

int main(int argc, char** argv) {
    TUI tui;
    PeerServer server (tui);
    server.run();
    return 0;
}
