#include "ircserv.h"

#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "ircs_connection.h"
#include "ircs_logger.h"

using ircs::log::Logger;

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    // struct rlimit rl;
    // if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
    //     ircs::log::Logger(ircs::log::kError, "getrlimit()", strerror(errno));
    //     return 1;
    // }
    // const int kMaxConnections = rl.rlim_cur;

    if (argv[1] == NULL || argv[1][0] == '\0') {
        std::cerr << "error: Port cannot be empty" << std::endl;
        return 1;
    }
    // TODO:(rtwobie): Check if password can be empty
    // if (argv[2] == NULL || argv[2][0] == '\0') {
    //     std::cerr << "error: Password cannot be empty" << std::endl;
    //     return 1;
    // }

    int listenfd;
    int epfd;

    if ((epfd = epoll_create1(0)) == -1) {
        Logger(ircs::log::kError, "epoll_create", strerror(errno));
        return -1;
    }

    if ((listenfd = ircs::CreateListener(argv[1])) == -1) {
        return 1;
    }

    ircs::EventLoop el;
    std::memset(&el, 0, sizeof(el));
    el.epfd = epfd;
    el.stop = false;

    if (CreateFileEvent(&el, listenfd, ircs::kOnReadable, ircs::OnConnectionAccept, NULL) == -1) {
        return 1;
    }

    RunEventLoop(&el);

    close(listenfd);
    close(epfd);
    return 0;
}
