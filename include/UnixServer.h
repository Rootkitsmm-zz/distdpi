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
        socketName = "/tmp/unix-socket1";
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

    int create() {
        struct sockaddr_un server_addr;
        int len;
        // setup socket address structure
        bzero(&server_addr,sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path,socketName,sizeof(server_addr.sun_path) - 1);
        unlink(server_addr.sun_path);

        len = strlen(server_addr.sun_path) + sizeof(server_addr.sun_family);
        // create socket
        server_ = socket(AF_UNIX,SOCK_STREAM,0);
        if (server_ == -1) {
            std::cout << " Could not open socket " << std::endl;
            perror("socket");
            return -1;
        }
        std::cout << "Socket fd returned " << server_ << std::endl;

        // call bind to associate the socket with the UNIX file system
        if (::bind(server_,(const struct sockaddr *)&server_addr,len) < 0) {
            std::cout << " bind failed ====== " << std::endl;
            perror("bind");
            return -1;
        }

        // convert the socket to listen for incoming connections
        if (listen(server_,5) < 0) {
            std::cout << " Listen Failed " << std::endl;
            perror("listen");
            return -1;
        }
        std::cout << "Listening on the unix socket " << std::endl;
        return 0;
    }

    void closeSocket() {
         close(server_);
         server_ = -1;
         unlink(socketName);
    }

};

}

#endif
