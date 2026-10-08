#include "ircs_message.h"

#include <stdint.h>
#include <sys/types.h>

#include <cstring>
#include <iostream>
#include <string>

#include "ircs_connection.h"
#include "ircs_logger.h"

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

void MessageBufferAppend(MessageBuffer &buffer, char *buf, ssize_t buflen)
{
    buffer.data.append(buf, buflen);
    if (buffer.data.find(kCrLf, 0) == std::string::npos && buffer.data.length() > kMaxMsgSize - 2) {
        buffer.overflow = true;
    }
}

ProcessStatus MessageBufferNext(MessageBuffer &buffer, std::string &out_line)
{
    size_t crlf_pos;
    size_t message_length;

    crlf_pos = buffer.data.find(kCrLf, buffer.read_pos);
    if (crlf_pos == std::string::npos) {
        message_length = buffer.data.length() - buffer.read_pos;
        if (message_length > kMaxMsgSize - 2) {
            buffer.overflow = true;
            return kMessageTooLong;
        }
        return kNeedMoreData;
    }

    message_length = crlf_pos - buffer.read_pos;
    if (message_length > kMaxMsgSize - 2) {
        buffer.overflow = true;
        return kMessageTooLong;
    }

    out_line = buffer.data.substr(buffer.read_pos, crlf_pos);
    buffer.read_pos = crlf_pos + 2;

    return kOk;
}

void MessageBufferCompact(MessageBuffer &buffer)
{
    if (buffer.read_pos == 0)
        return;
    buffer.data = buffer.data.substr(buffer.read_pos);
    buffer.read_pos = 0;
}

size_t MessageBufferSize(MessageBuffer &buffer)
{
    return buffer.data.length();
}

bool MessageBufferOverflow(MessageBuffer &buffer)
{
    return buffer.overflow;
}

void MessageBufferClear(MessageBuffer &buffer)
{
    buffer.data.clear();
    buffer.read_pos = 0;
    buffer.overflow = false;
}

int ParseMessage(const std::string &message, Message &msg)
{
    msg = Message();
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
    while (pos != std::string::npos && pos < message.length() &&
           i < ircs::msg::kMaxParameters - 1) {
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

// STUB
int DispatchMessage()
{
    return 0;
}

ProcessStatus ProcessTcpStream(EventLoopContext &context, char *buf, Client *client, ssize_t buflen)
{
    (void)context;
    std::string line;

    MessageBufferAppend(client->message_buffer, buf, buflen);
    if (MessageBufferOverflow(client->message_buffer) == true) {
        MessageBufferClear(client->message_buffer);
        return kMessageTooLong;
    }

    while (MessageBufferNext(client->message_buffer, line) == kOk) {
        if (ParseMessage(line, client->message) == -1) {
            return kProtocolError;
        }
        DispatchMessage();
    }

    MessageBufferCompact(client->message_buffer);
    if (MessageBufferOverflow(client->message_buffer) == true) {
        MessageBufferClear(client->message_buffer);
        return kMessageTooLong;
    }
    if (MessageBufferSize(client->message_buffer) > 0) {
        return kNeedMoreData;
    }
    return kOk;
}

}  // namespace msg
}  // namespace ircs
