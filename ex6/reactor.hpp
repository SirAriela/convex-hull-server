// reactor.hpp
#pragma once
#include <functional>
#include <chrono>

using reactorFunc = std::function<void(int)>;

// ממשק הראקטור
void* startReactor();
int addFdToReactor(void* reactor, int fd, reactorFunc func);
int removeFdFromReactor(void* reactor, int fd);
int stopReactor(void* reactor);