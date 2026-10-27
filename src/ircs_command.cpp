#include "ircs_command.h"

#include <vector>

#include "ircs_client.h"
#include "ircs_client_io.h"
#include "ircs_event_loop.h"
#include "ircs_logger.h"

namespace ircs {
using log::Logger;

namespace {

void SendWelcome(Client *client, EventLoopContext *context)
{
    if (!client)
        return;

    const std::string full_client_id =
        client->nick_name + "!" + client->user_name + "@" + client->host_name;

    const std::string rpl_welcome = ":Welcome to the Internet Relay Network " + full_client_id;
    QueueNumericReply(client, kRplWelcome, rpl_welcome);

    const std::string rpl_yourhost =
        ":Your host is " + context->host_name + ", running version " + context->version;
    QueueNumericReply(client, kRplYourHost, rpl_yourhost);

    const std::string rpl_created = "This server was created " + context->creation_date;
    QueueNumericReply(client, kRplCreated, rpl_created);

    const std::string rpl_myinfo = context->host_name + " " + context->version + " io itkol";
    QueueNumericReply(client, kRplMyInfo, rpl_myinfo);
}

CommandStatus HandlePass(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == kClientRegistered) {
        QueueNumericReply(client, kErrAlreadyRegistered, ":You may not reregister");
        return kContinue;
    }
    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNeedMoreParams, "PASS :Not enough parameters");
        return kContinue;
    }
    if (message.parameters[0] != context->password) {
        QueueNumericReply(client, kErrPasswdMismatch, ":Password incorrect");
        return kDisconnect;
    }

    client->pass_accepted = true;

    return kContinue;
}

CommandStatus HandleNick(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNoNicknameGiven, ":No nickname given");
        return kContinue;
    }

    const std::string nick_name = message.parameters[0];
    char first = nick_name[0];

    if (first == '$' || first == ':' || first == '#' || first == '&' || first == '@' ||
        first == '+') {
        QueueNumericReply(client, kErrErroneousNickname, " :Erroneous nickname");
        return kContinue;
    }

    if (nick_name.empty() || nick_name.length() > 30 || nick_name.length() < 2) {
        QueueNumericReply(client, kErrErroneousNickname, " :Erroneous nickname");
        return kContinue;
    }

    for (size_t i = 0; i < nick_name.length(); ++i) {
        char c = nick_name[i];
        if (c == ' ' || c == ',' || c == '*' || c == '?' || c == '!' || c == '@') {
            QueueNumericReply(client, kErrErroneousNickname, " :Erroneous nickname");
            return kContinue;
        }
    }

    for (std::map<int, Client *>::iterator it = context->clients.begin();
         it != context->clients.end(); ++it) {
        if (it->second->fd != client->fd) {
            if (it->second->nick_name == nick_name) {
                QueueNumericReply(client, kErrNicknameInUse, " :Nickname is already in use");
                return kContinue;
            }
        }
    }

    if (client->state == kClientHasUser && client->state != kClientRegistered) {
        client->nick_name = nick_name;
        client->state = kClientRegistered;
        SendWelcome(client, context);
        return kContinue;
    } else if (client->state == kClientConnected) {
        client->state = kClientHasNick;
    }

    client->nick_name = nick_name;

    return kContinue;
}

CommandStatus HandleUser(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == kClientRegistered) {
        QueueNumericReply(client, kErrAlreadyRegistered, ":You may not reregister");
        return kContinue;
    }
    if (message.param_count < command->min_params) {
        QueueNumericReply(client, kErrNeedMoreParams, "PASS :Not enough parameters");
        return kContinue;
    }

    std::string user_name = message.parameters[0];
    const std::string real_name = message.parameters[3];

    if (user_name.length() > 10) {
        user_name = user_name.substr(0, 10);
    }

    if (client->state == kClientHasNick && client->state != kClientRegistered) {
        client->user_name = user_name;
        client->host_name = "localhost";  // TO DO: change the hostname
        client->real_name = real_name;
        client->state = kClientRegistered;
        SendWelcome(client, context);
        return kContinue;
    } else if (client->state == kClientConnected) {
        client->state = kClientHasUser;
    }

    client->user_name = user_name;
    client->host_name = "localhost";  // TO DO: change the hostname
    client->real_name = real_name;

    return kContinue;
}

// NOTE: Add Commands in this table!
std::vector<CommandEntry> InitializeCommandTable()
{
    std::vector<CommandEntry> table;

    table.push_back((CommandEntry){kPass, HandlePass, 1, 1});
    table.push_back((CommandEntry){kUser, HandleUser, 4, 4});
    table.push_back((CommandEntry){kNick, HandleNick, 1, 1});

    return table;
}

const static std::vector<CommandEntry> kCommandTable = InitializeCommandTable();

void NormalizeCommandName(std::string &command)
{
    for (size_t i = 0; i < command.length(); i++) {
        command[i] = std::toupper(command[i]);
    }
}

const CommandEntry *LookupCommand(const std::string &command)
{
    for (size_t i = 0; i < kCommandTable.size(); i++) {
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
        QueueNumericReply(client, kErrUnknownCommand, message.command + " :Unknown command");
        return msg::kOk;
    }

    Logger(log::kDebug, "", "executing command %s...", message.command.c_str());
    CommandStatus status = cmd->handler(cmd, context, client, message);

    if (status == kDisconnect) {
        return msg::kCloseConnection;  // TO DO: new msg for failure
    }
    return msg::kOk;
}

}  // namespace ircs
