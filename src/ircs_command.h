#ifndef IRCS_COMMAND_H_
#define IRCS_COMMAND_H_

#include "ircs_connection.h"

namespace ircs {

const char *const kPrivMsg = "PRIVMSG";
const char *const kUser = "USER";
const char *const kNick = "NICK";
const char *const kPass = "PASS";

typedef void (*CommandFn)(EventLoopContext *ctx, Client *client, const msg::Message &message);
typedef std::map<std::string, CommandFn> CommandList;

}  // namespace ircs

#endif  // IRCS_COMMAND_H_
