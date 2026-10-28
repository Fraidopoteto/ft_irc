#include <iomanip>
#include <sstream>

#include "ircs_client.h"
#include "ircs_message.h"

namespace ircs {

// NOTE: The caller of this function does *NOT* need to add "\r\n" at the end.
void QueueRawReply(Client *client, std::string &message)
{
    message += msg::kCrLf;
    client->writebuf.append(message);
}

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

}  // namespace ircs
