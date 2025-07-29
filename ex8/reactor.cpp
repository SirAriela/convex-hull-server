#include "reactor.hpp"
#include <iostream>
#include <thread>
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <sys/select.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>




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
struct Procator {
    int sockfd;
    proactorFunc threadFunc;
    volatile bool* running;
};

// Wrapper function להתאמה בין pthread_create לproactorFunc
void* clientThreadWrapper(void* arg) {
    // פירוק הפרמטרים
    Procator* data = static_cast<Procator*>(((void**)arg)[0]);
    int client_fd = *(int*)(((void**)arg)[1]);
    
    // ניקוי זיכרון
    delete (int*)(((void**)arg)[1]);
    delete[] (void**)arg;
    
    // קריאה לפונקציה האמיתית עם int
    return data->threadFunc(client_fd);
}

//acceptor thread function
void* proactorAcceptLoop(void* arg) {
    Procator* data = static_cast<Procator*>(arg);
    
   
    while (*(data->running)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(data->sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (*(data->running)) {
                perror("accept");
            }
            continue;
        }
        
        // new thread for each client
        pthread_t client_thread;
        
        // הכנת פרמטרים לwrapper
        void** thread_args = new void*[2];
        thread_args[0] = data;              // Procator
        thread_args[1] = new int(client_fd); // client_fd
        
        if (pthread_create(&client_thread, nullptr, clientThreadWrapper, thread_args) != 0) {
            perror("pthread_create");
            close(client_fd);
            delete (int*)thread_args[1];
            delete[] thread_args;
            continue;
        }
        
        pthread_detach(client_thread);  // independent thread
    }
    
    return nullptr;
}

// proator threads map
std::map<pthread_t, Procator*> proactorThreads;
std::map<pthread_t,volatile bool*> proactorRunningFlags;
std::mutex proactorMutex;

pthread_t startProactor(int sockfd, proactorFunc threadFunc) {
    Procator* data = new Procator();
    data->sockfd = sockfd;
    data->threadFunc = threadFunc;
    data->running = new volatile bool(true);;
    
    
    pthread_t thread_id;
    if (pthread_create(&thread_id, nullptr, proactorAcceptLoop, data) != 0) {
        perror("pthread_create");
        delete data -> running;
        delete data;    
        return 0;
    }
    // Add to maps
    {
        std::lock_guard<std::mutex> lock(proactorMutex);
        proactorThreads[thread_id] = data;
        proactorRunningFlags[thread_id] = data -> running;
    }
    return thread_id;
}

int stopProactor(pthread_t tid) {
    std::lock_guard<std::mutex> lock(proactorMutex);
    
    if (proactorRunningFlags.find(tid) == proactorRunningFlags.end()) {
        return -1; // Thread not found
    }

    // stop the thread
    *(proactorRunningFlags[tid]) = false;
    
    //realase mutex to allow to prevent deadlock
    lock.~lock_guard();
    pthread_join(tid, nullptr);


{
    //clean memory
    std::lock_guard<std::mutex> lock(proactorMutex);
    delete proactorThreads[tid]->running;;
    delete proactorThreads[tid];
    proactorThreads.erase(tid);
    proactorRunningFlags.erase(tid);
}
    return 0;
}
