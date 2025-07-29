#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <cstdlib>
#include <arpa/inet.h>

#define EXIT_FAILURE 1
#define NFDS 2

int main(int argc, char *argv[])
{
    int running = 1;
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        return 1;
    }
    
    const char *server_ip = argv[1];
    int port = std::atoi(argv[2]);
    
    // Validate port
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number: " << port << std::endl;
        return 1;
    }
    
    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }
    
    // Setup polling
    struct pollfd fds[2];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;
    
    std::cout << "Connected to Convex Hull server on " << server_ip << ":" << port << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << " Newgraph - Create new empty graph" << std::endl;
    std::cout << " Newpoint x y - Add point to graph" << std::endl;
    std::cout << " Removepoint x y - Remove point from graph" << std::endl;
    std::cout << " CH - Compute convex hull" << std::endl;
    std::cout << " EXIT - Disconnect from server" << std::endl;
    std::cout << std::endl << "> ";
    std::cout.flush();
    
    while (running) {
        int poll_count = poll(fds, NFDS, -1);
        if (poll_count < 0) {
            perror("poll");
            break;
        }
        
        // Handle server response
        if (fds[0].revents & POLLIN) {
            char response[1024];
            std::memset(response, 0, sizeof(response));
            
            ssize_t bytes_read = read(sockfd, response, sizeof(response) - 1);
            if (bytes_read <= 0) {
                std::cout << "\nServer disconnected." << std::endl;
                break;
            }
            
            response[bytes_read] = '\0';
            std::cout << "Server: " << response;
            
            // Print prompt only if response doesn't end with newline
            if (response[bytes_read - 1] != '\n') {
                std::cout << std::endl;
            }
            std::cout << "> ";
            std::cout.flush();
        }
        
        // Handle user input
        if (fds[1].revents & POLLIN) {
            char buffer[1024];
            std::memset(buffer, 0, sizeof(buffer));
            
            if (fgets(buffer, sizeof(buffer), stdin) == nullptr) {
                std::cout << "\nEOF received, exiting." << std::endl;
                break;
            }
            
            // Remove newline
            buffer[std::strcspn(buffer, "\n")] = 0;
            
            if (std::strcmp(buffer, "EXIT") == 0 || std::strcmp(buffer, "exit") == 0) {
                std::cout << "Exiting by user request." << std::endl;
                running = 0;
                break;
            }
            
            // Send command to server (add newline back for server protocol)
            std::strcat(buffer, "\n");
            ssize_t sent = send(sockfd, buffer, std::strlen(buffer), 0);
            if (sent < 0) {
                perror("send");
                break;
            }
        }
    }
    
    close(sockfd);
    return 0;
}