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
#include <string.h>

PeerServer::PeerServer(TUI &interface) : interface(interface) {
    // Set up the TUI to monitor our client list
    interface.set_server_ref(this, &_clients_mutex, &_clients_cond, &_clients);
}

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

void PeerServer::addFile(std::string& file_name) {
    // Use binary mode and check for errors
    FILE* file = fopen(file_name.c_str(), "rb");
    if (!file) {
        perror(("Error opening file: " + file_name).c_str());
        return;
    }

    // Get file size properly
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);  // Important: reset file position to start
    
    if (file_size <= 0) {
        fclose(file);
        fprintf(stderr, "Invalid file size or unable to determine size\n");
        return;
    }

    // Allocate buffer (+1 for null terminator if needed)
    char* buffer = new char[file_size + 1];
    
    // Read file content with proper error checking
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != static_cast<size_t>(file_size)) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file\n");
        } else if (ferror(file)) {
            perror("Error reading file");
        }
        
        delete[] buffer;
        fclose(file);
        return;
    }

    // Null-terminate if treating as string
    buffer[file_size] = '\0';
    
    // Handle the buffer (assuming _script_buf is a class member)
    delete[] _script_buf;  // Clean up previous allocation if any
    _script_buf = buffer;
    
    fclose(file);
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

    print_bound_address(sock_fd);

    while (1) {
        Client c;
        if ((c.client_fd = accept(sock_fd, (struct sockaddr*)&c.address, &c.length)) < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Connection accepted from %s:%d\n", 
               inet_ntoa(c.address.sin_addr), ntohs(c.address.sin_port));

        // Thread-safe update of server's client list
        num_clients++;
        pthread_mutex_lock(&_clients_mutex);
        c.id = num_clients;  // Assign a client ID
        _clients.push_back(c);
        
        // Signal the condition variable - the TUI is listening to this
        pthread_cond_signal(&_clients_cond);
        pthread_mutex_unlock(&_clients_mutex);
    }
}

int PeerServer::send_files() {
    pthread_cancel(socket_thread);

    std::string status_message;

    pthread_mutex_lock(&_clients_mutex);

    for (int i = 0; i < num_clients; i++) {
            size_t data = _clients[i].send_buf(_script_buf, file_size);
            if (data != file_size) {
                    return -1;
            }

            status_message = "Sent file for client " + std::to_string(i) + "\n";
            interface.add_status_message(status_message);
    }
    pthread_mutex_unlock(&_clients_mutex);
    return 0;
}

void PeerServer::run() {
    pthread_create(&socket_thread, nullptr, &PeerServer::socket_thread_fn, this);
    
    // Run the TUI in the main thread
    interface.run();

    pthread_cancel(socket_thread);
    
}
