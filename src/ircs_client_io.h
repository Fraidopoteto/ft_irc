#ifndef IRCS_CLIENT_IO_H_
#define IRCS_CLIENT_IO_H_

#include <string>

#include "ircs_client.h"

namespace ircs {

void QueueRawReply(Client *client, std::string &message);
void QueueNumericReply(Client *client, int numeric, const std::string &message);

}  // namespace ircs

#endif  // IRCS_CLIENT_IO_H_
