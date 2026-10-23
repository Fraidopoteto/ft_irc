#include "ircs_client.h"

#include "ircs_event_loop.h"

namespace ircs {

Client *NewClient(int fd, char *ip)
{
    Client *c = new Client();

    c->fd = fd;
    c->ip = ip;
    c->message_buffer = msg::MessageBuffer();
    c->writebuf.reserve(msg::kMaxMsgSize);
    c->write_pos = 0;

    c->state = kClientConnected;

    c->pass_accepted = false;
    c->nick_accepted = false;
    c->name_accepted = false;

    return c;
}

void OnClientRead(EventLoop *event_loop, int fd, void *data, int mask)
{
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_received;
    char buf[kReadChunkSize];

    bytes_received = recv(fd, buf, sizeof(buf), 0);
    if (bytes_received == -1) {
        return;
    }

    if (bytes_received == 0) {
        CloseConnection(event_loop, fd, client);
        return;
    }

    if (bytes_received > 0) {
        msg::ProcessStatus status =
            msg::ProcessTcpStream(event_loop->context, buf, client, bytes_received);

        if (!client->writebuf.empty() && !(mask & kOnWritable)) {
            if (CreateFileEvent(event_loop, fd, kOnWritable, OnClientWrite, data) == -1) {
                CloseConnection(event_loop, fd, client);
                return;
            }
        }

        if (status == msg::kCloseConnection) {
            RequestClose(event_loop, fd, mask, client);
        }
    }
}

void OnClientWrite(EventLoop *event_loop, int fd, void *data, int mask)
{
    (void)mask;
    Client *client = static_cast<Client *>(data);
    ssize_t bytes_sent;
    size_t bytes_to_sent = client->writebuf.length() - client->write_pos;

    if (bytes_to_sent == 0) {
        RemoveFileEvent(event_loop, fd, kOnWritable, client);
        if (event_loop->events[fd].state == kFileEventClosing) {
            CloseConnection(event_loop, fd, client);
        }
        return;
    }

    bytes_sent =
        send(fd, client->writebuf.c_str() + client->write_pos, bytes_to_sent, MSG_NOSIGNAL);
    if (bytes_sent == -1) {
        return;
    }

    client->write_pos += bytes_sent;
    if (client->write_pos == client->writebuf.length()) {
        client->writebuf.clear();
        client->write_pos = 0;
        RemoveFileEvent(event_loop, fd, kOnWritable, client);
        if (event_loop->events[fd].state == kFileEventClosing) {
            CloseConnection(event_loop, fd, client);
            return;
        }
    }
}

}  // namespace ircs
