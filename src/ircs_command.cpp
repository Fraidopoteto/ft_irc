#include "ircs_command.h"

#include "ircs_connection.h"

namespace ircs {

namespace {

void NormalizeCommandName(std::string &command)
{
    for (size_t i = 0; i < command.length(); i++) {
        command[i] = std::toupper(command[i]);
    }
}

}  // namespace

int DispatchCommand(EventLoopContext *context, Client *client, msg::Message &message)
{
    (void)context;
    (void)client;
    NormalizeCommandName(message.command);

    return 0;
}

}  // namespace ircs
