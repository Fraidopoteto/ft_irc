#include "ircs_command.h"

#include "ircs_connection.h"

namespace ircs {

namespace {

void QueueNumericReply(Client *client, int numeric, const std::string &message)
{
    if (!client)
        return;

    std::ostringstream oss;

    oss << ":localhost ";

    oss << std::setfill('0') << std::setw(3) << numeric;

    oss << " ";
    if (client->nick_name.empty())
        oss << "*";
    else
        oss << client->nick_name;

    oss << " " << message;
    oss << "\r\n";

    client->writebuf += oss.str();
}

void SendWelcome(Client *client, EventLoopContext *context)
{
    if (!client)
        return;

    const std::string full_client_id =
        client->nick_name + "!" + client->user_name + "@" + client->host_name;

    const std::string RplWelcome = ":Welcome to the Internet Relay Network " + full_client_id;
    QueueNumericReply(client, kRplWelcome, RplWelcome);

    const std::string RplYourHost =
        ":Your host is " + context.host_name + ", running version " + context.kVersion;
    QueueNumericReply(client, kRplYourHost, RplYourHost);

    const std::string RplCreated = "This server was created " + context.creation_date;
    QueueNumericReply(client, kRplCreated, RplCreated);

    const std::string RplMyInfo = context.host_name + " " + context.kVersion + " io itkol";
    QueueNumericReply(client, kRplMyInfo, RplMyInfo);
}

CommandStatus HandlePass(const CommandEntry *command, EventLoopContext *context, Client *client,
                         const msg::Message &message)
{
    if (client->state == REGISTERED) {
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

    if (client->state == VALID && client->state != REGISTERED) {
        client->nick_name = nick_name;
        client->state = REGISTERED;
        SendWelcome(client, context);
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

    if (client->state == VALID && client->state != REGISTERED) {
        client->user_name = user_name;
        client->host_name = "localhost";  // TO DO: change the hostname
        client->real_name = real_name;
        client->state = REGISTERED;
        SendWelcome(client, context);
        return kContinue;
    } else if (client->state == CONNECTED) {
        client->state = VALID;
    }

    client->user_name = user_name;
    client->host_name = "localhost";  // TO DO: change the hostname
    client->real_name = real_name;

    return kContinue;
}

const static int kCommandTableSize = 3;
const static CommandEntry kCommandTable[] = {
    {"PASS", HandlePass, 1, 1},
    {"USER", NULL, 4, 4},
    {"NICK", NULL, 1, 1},
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
