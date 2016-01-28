#ifndef FLOWTABLE_H
#define FLOWTABLE_H

#include <unordered_map>

#include "PacketHandler.h"

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
        uint8_t  ipproto;
    };

    struct ConnValue {
        ConnValue(ConnKey *key)
        : key(*key),
          connid(0),
          packetnum(0) {}

        ConnKey key;
        uint64_t connid;
        uint32_t packetnum;
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

    std::unordered_map<ConnKey, ConnValue, ConnKeyHasher, ConnKeyEqual> conn_table;

    FlowTable(PacketHandler *hdl);
    ~FlowTable();
    void start();
    void PacketConsumer();

  private:
    PacketHandler *pkthdl;
};

}

#endif
