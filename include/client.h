#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

class Client {
public:
    int client_fd;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);
    int id;
    
    Client() : client_fd(-1), id(-1) {
        memset(&address, 0, sizeof(address));
    }
};
