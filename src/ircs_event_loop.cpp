#include "ircs_event_loop.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstring>

#include "ircs_client.h"
#include "ircs_epoll.h"
#include "ircs_logger.h"
#include "ircs_message.h"
#include "ircserv.h"

namespace ircs {
using log::Logger;

void CloseConnection(EventLoop *event_loop, int fd, void *data)
{
    if (fd == -1) {
        return;
    }
    if (data == NULL) {
        return;
    }
    if (event_loop->events[fd].state == kFileEventClosed) {
        return;
    }

    Client *client = static_cast<Client *>(data);

    Logger(log::kInfo, "", "closing connection on %s", client->ip.c_str());

    EpollDelEvent(event_loop, fd, kOnReadable | kOnWritable);
    close(fd);
    event_loop->context.clients.erase(fd);
    event_loop->events[fd] = FileEvent();
    SetFileEventState(event_loop->events[fd], kFileEventClosed);  // For clarity, but redundant
    delete client;
    client = NULL;
}

void RequestClose(EventLoop *event_loop, int fd, int mask, Client *client)
{
    if (client->writebuf.empty()) {
        CloseConnection(event_loop, fd, client);
        return;
    }

    SetFileEventState(event_loop->events[fd], kFileEventClosing);
    RemoveFileEvent(event_loop, fd, kOnReadable, client);

    // Ensure Write Event is registered
    if (!(mask & kOnWritable)) {
        if (CreateFileEvent(event_loop, fd, kOnWritable, OnClientWrite, client) == -1) {
            CloseConnection(event_loop, fd, client);
            return;
        }
    }
}

void SetFileEventState(FileEvent &file_event, FileEventState state)
{
    file_event.state = state;
}

int CreateFileEvent(EventLoop *event_loop, int fd, int mask, FileProc proc, void *data)
{
    FileEvent *fe = &event_loop->events[fd];

    if (EpollAddEvent(event_loop, fd, mask) == -1) {
        return -1;
    }

    fe->mask |= mask;
    if (mask & kOnReadable) {
        fe->OnRead = proc;
    }
    if (mask & kOnWritable) {
        fe->OnWrite = proc;
    }
    fe->data = data;
    return 0;
}

void RemoveFileEvent(EventLoop *event_loop, int fd, int delmask, void *data)
{
    FileEvent *fe = &event_loop->events[fd];

    EpollDelEvent(event_loop, fd, delmask);

    fe->mask = fe->mask & (~delmask);
    if (delmask & kOnReadable) {
        fe->OnRead = NULL;
    }
    if (delmask & kOnWritable) {
        fe->OnWrite = NULL;
    }
    fe->data = data;
}

int RunEventLoop(EventLoop *event_loop)
{
    int nfds;
    struct epoll_event ee[kMaxEvents];

    event_loop->stop = false;
    while (!event_loop->stop) {
        Logger(log::kDebug, "", "epoll wait");
        if ((nfds = epoll_wait(event_loop->epfd, ee, kMaxEvents, -1)) == -1) {
            Logger(log::kError, "epoll_wait", strerror(errno));
            return -1;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = ee[i].data.fd;
            uint32_t ev = ee[i].events;
            FileEvent *fe = &event_loop->events[fd];

            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                if (fd != event_loop->listenfd) {
                    CloseConnection(event_loop, fd, fe->data);
                }
                continue;
            }

            if (ev & EPOLLIN && fe->mask & kOnReadable) {
                fe->OnRead(event_loop, fd, fe->data, fe->mask);
            }

            if (ev & EPOLLOUT && fe->mask & kOnWritable) {
                fe->OnWrite(event_loop, fd, fe->data, fe->mask);
            }
        }
    }
    return 0;
}

}  // namespace ircs
