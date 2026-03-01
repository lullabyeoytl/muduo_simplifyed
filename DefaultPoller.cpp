#include "Poller.hpp"

#include <stdlib.h>
#include "EPollPoller.hpp"
Poller *Poller::newDefaultPoller(EventLoop *loop) {
  if (::getenv("MUDUO_USE_POLL")) {
    // todo: generate poll instance
    return nullptr;
  } else
    return new EPollPoller(loop);
}