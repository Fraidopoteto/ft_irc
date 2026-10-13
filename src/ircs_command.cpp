#include "ircs_command.h"

#include "ircs_connection.h"

namespace ircs {

namespace {

void QueueNumeicReply(Client *client, int numeric, const std::string &message)
{
    // TO DO: QueueNumeicReply-funcion
}

CommandStatus HandlePass(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == REGISTERED) {
        QueueNumeicReply(client, kErrAlreadyRegistered, message);
        return kContinue;
    }
    if (message.param_count < command->min_params) {
        QueueNumeicReply(client, kErrNeedMoreParams, message);
        return kContinue;
    }
    if (message.parameters[0] != context->password) {
        QueueNumeicReply(client, kErrPasswdMismatch, message);
        return kDisconnect;
    }

    client->pass_accepted = true;

    return kContinue;
}

CommandStatus HandleNick(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (message.param_count < command->min_params) {
        QueueNumeicReply(client, kErrNoNicknameGiven, message);
        return kContinue;
    }

    const std::string nick_name = message.parameters[0];
    char first = nick_name[0];

    if (first == '$' || first == ':' || first == '#' || first == '&' || first == '@' ||
        first == '+') {
        QueueNumeicReply(client, kErrErroneousNickname, message);
        return kContinue;
    }

    if (nick_name.empty() || nick_name.length() > 30 || nick_name.length() < 2) {
        QueueNumeicReply(client, kErrErroneousNickname, message);
        return kContinue;
    }

    for (size_t i = 0; i < nick_name.length(); ++i) {
        char c = nick_name[i];
        if (c == ' ' || c == ',' || c == '*' || c == '?' || c == '!' || c == '@') {
            QueueNumeicReply(client, kErrErroneousNickname, message);
            return kContinue;
        }
    }

    for (std::map<int, Client *>::iterator it = context->clients.begin();
         it != context->clients.end(); ++it) {
        if (it->second->fd != client->fd) {
            if (it->second->nick_name == nick_name) {
                QueueNumeicReply(client, kErrNicknameInUse, message);
                return kDisconnect;
            }
        }
    }

    if (client->state == VALID && client->state != REGISTERED) {
        client->nick_name = nick_name;
        client->state = REGISTERED;
        // welcome message
        return kContinue;
    } else if (client->state == CONNECTED) {
        client->state = VALID;
    }

    client->nick_name = nick_name;

    return kContinue;
}

CommandStatus HandleUser(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == REGISTERED) {
        QueueNumeicReply(client, kErrAlreadyRegistered, message);
        return kContinue;
    }
    if (message.param_count < command->min_params) {
        QueueNumeicReply(client, kErrNeedMoreParams, message);
        return kContinue;
    }

    const std::string user_name = message.parameters[0];

    if (user_name[0] == '~') {
        QueueNumeicReply(client, kErrErroneousNickname, message);  // TO DO: wich error ?
        return kContinue;
    }

    if (user_name.empty() || user_name.length() > 10) {
        QueueNumeicReply(client, kErrErroneousNickname, message);  // TO DO: wich error ?
        return kContinue;
    }

    for (size_t i = 0; i < user_name.length(); ++i) {
        char c = user_name[i];
        if (std::iscntrl(c) || c == ' ' || c == '@' || c == ':') {
            QueueNumeicReply(client, kErrErroneousNickname, message);  // TO DO: wich error ?
            return kContinue;
        }
    }

    if (client->state == VALID && client->state != REGISTERED) {
        client->user_name = user_name;  // TO DO: set user name "Identity Server"
        client->host_name = user_name;
        client->real_name = user_name;
        client->state = REGISTERED;
        // welcome message
        return kContinue;
    } else if (client->state == CONNECTED) {
        client->state = VALID;
    }

    client->user_name = user_name;  // TO DO: set user name "Identity Server"
    client->host_name = user_name;
    client->real_name = user_name;

    return kContinue;
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

    CommandStatus status = cmd->handler(cmd, context, client, message);

    if (status == kDisconnect) {
        return msg::knotok;  // TO DO: new msg for failure
    }
    return msg::kOk;
}

}  // namespace ircs
