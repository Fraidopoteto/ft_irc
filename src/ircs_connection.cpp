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
#include <cstddef>
#include <cstring>
#include <string>

#include "ircs_connection.h"
#include "ircs_epoll.h"
#include "ircs_logger.h"
#include "ircs_message.h"
#include "ircserv.h"

namespace ircs {
using log::Logger;
using internal::CloseClient;
using internal::RequestClose;

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

namespace internal {

void CloseClient(EventLoop *event_loop, int fd, void *data)
{
    if (fd == -1) {
        return;
    }
    if (data == NULL) {
        return;
    }
    if (event_loop->events[fd].state == kClosed) {
        return;
    }

    Client *client = static_cast<Client *>(data);

    Logger(log::kInfo, "", "closing connection on %s", client->ip.c_str());

    EpollDelEvent(event_loop, fd, kOnReadable | kOnWritable);
    close(fd);
    event_loop->context.clients.erase(fd);
    event_loop->events[fd] = FileEvent();
    SetFileEventState(event_loop->events[fd], kClosed);  // For clarity, but redundant
    delete client;
    client = NULL;
}

void RequestClose(EventLoop *event_loop, int fd, int mask, Client *client)
{
    if (client->writebuf.empty()) {
        CloseClient(event_loop, fd, client);
        return;
    }

    SetFileEventState(event_loop->events[fd], kClosing);
    RemoveFileEvent(event_loop, fd, kOnReadable, client);

    // Ensure Write Event is registered
    if (!(mask & kOnWritable)) {
        if (CreateFileEvent(event_loop, fd, kOnWritable, OnClientWrite, client) == -1) {
            CloseClient(event_loop, fd, client);
            return;
        }
    }
}

}  // namespace internal

void OnClientRead(EventLoop *event_loop, int fd, void *data, int mask)
{
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_received;
    char buf[kReadChunkSize];

    bytes_received = recv(fd, buf, sizeof(buf), 0);
    if (bytes_received == -1) {
        return;
    }

    if (bytes_received == 0) {
        CloseClient(event_loop, fd, client);
        return;
    }

    if (bytes_received > 0) {
        msg::ProcessStatus status =
            msg::ProcessTcpStream(event_loop->context, buf, client, bytes_received);

        if (!client->writebuf.empty() && !(mask & kOnWritable)) {
            if (CreateFileEvent(event_loop, fd, kOnWritable, OnClientWrite, data) == -1) {
                CloseClient(event_loop, fd, client);
                return;
            }
        }

        if (status == msg::kCloseConnection) {
            RequestClose(event_loop, fd, mask, client);
        }
    }
}

void OnClientWrite(EventLoop *event_loop, int fd, void *data, int mask)
{
    (void)mask;
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_sent;
    size_t bytes_to_sent = client->writebuf.length() - client->write_pos;

    if (bytes_to_sent == 0) {
        RemoveFileEvent(event_loop, fd, kOnWritable, client);
        if (event_loop->events[fd].state == kClosing) {
            CloseClient(event_loop, fd, client);
        }
        return;
    }

    bytes_sent =
        send(fd, client->writebuf.c_str() + client->write_pos, bytes_to_sent, MSG_NOSIGNAL);
    if (bytes_sent == -1) {
        return;
    }

    client->write_pos += bytes_sent;
    if (client->write_pos == client->writebuf.length()) {
        client->writebuf.clear();
        client->write_pos = 0;
        RemoveFileEvent(event_loop, fd, kOnWritable, client);
        if (event_loop->events[fd].state == kClosing) {
            CloseClient(event_loop, fd, client);
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

    if (CreateFileEvent(event_loop, client_fd, kOnReadable, OnClientRead, c) == -1) {
        Logger(log::kError, "OnConnectionAccept", "failed to add connection");
        close(client_fd);
        delete c;
        return;
    }
    SetFileEventState(event_loop->events[client_fd], kConnected);
    event_loop->context.clients[client_fd] = c;
}

Client *NewClient(int fd, char *ip)
{
    Client *c = new Client();

    c->fd = fd;
    c->ip = ip;
    c->message_buffer = msg::MessageBuffer();
    c->writebuf.reserve(msg::kMaxMsgSize);
    c->write_pos = 0;

    c->state = CONNECTED;

    c->pass_accepted = false;
    c->nick_accepted = false;
    c->name_accepted = false;

    return c;
}

bool is_ValidNickname(const std::string &nick_name, int client_fd, EventLoopContext &context)
{
    char first = nick_name[0];

    if (first == '$' || first == ':' || first == '#' || first == '&' || first == '@' ||
        first == '+') {
        return false;  // add log message and usermessage: bad nickname
    }

    if (nick_name.empty() || nick_name.length() > 30 || nick_name.length() < 2)
        return false;  // add log message and usermessage: bad nickname

    for (size_t i = 0; i < nick_name.length(); ++i) {
        char c = nick_name[i];
        if (c == ' ' || c == ',' || c == '*' || c == '?' || c == '!' || c == '@') {
            return false;  // add log message and usermessage: bad nickname
        }
    }

    for (std::map<int, Client *>::iterator it = context.clients.begin();
         it != context.clients.end(); ++it) {
        if (it->second->fd != client_fd) {
            if (it->second->nick_name == nick_name) {
                return false;  // add log message and usermessage: duplicate names
            }
        }
    }

    return true;
}

bool is_ValidUsername(const std::string &user_name)
{
    if (user_name[0] == '~') {
        return false;  // add log and usermessage: bad username
    }

    if (user_name.empty() || user_name.length() > 10) {
        return false;  // add log and usermessage: bad username
    }

    for (size_t i = 0; i < user_name.length(); ++i) {
        char c = user_name[i];
        if (std::iscntrl(c) || c == ' ' || c == '@' || c == ':') {
            return false;  // add log and usermessage: bad username
        }
    }

    return true;
}

bool is_ValidPassword(const std::string &user_pass, const char *server_pass)
{
    if (!server_pass)
        return true;

    if (server_pass != user_pass)
        return false;  // add log message and usermessage: bad password

    return true;
}

bool is_ValidClient(int client_fd, msg::Message &msg, EventLoopContext &context)
{
    if (msg.command == "PASS") {
        if (is_ValidPassword(msg.parameters[0], context.password)) {
            context.clients[client_fd]->pass_accepted = true;
        } else {
            return false;
        }
    }

    if (msg.command == "NICK") {
        if (is_ValidNickname(msg.parameters[0], client_fd, context)) {
            context.clients[client_fd]->nick_accepted = true;
            context.clients[client_fd]->nick_name = msg.parameters[0];
        } else {
            return false;
        }
    }

    if (msg.command == "USER") {
        if (is_ValidUsername(msg.parameters[0])) {
            context.clients[client_fd]->name_accepted = true;
            context.clients[client_fd]->user_name = msg.parameters[0];
        }
    }

    if (context.clients[client_fd]->state == CONNECTED &&
        context.clients[client_fd]->pass_accepted && context.clients[client_fd]->nick_accepted &&
        context.clients[client_fd]->name_accepted) {
        context.clients[client_fd]->state = REGISTERED;
        return true;
    }

    return false;
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
                    CloseClient(event_loop, fd, fe->data);
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
