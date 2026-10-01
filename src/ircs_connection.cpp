#include "ircs_connection.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstring>
#include <string>

#include "ircs_connection.h"
#include "ircs_epoll.h"
#include "ircs_logger.h"
#include "ircs_message.h"
#include "ircserv.h"

namespace ircs {
using log::Logger;

std::map<int, Client *> clients;

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

void OnClientReadStub(EventLoop *event_loop, int fd, void *data, int mask)
{
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_received;
    char buf[kReadChunkSize];

    bytes_received = recv(fd, buf, sizeof(buf), 0);
    if (bytes_received == -1) {
        return;
    }

    if (bytes_received == 0) {
        // close the connection;
    }

    if (bytes_received > 0) {
        msg::ProcessTcpStream(event_loop->context, buf, client, bytes_received);
    }
    (void)event_loop;
    (void)fd;
    (void)data;
    (void)mask;
    return;
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

Client *NewClient(char *ip)
{
    // Initialize Client data
    Client *c = new Client();
    c->ip = ip;
    c->msgbuf.reserve(msg::kMsgBufSize);
    // c->bytes_received = 0;

    return c;
}

void OnConnectionAccept(EventLoop *event_loop, int fd, void *data, int mask)
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

    clients[client_fd] = c;

    if (CreateFileEvent(event_loop, client_fd, kOnReadable, OnClientReadStub, c) == -1) {
        Logger(log::kError, "OnConnectionAccept", "failed to add connection");
        close(client_fd);
        delete c;
        return;
    }
}

Client *NewClient(int fd, char *ip)
{
    Client *c = new Client();

    c->fd = fd;
    c->ip = ip;
    c->msg_buf.reserve(kMsgBufSize);
    c->write_buf.reserve(kMsgBufSize);

    c->state = CONNECTED;

    c->pass_accepted = false;
    c->nick_accepted = false;
    c->name_accepted = false;

    return c;
}

bool ValidateNickname(const std::string &nick_name, int client_fd)
{
    char first = nick_name[0];

    if (first == '$' || first == ':' || first == '#' || first == '&' || first == '@' ||
        first == '+') {
        return false;  // add log message and usermessage: bad nickname
    }

    if (nick_name.is_empty() || nick_name.length() > 30 || nick_name.length() < 2)
        return false;  // add log message and usermessage: bad nickname

    for (size_t i = 0; i < nick_name.lenght(); ++i) {
        char c = nick_name[i];
        if (c == ' ' || c == ',' || c == '*' || c == '?' || c == '!' || c == '@') {
            return false;  // add log message and usermessage: bad nickname
        }
    }

    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->fd != client_fd) {
            if (it->second->nick_name == nick_name) {
                return false;  // add log message and usermessage: duplicate names
            }
        }
    }

    return true;
}

bool ValidateUsername(const std::string &user_name)
{
    if (user_name[0] == '~') {
        return false;  // add log and usermessage: bad username
    }

    if (user_name.empty() || user_name.length() > 10) {
        return false;  // add log and usermessage: bad username
    }

    for (size_t i = 0; i < user_name.lenght(); ++i) {
        char c = user_name[i];
        if (std::iscntrl(c) || c == ' ' || c == '@' || c == ':') {
            return false;  // add log and usermessage: bad username
        }
    }

    return true;
}

bool ValidatePassword(const std::string password)
{
    if (!EventLoopContext.password)
        return true;

    if (EventLoopContext.password != password)
        return false;  // add log message and usermessage: bad password

    return true;
}

bool ValidateClient(int client_fd, msg::Message &msg)
{
    if (msg.command == "PASS") {
        if (ValidatePassword(msg.parameters[0])) {
            clients[client_fd]->pass_accepted = true;
        } else {
            return false;
        }
    }

    if (msg.command == "NICK") {
        if (ValidateNickname(msg.parameters[0], client_fd)) {
            clients[client_fd]->nick_accepted = true;
            clients[client_fd]->nick_name = msg.parameters[0];
        } else {
            return false;
        }
    }

    if (msg.command == "USER") {
        if (ValidateUsername(msg.parameters[0])) {
            clients[client_fd]->name_accepted = true;
            clients[client_fd]->user_name = msg.parameters[0];
        }
    }

    if (clients[client_fd]->state == CONNECTED && clients[client_fd]->pass_accepted &&
        clients[client_fd]->nick_accepted && clients[client_fd]->name_accepted) {
        clients[client_fd]->state = REGISTERED;
    }

    return true;
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
                // TODO: handle close
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
