#ifndef IRCS_CONNECTION_H_
#define IRCS_CONNECTION_H_

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>

#include <string>

#include "ircs_message.h"
#include "ircserv.h"

namespace ircs {

const int kNone = 0x00000000;
const int kOnReadable = 0x00000001;
const int kOnWritable = 0x00000002;

typedef void (*FileProc)(struct EventLoop *event_loop, int fd, void *data, int mask);

struct FileEvent {
    int mask;
    FileProc OnRead;
    FileProc OnWrite;
    void *data;
};

struct EventLoopContext {
    int port;
    char *password;
};

struct EventLoop {
    int epfd;
    int maxfds;                             // amount of file descriptors currently registered
    FileEvent events[kMaxConnections + 1];  // +1 to say the server is full to the last connection
    EventLoopContext context;
    bool stop;
};

struct Client {
    std::string ip;
    std::string msgbuf;
    msg::Message message;
    // char msgbuf[kMsgBufSize];
    // ssize_t bytes_received;
};

int CreateListener(char *port);
int CreateFileEvent(EventLoop *event_loop, int fd, int mask, FileProc proc, void *data);
int RunEventLoop(EventLoop *event_loop);
void OnConnectionAccept(EventLoop *event_loop, int fd, void *data, int mask);

}  // namespace ircs

#endif  // IRCS_CONNECTION_H_
