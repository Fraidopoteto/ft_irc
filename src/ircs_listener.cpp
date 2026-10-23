#include "ircs_listener.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "ircs_client.h"
#include "ircs_logger.h"

namespace ircs {
using log::Logger;

namespace {

int GetInAddress(struct sockaddr *sa, char *in_addr, size_t maxlen)
{
    void *addr;

    if (sa->sa_family == AF_INET) {
        addr = &((reinterpret_cast<struct sockaddr_in *>(sa))->sin_addr);
    } else if (sa->sa_family == AF_INET6) {
        addr = &((reinterpret_cast<struct sockaddr_in6 *>(sa))->sin6_addr);
    } else {
        Logger(log::kError, "GetInAddress", "unknown address family");
        return -1;
    }

    if (inet_ntop(sa->sa_family, addr, in_addr, maxlen) == NULL) {
        Logger(log::kError, "GetInAddress", "inet_ntop: %s", strerror(errno));
        return -1;
    }

    return 0;
}

}  // namespace

int CreateListener(char *port)
{
    int listenfd;
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    std::memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        Logger(log::kError, "getaddrinfo", gai_strerror(rv));
        return -1;
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listenfd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK, p->ai_protocol);
        if (listenfd < 0) {
            continue;
        }
        // Lose the pesky "address already in use" error message
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(listenfd);
            continue;
        }

        break;
    }

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        freeaddrinfo(ai);
        return -1;
    }

    freeaddrinfo(ai);  // All done with this

    // Listen
    if (listen(listenfd, kDefaultBacklog) == -1) {
        Logger(log::kError, "listen", strerror(errno));
        return -1;
    }

    Logger(log::kInfo, "", "listening on port %s...", port);
    return listenfd;
}

void OnListenerRead(EventLoop *event_loop, int fd, void *data, int mask)
{
    (void)mask;  // not sure what to do with mask yet
    (void)data;  // Listener does not need data
    char ip[INET6_ADDRSTRLEN];
    socklen_t socklen;
    sockaddr_storage ss;
    int client_fd;

    socklen = sizeof(ss);
    if ((client_fd = accept4(fd, (struct sockaddr *)&ss, &socklen, SOCK_NONBLOCK)) == -1) {
        Logger(log::kError, "OnConnectionAccept", "accept: %s", strerror(errno));
        return;
    }

    if (GetInAddress(reinterpret_cast<struct sockaddr *>(&ss), ip, sizeof(ip)) == -1) {
        close(client_fd);
        return;
    }
    Logger(log::kInfo, "", "got connection from %s", ip);

    Client *c = NewClient(client_fd, ip);

    if (CreateFileEvent(event_loop, client_fd, kOnReadable, OnClientRead, c) == -1) {
        Logger(log::kError, "OnConnectionAccept", "failed to add connection");
        close(client_fd);
        delete c;
        return;
    }
    SetFileEventState(event_loop->events[client_fd], kFileEventRegistered);
    event_loop->context.clients[client_fd] = c;
}

}  // namespace ircs
