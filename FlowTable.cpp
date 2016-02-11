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
#include <syslog.h>

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

static counter = 0;
static int newc = 0;
std::mutex m_mutex;
std::condition_variable m_cv;
bool notify = false;

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

FlowTable::unorderedmap::iterator FlowTable::findOrDeleteTableEntry(ConnKey *key, bool delete_flag) {
    std::lock_guard<std::mutex> lk(ftbl_mutex);
    FlowTable::unorderedmap::iterator it;

    int inspec = 0;
    if (!delete_flag) {
/*
        it = this->conn_table.find(*key);
        if (it == conn_table.end()) {
            std::cout << "New connection key src " << key->srcaddr << " key dst " << key->dstaddr << std::endl; 
            std::pair<FlowTable::unorderedmap::iterator,bool> ret;
            ret = this->conn_table.insert(std::make_pair(*key, FlowTable::ConnInfo(key)));
            it = ret.first;
            it->second.packetnum = 0;
        }
        return it;
*/
    }
    else {
        //syslog(LOG_INFO, "Before delete Conn table size %d classified flows till now %d ", conn_table.size(), counter);
        int current_time = time(0);
        for (auto it = this->conn_table.begin(); it != this->conn_table.end(); ++it) {
            if((it->second.pkt_ref_cnt == 0 &&
               (it->second.class_state == NAVL_STATE_CLASSIFIED || it->second.class_state == NAVL_STATE_TERMINATED)) ||
               difftime(current_time, it->second.lastpacket_timestamp) > 30 ||
               difftime(current_time, it->second.classified_timestamp) > 30) {
                //conn_table.erase(it);    
            }
            if (it->second.class_state == NAVL_STATE_INSPECTING)
                inspec++;
        }
        //syslog(LOG_INFO, " Conn In Inspecting stage %d", inspec);
    }
    return it;
}

void
FlowTable::InsertOrUpdateFlows(ConnKey *key, std::string pkt_string, void *filter) {
    std::lock_guard<std::mutex> lk(ftbl_mutex);
    ConnMetadata mdata;
    int hash;
    ConnInfo *info;
    ConnKey *key1;
    static int conn = 0;
    auto it = conn_table.find(*key);
    if (it == conn_table.end())
    {
        std::pair<FlowTable::unorderedmap::iterator,bool> ret;
        ret = conn_table.insert(std::make_pair(*key, FlowTable::ConnInfo(key)));
        if (ret.second == false) {
            syslog(LOG_INFO , "flow insert failed ");
            return;
        }
        it = ret.first;
        it->second.packetnum = 0;
        key1 = &it->first;
        info = &it->second;
        newc++;
        //syslog(LOG_INFO, "New conn  src addr %d dst addr %d conn num %d", key1->srcaddr, key1->dstaddr, newc);
        mdata.key = key1;
        mdata.info = info;
        mdata.dir = 0;
        mdata.pktnum = it->second.packetnum;
        mdata.data.append(pkt_string);
        mdata.exit_flag = 0;
        hash = conn_table.hash_function()(it->first) % numQueues_;
        it->second.queue = hash;
        it->second.lastpacket_timestamp = time(0);
        ftbl_queue_list_[hash]->push(mdata);
    }
    else {
        it->second.packetnum++;
        if (it->second.class_state == NAVL_STATE_CLASSIFIED ||
            it->second.class_state == NAVL_STATE_TERMINATED) {
            conn++;
            return;
        }
        int dir = key->srcaddr == it->first.srcaddr ? 0 : 1;
        key1 = &it->first;
        info = &it->second;
        //std::cout << "Old conn  src addr " << key1->srcaddr << " dst addr " << key1->dstaddr << std::endl;
        mdata.key = key1;
        mdata.info = info;
        mdata.dir = dir;
        mdata.pktnum = it->second.packetnum;
        mdata.data.append(pkt_string);
        mdata.exit_flag = 0;
        hash = conn_table.hash_function()(it->first) % numQueues_;
        if (it->second.class_state == NAVL_STATE_INSPECTING) {
            if(it->second.dpi_state != NULL) {
                it->second.pkt_ref_cnt++;
                it->second.lastpacket_timestamp = time(0);
                ftbl_queue_list_[it->second.queue]->push(mdata);
            }
        }
        else {
            //std::cout << "Ignoring packets now " << std::endl;
        }
    }
}

int FlowTable::updateFlowTableDPIData(ConnInfo *info,
                                      navl_handle_t handle,
                                      navl_result_t result, 
                                      navl_state_t state,
                                      navl_conn_t nc,
                                      int error) {
    static char name[9];
    int ret = 0;
    auto it = conn_table.find(info->key);
    if (it != conn_table.end()) {
        ConnInfo *conninfo = &it->second;
        if (conninfo->class_state == NAVL_STATE_INSPECTING)
            conninfo->pkt_ref_cnt = 0;
        else
            conninfo->pkt_ref_cnt--;
        conninfo->error = error;
        conninfo->handle = handle;
        conninfo->class_state = state;
        conninfo->dpi_result = navl_app_get(handle, result, &conninfo->dpi_confidence);
        if (conninfo->class_state == NAVL_STATE_CLASSIFIED ||
            conninfo->class_state == NAVL_STATE_TERMINATED) {
            navl_conn_destroy(it->second.handle, it->second.dpi_state);
            counter++;
            std::string dpi_data;
            conninfo->classified_timestamp = time(0);
            if (conninfo->dpi_result) {
                navl_proto_get_name(handle, conninfo->dpi_result, name, sizeof(name));
                dpi_data.assign(name, strlen(name));
            }
            else {
                dpi_data = "UNKNOWN";
            }
            ConnKey key;
            memcpy(&key, &(conninfo->key), sizeof(key));
            //InsertIntoClassifiedTbl(&key, dpi_data);
            ret = -1;
        }

        printf("    Classification: %s %s after %u packets %u%% confidence\n", conninfo->dpi_result ? navl_proto_get_name(handle, conninfo->dpi_result, name, sizeof(name)) : "UNKNOWN"
            , get_state_string(conninfo->class_state)
            , conninfo->classified_num
            , conninfo->dpi_confidence);
  
    }
    return ret;
}

void FlowTable::InsertIntoClassifiedTbl(ConnKey *key, std::string &dpi_data) {
    auto it = classified_table.find(*key);
    if (it == classified_table.end())
    {
        std::pair<FlowTable::classified_unordmap::iterator,bool> ret;
        ret = classified_table.insert(std::make_pair(*key, dpi_data));
        if (ret.second == false) {
            std::cout << "flow insert failed " << std::endl;
            return;
        }
        //notify_ = true;
        //datapathUpdatecv_.notify_one();
    }
    else {
    }
}

void FlowTable::DatapathUpdate() {
    for (;;) {
        {
            std::unique_lock<std::mutex> lk(datapathUpdateMutex_);
            while(!notify_)
                //datapathUpdatecv_.wait(lock);
            if (!running_)
                break;
            notify = false;
        }
    }
}

void FlowTable::FlowTableCleanup() {
    std::cout << "Flow Table cleanup thread started " << std::endl;
    for (;;) {
        {
            std::unique_lock<std::mutex> lk(cleanupThreadMutex_);
            cleanupThreadcv_.wait_for(lk, std::chrono::milliseconds(100));
            if(!running_)
                break;
        }
        if (conn_table.size() > 0)    
            findOrDeleteTableEntry(NULL, true);
        //std::cout << "Conn table size " << conn_table.size() << " Classified table size " << classified_table.size() << std::endl;
    }
    std::cout << "Flow Table cleanup exiting " << std::endl;
}

void FlowTable::start() {
    running_ = true;
    flowCleanupThread_ = std::thread(&FlowTable::FlowTableCleanup, this);
    //datapathUpdateThread_ = std::thread(&FlowTable::DatapathUpdate, this);
}

void FlowTable::stop () {
    {
        std::unique_lock<std::mutex> lk(cleanupThreadMutex_);
        running_ = false;
        cleanupThreadcv_.notify_one();
    }
    flowCleanupThread_.join();
    ConnMetadata m;
    m.exit_flag = 1;
    for (int i = 0; i < numQueues_; i++)
        ftbl_queue_list_[i]->push(m);
    std::cout << "Flow Table stop called " << std::endl;
}

FlowTable::FlowTable(int numOfQueues):
    numQueues_(numOfQueues) {
    for (int i = 0; i < numOfQueues; i++)
        ftbl_queue_list_.push_back(std::unique_ptr<Queue<ConnMetadata>>(new Queue<ConnMetadata>()));
}

FlowTable::~FlowTable() {
    std::cout << "Calling FlowTable Destructor" << std::endl;
}

} 
