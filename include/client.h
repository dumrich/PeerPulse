#pragma once

#include <sys/socket.h>

class Client {
public:
    int client_fd;
    struct sockaddr* address;
    socklen_t length;
};
