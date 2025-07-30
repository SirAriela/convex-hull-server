#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <pthread.h>
#include "../ex3/convex_hull.hpp"
#include "../ex3/point.hpp"
#include "../ex8/reactor.hpp"


#define BACKLOG 10

// Function declarations
void* handleClient(int client_fd);
void* areaMonitorThread(void* arg);
void* printerThread(void* arg);

//------------------- Thread-safe Graph functions ------------------------------------

// Global shared graph and convex hull object - WITH MUTEX PROTECTION
std::vector<Point> shared_points;
ConvexHull* ch = nullptr;
std::mutex graph_mutex;

// Global variable to control server shutdown
volatile sig_atomic_t running = 1;
int listen_fd = -1;
pthread_t proactor_thread = 0;

// POSIX condition variables for area monitoring
pthread_mutex_t area_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t area_above_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t area_below_condition = PTHREAD_COND_INITIALIZER;
bool ch_area_above_100 = false;
bool ch_area_below_100 = false;
bool area_above_processed = false;
bool area_below_processed = false;
pthread_t area_monitor_thread;
pthread_t printer_thread;

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
    
    // Stop proactor
    if (proactor_thread != 0) {
        printf("Stopping proactor...\n");
        stopProactor(proactor_thread);
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
    
    // Reset area flags when creating new graph
    pthread_mutex_lock(&area_mutex);
    ch_area_above_100 = false;
    ch_area_below_100 = false;
    area_above_processed = false;
    area_below_processed = false;
    pthread_mutex_unlock(&area_mutex);
    
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
        printf("Computing convex hull for %zu points...\n", shared_points.size());
        ch->findConvexHull();
        hull_points = ch->getConvexHullPoints();
        area = ch->polygonArea();
        
        printf("Computed convex hull with %zu points, area: %.2f\n", hull_points.size(), area);
        if (hull_points.size() > 0) {
            for (const auto& p : hull_points) {
                printf("  (%.2f, %.2f)\n", p.getX(), p.getY());
            }
        } else {
            printf("  No convex hull points (collinear points)\n");
        }
        
        // Check area conditions for producer-consumer
        pthread_mutex_lock(&area_mutex);
        bool was_above_100 = ch_area_above_100;
        
                if (area >= 100.0) {
            if (!was_above_100 && !area_above_processed) {
                // Crossing threshold from below to above
                ch_area_above_100 = true;
                ch_area_below_100 = false;
                area_above_processed = false;
                area_below_processed = false;
                printf("CH area crossed threshold >= 100 units! Signaling area monitor thread.\n");
                pthread_cond_signal(&area_above_condition);
            } else if (was_above_100 && area >= 100.0) {
                // Still above 100, make sure flags are correct
                ch_area_above_100 = true;
                ch_area_below_100 = false;
                area_above_processed = false;
                area_below_processed = false;
            } else if (!was_above_100 && area >= 100.0) {
                // First time above 100, set flags
                ch_area_above_100 = true;
                ch_area_below_100 = false;
                area_above_processed = false;
                area_below_processed = false;
            }
        } else {
            if (was_above_100 && !area_below_processed) {
                // Crossing threshold from above to below
                ch_area_above_100 = false;
                ch_area_below_100 = true;
                area_above_processed = false;
                area_below_processed = false;
                printf("CH area crossed threshold below 100 units! Signaling printer thread.\n");
                pthread_cond_signal(&area_below_condition);
            } else if (!was_above_100 && area < 100.0) {
                // Still below 100, make sure flags are correct
                ch_area_above_100 = false;
                ch_area_below_100 = false;
                area_above_processed = false;
                area_below_processed = false;
            } else if (was_above_100 && area < 100.0) {
                // Crossing threshold from above to below (alternative path)
                ch_area_above_100 = false;
                ch_area_below_100 = true;
                area_above_processed = false;
                area_below_processed = false;
                printf("CH area crossed threshold below 100 units! Signaling printer thread.\n");
                pthread_cond_signal(&area_below_condition);
            }
        }
        pthread_mutex_unlock(&area_mutex);
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

void* handleClient(int client_fd) {
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
    return nullptr;
}

// Area monitor thread - waits for CH area >= 100
void* areaMonitorThread(void* arg) {
    (void)arg;
    printf("[Area Monitor Thread] Started monitoring CH area\n");
    
    while (running) {
        pthread_mutex_lock(&area_mutex);
        
        // Wait for CH area to reach >= 100
        while (!ch_area_above_100 && running) {
            // Use timed wait to prevent infinite loop
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1; // Wait 1 second
            
            int ret = pthread_cond_timedwait(&area_above_condition, &area_mutex, &ts);
            if (ret == ETIMEDOUT) {
                // Timeout - check if we should exit
                if (!running) {
                    pthread_mutex_unlock(&area_mutex);
                    return nullptr;
                }
                continue;
            }
            
            if (!running) {
                pthread_mutex_unlock(&area_mutex);
                return nullptr;
            }
        }
        
        if (ch_area_above_100 && running) {
            printf("[Area Monitor Thread] CH area crossed threshold >= 100 units!\n");
            area_above_processed = true; // Mark as processed
            ch_area_above_100 = false; // Reset flag to exit loop
        }
        
        pthread_mutex_unlock(&area_mutex);
        
        if (!running) break;
    }
    
    printf("[Area Monitor Thread] Exiting\n");
    return nullptr;
}

// Printer thread - waits for CH area to drop below 100
void* printerThread(void* arg) {
    (void)arg;
    printf("[Printer Thread] Started waiting for CH area to drop below 100\n");
    
    while (running) {
        pthread_mutex_lock(&area_mutex);
        
        // Wait for CH area to drop below 100
        while (!ch_area_below_100 && running) {
            // Use timed wait to prevent infinite loop
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1; // Wait 1 second
            
            int ret = pthread_cond_timedwait(&area_below_condition, &area_mutex, &ts);
            if (ret == ETIMEDOUT) {
                // Timeout - check if we should exit
                if (!running) {
                    pthread_mutex_unlock(&area_mutex);
                    return nullptr;
                }
                continue;
            }
            
            if (!running) {
                pthread_mutex_unlock(&area_mutex);
                return nullptr;
            }
        }
        
        if (ch_area_below_100 && running) {
            printf("[Printer Thread] CH area crossed threshold below 100! Printing to stdout.\n");
            printf("=== CH AREA ALERT ===\n");
            printf("The Convex Hull area has crossed threshold below 100 units!\n");
            printf("Current area: %.2f units\n", ch ? ch->polygonArea() : 0.0);
            printf("=====================\n");
            
            area_below_processed = true; // Mark as processed
            ch_area_below_100 = false; // Reset flag to exit loop
        }
        
        pthread_mutex_unlock(&area_mutex);
        
        if (!running) break;
    }
    
    printf("[Printer Thread] Exiting\n");
    return nullptr;
}

//------------------------------------------------------------------------
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

    printf("Convex Hull Server (Producer-Consumer) running on port %d...\n", port);
    printf("Available commands:\n");
    printf("  Newgraph - Create new empty graph\n");
    printf("  Newpoint x y - Add point to graph\n");
    printf("  Removepoint x y - Remove point from graph\n");
    printf("  CH - Compute convex hull\n");

    // Store listen_fd globally for signal handler
    ::listen_fd = listen_fd;
    
    // Start area monitor thread
    if (pthread_create(&area_monitor_thread, nullptr, areaMonitorThread, nullptr) != 0) {
        perror("Failed to create area monitor thread");
        close(listen_fd);
        return 1;
    }
    
    // Start printer thread
    if (pthread_create(&printer_thread, nullptr, printerThread, nullptr) != 0) {
        perror("Failed to create printer thread");
        close(listen_fd);
        return 1;
    }
    
    // proactor thread
    ::proactor_thread = startProactor(listen_fd, handleClient);

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
    
    // Wait for proactor thread to finish
    stopProactor(proactor_thread);
    
    // Wait for area monitor thread
    pthread_join(area_monitor_thread, nullptr);
    
    // Wait for printer thread
    pthread_join(printer_thread, nullptr);
    
    if (listen_fd > 0) {
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }

    printf("Waiting for connections to close...\n");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (ch != nullptr) {
        delete ch;
    }
    
    printf("Server terminated.\n");
    return 0;
}