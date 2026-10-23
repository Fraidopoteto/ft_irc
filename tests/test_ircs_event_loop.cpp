#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <string>

#include "catch.hpp"
#include "ircs_client.h"
#include "ircs_epoll.h"
#include "ircs_event_loop.h"

// Helper
void SetUpFileEventAndClient(ircs::EventLoop *loop, int fd)
{
    char ip[] = "127.0.0.1";
    ircs::Client *client = ircs::NewClient(fd, ip);

    int status = ircs::CreateFileEvent(loop, fd, ircs::kOnReadable, ircs::OnClientRead, client);
    REQUIRE(status == 0);

    ircs::SetFileEventState(loop->events[fd], ircs::kFileEventRegistered);
}


TEST_CASE("Create-/RemoveFileEvent")
{
    int epfd = ircs::CreateEpollInstance();
    int pipefd[2];

    REQUIRE(epfd > 0);
    REQUIRE(pipe(pipefd) == 0);

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

TEST_CASE("RequestClose")
{
    int status;
    int epfd = ircs::CreateEpollInstance();

    REQUIRE(epfd > 0);

    ircs::EventLoop loop = ircs::EventLoop();
    loop.epfd = epfd;

    ircs::FileEvent *fe = loop.events;

    SECTION("writebuf empty")
    {
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);

        ircs::Client *client = static_cast<ircs::Client *>(fe[sockfd[0]].data);

        ircs::RequestClose(&loop, sockfd[0], fe[sockfd[0]].mask, client);
        CHECK(fe[sockfd[0]].data == NULL);

        close(sockfd[0]);
        close(sockfd[1]);
    }

    SECTION("writebuf not empty")
    {
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        ircs::Client *client = static_cast<ircs::Client *>(fe[sockfd[0]].data);

        client->writebuf.append("Hello\r\n");

        ircs::RequestClose(&loop, sockfd[0], fe[sockfd[0]].mask, client);
        CHECK(fe[sockfd[0]].data != NULL);
        CHECK(fe[sockfd[0]].state == ircs::kFileEventClosing);
        CHECK(fe[sockfd[0]].mask == ircs::kOnWritable);

        close(sockfd[0]);
        close(sockfd[1]);
    }
}

TEST_CASE("CloseConnection")
{
    int status;
    int epfd = ircs::CreateEpollInstance();

    REQUIRE(epfd > 0);

    ircs::EventLoop loop = ircs::EventLoop();
    loop.epfd = epfd;

    ircs::FileEvent *fe = loop.events;

    SECTION("double close")
    {
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);

        ircs::Client *client = static_cast<ircs::Client *>(fe[sockfd[0]].data);

        client->writebuf.append("Hello\r\n");

        ircs::CloseConnection(&loop, sockfd[0], client);
        CHECK(fe[sockfd[0]].data == NULL);
        CHECK(fe[sockfd[0]].state == ircs::kFileEventClosed);
        CHECK(fe[sockfd[0]].mask == ircs::kNone);
        ircs::CloseConnection(&loop, sockfd[0], client);
    }
}
