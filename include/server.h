#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdio.h> // Standard C File IO for simplicity
#include <client.h>
#include <tui.h>
#include <pthread.h>


constexpr int PORT = 8000;

class PeerServer {
private:
    std::string hostname;

    int sock_fd;

    // Multithread safe
    pthread_mutex_t _clients_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t _clients_cond = PTHREAD_COND_INITIALIZER;
    pthread_t socket_thread;
    std::vector<Client> _clients;

    TUI &interface;

    int num_clients = 0;
    
    FILE *file;
    char* _script_buf;
    
public:
    PeerServer(TUI &interface);
    void addFile(std::string& file_name);

    static void *socket_thread_fn(void *v) {
        PeerServer* app = static_cast<PeerServer*>(v);
        app->start_socket();
        return NULL;
    };
    
    int start_socket();

    void run();

};
