#include <iostream>
#include <string.h>
#include <errno.h>       // errno
#include <stdlib.h>      // NULL
#include <stdio.h>       // FILE
#include <unistd.h>      // close
#include <stdarg.h>      // va_start(), ...
#include <string.h>      // memcpy()
#include <ctype.h>       // toupper()
#include <getopt.h>      // getopt_long()
#include <sys/socket.h>  // socket()
#include <sys/types.h>   // uint8_t
#include <netinet/in.h>  // IF_NET
#include <sys/ioctl.h>     // ioctl
#include <sys/types.h>
#include <sys/stat.h>

#include <PacketHandler.h>
#include <Timer.h>
#include <UnixServer.h>

using namespace std::placeholders; 

namespace distdpi {

PacketHandler::PacketHandler(PacketHandlerOptions options):
    Timer(5.0) {
    options_ = (std::make_shared<PacketHandlerOptions>(std::move(options)));
}

void PacketHandler::PacketProducer(uint8_t *pkt, uint32_t len) {
    std::cout << "Got pkt of len " << len << std::endl;
}

void PacketHandler::start() {
    //this->addTask(this);
    std::function<void(uint8_t*, uint32_t)> callback;
    callback = std::bind(&PacketHandler::PacketProducer, this, _1, _2);
    UnixServer server;
    server.registerPacketCb(callback);
    server.run();
}

void PacketHandler::executeCb()
{
    std::cout << "Timer called " << std::endl;
}

PacketHandler::~PacketHandler() {
    std::cout << "Calling destructor" << std::endl;
}

} 
