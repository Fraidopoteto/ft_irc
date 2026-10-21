#include <sys/epoll.h>
#include <unistd.h>
#include <cstdio>

#include "catch.hpp"
#include "ircs_connection.h"
#include "ircs_epoll.h"

TEST_CASE("Create-/RemoveFileEvent")
{
    int epfd = ircs::CreateEpollInstance();
    int pipefd[2];
    pipe(pipefd);

    ircs::EventLoop loop = ircs::EventLoop();
    ircs::FileEvent &fe = loop.events[pipefd[0]];
    loop.epfd = epfd;

    int status;
    status = ircs::CreateFileEvent(&loop, pipefd[0], ircs::kOnReadable, ircs::OnClientRead, NULL);
    REQUIRE(status == 0);
    status = ircs::CreateFileEvent(&loop, pipefd[0], ircs::kOnWritable, ircs::OnClientWrite, NULL);
    REQUIRE(status == 0);

    REQUIRE(fe.mask == (ircs::kOnReadable | ircs::kOnWritable));
    REQUIRE(fe.OnRead != NULL);
    REQUIRE(fe.OnWrite != NULL);

    ircs::RemoveFileEvent(&loop, pipefd[0], ircs::kOnWritable, NULL);
    REQUIRE(fe.mask == ircs::kOnReadable);
    REQUIRE(fe.OnRead != NULL);
    REQUIRE(fe.OnWrite == NULL);

    ircs::RemoveFileEvent(&loop, pipefd[0], ircs::kOnReadable, NULL);
    REQUIRE(fe.mask == ircs::kNone);
    REQUIRE(fe.OnRead == NULL);
    REQUIRE(fe.OnWrite == NULL);

    close(pipefd[0]);
    close(pipefd[1]);
    close(epfd);
}
