#include <memory>
#include <thread>
#include <chrono>
#include <pthread.h>
#include <server.h>
#include <string>
#include <tui.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h> 
#include <string.h>
#include <utility>
#include <fcntl.h>

PeerServer::PeerServer(TUI &interface) : interface(interface) {
    // Set up the TUI to monitor our client list
    interface.set_server_ref(this, &_clients_mutex, &_clients_cond, &_clients);
}

void PeerServer::set_item_count(int item_count) {
        this->item_count = item_count;
}

int PeerServer::get_item_count() {
        return item_count;
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

    while (1) {
        Client c;
        if ((c.client_fd = accept(sock_fd, (struct sockaddr*)&c.address, &c.length)) < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Connection accepted from %s:%d\n", 
               inet_ntoa(c.address.sin_addr), ntohs(c.address.sin_port));

        // Thread-safe update of server's client list
        pthread_mutex_lock(&_clients_mutex);
        num_clients++;
        c.id = num_clients;  // Assign a client ID
        _clients.push_back(c);
        
        // Signal the condition variable - the TUI is listening to this
        pthread_cond_signal(&_clients_cond);
        pthread_mutex_unlock(&_clients_mutex);
    }
}

static std::vector<std::pair<int, int>> splitNumber(int total, int n) {
    std::vector<std::pair<int, int>> pairs;
    
    if (n <= 0 || total < 0) {
        return pairs; // return empty vector for invalid input
    }


    int chunkSize = total / n;
    int remainder = total % n;
    int start = 0;

    for (int i = 0; i < n; ++i) {
        int end = start + chunkSize - 1;
        
        // Distribute remainder across first few chunks
        if (i < remainder) {
            end += 1;
        }

        pairs.emplace_back(start, end);
        start = end + 1;
    }

    return pairs;
}

int PeerServer::send_files() {
    pthread_cancel(socket_thread);

    std::string status_message;

    std::vector<std::pair<int, int>> pairs = splitNumber(get_item_count(), num_clients);

    std::string bounds;
    
    pthread_mutex_lock(&_clients_mutex);

    for (int i = 0; i < num_clients; i++) {
        size_t data = _clients[i].send_buf(_script_buf, file_size);
        std::pair<int, int>& set = pairs[i];
      
        if (data != file_size) {
                return -1;
        }
        status_message = "Sent file for client " + std::to_string(i) + "\n";
        interface.add_status_message(status_message);
        bounds = std::to_string(set.first) + " " + std::to_string(set.second);
        interface.add_status_message("Computing bounds " + bounds);
        _clients[i].send_buf(bounds.c_str(), bounds.size());
    }
    pthread_mutex_unlock(&_clients_mutex);
    return 0;
}

void write_buffer_to_file(const char* filename, const char* buffer, size_t buffer_size) {
    FILE* file = fopen(filename, "ab"); // Open in append binary mode
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    size_t written = fwrite(buffer, 1, buffer_size, file);
    if (written != buffer_size) {
        perror("Error writing to file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    fclose(file);
}

int PeerServer::recv_output() {
    constexpr size_t outbuf_size = 4096*10;
    char* output_buf = new char[outbuf_size];
    
    interface.add_status_message("Receiving output from clients...");

    for (int i = 0; i < num_clients; i++) {
      //std::this_thread::sleep_for(std::chrono::milliseconds(400));
        interface.add_status_message("Reading output from client " + std::to_string(i));
        
        size_t total_received = 0;
        bool more_data = true;
        
        // Continue receiving until no more data or connection closed
        while (more_data) {
            pthread_mutex_lock(&_clients_mutex);
            
            // Check if client connection is still valid
            if (i >= _clients.size() || _clients[i].client_fd < 0) {
                interface.add_status_message("Client " + std::to_string(i) + " connection closed");
                pthread_mutex_unlock(&_clients_mutex);
                break;
            }
            
            // Set non-blocking mode to check for more data
            int flags = fcntl(_clients[i].client_fd, F_GETFL, 0);
            fcntl(_clients[i].client_fd, F_SETFL, flags | O_NONBLOCK);
            
            // Try to receive data
            ssize_t received = recv(_clients[i].client_fd, output_buf, outbuf_size, 0);
            
            // Restore blocking mode
            fcntl(_clients[i].client_fd, F_SETFL, flags);
            
            if (received > 0) {
                // Got data, write it to file
                pthread_mutex_unlock(&_clients_mutex);
                write_buffer_to_file("out.txt", output_buf, received);
                total_received += received;
                interface.add_status_message("Received " + std::to_string(received) + " bytes from client " + std::to_string(i));
            }
            else if (received == 0) {
                // Connection closed
                interface.add_status_message("Client " + std::to_string(i) + " connection closed");
                pthread_mutex_unlock(&_clients_mutex);
                more_data = false;
            }
            else {
                // No data available or error
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more data available right now
                    pthread_mutex_unlock(&_clients_mutex);
                    
                    // If we've received anything, break and consider it complete
                    if (total_received > 0) {
                        more_data = false;
                    } else {
                        // Wait a bit and try again
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                } 
                else {
                    // Actual error
                    interface.add_status_message("Error receiving from client " + std::to_string(i) + ": " + strerror(errno));
                    pthread_mutex_unlock(&_clients_mutex);
                    more_data = false;
                }
            }
        }
        
        interface.add_status_message("Total received from client " + std::to_string(i) + ": " + std::to_string(total_received) + " bytes");
    }
    
    delete[] output_buf;
    interface.add_status_message("Finished receiving output from all clients");
    return 0;
}

void PeerServer::run() {
    pthread_create(&socket_thread, nullptr, &PeerServer::socket_thread_fn, this);
    
    // Run the TUI in the main thread
    interface.run();

    pthread_cancel(socket_thread);
    
}
