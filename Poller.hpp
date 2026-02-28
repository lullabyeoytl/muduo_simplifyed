#pragma once

#include "noncopyable.hpp"
#include "timestamp.hpp"
#include <unordered_map>
#include <vector>

class Channel;
class EventLoop;

class Poller : noncopyable {
public:
  using ChannelList = std::vector<Channel *>;
  Poller(EventLoop *loop);
  virtual ~Poller();

  // 所有IO复用保留的统一接口
  virtual Timestamp poll(int timeoutsMs, ChannelList *activeChannels) = 0;
  virtual void updateChannel(Channel *channel) = 0;
  virtual void removeChannel(Channel *channel) = 0;

  // 参数是否在当前Poller
  bool hasChannel(Channel *channel) const;

  static Poller* newDefaultPoller(EventLoop *loop);

protected:
  using ChannelMap = std::unordered_map<int, Channel *>;
  ChannelMap channels_;

private:
  EventLoop *ownerLoop_;    // 定义Poller所属的事件循环EventLoop
};