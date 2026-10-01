#include "ircs_message.h"

#include <stdint.h>
#include <sys/types.h>

#include <cstring>
#include <string>

#include "ircs_connection.h"

namespace {

const static std::string kCommandList[] = {ircs::msg::kPass, ircs::msg::kNick, ircs::msg::kUser};

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

bool IsMessagePrefixValid(const std::string &prefix)
{
    char c;

    if (!prefix.empty()) {
        bool exclamation_seen = false;
        bool at_seen = false;

        for (size_t i = 0; i < prefix.length(); i++) {
            c = prefix[i];

            if (c == ' ' || c == '\r' || c == '\n' || c == '\0') {
                return false;
            }

            if (c == '!') {
                if (exclamation_seen || at_seen || i == 0) {
                    return false;
                }
                exclamation_seen = true;
            } else if (c == '@') {
                if (at_seen || i == 0) {
                    return false;
                }
                at_seen = true;
            }
        }
    }
    return true;
}

bool IsMessageCommandValid(const std::string &command)
{
    char c;
    if (command.empty()) {
        return false;
    }
    for (size_t i = 0; i < command.length(); i++) {
        c = command[i];
        if (!std::isalpha(c)) {
            return false;
        }
    }
    return true;
}

bool IsMessageParameterValid(const std::string &parameter)
{
    char c;
    for (size_t i = 0; i < parameter.length(); i++) {
        c = parameter[i];
        if (c == '\0' || c == '\r' || c == '\n' || c == ' ' || c == ':') {
            return false;
        }
    }
    return true;
}

bool IsMessageTrailingValid(const std::string &trailing)
{
    char c;

    for (size_t i = 0; i < trailing.length(); i++) {
        c = trailing[i];
        // Spaces are allowed here
        if (c == '\0' || c == '\r' || c == '\n') {
            return false;
        }
    }
    return true;
}

}  // namespace

namespace ircs {
namespace msg {

int ParseMessage(const std::string &message, ircs::msg::Message &msg)
{
    std::string token;
    size_t pos = 0;

    if (message[pos] == ':') {
        pos += 1;
        token = ConsumeToken(message, pos);
        if (!IsMessagePrefixValid(token)) {
            return -1;
        }
        msg.prefix = token;
    }

    token = ConsumeToken(message, pos);
    if (!IsMessageCommandValid(token)) {
        return -1;
    }
    msg.command = token;

    int i = 0;
    while (pos != std::string::npos && pos < message.length() && i < ircs::msg::kMaxParameters - 1) {
        if (message[pos] == ':') {
            token = message.substr(pos + 1);
            if (!IsMessageTrailingValid(token)) {
                return -1;
            }
            msg.trailing = token;
            pos = std::string::npos;
            break;
        }
        token = ConsumeToken(message, pos);
        if (!IsMessageParameterValid(token)) {
            return -1;
        }
        msg.parameters[i] = token;
        ++i;
    }

    if (pos != std::string::npos && pos < message.length()) {
        if (message[pos] == ':') {
            pos += 1;
        }
        token = message.substr(pos);
        if (!IsMessageTrailingValid(token)) {
            return -1;
        }
        msg.trailing = token;
    }

    return 0;
}

int ProcessMessage(Message &msg)
{
    char c;

    if (!msg.prefix.empty()) {
        bool exclamation_seen = false;
        bool at_seen = false;

        for (size_t i = 0; i < msg.prefix.length(); i++) {
            c = msg.prefix[i];

            if (c == ' ' || c == '\r' || c == '\n' || c == '\0') {
                return -1;
            }

            if (c == '!') {
                if (exclamation_seen || at_seen || i == 0) {
                    return -1;
                }
                exclamation_seen = true;
            } else if (c == '@') {
                if (at_seen || i == 0) {
                    return -1;
                }
                at_seen = true;
            }
        }
    }

    if (msg.command.empty()) {
        return -1;
    }
    for (size_t i = 0; i < msg.command.length(); i++) {
        c = msg.command[i];
        if (!std::isalpha(c)) {
            return -1;
        }
    }

    int n = 0;
    while (n < kMaxParameters - 1) {
        for (size_t i = 0; i < msg.parameters[n].length(); i++) {
            c = msg.parameters[n][i];
            if (c == '\0' || c == '\r' || c == '\n' || c == ' ' || c == ':') {
                return -1;
            }
        }
        ++n;
    }

    for (size_t i = 0; i < msg.trailing.length(); i++) {
        c = msg.trailing[i];
        // Spaces are allowed here
        if (c == '\0' || c == '\r' || c == '\n' || c == ':') {
            return -1;
        }
    }
    return 0;
}

int ProcessTcpStream(EventLoopContext &context, char *buf, Client *client, ssize_t buflen)
{
    (void)context;
    client->message = Message();
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
    // TODO: ParseMessage should be called multiple times if more than one
    //       command are in the stream
    if (ParseMessage(client->msgbuf.substr(0, message_size), client->message) == -1) {
        // Potential IRC Error Code Handling (e.g. ERR_UNKNOWNCOMMAND)
        // Set Epoll event to EPOLLOUT to send Error to client
        return -1;
    }

    return 0;
}

}  // namespace msg
}  // namespace ircs
