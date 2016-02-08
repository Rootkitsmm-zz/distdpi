#ifndef FLOWTABLE_H
#define FLOWTABLE_H

#include "Queue.h"
#include "Timer.h"
#include "ProducerConsumerQueue.h"
#include "navl.h"

#include <unordered_map>
#include <list>
#include <vector>
#include <queue>
#include <memory>

namespace distdpi {

class FlowTable {
  public:

    struct ConnKey {
        ConnKey()
        : srcaddr(0),
          dstaddr(0),
          srcport(0),
          dstport(0),
          ipproto(0) {}

        uint32_t srcaddr;
        uint32_t dstaddr;
        uint16_t srcport;
        uint16_t dstport;
        u_char  ipproto;
    };

    struct ConnInfo {
        ConnInfo(ConnKey *key)
        : key(*key),
          queue(0),
          connid(0),
          packetnum(0),
          classified_num(0),
          class_state(NAVL_STATE_INSPECTING),
          dpi_state(NULL), dpi_result(0),
          dpi_confidence(0),
          error(0) {}


        ConnKey key;
        int queue;
        uint64_t connid;
        uint32_t packetnum;
        u_int   classified_num;
        navl_state_t class_state;

        void    *dpi_state;
        u_int   dpi_result;
        int     dpi_confidence;
        int     error;
        int     classified_timestamp;
    }; 

    struct ConnMetadata {
        ConnKey *key;
        ConnInfo *info;
        uint32_t pktnum;
        uint32_t dir;
        std::string data;
        uint8_t exit_flag;
    };

    struct ConnKeyHasher {
        size_t operator()(const ConnKey &key) const {
            return std::hash<uint32_t>() (key.srcaddr +
                                          key.dstaddr +
                                          key.srcport +
                                          key.dstport +
                                          key.ipproto);
        }
    };

    struct ConnKeyEqual {
        bool forward_lookup(const ConnKey &k1, const ConnKey &k2) const {
            return
                k1.srcaddr == k2.srcaddr &&
                k1.dstaddr == k2.dstaddr &&
                k1.srcport == k2.srcport &&
                k1.dstport == k2.dstport;
        }

        bool reverse_lookup(const ConnKey &k1, const ConnKey &k2) const {
            return
                k1.srcaddr == k2.dstaddr &&
                k1.dstaddr == k2.srcaddr &&
                k1.srcport == k2.dstport &&
                k1.dstport == k2.srcport;
        }

        bool operator()(const ConnKey &k1, const ConnKey &k2) const {
            return
                (k1.ipproto == k2.ipproto &&
                (forward_lookup(k1, k2) || reverse_lookup(k1, k2)));
        }
    };

    typedef std::unordered_map<ConnKey, ConnInfo, ConnKeyHasher, ConnKeyEqual> unorderedmap;
    std::unordered_map<ConnKey, ConnInfo, ConnKeyHasher, ConnKeyEqual> conn_table;

    typedef std::unordered_map<ConnKey, std::string, ConnKeyHasher, ConnKeyEqual> classified_unordmap;
    std::unordered_map<ConnKey, std::string, ConnKeyHasher, ConnKeyEqual> classified_table;

    std::vector<std::unique_ptr<Queue<ConnMetadata>>> ftbl_queue_list_;
    
    FlowTable(int numOfQueues);
    ~FlowTable();

    void InsertOrUpdateFlows(ConnKey *key, std::string pkt_string);

    void InsertIntoClassifiedTbl(ConnKey *key, std::string &dpi_data);

    void updateFlowTableDPIData(ConnInfo *info, 
                                navl_handle_t handle, 
                                navl_result_t result, 
                                navl_state_t state, 
                                navl_conn_t nc, 
                                int error);

    void start();
    void stop();

  private:
    void FlowTableCleanup();
    void DatapathUpdate();
    std::mutex ftbl_mutex;
    int numQueues_;
    bool running_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread flowCleanupThread_;
    std::thread datapathUpdateThread_;
};

}

#endif
