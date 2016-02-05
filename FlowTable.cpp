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
#include <memory>

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

static const char *
get_state_string(navl_state_t state)
{
    switch (state)
    {
    case NAVL_STATE_INSPECTING:
        return "INSPECTING";
    case NAVL_STATE_CLASSIFIED:
        return "CLASSIFIED";
    case NAVL_STATE_TERMINATED:
        return "TERMINATED";
    default:
        return "UNKNOWN";
    }
}

void
FlowTable::InsertOrUpdateFlows(ConnKey *key, std::string pkt_string) {
    std::lock_guard<std::mutex> lk(ftbl_mutex);
    ConnMetadata mdata;
    int hash;
    ConnInfo *info;
    ConnKey *key1;

    auto it = conn_table.find(*key);
    if (it == conn_table.end())
    {
        std::pair<FlowTable::unorderedmap::iterator,bool> ret;
        ret = this->conn_table.insert(std::make_pair(*key, FlowTable::ConnInfo(key)));
        if (ret.second == false) {
            std::cout << "flow insert failed " << std::endl;
            return;
        }
        it = ret.first;
        it->second.packetnum = 0;
        key1 = &it->first;
        info = &it->second;
        mdata.key = key1;
        mdata.info = info;
        mdata.dir = 0;
        mdata.pktnum = it->second.packetnum;
        mdata.data.append(pkt_string);
        hash = this->conn_table.hash_function()(it->first) % numQueues_;
        it->second.queue = hash;
        this->ftbl_queue_list_[hash]->push(mdata);
    }
    else {
        it->second.packetnum++;
        int dir = key->srcaddr == it->first.srcaddr ? 0 : 1;
        key1 = &it->first;
        info = &it->second;
        mdata.key = key1;
        mdata.info = info;
        mdata.dir = dir;
        mdata.pktnum = it->second.packetnum;
        mdata.data.append(pkt_string);
        hash = conn_table.hash_function()(it->first) % numQueues_;
        if (it->second.class_state == NAVL_STATE_INSPECTING) {
            if(it->second.dpi_state != NULL)
                this->ftbl_queue_list_[it->second.queue]->push(mdata);
        }
        else {
            //std::cout << "Ignoring packets now " << std::endl;
        }
    }
}

void FlowTable::updateFlowTableDPIData(ConnInfo *info,
                                       navl_handle_t handle,
                                       navl_result_t result, 
                                       navl_state_t state,
                                       navl_conn_t nc,
                                       int error) {
    //std::lock_guard<std::mutex> lk(ftbl_mutex);
    ftbl_mutex.lock();
    static char name[9];

    info->error = error;
    info->class_state = state;
    info->dpi_result = navl_app_get(handle, result, &info->dpi_confidence);


    printf("    Classification: %s %s after %u packets %u%% confidence\n", info->dpi_result ? navl_proto_get_name(handle, info->dpi_result, name, sizeof(name)) : "UNKNOWN"
        , get_state_string(info->class_state)
        , info->classified_num
        , info->dpi_confidence);
    printf("    Flow table size %d\n", conn_table.size()); 
    ftbl_mutex.unlock();
}

FlowTable::FlowTable(int numOfQueues):
    numQueues_(numOfQueues) {
    for (int i = 0; i < numOfQueues; i++)
        ftbl_queue_list_.push_back(std::unique_ptr<Queue<ConnMetadata>>(new Queue<ConnMetadata>()));
}

FlowTable::~FlowTable() {
    std::cout << "Calling destructor" << std::endl;
}

} 
