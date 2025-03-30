#include <client.h>
#include <sys/socket.h>
#include <unistd.h>


size_t Client::send_buf(const char *buf, size_t file_size) {
    size_t bytes_sent = send(client_fd, buf, file_size, 0);

    return bytes_sent;
}

size_t Client::recv_buf(char *buf, size_t file_size) {
    size_t bytes_sent = recv(client_fd, buf, file_size, 0);

    return bytes_sent;
}

size_t Client::send_int(int value) {
    // Convert to network byte order (big-endian)
    uint32_t net_value = htonl(static_cast<uint32_t>(value));
    
    // Send the 4-byte integer
    ssize_t sent = send(client_fd, &net_value, sizeof(net_value), 0);
    return sent;
}

