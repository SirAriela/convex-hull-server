#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
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

    char buffer[1024];
    
    ssize_t n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (n > 0) {
        buffer[n] = 0;
        printf("Server welcome:\n%s\n", buffer);
    }
    
    while (true) {
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

        n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) {
            if (n == 0) {
                printf("Server disconnected gracefully.\n");
            } else {
                printf("Server disconnected (recv error: %s).\n", strerror(errno));
            }
            break;
        }
        buffer[n] = 0;
        printf("Server response:\n%s\n", buffer);
    }

    printf("Closing connection...\n");
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return 0;
} 