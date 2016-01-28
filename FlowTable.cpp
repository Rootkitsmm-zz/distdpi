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
#include <thread>

#include <FlowTable.h>

namespace distdpi {

FlowTable::FlowTable(PacketHandler *hdl):
    pkthdl(hdl) {
}

void FlowTable::PacketConsumer() {
    for (;;) {
        std::string pkt;
        while(!pkthdl->queue_.read(pkt)) {
            continue;
        }
        std::cout << "Consumer got packet " << pkt << std::endl;
    } 
}

void FlowTable::start() {
}

FlowTable::~FlowTable() {
    std::cout << "Calling destructor" << std::endl;
}

} 
