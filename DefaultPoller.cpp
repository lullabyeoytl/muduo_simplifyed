#include "Poller.hpp"

#include <stdlib.h>

Poller *Poller::newDefaultPoller(EventLoop *loop) {
  if (::getenv("MUDUO_USE_POLL")) {
    // todo: generate poll instance
    return nullptr;
  } else
    // todo: generate epoll instance
    return nullptr;
}