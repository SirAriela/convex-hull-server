#include "reactor.hpp"

#include <thread>
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <sys/select.h>
#include <unistd.h>
#include <cstdio>
#include <algorithm>

class Reactor {
private:
    std::map<int, reactorFunc> fdToFunc;
    std::set<int> fds;
    std::atomic<bool> running;
    std::thread reactorThread;
    std::mutex mutex;

    void loop() {
        while (running) {
            fd_set readfds;
            FD_ZERO(&readfds);
            int maxfd = -1;

            // Copy fds to avoid holding lock during select
            std::set<int> current_fds;
            std::map<int, reactorFunc> current_handlers;
            
            {
                std::lock_guard<std::mutex> lock(mutex);
                current_fds = fds;
                current_handlers = fdToFunc;
            }

            // Set up file descriptors for select
            for (int fd : current_fds) {
                FD_SET(fd, &readfds);
                maxfd = std::max(maxfd, fd);
            }

            if (maxfd == -1) {
                // No file descriptors to monitor, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Use a shorter timeout for more responsive shutdown
            timeval tv = {0, 100000}; // 100ms timeout
            int ready = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
            
            if (ready > 0) {
                // Process ready file descriptors
                for (int fd : current_fds) {
                    if (FD_ISSET(fd, &readfds)) {
                        auto handler_it = current_handlers.find(fd);
                        if (handler_it != current_handlers.end()) {
                            printf("DEBUG: Calling handler for fd=%d\n", fd);
                            try {
                                handler_it->second(fd);
                            } catch (const std::exception& e) {
                                printf("ERROR: Exception in handler for fd=%d: %s\n", fd, e.what());
                            } catch (...) {
                                printf("ERROR: Unknown exception in handler for fd=%d\n", fd);
                            }
                        }
                    }
                }
            } else if (ready == 0) {
                // Timeout - continue loop to check if still running
                continue;
            } else {
                if (running) {  // Only print error if we're still supposed to be running
                    perror("select error");
                }
                break;
            }
        }
        printf("DEBUG: Reactor loop exiting\n");
    }

public:
    Reactor() : running(false) {}

    void start() {
        if (running) {
            printf("WARNING: Reactor is already running\n");
            return;
        }
        
        running = true;
        reactorThread = std::thread(&Reactor::loop, this);
        printf("DEBUG: Reactor started\n");
    }

    int addFd(int fd, reactorFunc func) {
        std::lock_guard<std::mutex> lock(mutex);
        if (fds.count(fd)) {
            printf("WARNING: fd=%d already exists in reactor\n", fd);
            return -1;
        }
        
        fds.insert(fd);
        fdToFunc[fd] = func;
        printf("DEBUG: Added fd=%d to reactor (total fds: %zu)\n", fd, fds.size());
        return 0;
    }

    int removeFd(int fd) {
        std::lock_guard<std::mutex> lock(mutex);
        
        auto fd_it = fds.find(fd);
        auto func_it = fdToFunc.find(fd);
        
        if (fd_it != fds.end()) {
            fds.erase(fd_it);
            printf("DEBUG: Removed fd=%d from fds set\n", fd);
        }
        
        if (func_it != fdToFunc.end()) {
            fdToFunc.erase(func_it);
            printf("DEBUG: Removed handler for fd=%d\n", fd);
        }
        
        printf("DEBUG: Removed fd=%d from reactor (remaining fds: %zu)\n", fd, fds.size());
        return 0;
    }

    void stop() {
        if (!running) {
            return;
        }
        
        printf("DEBUG: Stopping reactor\n");
        running = false;
        
        if (reactorThread.joinable()) {
            reactorThread.join();
            printf("DEBUG: Reactor thread joined\n");
        }
    }

    ~Reactor() {
        stop();
    }
    
    // Debug method to print current state
    void printStatus() {
        std::lock_guard<std::mutex> lock(mutex);
        printf("DEBUG: Reactor status - running: %s, fds: %zu\n", 
               running ? "true" : "false", fds.size());
        for (int fd : fds) {
            printf("  fd=%d\n", fd);
        }
    }
};

// C-style interface
void* startReactor() {
    Reactor* r = new Reactor();
    r->start();
    return r;
}

int addFdToReactor(void* reactor, int fd, reactorFunc func) {
    if (!reactor) {
        printf("ERROR: reactor is null\n");
        return -1;
    }
    return static_cast<Reactor*>(reactor)->addFd(fd, func);
}

int removeFdFromReactor(void* reactor, int fd) {
    if (!reactor) {
        printf("ERROR: reactor is null\n");
        return -1;
    }
    return static_cast<Reactor*>(reactor)->removeFd(fd);
}

int stopReactor(void* reactor) {
    if (!reactor) {
        printf("ERROR: reactor is null\n");
        return -1;
    }
    
    Reactor* r = static_cast<Reactor*>(reactor);
    r->stop();
    delete r;
    return 0;
}