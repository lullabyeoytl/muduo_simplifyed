#include <sys/epoll.h>
#include <vector>
#include "Poller.hpp"
#include "timestamp.hpp"

class EPollPoller: public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutsMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel *channel) override;
private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    //update channel
    void update(int operation, Channel* channel);
    
    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};