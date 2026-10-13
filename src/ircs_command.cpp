#include "ircs_command.h"

#include "ircs_connection.h"

namespace ircs {

namespace {

// TODO: What should Handle Command function actually return?
//          should it return a NumericReply or something else?
int HandlePass(const CommandEntry *command, EventLoopContext *context, Client *client,
               const msg::Message &message)
{
    // Do all the checks that are necessary for this specific command
    if (client->state == REGISTERED) {
        return kErrAlreadyRegistered;
    }
    if (message.param_count < command->min_params) {
        return kErrNeedMoreParams;
    }
    if (message.parameters[0] != context->password) {
        return kErrPasswdMismatch;
    }

    // Behaviour of the code comes here...
    client->pass_accepted = true;

    return 0;
}

int HandleNick(const CommandEntry *command, EventLoopContext *context, Client *client,
               const msg::Message &message)
{
    return 0;
}

int HandleUser(const CommandEntry *command, EventLoopContext *context, Client *client,
               const msg::Message &message)
{
    return 0;
}

const static int kCommandTableSize = 3;
const static CommandEntry kCommandTable[] = {
    {"PASS", HandlePass, 1, 1},
    // {"USER", NULL, 1, 1},
    // {"NICK", NULL, 1, 1},
};

void NormalizeCommandName(std::string &command)
{
    for (size_t i = 0; i < command.length(); i++) {
        command[i] = std::toupper(command[i]);
    }
}

const CommandEntry *LookupCommand(const std::string &command)
{
    for (size_t i = 0; i < kCommandTableSize; i++) {
        if (command == kCommandTable[i].name) {
            return &kCommandTable[i];
        }
    }
    return NULL;
}

}  // namespace

msg::ProcessStatus DispatchCommand(EventLoopContext *context, Client *client, msg::Message &message)
{
    NormalizeCommandName(message.command);
    const CommandEntry *cmd = LookupCommand(message.command);
    if (cmd == NULL) {
        // ERR_UNKNOWNCOMMAND;
    }
    cmd->handler(cmd, context, client, message);

    return msg::kOk;
}

}  // namespace ircs
