#include "reactor.hpp"

#include <thread>
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <sys/select.h>
#include <unistd.h>

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

            {
                std::lock_guard<std::mutex> lock(mutex);
                for (int fd : fds) {
                    FD_SET(fd, &readfds);
                    if (fd > maxfd)
                        maxfd = fd;
                }
            }

            timeval tv = {1, 0};
            int ready = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
            if (ready > 0) {
                std::lock_guard<std::mutex> lock(mutex);
                for (int fd : fds) {
                    if (FD_ISSET(fd, &readfds) && fdToFunc.count(fd)) {
                        fdToFunc[fd](fd);
                    }
                }
            }
        }
    }

public:
    Reactor() : running(false) {}

    void start() {
        running = true;
        reactorThread = std::thread(&Reactor::loop, this);
    }

    int addFd(int fd, reactorFunc func) {
        std::lock_guard<std::mutex> lock(mutex);
        if (fds.count(fd)) return -1;
        fds.insert(fd);
        fdToFunc[fd] = func;
        return 0;
    }

    int removeFd(int fd) {
        std::lock_guard<std::mutex> lock(mutex);
        fds.erase(fd);
        fdToFunc.erase(fd);
        return 0;
    }

    void stop() {
        running = false;
        if (reactorThread.joinable())
            reactorThread.join();
    }

    ~Reactor() {
        stop();
    }
};

// ממשק C-style (כפי שנדרש בשאלה)

void* startReactor() {
    Reactor* r = new Reactor();
    r->start();
    return r;
}

int addFdToReactor(void* reactor, int fd, reactorFunc func) {
    return static_cast<Reactor*>(reactor)->addFd(fd, func);
}

int removeFdFromReactor(void* reactor, int fd) {
    return static_cast<Reactor*>(reactor)->removeFd(fd);
}

int stopReactor(void* reactor) {
    Reactor* r = static_cast<Reactor*>(reactor);
    r->stop();
    delete r;
    return 0;
}
