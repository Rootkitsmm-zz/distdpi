#ifndef FLOWTABLE_H
#define FLOWTABLE_H

#include "Queue.h"
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
          connid(0),
          packetnum(0),
          classified_num(0),
          class_state(NAVL_STATE_INSPECTING),
          initiator_total_packets(0),
          recipient_total_packets(0),
          initiator_total_bytes(0),
          recipient_total_bytes(0),
          dpi_state(0), dpi_result(0),
          dpi_confidence(0),
          error(0) {}


        ConnKey key;
        uint64_t connid;
        uint32_t packetnum;
        u_int   classified_num;
        navl_state_t class_state;

        u_int   initiator_total_packets;
        u_int   recipient_total_packets;

        u_int   initiator_total_bytes;
        u_int   recipient_total_bytes;

        void    *dpi_state;
        u_int   dpi_result;
        int     dpi_confidence;
        int     error;
    }; 

    struct ConnMetadata {
        ConnKey *key;
        ConnInfo *info;
        std::string data;
        uint32_t dir;
    };

    struct ConnKeyHasher {
        size_t operator()(const ConnKey &key) const {
            return key.srcaddr + key.dstaddr + key.srcport + key.dstport + key.ipproto;
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

    std::vector<std::unique_ptr<Queue<ConnMetadata>>> ftbl_queue_list_;
    
    FlowTable(int numOfQueues);
    ~FlowTable();
    int numQueues_;

  private:

    void printFlowTable(int signal);
};

}

#endif
