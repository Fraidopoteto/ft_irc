#include "ircs_message.h"

#include <stdint.h>
#include <sys/types.h>

#include <cstring>
#include <string>

#include "ircs_connection.h"

namespace ircs {
namespace msg {

const static std::string kCommandList[] = {kPass, kNick, kUser};

int IsCommand(const std::string &cmd)
{
    for (int i = 0; i < 3; i++) {
        if (cmd == kCommandList[i]) {
            return 1;
        }
    }
    return 0;
}

std::string ConsumeToken(const std::string &message, size_t &pos)
{
    if (pos == std::string::npos || pos >= message.length()) {
        return "";
    }

    size_t space_pos = message.find(' ', pos);
    if (space_pos == std::string::npos) {
        std::string token = message.substr(pos);
        pos = std::string::npos;
        return token;
    }
    std::string token = message.substr(pos, space_pos - pos);
    pos = space_pos + 1;
    return token;
}

int ParseMessage(const std::string &message, Message &msg)
{
    size_t pos = 0;

    if (message[pos] == ':') {
        pos += 1;
        msg.prefix = ConsumeToken(message, pos);
    }

    msg.command = ConsumeToken(message, pos);

    int i = 0;
    while (pos != std::string::npos && pos < message.length() && i < kMaxParameters - 1) {
        if (message[pos] == ':') {
            msg.trailing = message.substr(pos + 1);
            pos = std::string::npos;
            break;
        }
        msg.parameters[i] = ConsumeToken(message, pos);
        ++i;
    }

    if (pos != std::string::npos && pos < message.length()) {
        if (message[pos] == ':') {
            pos += 1;
        }
        msg.trailing = message.substr(pos);
    }
}

int ProcessMessage(EventLoopContext &context, Message &msg)
{
    (void)context;
    // parse prefix
    // ...

    // parse command
    if (msg.command.length() == 3 && std::isdigit(msg.command[0]) && std::isdigit(msg.command[1]) &&
        std::isdigit(msg.command[2])) {
        // numeric =
        //     (message[pos] - '0') * 100 + (message[pos + 1] - '0') * 10 + (message[pos + 2] -
        //     '0');
        // parse digit command
    } else {
        if (!IsCommand(msg.command)) {
            return 1;
        }
    }
    return 0;
}

int ProcessTcpStream(EventLoopContext &context, char *buf, Client *client, ssize_t buflen)
{
    Message msg;
    ssize_t bytes_remaining = 0;
    ssize_t bytes_to_copy = buflen;
    size_t message_size;

    if (client->msgbuf.length() + buflen > kMsgBufSize) {
        bytes_remaining = (client->msgbuf.length() + buflen) - kMsgBufSize;
        bytes_to_copy = buflen - bytes_remaining;
    }
    client->msgbuf.append(buf, bytes_to_copy);

    if ((message_size = client->msgbuf.find(kCrLf)) == std::string::npos) {
        if (client->msgbuf.length() > kMsgBufSize - 2) {
            // Message invalid
        } else {
            // Message might not be fully received yet, wait for next read cycle
        }
        return -1;
    }
    if (ParseMessage(client->msgbuf.substr(0, message_size), msg) == -1) {
        return -1;
    }
    ProcessMessage(context, msg);

    return 0;
}

}  // namespace msg
}  // namespace ircs
