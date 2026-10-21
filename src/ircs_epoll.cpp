#include <stdint.h>
#include <sys/epoll.h>

#include "ircs_connection.h"

namespace ircs {

int EpollAddEvent(EventLoop *event_loop, int fd, int mask)
{
    struct epoll_event ee;

    int op = (event_loop->events[fd].mask == kNone) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= event_loop->events[fd].mask;
    if (mask & kOnReadable) {
        ee.events |= EPOLLIN;
    }
    if (mask & kOnWritable) {
        ee.events |= EPOLLOUT;
    }
    ee.data.fd = fd;
    if (epoll_ctl(event_loop->epfd, op, fd, &ee) == -1) {
        return -1;
    }

    return 0;
}

void EpollDelEvent(EventLoop *event_loop, int fd, int delmask)
{
    struct epoll_event ee;
    int mask = event_loop->events[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & kOnReadable) {
        ee.events |= EPOLLIN;
    }
    if (mask & kOnWritable) {
        ee.events |= EPOLLOUT;
    }
    ee.data.fd = fd;
    int op = (mask != kNone) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_ctl(event_loop->epfd, op, fd, &ee);
}

int CreateEpollInstance()
{
    int epfd;

    if ((epfd = epoll_create1(0)) == -1) {
        return -1;
    }
    return epfd;
}

}  // namespace ircs
