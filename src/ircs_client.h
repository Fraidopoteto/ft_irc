#ifndef IRCS_CLIENT_H_
#define IRCS_CLIENT_H_

#include <string>

#include "ircs_event_loop.h"
#include "ircs_message.h"

namespace ircs {

enum ClientRegistrationState {
    kClientConnected,
    kClientHasNick,
    kClientHasUser,
    kClientRegistered
};

struct Client {
    int fd;
    std::string ip;
    msg::MessageBuffer message_buffer;
    std::string writebuf;
    size_t write_pos;

    bool pass_accepted;
    bool nick_accepted;
    bool name_accepted;

    std::string nick_name;
    std::string user_name;
    std::string real_name;
    std::string host_name;

    msg::Message message;

    ClientRegistrationState state;
};

Client *NewClient(int fd, char *ip);
void OnClientWrite(EventLoop *event_loop, int fd, void *data, int mask);
void OnClientRead(EventLoop *event_loop, int fd, void *data, int mask);

}  // namespace ircs

#endif  // IRCS_CLIENT_H_
