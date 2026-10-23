#include <sys/socket.h>
#include <unistd.h>

#include "catch.hpp"
#include "ircs_client.h"
#include "ircs_epoll.h"

void SetUpFileEventAndClient(ircs::EventLoop *loop, int fd);

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
        CHECK(fe[sockfd[0]].state == ircs::kFileEventClosed);
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
        ircs::SetFileEventState(fe[sockfd[0]], ircs::kFileEventClosing);

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

        ircs::SetFileEventState(fe[sockfd[0]], ircs::kFileEventClosing);

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

        ircs::SetFileEventState(fe[sockfd[0]], ircs::kFileEventClosing);

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
