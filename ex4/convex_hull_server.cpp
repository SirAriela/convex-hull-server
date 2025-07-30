#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include "../ex3/convex_hull.hpp"
#include "../ex3/point.hpp"

#define BACKLOG 10
#define MAX_CLIENTS 12

// Global variable to control server shutdown
volatile sig_atomic_t running = 1;

void handle_sigint(int sig)
{
  (void)sig; // Explicitly mark parameter as used
  running = 0;
  printf("\nSIGINT received — shutting down server gracefully...\n");
  printf("Waiting for current operations to complete...\n");
}

//------------------- Graph functions ------------------------------------

// Global shared graph and convex hull object
std::vector<Point> shared_points;
ConvexHull* ch = nullptr;

void initializeGraph() {
    if (ch != nullptr) {
        delete ch;
    }
    shared_points.clear();
    ch = new ConvexHull(shared_points);
    printf("New graph initialized\n");
}

void addPointToGraph(float x, float y) {
    shared_points.emplace_back(x, y);
    if (ch != nullptr) {
        delete ch;
    }
    ch = new ConvexHull(shared_points);
    printf("Added point (%.2f, %.2f). Total points: %zu\n", x, y, shared_points.size());
}

void removePointFromGraph(float x, float y) {
    Point targetPoint(x, y);
    auto it = std::remove_if(shared_points.begin(), shared_points.end(),
        [&targetPoint](const Point& p) {
            return std::abs(p.getX() - targetPoint.getX()) < 0.001f && 
                   std::abs(p.getY() - targetPoint.getY()) < 0.001f;
        });
    
    if (it != shared_points.end()) {
        shared_points.erase(it, shared_points.end());
        if (ch != nullptr) {
            delete ch;
        }
        ch = new ConvexHull(shared_points);
        printf("Removed point (%.2f, %.2f). Total points: %zu\n", x, y, shared_points.size());
    } else {
        printf("Point (%.2f, %.2f) not found\n", x, y);
    }
}

void computeConvexHull() {
    if (shared_points.size() < 3) {
        printf("Need at least 3 points to compute convex hull\n");
        return;
    }
    
    if (ch != nullptr) {
        ch->findConvexHull();
        std::vector<Point> hull_points = ch->getConvexHullPoints();
        printf("Computed convex hull with %zu points:\n", hull_points.size());
        for (const auto& p : hull_points) {
            printf("  (%.2f, %.2f)\n", p.getX(), p.getY());
        }
        double area = ch->polygonArea();
        printf("Hull area: %.2f\n", area);
    }
}

void printCurrentGraph() {
    printf("Current graph has %zu points:\n", shared_points.size());
    for (size_t i = 0; i < shared_points.size(); ++i) {
        printf("  %zu: (%.2f, %.2f)\n", i+1, shared_points[i].getX(), shared_points[i].getY());
    }
}

//------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, handle_sigint);

    // Initialize graph
    initializeGraph();
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %d\n", port);
        return 1;
    }

    //--------------------tcp socket setup-------------------------------
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, BACKLOG) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    //---------------- fds setup for poll ------------------------
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 2;
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    printf("Convex Hull Server running on port %d...\n", port);
    printf("Available commands:\n");
    printf("  Newgraph - Create new empty graph\n");
    printf("  Newpoint x y - Add point to graph\n");
    printf("  Removepoint x y - Remove point from graph\n");
    printf("  CH - Compute convex hull\n");

    while (running)
    {
        int ready = poll(fds, nfds, 1000);
        if (ready < 0)
        {
            if (!running)
                break;
            perror("poll");
            break;
        }

        // Check if we should shutdown
        if (!running)
        {
            printf("Shutdown signal received, closing all connections...\n");
            break;
        }

        if (ready == 0)
            continue; // timeout, no events

        // Handle new connections
        if (fds[0].revents & POLLIN)
        {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd >= 0 && nfds < MAX_CLIENTS)
            {
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                fds[nfds].revents = 0;
                nfds++;
                printf("New client connected: fd=%d\n", client_fd);
                
                // Send welcome message
                const char* welcome = "Connected to Convex Hull Server\n"
                                     "Commands: Newgraph, Newpoint x y, Removepoint x y, CH\n";
                send(client_fd, welcome, strlen(welcome), 0);
            }
            else if (client_fd >= 0)
            {
                printf("Max clients reached, rejecting connection\n");
                close(client_fd);
            }
        }

        // Handle client data
        for (int i = nfds - 1; i >= 2; i--)
        {
            if (fds[i].revents & POLLIN)
            {
                char buffer[256];
                ssize_t len = read(fds[i].fd, buffer, sizeof(buffer) - 1);

                if (len <= 0)
                {
                    printf("Client disconnected: fd=%d\n", fds[i].fd);
                    close(fds[i].fd);
                    if (i < nfds - 1)
                    {
                        fds[i] = fds[nfds - 1];
                    }
                    nfds--;
                }
                else
                {
                    buffer[len] = '\0';
                    
                   
                    char cleaned[256];
                    int j = 0;
                    for (int k = 0; k < len && j < 255; k++) {
                        if (buffer[k] >= 32 && buffer[k] <= 126) { // רק תווים מדפיסים
                            cleaned[j++] = buffer[k];
                        }
                    }
                    cleaned[j] = '\0';
                    
                    // העתק בחזרה
                    strcpy(buffer, cleaned);
                    
                    printf("Received command: '%s'\n", buffer);

                    // Parse commands
                    if (strcmp(buffer, "Newgraph") == 0)
                    {
                        initializeGraph();
                        const char* response = "New graph created\n";
                        send(fds[i].fd, response, strlen(response), 0);
                    }
                    else if (strncmp(buffer, "Newpoint ", 9) == 0)
                    {
                        float x, y;
                        if (sscanf(buffer + 9, "%f %f", &x, &y) == 2)
                        {
                            addPointToGraph(x, y);
                            char response[128];
                            snprintf(response, sizeof(response), "Point (%.2f, %.2f) added\n", x, y);
                            send(fds[i].fd, response, strlen(response), 0);
                        }
                        else
                        {
                            const char* error = "Invalid format. Use: Newpoint x y\n";
                            send(fds[i].fd, error, strlen(error), 0);
                        }
                    }
                    else if (strncmp(buffer, "Removepoint ", 12) == 0)
                    {
                        float x, y;
                        if (sscanf(buffer + 12, "%f %f", &x, &y) == 2)
                        {
                            size_t old_size = shared_points.size();
                            removePointFromGraph(x, y);
                            if (shared_points.size() < old_size)
                            {
                                char response[128];
                                snprintf(response, sizeof(response), "Point (%.2f, %.2f) removed\n", x, y);
                                send(fds[i].fd, response, strlen(response), 0);
                            }
                            else
                            {
                                char response[128];
                                snprintf(response, sizeof(response), "Point (%.2f, %.2f) not found\n", x, y);
                                send(fds[i].fd, response, strlen(response), 0);
                            }
                        }
                        else
                        {
                            const char* error = "Invalid format. Use: Removepoint x y\n";
                            send(fds[i].fd, error, strlen(error), 0);
                        }
                    }
                   else if (strcmp(buffer, "CH") == 0)
                   {
                       if (shared_points.size() < 3)
                       {
                           const char* error = "Need at least 3 points to compute convex hull\n";
                           send(fds[i].fd, error, strlen(error), 0);
                       }
                       else
                       {
                           // קרא לפונקציה שכבר קיימת במקום לחזור על הקוד
                           computeConvexHull();
                           
                           // עכשיו תכין את התגובה ללקוח
                           std::vector<Point> hull_points = ch->getConvexHullPoints();
                           double area = ch->polygonArea();

                           char response[1024];
                           int offset = snprintf(response, sizeof(response), 
                                                 "Convex Hull (%zu points):\n", hull_points.size());

                           for (const auto& p : hull_points)
                           {
                               offset += snprintf(response + offset, sizeof(response) - offset,
                                                  "(%.2f, %.2f)\n", p.getX(), p.getY());
                           }

                           offset += snprintf(response + offset, sizeof(response) - offset,
                                              "Area: %.2f\n", area);

                           send(fds[i].fd, response, offset, 0);
                       }
                    }
                    else
                    {
                        printf("Unknown command: '%s'\n", buffer);
                        const char* error = "Unknown command. Available: Newgraph, Newpoint x y, Removepoint x y, CH\n";
                        send(fds[i].fd, error, strlen(error), 0);
                    }
                }
            }
        }

        // Handle stdin (server console commands)
        if (fds[1].revents & POLLIN)
        {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), stdin) != NULL)
            {
                char *newline = strchr(buffer, '\n');
                if (newline)
                    *newline = '\0';

                if (strcmp(buffer, "status") == 0)
                {
                    printCurrentGraph();
                }
                else if (strcmp(buffer, "quit") == 0)
                {
                    running = 0;
                }
                else
                {
                    printf("Server commands: status, quit\n");
                }
            }
        }

        // Clear all revents for next iteration
        for (int i = 0; i < nfds; i++)
        {
            fds[i].revents = 0;
        }
    }

    // Cleanup
    printf("Shutting down server...\n");
    
    // Close all client connections
    for (int i = 2; i < nfds; i++)
    {
        printf("Closing client connection: fd=%d\n", fds[i].fd);
        close(fds[i].fd);
    }
    
    // Close listening socket
    printf("Closing listening socket: fd=%d\n", listen_fd);
    close(listen_fd);
    
    // Clean up memory
    if (ch != nullptr) {
        delete ch;
        ch = nullptr;
    }
    
    printf("Server terminated.\n");
    return 0;
}