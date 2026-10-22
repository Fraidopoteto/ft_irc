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
#include <ctime>
#include <iostream>

#include "ircs_connection.h"
#include "ircs_logger.h"

std::string getCreationDate()
{
    time_t now = time(NULL);
    char buf[64];
    strftime(buf, sizeof(buf), "%d %b %Y at %H:%M:%S", localtime(&now));
    return buf;
}

using ircs::log::Logger;

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    int epfd;
    int listenfd;

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

    char *endptr;
    long port = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || port < 1 || port > 65535) {
        std::cerr << "error: invalid Port" << std::endl;
        return -1;
    }

    if ((epfd = epoll_create1(0)) == -1) {
        Logger(ircs::log::kError, "epoll_create", strerror(errno));
        return -1;
    }

    if ((listenfd = ircs::CreateListener(argv[1])) == -1) {
        return 1;
    }

    ircs::EventLoop el = ircs::EventLoop();
    el.epfd = epfd;
    el.stop = false;
    el.context.port = static_cast<int>(port);
    el.context.password = (argv[2] == NULL || argv[2][0] == '\0') ? NULL : argv[2];
    el.context.creation_date = getCreationDate();

    if (CreateFileEvent(&el, listenfd, ircs::kOnReadable, ircs::OnConnectionAccept, NULL) == -1) {
        return 1;
    }

    RunEventLoop(&el);

    close(listenfd);
    close(epfd);
    return 0;
}
