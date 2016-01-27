#ifndef UNIXSERVER_H
#define UNIXSERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/un.h>

#include "Server.h"

namespace distdpi {

class UnixServer : public Server {
public:

    UnixServer() {
        socketName = "/tmp/unix-socket";
    }
    ~UnixServer() {}

    void registerPacketCb(std::function<void(uint8_t*, uint32_t)> cb) {
        packetCallback_ = cb;
    }

private:

    const char* socketName;
    std::function<void(uint8_t*, uint32_t)> packetCallback_;

protected:
    void pktcb(uint8_t *pkt, uint32_t len) {
        packetCallback_(pkt, len);
    }

    void create() {
        struct sockaddr_un server_addr;

        // setup socket address structure
        bzero(&server_addr,sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path,socketName,sizeof(server_addr.sun_path) - 1);

        // create socket
        server_ = socket(PF_UNIX,SOCK_STREAM,0);
        if (!server_) {
            perror("socket");
            exit(-1);
        }

        // call bind to associate the socket with the UNIX file system
        if (::bind(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
            perror("bind");
            exit(-1);
        }

        // convert the socket to listen for incoming connections
        if (listen(server_,SOMAXCONN) < 0) {
            perror("listen");
            exit(-1);
        }
    }

    void closeSocket() {
         unlink(socketName);
    }

};

}

#endif
