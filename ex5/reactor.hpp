// reactor.hpp
#pragma once
#include <functional>

using reactorFunc = std::function<void(int)>;

void* startReactor();
int addFdToReactor(void* reactor, int fd, reactorFunc func);
int removeFdFromReactor(void* reactor, int fd);
int stopReactor(void* reactor);
