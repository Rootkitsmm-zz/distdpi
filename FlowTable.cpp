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
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <signal.h>

#include <FlowTable.h>

#ifdef FREEBSD
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#define ETH_P_IP    0x0800      /* Internet Protocol packet */
#define ETH_P_8021Q 0x8100          /* 802.1Q VLAN Extended Header  */
#else
#include <linux/if_ether.h>
#endif

namespace distdpi {

FlowTable::FlowTable() {
    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = (void *)&FlowTable::printFlowTable;
    sigaction(SIGTERM, &sigAct, 0);
}

void FlowTable::printFlowTable(int signal) {
    std::cout << "Printing flow table " << conn_table.size() << std::endl;
    if (conn_table.size() != 0) {
    for (auto it = conn_table.begin(); it != conn_table.end(); ++it)
    {
        std::cout << " " << it->first.srcaddr << " : " << it->first.dstaddr << " : " 
        << it->first.srcport << " : " << it->first.dstport << std::endl;
    }
    }
}

FlowTable::~FlowTable() {
    std::cout << "Calling destructor" << std::endl;
}

} 
