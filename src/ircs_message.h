#ifndef IRCS_MESSAGE_H_
#define IRCS_MESSAGE_H_

#include <sys/types.h>

#include <string>

namespace ircs {

struct Client;
struct EventLoopContext;

namespace msg {

const int kMsgBufSize = 512;
const int kMaxParameters = 15;
const char kCrLf[] = "\r\n";

const char *const kPrivMsg = "PRIVMSG";
const char *const kUser = "USER";
const char *const kNick = "NICK";
const char *const kPass = "PASS";

// enum Command { kPrivMsg,
//     kUser,
//     kNick,
//     kPass,
//     kJoin,
//     kPart,
//     kKick,
// };

struct Message {
    std::string prefix;
    std::string command;
    std::string parameters[kMaxParameters - 1];
    std::string trailing;  // The last parameter
};

int ParseMessage(const std::string &message, Message &msg);
int ProcessTcpStream(EventLoopContext &context, char *buf, Client *client, ssize_t buflen);

}  // namespace msg
}  // namespace ircs

#endif  // IRCS_MESSAGE_H_
