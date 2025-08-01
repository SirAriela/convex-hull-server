#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include "../ex3/convex_hull.hpp"
#include "../ex3/point.hpp"

#define BACKLOG 10

// Function declarations
void acceptClients(int listen_fd);
void handleClient(int client_fd);

//------------------- Thread-safe Graph functions ------------------------------------

// Global shared graph and convex hull object - WITH MUTEX PROTECTION
std::vector<Point> shared_points;
ConvexHull* ch = nullptr;
std::mutex graph_mutex;

// Global variable to control server shutdown
volatile sig_atomic_t running = 1;
int listen_fd = -1;
std::vector<std::thread> client_threads;
std::mutex threads_mutex;

void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
    printf("\nSIGINT received — shutting down server gracefully...\n");
    
    // Close listening socket to stop accepting new connections
    if (listen_fd > 0) {
        printf("Closing listening socket: fd=%d\n", listen_fd);
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
        listen_fd = -1;
    }
    
    // Clean up memory
    if (ch != nullptr) {
        delete ch;
        ch = nullptr;
    }
    
    printf("Server shut down gracefully.\n");
    exit(0);
}

void initializeGraph() {
    std::lock_guard<std::mutex> lock(graph_mutex);
    if (ch != nullptr) {
        delete ch;
    }
    shared_points.clear();
    ch = new ConvexHull(shared_points);
    printf("New graph initialized\n");
}

void addPointToGraph(float x, float y) {
    std::lock_guard<std::mutex> lock(graph_mutex);
    
    // Check if point already exists
    for (const auto& p : shared_points) {
        if (std::abs(p.getX() - x) < 0.001f && std::abs(p.getY() - y) < 0.001f) {
            printf("Point (%.2f, %.2f) already exists, skipping\n", x, y);
            return;
        }
    }
    
    shared_points.emplace_back(x, y);
    if (ch != nullptr) {
        delete ch;
    }
    ch = new ConvexHull(shared_points);
    printf("Added point (%.2f, %.2f). Total points: %zu\n", x, y, shared_points.size());
}

void removePointFromGraph(float x, float y) {
    std::lock_guard<std::mutex> lock(graph_mutex);
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

std::pair<std::vector<Point>, double> computeConvexHullSafe() {
    std::lock_guard<std::mutex> lock(graph_mutex);
    
    std::vector<Point> hull_points;
    double area = 0.0;
    
    if (shared_points.size() >= 3 && ch != nullptr) {
        ch->findConvexHull();
        hull_points = ch->getConvexHullPoints();
        area = ch->polygonArea();
        
        printf("Computed convex hull with %zu points, area: %.2f\n", hull_points.size(), area);
        for (const auto& p : hull_points) {
            printf("  (%.2f, %.2f)\n", p.getX(), p.getY());
        }
    }
    
    return {hull_points, area};
}

void printCurrentGraph() {
    std::lock_guard<std::mutex> lock(graph_mutex);
    printf("Current graph has %zu points:\n", shared_points.size());
    for (size_t i = 0; i < shared_points.size(); ++i) {
        printf("  %zu: (%.2f, %.2f)\n", i+1, shared_points[i].getX(), shared_points[i].getY());
    }
}

size_t getPointCount() {
    std::lock_guard<std::mutex> lock(graph_mutex);
    return shared_points.size();
}

//------------------------------------------------------------------------

void handleClient(int client_fd) {
    printf("[Thread %lu] Handling client fd=%d\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), client_fd);
    
    // Send welcome message
    const char* welcome = "Connected to Convex Hull Server\n"
                         "Commands: Newgraph, Newpoint x y, Removepoint x y, CH\n";
    send(client_fd, welcome, strlen(welcome), 0);
    
    while (running) {
        char buffer[256];
        ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);

        if (len <= 0) {
            if (len == 0) {
                printf("[Thread %lu] Client disconnected gracefully: fd=%d\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), client_fd);
            } else {
                printf("[Thread %lu] Client connection error: fd=%d (%s)\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), client_fd, strerror(errno));
            }
            break;
        }

        buffer[len] = '\0';
        
        // Clean non-printable characters
        char cleaned[256];
        int j = 0;
        for (int k = 0; k < len && j < 255; k++) {
            if (buffer[k] >= 32 && buffer[k] <= 126) {
                cleaned[j++] = buffer[k];
            }
        }
        cleaned[j] = '\0';
        strcpy(buffer, cleaned);
        
        printf("[Thread %lu] Received command: '%s'\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), buffer);

        // Parse commands
        if (strcmp(buffer, "Newgraph") == 0) {
            initializeGraph();
            const char* response = "New graph created\n";
            send(client_fd, response, strlen(response), 0);
        }
        else if (strncmp(buffer, "Newpoint ", 9) == 0) {
            float x, y;
            if (sscanf(buffer + 9, "%f %f", &x, &y) == 2) {
                addPointToGraph(x, y);
                char response[128];
                snprintf(response, sizeof(response), "Point (%.2f, %.2f) added\n", x, y);
                send(client_fd, response, strlen(response), 0);
            } else {
                const char* error = "Invalid format. Use: Newpoint x y\n";
                send(client_fd, error, strlen(error), 0);
            }
        }
        else if (strncmp(buffer, "Removepoint ", 12) == 0) {
            float x, y;
            if (sscanf(buffer + 12, "%f %f", &x, &y) == 2) {
                size_t old_size = getPointCount();
                removePointFromGraph(x, y);
                if (getPointCount() < old_size) {
                    char response[128];
                    snprintf(response, sizeof(response), "Point (%.2f, %.2f) removed\n", x, y);
                    send(client_fd, response, strlen(response), 0);
                } else {
                    char response[128];
                    snprintf(response, sizeof(response), "Point (%.2f, %.2f) not found\n", x, y);
                    send(client_fd, response, strlen(response), 0);
                }
            } else {
                const char* error = "Invalid format. Use: Removepoint x y\n";
                send(client_fd, error, strlen(error), 0);
            }
        }
        else if (strcmp(buffer, "CH") == 0) {
            auto [hull_points, area] = computeConvexHullSafe();
            
            if (hull_points.empty()) {
                const char* error = "Need at least 3 points to compute convex hull\n";
                send(client_fd, error, strlen(error), 0);
            } else {
                char response[1024];
                int offset = snprintf(response, sizeof(response), 
                                     "Convex Hull (%zu points):\n", hull_points.size());

                for (const auto& p : hull_points) {
                    offset += snprintf(response + offset, sizeof(response) - offset,
                                      "(%.2f, %.2f)\n", p.getX(), p.getY());
                }

                offset += snprintf(response + offset, sizeof(response) - offset,
                                  "Area: %.2f\n", area);

                send(client_fd, response, offset, 0);
            }
        }
        else {
            printf("[Thread %lu] Unknown command: '%s'\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), buffer);
            const char* error = "Unknown command. Available: Newgraph, Newpoint x y, Removepoint x y, CH\n";
            send(client_fd, error, strlen(error), 0);
        }
    }
    
    close(client_fd);
    printf("[Thread %lu] Client thread ended for fd=%d\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), client_fd);
}

void acceptClients(int listen_fd) {
    printf("[Accept Thread] Started accepting connections\n");
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running) {
                perror("accept");
            }
            continue;
        }
        
        printf("[Accept Thread] New client connected: fd=%d\n", client_fd);
        
        // Create new thread for this client
        std::thread client_thread(handleClient, client_fd);
        
        // Store thread reference for cleanup
        {
            std::lock_guard<std::mutex> lock(threads_mutex);
            client_threads.push_back(std::move(client_thread));
        }
        
        // Clean up finished threads
        {
            std::lock_guard<std::mutex> lock(threads_mutex);
            client_threads.erase(
                std::remove_if(client_threads.begin(), client_threads.end(),
                    [](std::thread& t) {
                        return !t.joinable();
                    }),
                client_threads.end()
            );
        }
    }
    
    printf("[Accept Thread] Stopped accepting connections\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, handle_sigint);

    // Initialize graph
    initializeGraph();
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %d\n", port);
        return 1;
    }

    //--------------------tcp socket setup-------------------------------
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Convex Hull Server running on port %d...\n", port);
    printf("Available commands:\n");
    printf("  Newgraph - Create new empty graph\n");
    printf("  Newpoint x y - Add point to graph\n");
    printf("  Removepoint x y - Remove point from graph\n");
    printf("  CH - Compute convex hull\n");

    // Store listen_fd globally for signal handler
    ::listen_fd = listen_fd;
    
    // Accept thread
    std::thread accept_thread(acceptClients, listen_fd);

    // Main thread handles stdin
    char buffer[256];
    while (running) {
        printf("Server> ");
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';

            if (strcmp(buffer, "status") == 0) {
                printCurrentGraph();
            }
            else if (strcmp(buffer, "quit") == 0) {
                running = 0;
            }
            else {
                printf("Server commands: status, quit\n");
            }
        }
    }

    // Cleanup
    printf("Shutting down server...\n");
    running = 0;
    
    // Wait for accept thread to finish
    accept_thread.join();
    
    // Clean up client threads
    {
        std::lock_guard<std::mutex> lock(threads_mutex);
        printf("Waiting for %zu client threads to finish...\n", client_threads.size());
        for (auto& thread : client_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    if (listen_fd > 0) {
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
    
    if (ch != nullptr) {
        delete ch;
        ch = nullptr;
    }
    
    printf("Server terminated.\n");
    return 0;
}