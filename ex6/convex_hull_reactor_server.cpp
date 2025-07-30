#include "reactor.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <mutex>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>
#include <thread>
#include <chrono>
#include "../ex3/convex_hull.hpp"
#include "../ex3/point.hpp"

// ---------------- Shared Graph --------------------
std::vector<Point> shared_points;
ConvexHull* ch = nullptr;
std::mutex graphMutex;

// Reactor global
void* reactorInstance = nullptr;
int listen_fd = -1;
std::set<int> active_clients;
std::mutex clientsMutex;

// Function declarations
void initializeGraph();
void addPointToGraph(float x, float y);
void removePointFromGraph(float x, float y);
void computeConvexHull();
void clientHandler(int client_fd);
void acceptHandler(int listen_fd);
void cleanupAllClients();

// Signal handling
volatile sig_atomic_t running = 1;
void handle_sigint(int sig) {
    printf("\nReceived SIGINT, shutting down gracefully...\n");
    running = 0;
    
    // Give a moment for current operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Close all client connections
    cleanupAllClients();
    
    // Give another moment for cleanup to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop reactor
    if (reactorInstance) {
        printf("Stopping reactor...\n");
        stopReactor(reactorInstance);
        reactorInstance = nullptr;
    }
    
    // Close listening socket
    if (listen_fd > 0) {
        printf("Closing listening socket: fd=%d\n", listen_fd);
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
        listen_fd = -1;
    }
    
    // Clean up memory
    if (ch) {
        delete ch;
        ch = nullptr;
    }
    
    printf("Server shut down gracefully.\n");
    exit(0);
}

// ---------------- main ----------------------------
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    signal(SIGINT, handle_sigint);

    // Prepare listening socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { 
        perror("socket"); 
        return 1; 
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind"); 
        close(listen_fd);
        return 1;
    }
    
    if (listen(listen_fd, 10) < 0) {
        perror("listen"); 
        close(listen_fd);
        return 1;
    }

    printf("Convex Hull Reactor Server running on port %d...\n", port);

    // Initialize graph
    initializeGraph();

    // Start reactor
    reactorInstance = startReactor();
    if (!reactorInstance) {
        fprintf(stderr, "Failed to start reactor\n");
        close(listen_fd);
        return 1;
    }
    
    // Add listening socket to reactor
    if (addFdToReactor(reactorInstance, listen_fd, acceptHandler) != 0) {
        fprintf(stderr, "Failed to add listen_fd to reactor\n");
        stopReactor(reactorInstance);
        close(listen_fd);
        return 1;
    }

    printf("Reactor started, waiting for connections...\n");

    // Main server loop - just wait for shutdown signal
    while (running) {
        sleep(1);
        
        // Check if we should shutdown
        if (!running) {
            printf("Shutdown signal received in main loop\n");
            break;
        }
    }
    
    // Additional cleanup in case signal handler didn't run
    printf("Main loop ended, performing final cleanup...\n");
    
    // Give a moment for current operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Clean up all clients
    cleanupAllClients();
    
    // Give another moment for cleanup to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (reactorInstance) {
        stopReactor(reactorInstance);
        reactorInstance = nullptr;
    }
    
    if (listen_fd > 0) {
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
        listen_fd = -1;
    }
    
    if (ch) {
        delete ch;
        ch = nullptr;
    }
    
    return 0;
}

// ---------------- acceptHandler -------------------
void acceptHandler(int listen_fd) {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    printf("New client connected: fd=%d from %s:%d\n", 
           client_fd, client_ip, ntohs(client_addr.sin_port));
    
    // Add client to active clients set
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        active_clients.insert(client_fd);
    }
    
    // Add client to reactor
    if (addFdToReactor(reactorInstance, client_fd, clientHandler) != 0) {
        fprintf(stderr, "Failed to add client fd=%d to reactor\n", client_fd);
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            active_clients.erase(client_fd);
        }
        shutdown(client_fd, SHUT_RDWR);  // Force close the connection
        close(client_fd);
        return;
    }

    // Send welcome message
    const char* welcome =
        "Connected to Convex Hull Server\n"
        "Commands: Newgraph, Newpoint x y, Removepoint x y, CH\n";
    
    ssize_t sent = send(client_fd, welcome, strlen(welcome), MSG_NOSIGNAL);
    if (sent < 0) {
        perror("send welcome message");
        // Clean up client immediately if welcome message fails
        printf("Failed to send welcome message to fd=%d, cleaning up\n", client_fd);
        removeFdFromReactor(reactorInstance, client_fd);
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            active_clients.erase(client_fd);
        }
        close(client_fd);
    }
}

// ---------------- clientHandler -------------------
void clientHandler(int client_fd) {
    // Check if client is still in active clients before processing
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        if (active_clients.find(client_fd) == active_clients.end()) {
            printf("Client fd=%d no longer active, skipping\n", client_fd);
            return;
        }
    }
    
    // Set socket to non-blocking mode for better error detection
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    char buffer[256];
    ssize_t len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (len <= 0) {
        if (len == 0) {
            printf("Client disconnected gracefully: fd=%d\n", client_fd);
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, this is normal for non-blocking sockets
            return;
        } else {
            printf("Client connection error: fd=%d (%s)\n", client_fd, strerror(errno));
        }
        
        // Remove from reactor and active clients list
        printf("Cleaning up disconnected client: fd=%d\n", client_fd);
        removeFdFromReactor(reactorInstance, client_fd);
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            active_clients.erase(client_fd);
        }
        shutdown(client_fd, SHUT_RDWR);  // Force close the connection
        close(client_fd);
        return;
    }
    
    buffer[len] = '\0';
    
    // Clean up the input string
    char* end = buffer + len - 1;
    while (end >= buffer && (*end == '\n' || *end == '\r' || *end == ' ')) {
        *end = '\0';
        end--;
    }
    
    if (strlen(buffer) == 0) {
        return; // Empty command, ignore
    }
    
    printf("Received from fd=%d: '%s'\n", client_fd, buffer);

    // Check if client is still connected before processing
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        if (active_clients.find(client_fd) == active_clients.end()) {
            printf("Client fd=%d no longer in active clients, skipping command\n", client_fd);
            return;
        }
    }

    // Process command with graph mutex
    std::string response;
    {
        std::lock_guard<std::mutex> lock(graphMutex);

        if (strncasecmp(buffer, "Newgraph", 8) == 0) {
            initializeGraph();
            response = "New graph created\n";
        }
        else if (strncasecmp(buffer, "Newpoint", 8) == 0) {
            float x, y;
            if (sscanf(buffer + 8, "%f %f", &x, &y) == 2) {
                addPointToGraph(x, y);
                char resp[64];
                snprintf(resp, sizeof(resp), "Added point (%.2f, %.2f)\n", x, y);
                response = resp;
            } else {
                response = "Invalid format. Use: Newpoint x y\n";
            }
        }
        else if (strncasecmp(buffer, "Removepoint", 11) == 0) {
            float x, y;
            if (sscanf(buffer + 11, "%f %f", &x, &y) == 2) {
                removePointFromGraph(x, y);
                char resp[64];
                snprintf(resp, sizeof(resp), "Removed point (%.2f, %.2f)\n", x, y);
                response = resp;
            } else {
                response = "Invalid format. Use: Removepoint x y\n";
            }
        }
        else if (strncasecmp(buffer, "CH", 2) == 0) {
            if (shared_points.size() < 3) {
                response = "Need at least 3 points to compute convex hull\n";
            } else {
                computeConvexHull();
                std::vector<Point> hull = ch->getConvexHullPoints();
                double area = ch->polygonArea();
                
                char resp[1024];
                int off = snprintf(resp, sizeof(resp), "Convex Hull (%zu points):\n", hull.size());
                for (const auto& p : hull) {
                    off += snprintf(resp + off, sizeof(resp) - off,
                                   "(%.2f, %.2f)\n", p.getX(), p.getY());
                }
                off += snprintf(resp + off, sizeof(resp) - off, "Area: %.2f\n", area);
                response = resp;
            }
        }
        else {
            response = "Unknown command. Available: Newgraph, Newpoint x y, Removepoint x y, CH\n";
        }
    }

    // Check if client is still connected before sending response
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        if (active_clients.find(client_fd) == active_clients.end()) {
            printf("Client fd=%d no longer in active clients, skipping response\n", client_fd);
            return;
        }
    }

    // Send response
    ssize_t sent = send(client_fd, response.c_str(), response.length(), MSG_NOSIGNAL);
    if (sent < 0) {
        if (errno == EPIPE || errno == ECONNRESET) {
            printf("Client fd=%d disconnected while sending response\n", client_fd);
            // Remove from reactor and clean up
            printf("Cleaning up client after send error: fd=%d\n", client_fd);
            removeFdFromReactor(reactorInstance, client_fd);
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                active_clients.erase(client_fd);
            }
            shutdown(client_fd, SHUT_RDWR);  // Force close the connection
            close(client_fd);
        } else {
            perror("send response");
            // Also clean up on other send errors
            printf("Cleaning up client after send error: fd=%d\n", client_fd);
            removeFdFromReactor(reactorInstance, client_fd);
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                active_clients.erase(client_fd);
            }
            shutdown(client_fd, SHUT_RDWR);  // Force close the connection
            close(client_fd);
        }
    }
}

// ---------------- Graph Management Functions -------------------
void initializeGraph() {
    if (ch) {
        delete ch;
        ch = nullptr;
    }
    shared_points.clear();
    ch = new ConvexHull(shared_points);
    printf("DEBUG: Graph initialized\n");
}

void addPointToGraph(float x, float y) {
    // Check if point already exists
    for (const auto& p : shared_points) {
        if (std::abs(p.getX() - x) < 0.001f && std::abs(p.getY() - y) < 0.001f) {
            printf("DEBUG: Point (%.2f, %.2f) already exists, skipping\n", x, y);
            return;
        }
    }
    
    shared_points.emplace_back(x, y);
    if (ch) {
        delete ch;
    }
    ch = new ConvexHull(shared_points);
    printf("DEBUG: Added point (%.2f, %.2f), total points: %zu\n", x, y, shared_points.size());
}

void removePointFromGraph(float x, float y) {
    auto it = std::remove_if(shared_points.begin(), shared_points.end(),
        [x, y](const Point& p) { 
            return std::abs(p.getX() - x) < 0.001f && std::abs(p.getY() - y) < 0.001f; 
        });
    
    if (it != shared_points.end()) {
        shared_points.erase(it, shared_points.end());
        if (ch) {
            delete ch;
        }
        ch = new ConvexHull(shared_points);
        printf("DEBUG: Removed point (%.2f, %.2f), remaining points: %zu\n", x, y, shared_points.size());
    } else {
        printf("DEBUG: Point (%.2f, %.2f) not found for removal\n", x, y);
    }
}

void computeConvexHull() {
    if (ch && shared_points.size() >= 3) {
        ch->findConvexHull();
        printf("DEBUG: Computed convex hull for %zu points\n", shared_points.size());
    }
}

void cleanupAllClients() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    printf("Cleaning up all %zu client connections...\n", active_clients.size());
    
    // Create a copy of the set to avoid iterator invalidation
    std::set<int> clients_to_close = active_clients;
    active_clients.clear();
    
    // Release the lock before closing sockets
    lock.~lock_guard();
    
    for (int client_fd : clients_to_close) {
        printf("Closing client connection: fd=%d\n", client_fd);
        removeFdFromReactor(reactorInstance, client_fd);
        
        // Set socket to blocking mode before shutdown
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags & ~O_NONBLOCK);
        
        shutdown(client_fd, SHUT_RDWR);  // Force close the connection
        close(client_fd);
    }
}