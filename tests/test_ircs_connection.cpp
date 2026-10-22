#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <string>
#include <vector>

#include "catch.hpp"
#include "ircs_connection.h"
#include "ircs_epoll.h"

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

void SetUpFileEventAndClient(ircs::EventLoop *loop, int fd)
{
    char ip[] = "127.0.0.1";
    ircs::Client *client = ircs::NewClient(fd, ip);

    int status = ircs::CreateFileEvent(loop, fd, ircs::kOnReadable, ircs::OnClientRead, client);
    REQUIRE(status == 0);

    ircs::SetFileEventState(loop->events[fd], ircs::kConnected);
}

TEST_CASE("OnClientRead")
{
    int status;
    int epfd = ircs::CreateEpollInstance();

    REQUIRE(epfd > 0);

    ircs::EventLoop loop = ircs::EventLoop();
    loop.epfd = epfd;

    ircs::FileEvent *fe = loop.events;

    SECTION("peer closed", "EOF")
    {
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        shutdown(sockfd[1], SHUT_RDWR);
        ircs::OnClientRead(&loop, sockfd[0], fe[sockfd[0]].data, fe[sockfd[0]].mask);
        CHECK(fe[sockfd[0]].state == ircs::kClosed);
        CHECK(fe[sockfd[0]].data == NULL);

        close(sockfd[0]);
        close(sockfd[1]);
    }

    // TODO: This control path can't be properly tested yet, due to
    //       ProcessTcpStream missing features to populate the write buffer
    // SECTION("process byte")
    // {
    //     write(pipefd[1], "PASS 1234\r\n", 11);
    //     ircs::OnClientRead(&loop, pipefd[0], &client, fe.mask);
    //     close(sockfd[0]);
    //     close(sockfd[1]);
    // }

    close(epfd);
}

TEST_CASE("OnClientWrite")
{
    int status;
    int epfd = ircs::CreateEpollInstance();

    REQUIRE(epfd > 0);

    ircs::EventLoop loop = ircs::EventLoop();
    loop.epfd = epfd;

    ircs::FileEvent *fe = loop.events;

    SECTION("0 bytes")
    {
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);

        ircs::OnClientWrite(&loop, sockfd[0], fe[sockfd[0]].data, fe[sockfd[0]].mask);
        CHECK(fe[sockfd[0]].mask == ircs::kOnReadable);
        CHECK(fe[sockfd[0]].data != NULL);
        close(sockfd[0]);
        close(sockfd[1]);
    }

    SECTION("0 bytes, closing")
    {
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);
        SetFileEventState(fe[sockfd[0]], ircs::kClosing);

        ircs::OnClientWrite(&loop, sockfd[0], fe[sockfd[0]].data, fe[sockfd[0]].mask);
        CHECK(fe[sockfd[0]].mask == ircs::kNone);
        CHECK(fe[sockfd[0]].data == NULL);
        close(sockfd[0]);
        close(sockfd[1]);
    }

    SECTION("1 message")
    {
        std::string msg = "001 :Welcome\r\n";
        size_t msg_len = msg.length();
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);

        ircs::Client *client = static_cast<ircs::Client *>(fe[sockfd[0]].data);
        client->writebuf.append(msg);

        ircs::OnClientWrite(&loop, sockfd[0], fe[sockfd[0]].data, fe[sockfd[0]].mask);
        std::vector<char> buf(msg_len + 1);
        buf[msg_len] = '\0';
        size_t n = read(sockfd[1], buf.data(), msg_len);
        REQUIRE(n == msg_len);
        CHECK(fe[sockfd[0]].mask == ircs::kOnReadable);
        CHECK(buf.data() == msg);
        CHECK(fe[sockfd[0]].data != NULL);
        CHECK(client->writebuf.length() == 0);
        CHECK(client->write_pos == 0);
        close(sockfd[0]);
        close(sockfd[1]);
    }

    SECTION("1 message and close")
    {
        std::string msg = "ERROR :Closing Link\r\n";
        size_t msg_len = msg.length();
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);

        ircs::Client *client = static_cast<ircs::Client *>(fe[sockfd[0]].data);
        client->writebuf.append(msg);

        ircs::SetFileEventState(fe[sockfd[0]], ircs::kClosing);

        ircs::OnClientWrite(&loop, sockfd[0], fe[sockfd[0]].data, fe[sockfd[0]].mask);
        std::vector<char> buf(msg_len + 1);
        buf[msg_len] = '\0';
        size_t n = read(sockfd[1], buf.data(), msg_len);
        REQUIRE(n == msg_len);
        CHECK(fe[sockfd[0]].mask == ircs::kNone);
        CHECK(buf.data() == msg);
        CHECK(fe[sockfd[0]].data == NULL);
        CHECK(client->writebuf.length() == 0);
        CHECK(client->write_pos == 0);
        close(sockfd[0]);
        close(sockfd[1]);
    }

    SECTION("2 messages")
    {
        std::string msg1 = "001 :Welcome\r\n";
        std::string msg2 = "002 :Host\r\n";
        std::string msg = msg1 + msg2;
        size_t msg_len = msg.length();
        int sockfd[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);
        SetUpFileEventAndClient(&loop, sockfd[0]);

        status = ircs::CreateFileEvent(&loop, sockfd[0], ircs::kOnWritable, ircs::OnClientWrite,
                                       fe[sockfd[0]].data);
        REQUIRE(status == 0);

        ircs::Client *client = static_cast<ircs::Client *>(fe[sockfd[0]].data);
        client->writebuf.append(msg);

        ircs::SetFileEventState(fe[sockfd[0]], ircs::kClosing);

        ircs::OnClientWrite(&loop, sockfd[0], fe[sockfd[0]].data, fe[sockfd[0]].mask);
        std::vector<char> buf(msg_len + 1);
        buf[msg_len] = '\0';
        size_t n = read(sockfd[1], buf.data(), msg_len);
        REQUIRE(n == msg_len);
        CHECK(fe[sockfd[0]].mask == ircs::kNone);
        CHECK(buf.data() == msg);
        CHECK(fe[sockfd[0]].data == NULL);
        CHECK(client->writebuf.length() == 0);
        CHECK(client->write_pos == 0);
        close(sockfd[0]);
        close(sockfd[1]);
    }

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

        ircs::internal::RequestClose(&loop, sockfd[0], fe[sockfd[0]].mask, client);
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

        ircs::internal::RequestClose(&loop, sockfd[0], fe[sockfd[0]].mask, client);
        CHECK(fe[sockfd[0]].data != NULL);
        CHECK(fe[sockfd[0]].state == ircs::kClosing);
        CHECK(fe[sockfd[0]].mask == ircs::kOnWritable);

        close(sockfd[0]);
        close(sockfd[1]);
    }
}

TEST_CASE("CloseClient")
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

        ircs::internal::CloseClient(&loop, sockfd[0], client);
        CHECK(fe[sockfd[0]].data == NULL);
        CHECK(fe[sockfd[0]].state == ircs::kClosed);
        CHECK(fe[sockfd[0]].mask == ircs::kNone);
        ircs::internal::CloseClient(&loop, sockfd[0], client);
    }
}
