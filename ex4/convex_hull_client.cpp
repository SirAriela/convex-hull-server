#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define EXIT_FAILURE 1
#define NFDS 2
int main(int argc, char *argv[])
{
    int running = 1;
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    
    // Validate port
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %d\n", port);
        return 1;
    }
    
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;  // No need to close if socket creation failed
    }
    
    
    // Setup polling for socket and stdin
    struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(port);
if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
{
    perror("inet_pton");
    close(sockfd);
    return 1;
}

// Connect to server
if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
{
    perror("connect");
    close(sockfd);
    return 1;
}
    struct pollfd fds[2];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    printf("Connected to Convex Hull server on localhost:%d\n", port);
    printf("Available commands:\n");
    printf("  Newgraph - Create new empty graph\n");
    printf("  Newpoint x y - Add point to graph\n");
    printf("  Removepoint x y - Remove point from graph\n");
    printf("  CH - Compute convex hull\n");
    printf("  EXIT - Disconnect from server\n");
    printf("\n");

    while (running)
    {
        int poll_count = poll(fds, NFDS, -1);
        if (poll_count < 0)
        {
            perror("poll");
            break;
        }

        // Handle server response
        if (fds[0].revents & POLLIN)
        {
            char response[1024];
            ssize_t bytes_read = read(sockfd, response, sizeof(response) - 1);
            if (bytes_read <= 0)
            {
                printf("Server disconnected.\n");
                break;
            }
            response[bytes_read] = '\0';
            printf("Server: %s", response);
            
            // Print prompt for next command
            printf("> ");
            fflush(stdout);
        }

        // Handle user input
        if (fds[1].revents & POLLIN)
        {
            char buffer[1024];
            printf("Enter command: ");
            if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            {
                printf("\nEOF received, exiting.\n");
                break;
            }

            // Remove newline
            buffer[strcspn(buffer, "\n")] = 0;

            if (strcmp(buffer, "EXIT") == 0 || strcmp(buffer, "exit") == 0)
            {
                printf("Exiting by user request.\n");
                running = 0;
                break;
            }

            // Send command to server (add newline back for server protocol)
            strcat(buffer, "\n");
            ssize_t sent = send(sockfd, buffer, strlen(buffer), 0);
            if (sent < 0)
            {
                perror("send");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}