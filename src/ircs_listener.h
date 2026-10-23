#ifndef IRCS_LISTENER_H_
#define IRCS_LISTENER_H_

#include "ircs_event_loop.h"

namespace ircs {

int CreateListener(char *port);
void OnListenerRead(EventLoop *event_loop, int fd, void *data, int mask);

}  // namespace ircs

#endif  // IRCS_LISTENER_H_
