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
        this->classifyFlows(pkt);
    } 
}

void FlowTable::classifyFlows(std::string &packet) {
#ifdef FREEBSD
    typedef struct ip iphdr;
#endif
    const u_char *ptr = (u_char *) packet.c_str();
    u_int len = packet.size();
    const u_short *eth_type;
    const iphdr *iph;
    ConnKey key;

    // Read the first ether type
    len -= 12;
    eth_type = reinterpret_cast<const u_short *>(&ptr[12]);

    // Strip any vlans if present
    while (ntohs(*eth_type) == ETH_P_8021Q)
    {
            eth_type += 2;
            len -= 4;
    }

    // Ignore non-ip packets
    if (ntohs(*eth_type) != ETH_P_IP)
        return;

    len -= 2;
    iph = reinterpret_cast<const iphdr *>(++eth_type);

#ifdef FREEBSD
    // Do basic sanity of the ip header
    if (iph->ip_hl < 5 || iph->ip_v != 4 || len < ntohs(iph->ip_len))
        return;

    // Fix up the length as it may have been padded
    len = ntohs(iph->ip_len);

    // Build the 5-tuple key
    key.ip_proto = iph->ip_p;
    key.src_addr = ntohl(iph->ip_src.s_addr);
    key.dst_addr = ntohl(iph->ip_dst.s_addr);

    // Find the tcp header offset
    len -= (iph->ip_hl << 2);
    ptr = reinterpret_cast<const u_char *>(iph) + (iph->ip_hl << 2);
#else
    // Do basic sanity of the ip header
    if (iph->ihl < 5 || iph->version != 4 || len < ntohs(iph->tot_len))
        return;

    // Fix up the length as it may have been padded
    len = ntohs(iph->tot_len);

    // Build the 5-tuple key
    key.ipproto = iph->protocol;
    key.srcaddr = ntohl(iph->saddr);
    key.dstaddr = ntohl(iph->daddr);

    // Find the tcp header offset
    len -= (iph->ihl << 2);
    ptr = reinterpret_cast<const u_char *>(iph) + (iph->ihl << 2);
#endif

    populateFlowTable(ptr, len, &key); 
}

void FlowTable::populateFlowTable(const u_char *ptr,  u_int len, ConnKey *key) {
    const tcphdr *th;
    const udphdr *uh;

    switch (key->ipproto)
    {
    case IPPROTO_TCP:
        th = reinterpret_cast<const tcphdr *>(ptr);
#ifdef FREEBSD
        key->srcport = ntohs(th->th_sport);
        key->dstport = ntohs(th->th_dport);
        ptr += (th->th_off << 2);
        len -= (th->th_off << 2);
#else
        key->srcport = ntohs(th->source);
        key->dstport = ntohs(th->dest);
        ptr += (th->doff << 2);
        len -= (th->doff << 2);
#endif
        break;
    case IPPROTO_UDP:
        uh = reinterpret_cast<const udphdr *>(ptr);
#ifdef FREEBSD
        key->srcport = ntohs(uh->uh_sport);
        key->dstport = ntohs(uh->uh_dport);
#else
        key->srcport = ntohs(uh->source);
        key->dstport = ntohs(uh->dest);
#endif
        ptr += sizeof(*uh);
        len -= sizeof(*uh);
        break;
    };

    auto it = conn_table.find(*key);
    if (it == conn_table.end())
    {
        conn_table.insert(std::make_pair(*key, ConnInfo(key)));
    }
}

FlowTable::~FlowTable() {
    std::cout << "Calling destructor" << std::endl;
}

} 
