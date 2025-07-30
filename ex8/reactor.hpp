// reactor.hpp
#pragma once
#include <functional>
#include <pthread.h>

using reactorFunc = std::function<void(int)>;
typedef void* (*proactorFunc)(int sockfd);
void* startReactor();
int addFdToReactor(void* reactor, int fd, reactorFunc func);
int removeFdFromReactor(void* reactor, int fd);
int stopReactor(void* reactor);

//proactorFunc
pthread_t startProactor(int sockfd, proactorFunc threadFunc);

int stopProactor(pthread_t tid);