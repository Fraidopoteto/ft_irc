#ifndef IRCS_EVENT_LOOP_H_
#define IRCS_EVENT_LOOP_H_

#include <map>

#include "ircs_message.h"
#include "ircserv.h"

namespace ircs {

const int kNone = 0x00000000;
const int kOnReadable = 0x00000001;
const int kOnWritable = 0x00000002;

typedef void (*FileProc)(struct EventLoop *event_loop, int fd, void *data, int mask);

enum FileEventState { kFileEventClosed = 0, kFileEventClosing = 1, kFileEventRegistered = 2 };

struct FileEvent {
    int mask;
    FileEventState state;
    FileProc OnRead;
    FileProc OnWrite;
    void *data;
};

struct EventLoopContext {
    int port;
    char *password;
    std::string version;
    std::string creation_date;
    std::string host_name;
    std::map<int, Client *> clients;
};

struct EventLoop {
    int epfd;
    int listenfd;
    int maxfds;                             // amount of file descriptors currently registered
    FileEvent events[kMaxConnections + 1];  // +1 to say the server is full to the last connection
    EventLoopContext context;
    bool stop;
};

void CloseConnection(EventLoop *event_loop, int fd, void *data);
void RequestClose(EventLoop *event_loop, int fd, int mask, Client *client);
int CreateFileEvent(EventLoop *event_loop, int fd, int mask, FileProc proc, void *data);
void RemoveFileEvent(EventLoop *event_loop, int fd, int delmask, void *data);
void SetFileEventState(FileEvent &file_event, FileEventState state);
int RunEventLoop(EventLoop *event_loop);

}  // namespace ircs

#endif  // IRCS_EVENT_LOOP_H_
