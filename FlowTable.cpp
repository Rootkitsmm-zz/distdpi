#include <iostream>
#include <string.h>
#include <errno.h>       // errno
#include <stdlib.h>      // NULL
#include <stdio.h>       // FILE
#include <unistd.h>      // close
#include <string.h>      // memcpy()
#include <sys/types.h>   // uint8_t
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include <memory>
#include <syslog.h>
#include <arpa/inet.h>

#include <FlowTable.h>
#include <DPIEngine.h>
#include <DataPathUpdate.h>

namespace distdpi {

static int counter = 0;
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

void FlowTable::cleanupFlows(bool final_delete) {
    std::lock_guard<std::mutex> lk(ftbl_mutex);
    FlowTable::unorderedmap::iterator it;

    int inspec = 0;
    if (final_delete) {
        auto it = conn_table.begin();
        while (it != conn_table.end()) {
            navl_conn_destroy(it->second.handle, it->second.dpi_state);
            it = conn_table.erase(it);
        }
    }
    else {
        int current_time = time(0);
        auto it = conn_table.begin();
        while (it != conn_table.end()) {
            if (it->second.pkt_ref_cnt == 0 &&
               (it->second.class_state == NAVL_STATE_CLASSIFIED ||
                it->second.class_state == NAVL_STATE_TERMINATED)) {
                it = conn_table.erase(it);    
            }
            else if (/*it->second.pkt_ref_cnt == 0 &&*/
                it->second.class_state == NAVL_STATE_INSPECTING &&
                difftime(current_time, it->second.lastpacket_timestamp) > 10) {
                inspec++;
                int ret = navl_conn_destroy(it->second.handle, it->second.dpi_state);
                it = conn_table.erase(it);
            }
            else {
                it++;
            }
        }
    }
}

void
FlowTable::InsertOrUpdateFlows(ConnKey *key, std::string pkt_string, void *filter, uint8_t dir) {
    std::lock_guard<std::mutex> lk(ftbl_mutex);
    ConnMetadata mdata;
    int hash;
    static int conn = 0;

    if (numQueues_ == 0)
        return;

    auto it = conn_table.find(*key);
    if (it == conn_table.end())
    {
        std::pair<FlowTable::unorderedmap::iterator,bool> ret;
        ret = conn_table.insert(std::make_pair(*key, ConnInfo(key)));
        if (ret.second == false) {
            syslog(LOG_INFO , "flow insert failed ");
            return;
        }
        it = ret.first;
        it->second.packetnum = 0;
        it->second.filter = filter;
        it->second.dir = dir;
        mdata.key = &it->first;
        mdata.info = &it->second;
        mdata.dir = 0;
        mdata.pktnum = it->second.packetnum;
        mdata.data.append(pkt_string);
        mdata.exit_flag = 0;
        hash = conn_table.hash_function()(it->first) % numQueues_;
        it->second.queue = hash;
        it->second.lastpacket_timestamp = time(0);
        dpiEngine_->dpiEngineQueueList_[hash]->push(mdata);
    }
    else {
        it->second.packetnum++;
        if (it->second.class_state == NAVL_STATE_CLASSIFIED ||
            it->second.class_state == NAVL_STATE_TERMINATED) {
            conn++;
            return;
        }
        int dir = key->srcaddr == it->first.srcaddr ? 0 : 1;
        mdata.key = &it->first;
        mdata.info = &it->second;
        mdata.dir = dir;
        mdata.pktnum = it->second.packetnum;
        mdata.data.append(pkt_string);
        mdata.exit_flag = 0;
        hash = conn_table.hash_function()(it->first) % numQueues_;
        if (it->second.class_state == NAVL_STATE_INSPECTING) {
            if(it->second.dpi_state != NULL) {
                it->second.pkt_ref_cnt++;
                it->second.lastpacket_timestamp = time(0);
                dpiEngine_->dpiEngineQueueList_[it->second.queue]->push(mdata);
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
        ConnKey *key = &it->first;
        
        if (conninfo->class_state == NAVL_STATE_INSPECTING)
            conninfo->pkt_ref_cnt = 0;
        else
            conninfo->pkt_ref_cnt--;

        conninfo->error = error;
        conninfo->class_state = state;
        conninfo->dpi_result = navl_app_get(handle,
                                            result,
                                            &conninfo->dpi_confidence);

        if (conninfo->class_state == NAVL_STATE_CLASSIFIED ||
            conninfo->class_state == NAVL_STATE_TERMINATED) {
        
            DPIFlowData data;
            int ret = navl_conn_destroy(it->second.handle, it->second.dpi_state);
            counter++;
            conninfo->classified_timestamp = time(0);
            if (conninfo->dpi_result) {
                navl_proto_get_name(handle,
                                    conninfo->dpi_result,
                                    name, sizeof(name));
                data.dpiresult.assign(name, strlen(name));
                data.len = strlen(name);
            }
            else {
                data.dpiresult = "UNKNOWN";
                data.len = strlen("UNKNOWN");
            }
            data.key.srcaddr = key->srcaddr;
            data.key.dstaddr = key->dstaddr;
            data.key.srcport = key->srcport;
            data.key.dstport = key->dstport;
            data.key.ipproto = key->ipproto;
            data.dir = conninfo->dir;
            data.filter = conninfo->filter;
            data.exit_flag = 0;
/*
            printf("    Classification: %s %s after %u packets %u%% confidence\n", conninfo->dpi_result ? navl_proto_get_name(handle, conninfo->dpi_result, name, sizeof(name)) : "UNKNOWN"
                , get_state_string(conninfo->class_state)
                , conninfo->classified_num
                , conninfo->dpi_confidence);
*/
            dpUpdate_->updateDPQueue_->push(data);
            ret = 0;
        }
        else {
        }
    }
    return ret;
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
            cleanupFlows(false);
    }
    std::cout << "Flow Table cleanup exiting " << std::endl;
}

void FlowTable::start(std::shared_ptr<DPIEngine> &dpiEngine) {
    running_ = true;
    dpiEngine_ = dpiEngine;
    flowCleanupThread_ = std::thread(&FlowTable::FlowTableCleanup, this);
}

void FlowTable::stop () {
    {
        std::unique_lock<std::mutex> lk(cleanupThreadMutex_);
        running_ = false;
        cleanupThreadcv_.notify_one();
    }
    flowCleanupThread_.join();

    ConnMetadata m;
    DPIFlowData data;

    data.exit_flag = 1;
    m.exit_flag = 1;

    cleanupFlows(true);

    dpUpdate_->updateDPQueue_->push(data);
    for (int i = 0; i < numQueues_; i++)
        dpiEngine_->dpiEngineQueueList_[i]->push(m);
    
    std::cout << "Flow Table stop called " << std::endl;
}

FlowTable::FlowTable(int numOfQueues,
                     std::shared_ptr<DataPathUpdate> dp_update):
   numQueues_(numOfQueues), dpUpdate_(dp_update) {
}

FlowTable::~FlowTable() {
    std::cout << "Calling FlowTable Destructor" << std::endl;
}

} 
