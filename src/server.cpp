#include <memory>
#include <pthread.h>
#include <server.h>
#include <tui.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h> 

PeerServer::PeerServer(TUI &interface) : interface(interface){};

// Function to print the actual bound address
void print_bound_address(int sockfd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    if (getsockname(sockfd, (struct sockaddr *)&addr, &addr_len) < 0) {
        perror("getsockname");
        return;
    }
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    
    printf("\nServer is bound to:\n");
    printf("  IP Address: %s\n", ip);
    printf("  Port: %d\n", ntohs(addr.sin_port));
    
    if (strcmp(ip, "0.0.0.0") == 0) {
        printf("\nNote: 0.0.0.0 means the server is listening on all available interfaces\n");
    }
}

long get_file_size(FILE* file) {
    // Save current position
    long original_pos = ftell(file);
    if (original_pos == -1L) {
        fclose(file);
        return -1;
    }

    // Seek to end
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }

    // Get size
    long size = ftell(file);
    
    // Restore original position
    if (fseek(file, original_pos, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    fclose(file);
    return size;
}

void PeerServer::addFile(std::string &file_name) {
        long file_size;
        size_t read;
        file = fopen(file_name.c_str(), "w");
        if (file == NULL) {
            perror("Error opening file.");
            return;
        }

        file_size = get_file_size(file);
        char *files = new char[file_size];
        read = fread(files, sizeof(char), file_size, file);
        _script_buf = files;
}

int PeerServer::start_socket() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Create socket file descriptor
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options (reuse address)
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
      
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket to port
    if (bind(sock_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening
    if (listen(sock_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        Client c;
        if ((c.client_fd = accept(sock_fd, (struct sockaddr*)&c.address, &c.length)) < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Connection accepted from %s:%d\n", 
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        pthread_cond_signal(&_clients_cond);
        
    }
    
}

void PeerServer::run() {
    pthread_create(&socket_thread, nullptr, &PeerServer::socket_thread_fn, this);
    pthread_join(socket_thread, nullptr);
}
