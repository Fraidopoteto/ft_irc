#ifndef IRCS_EPOLL_H_
#define IRCS_EPOLL_H_

#include <stdint.h>
#include "ircs_connection.h"

namespace ircs {

int EpollAddEvent(EventLoop *event_loop, int fd, int mask);
void EpollDelEvent(EventLoop *event_loop, int fd, int delmask);
int CreateEpollInstance();

}  // namespace ircs

#endif  // IRCS_EPOLL_H_
