#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton"); return 1;
    }

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); close(sockfd); return 1;
    }

    printf("Connected to Convex Hull server on %s:%d\n", server_ip, port);
    printf("Available commands:\n");
    printf("  Newgraph\n  Newpoint x y\n  Removepoint x y\n  CH\n  EXIT\n\n");

    // Set socket to non-blocking mode for better error detection
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    char buffer[1024];
    
    ssize_t n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (n > 0) {
        buffer[n] = 0;
        printf("Server welcome:\n%s\n", buffer);
    } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // No data available yet, this is normal for non-blocking sockets
        printf("Waiting for server welcome message...\n");
        // Wait a bit and try again
        usleep(100000); // 100ms
        n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
        if (n > 0) {
            buffer[n] = 0;
            printf("Server welcome:\n%s\n", buffer);
        }
    }
    
    while (true) {
        // Check if server is still connected before showing prompt
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            printf("Server disconnected (connection check).\n");
            break;
        }
        
        printf("> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcasecmp(buffer, "EXIT") == 0) {
            printf("Exiting...\n");
            break;
        }

        strcat(buffer, "\n");
        if (send(sockfd, buffer, strlen(buffer), 0) <= 0) {
            perror("send");
            printf("Server disconnected (send error).\n");
            break;
        }

        // Check if server is still connected before waiting for response
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            printf("Server disconnected (socket error).\n");
            break;
        }

        // Use select to check if data is available or connection is closed
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 2;  // 2 seconds timeout
        timeout.tv_usec = 0;
        
        int ready = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
        if (ready <= 0) {
            if (ready == 0) {
                printf("Server response timeout - server may have disconnected.\n");
            } else {
                printf("Select error - server may have disconnected.\n");
            }
            break;
        }
        
        // Check if socket is ready for reading
        if (!FD_ISSET(sockfd, &readfds)) {
            printf("Socket not ready for reading - server may have disconnected.\n");
            break;
        }
        
        n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) {
            if (n == 0) {
                printf("Server disconnected gracefully.\n");
                break;  // Exit immediately when server disconnects
                    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, this is normal for non-blocking sockets
            // Wait a bit and try again
            usleep(10000); // 10ms
            n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
            if (n <= 0) {
                if (n == 0) {
                    printf("Server disconnected gracefully.\n");
                } else {
                    printf("Server disconnected (recv error: %s).\n", strerror(errno));
                }
                break;
            }
        } else {
            printf("Server disconnected (recv error: %s).\n", strerror(errno));
            break;
        }
        }
        buffer[n] = 0;
        printf("Server response:\n%s\n", buffer);
    }

    printf("Closing connection...\n");
    shutdown(sockfd, SHUT_RDWR);  // Force close the connection
    close(sockfd);
    return 0;
}